"""Integration checks use temporary copies; game data is never written."""
import base64
from concurrent.futures import ThreadPoolExecutor
import copy
import importlib.util
import io
import json
from pathlib import Path
import shutil
import struct
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
import zipfile

EDITOR = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('editor_server', EDITOR / 'server.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ServerTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        (self.root / 'stages').mkdir()
        shutil.copy(EDITOR.parent.parent / 'stages/stages.json', self.root / 'stages/stages.json')
        (self.root / 'data/stages').mkdir(parents=True)
        for name in ('BG8.png', 'BG8_0.png', 'BG8_1.png'):
            shutil.copy(EDITOR.parent.parent / 'data/stages' / name, self.root / 'data/stages' / name)
        self.server = module.create_server(self.root, self.root / 'psxstudio/stage_editor', 0)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.url = 'http://127.0.0.1:%s' % self.server.server_port
        self.project = self.request('/api/project')[1]

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.directory.cleanup()

    def request(self, route, data=None, headers=None):
        extra = {'X-Editor-Token': self.server.project.token}
        extra.update(headers or {})
        req = urllib.request.Request(self.url + route, data=json.dumps(data).encode() if data is not None else None, headers=extra)
        try:
            response = urllib.request.urlopen(req)
        except urllib.error.HTTPError as error:
            response = error
        with response:
            raw = response.read()
            return response.status, json.loads(raw) if 'application/json' in response.headers.get('Content-Type', '') else raw

    def test_catalog_and_static(self):
        self.assertEqual(len(self.project['stages']), 9)
        self.assertEqual(len(self.project['catalog']['files']), 3)
        self.assertEqual(self.request('/')[0], 200)
        self.assertEqual(self.request('/offline-bundle.js')[0], 200)
        self.assertEqual(self.request('/backgrounds/BG8.png')[0], 200)
        self.assertEqual(self.request('/../stages/stages.json')[0], 404)
        self.assertEqual(self.request('/%2e%2e/server.py')[0], 404)
        self.assertEqual(self.request('/api/project', headers={'Host': 'evil.example'})[0], 403)

    def test_save_backup_and_conflict(self):
        target = self.root / 'stages/stages.json'
        original = target.read_bytes()
        stages = copy.deepcopy(self.project['stages'])
        stages[8]['name'] = '8_prova'
        code, result = self.request('/api/stages', dict(stages=stages, revision=self.project['revision']))
        self.assertEqual(code, 200)
        self.assertEqual(json.loads(target.read_bytes()), stages)
        self.assertEqual(json.loads(target.read_bytes())[7]['npcs'], self.project['stages'][7]['npcs'])
        backups = list((self.root / 'psxstudio/stage_editor/backups').glob('*.json'))
        self.assertEqual(len(backups), 1)
        self.assertEqual(backups[0].read_bytes(), original)
        self.assertEqual(self.request('/api/stages', dict(stages=stages, revision=self.project['revision']))[0], 409)
        self.assertEqual(self.request('/api/stages', dict(stages=stages, revision=result['revision']))[0], 200)
        self.assertEqual(len(list((self.root / 'psxstudio/stage_editor/backups').glob('*.json'))), 1)

    def test_validation_and_auth(self):
        stages = copy.deepcopy(self.project['stages'])
        stages[0]['planes'] *= 11
        self.assertEqual(self.request('/api/stages', dict(stages=stages, revision=self.project['revision']))[0], 422)
        self.assertEqual(self.request('/api/stages', {}, {'X-Editor-Token': 'bad'})[0], 403)
        self.assertEqual(self.request('/api/stages', {}, {'Origin': 'https://evil.example'})[0], 403)
        self.assertEqual(self.request('/api/stages', [1])[0], 400)
        for change in (lambda s: s[0].update(cam_x=True), lambda s: s[0].update(tim_0='LONGNAME.TIM'), lambda s: s[0]['zones'][0].update(stage_id=100)):
            stages = copy.deepcopy(self.project['stages'])
            change(stages)
            self.assertTrue(module.validate(stages))
        self.assertEqual(module.validate(self.project['stages']), [])
        self.assertEqual(json.loads(module.serialize(self.project['stages'])), self.project['stages'])

    def test_concurrent_saves(self):
        data = dict(stages=copy.deepcopy(self.project['stages']), revision=self.project['revision'])
        data['stages'][8]['name'] = '8_concurrent'
        with ThreadPoolExecutor(2) as executor:
            results = list(executor.map(lambda _: self.request('/api/stages', data)[0], range(2)))
        self.assertEqual(sorted(results), [200, 409])

    def test_background_persistence_and_zip(self):
        def encode(data):
            return base64.b64encode(data).decode()
        parts = []
        for name, width, x in (('BG8_0.png', 256, 320), ('BG8_1.png', 64, 576)):
            tim = struct.pack('<IIIHHHH', 16, 2, 12 + width * 256 * 2, x, 0, width, 256) + b'\x00\x80' * (width * 256)
            parts.append(dict(png=encode((self.root / 'data/stages' / name).read_bytes()), tim=encode(tim)))
        data = dict(name='Prova', tim0='BG9_0.TIM', tim1='BG9_1.TIM', image=encode((self.root / 'data/stages/BG8.png').read_bytes()), parts=parts)
        code, item = self.request('/api/backgrounds', data)
        self.assertEqual(code, 200)
        self.assertEqual(self.request(item['url'])[0], 200)
        self.assertEqual(self.request('/api/project')[1]['catalog']['imported'][0], item)
        self.assertEqual(module.Project(self.root, self.root / 'psxstudio/stage_editor').manifest()[0], item)
        status, binary = self.request('/api/export/' + item['id'])
        self.assertEqual(status, 200)
        with zipfile.ZipFile(io.BytesIO(binary)) as archive:
            self.assertIsNone(archive.testzip())
            self.assertEqual(len(archive.namelist()), 6)
            self.assertIn('data/stages/BG9_0.tim', archive.namelist())
            self.assertIn(b'BG9_0.TIM', archive.read('CONF_ENTRIES.TXT'))
        self.assertFalse((self.root / 'data/stages/BG9_0.tim').exists())
        data['parts'][0]['tim'] = encode(b'bad')
        self.assertEqual(self.request('/api/backgrounds', data)[0], 422)


if __name__ == '__main__':
    unittest.main()
