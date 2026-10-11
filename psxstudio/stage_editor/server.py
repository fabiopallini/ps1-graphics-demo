"""Crystal Fantasy editor: local HTTP server, Python standard library only."""
import argparse
import base64
import hashlib
import io
import json
import os
from pathlib import Path
import re
import secrets
import struct
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import unquote, urlsplit
import zipfile

EDITOR = Path(__file__).resolve().parent
LIMITS = {'planes': 10, 'zones': 5, 'spawns': 5, 'npcs': 5}
TIM_NAME = re.compile(r'[A-Z0-9_]{1,5}\.TIM\Z')


class RequestError(Exception):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status


def integer(value, rotation=False):
    low, high = (-32768, 32767) if rotation else (-2147483648, 2147483647)
    return type(value) is int and low <= value <= high


def validate(stages):
    errors = []
    if not isinstance(stages, list) or not stages:
        return ['At least one stage is required.']
    for sid, stage in enumerate(stages):
        prefix = 'Area %s: ' % sid
        if not isinstance(stage, dict):
            errors.append(prefix + 'Invalid data.')
            continue
        if not isinstance(stage.get('name'), str) or not stage['name'].strip():
            errors.append(prefix + 'Missing name.')
        for field in ('cam_x', 'cam_y', 'cam_z', 'cam_rx', 'cam_ry', 'cam_rz'):
            if not integer(stage.get(field), field.startswith('cam_r')):
                errors.append(prefix + field + ' is invalid.')
        for field in ('tim_0', 'tim_1'):
            if not isinstance(stage.get(field), str) or not TIM_NAME.fullmatch(stage[field]):
                errors.append(prefix + field + ' is incompatible with StageData.')
        for kind, capacity in LIMITS.items():
            items = stage.get(kind, [] if kind == 'npcs' else None)
            if not isinstance(items, list) or len(items) > capacity or (kind == 'spawns' and not items):
                errors.append(prefix + kind + ': limit %s, at least one spawn point required.' % capacity)
                continue
            if kind == 'npcs':
                count = 0
                for group in items:
                    if not isinstance(group, dict):
                        errors.append(prefix + 'NPC is invalid.')
                        continue
                    count += len(group)
                    for npc in group.values():
                        if not isinstance(npc, dict):
                            errors.append(prefix + 'NPC is invalid.')
                            continue
                        pages = npc.get('talk_chars')
                        if (not all(integer(npc.get(k), len(k) == 2) for k in ('x', 'y', 'z', 'rx', 'ry', 'rz'))
                                or not isinstance(pages, list) or not pages
                                or any(not isinstance(t, str) or '\0' in t or len(t.encode('utf-8')) > 77 for t in pages)):
                            errors.append(prefix + 'Invalid NPC/dialogue (77 bytes/page).')
                if count > capacity:
                    errors.append(prefix + 'Too many NPCs.')
                continue
            for item in items:
                if not isinstance(item, dict):
                    errors.append(prefix + kind + ' is invalid.')
                    continue
                fields = ('x', 'y', 'z', 'rx', 'ry', 'rz') if kind == 'spawns' else ('x', 'y', 'z', 'w', 'h', 'd')
                if not all(integer(item.get(k), len(k) == 2) for k in fields):
                    errors.append(prefix + kind + ': invalid coordinates.')
                if kind != 'spawns' and not (integer(item.get('w')) and item['w'] > 0 and integer(item.get('d')) and item['d'] < 0):
                    errors.append(prefix + 'Positive W and negative D required.')
                if kind == 'zones':
                    dest, spawn = item.get('stage_id'), item.get('spawn_id')
                    target = stages[dest] if type(dest) is int and 0 <= dest < len(stages) else None
                    spawns = target.get('spawns') if isinstance(target, dict) else None
                    if not isinstance(spawns, list) or type(spawn) is not int or not 0 <= spawn < len(spawns):
                        errors.append(prefix + 'Invalid zone destination.')
    return errors


def serialize(stages):
    text = json.dumps(stages, ensure_ascii=False, indent='\t', allow_nan=False)
    pattern = r'\{\n(?:\t+"(?:x|y|z|w|h|d|rx|ry|rz|stage_id|spawn_id)": -?\d+,?\n)+\t+\}'
    return re.sub(pattern, lambda m: '{' + ' '.join(line.strip() for line in m[0][1:-1].strip().splitlines()) + '}', text) + '\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def atomic_write(target, data, expected=None):
    target.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix='.' + target.name + '-', suffix='.tmp', dir=target.parent)
    try:
        if target.exists():
            os.fchmod(fd, target.stat().st_mode & 0o777)
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        if expected is not None and digest(target.read_bytes()) != expected:
            raise RequestError(409, 'The file changed while saving. Reload it.')
        os.replace(name, target)
    finally:
        Path(name).unlink(missing_ok=True)


def png_size(data):
    if len(data) < 24 or data[:8] != b'\x89PNG\r\n\x1a\n' or data[12:16] != b'IHDR':
        raise RequestError(422, 'A valid PNG is required.')
    return struct.unpack('>II', data[16:24])


class Project:
    def __init__(self, root, storage):
        self.root = Path(root).resolve()
        self.storage = Path(storage).resolve()
        self.target = self.root / 'stages/stages.json'
        self.assets = self.storage / 'assets'
        self.token = secrets.token_hex(24)
        self.lock = threading.RLock()

    def manifest(self):
        file = self.assets / 'index.json'
        return json.loads(file.read_text('utf-8')) if file.exists() else []

    def catalog(self):
        files = []
        for file in sorted((self.root / 'data/stages').glob('*')):
            if not file.is_file() or not re.fullmatch(r'[A-Za-z0-9_]+\.png', file.name, re.I):
                continue
            width, height = png_size(file.read_bytes())
            files.append(dict(name=file.name, width=width, height=height, url='/backgrounds/' + file.name))
        return dict(files=files, imported=self.manifest())

    def save(self, data):
        stages = data.get('stages')
        errors = validate(stages)
        if errors:
            raise RequestError(422, '\n'.join(errors))
        with self.lock:
            previous = self.target.read_bytes()
            if digest(previous) != data.get('revision'):
                raise RequestError(409, 'stages.json changed outside the editor. Download a JSON copy and reload the file.')
            contents = serialize(stages).encode('utf-8')
            if contents != previous:
                backup = self.storage / 'backups' / (secrets.token_hex(12) + '.json')
                atomic_write(backup, previous)
                atomic_write(self.target, contents, digest(previous))
            return dict(revision=digest(contents), warnings=[])

    def upload(self, data):
        names = [data.get('tim0'), data.get('tim1')]
        if any(not isinstance(n, str) or not TIM_NAME.fullmatch(n) for n in names) or names[0] == names[1]:
            raise RequestError(422, 'TIM names are incompatible with StageData.')
        def decode(value):
            try:
                return base64.b64decode(value, validate=True)
            except (ValueError, TypeError):
                raise RequestError(422, 'Invalid image data.')
        main = decode(data.get('image'))
        if png_size(main) != (320, 240):
            raise RequestError(422, 'The background must be 320x240.')
        parts = data.get('parts')
        if not isinstance(parts, list) or len(parts) != 2 or any(not isinstance(p, dict) for p in parts):
            raise RequestError(422, 'Both PNG/TIM pages are required.')
        files = {}
        for i, (width, x) in enumerate(((256, 320), (64, 576))):
            png, tim = decode(parts[i].get('png')), decode(parts[i].get('tim'))
            expected = struct.pack('<IIIHHHH', 16, 2, 12 + width * 256 * 2, x, 0, width, 256)
            if png_size(png) != (width, 256) or len(tim) != 20 + width * 256 * 2 or tim[:20] != expected:
                raise RequestError(422, 'Invalid PNG/TIM pages.')
            files[names[i][:-4] + '.png'] = png
            files[names[i][:-4] + '.tim'] = tim
        with self.lock:
            ident = secrets.token_hex(8)
            destination = self.assets / ident
            destination.mkdir(parents=True)
            (destination / 'preview.png').write_bytes(main)
            for name, contents in files.items():
                (destination / name).write_bytes(contents)
            item = dict(id=ident, tim0=names[0], tim1=names[1], name=str(data.get('name', 'New background'))[:120],
                        url='/assets/' + ident + '/preview.png', files=list(files))
            manifest = self.manifest()
            manifest.append(item)
            atomic_write(self.assets / 'index.json', json.dumps(manifest, ensure_ascii=False, indent=2).encode('utf-8'))
            return item

    def export(self, ident):
        with self.lock:
            item = next((i for i in self.manifest() if i['id'] == ident), None)
            if item is None:
                raise RequestError(404, 'Background not found.')
            output = io.BytesIO()
            with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_STORED) as archive:
                for name in item['files']:
                    archive.writestr('data/stages/' + name, (self.assets / ident / name).read_bytes())
                entries = '\r\n'.join('File %s\r\n    XAFileAttributes Form1 Data\r\n    Source [root]\\data\\stages\\%s.tim\r\nEndFile\r\n' % (n, n[:-4]) for n in (item['tim0'], item['tim1']))
                archive.writestr('CONF_ENTRIES.TXT', entries)
                archive.writestr('README.txt', 'Copy data/stages into the project. Add the CTI entries at the root of cdrom/CONF.CTI without duplicates.\nRun make run in stages, then bash build.sh from the repository root.\n16-bit TIM, VRAM (320,0) and (576,0).\n')
            return output.getvalue()


def create_server(root=EDITOR.parent.parent, storage=EDITOR, port=8000):
    project = Project(root, storage)

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def send(self, status, data, content_type='application/json; charset=utf-8', extra=None):
            if isinstance(data, dict):
                data = json.dumps(data, ensure_ascii=False).encode('utf-8')
            self.send_response(status)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            for key, value in (extra or {}).items():
                self.send_header(key, value)
            self.end_headers()
            self.wfile.write(data)

        def handle_request(self):
            try:
                host = self.headers.get('Host', '')
                if not re.fullmatch(r'(127\.0\.0\.1|localhost)(:\d+)?', host):
                    raise RequestError(403, 'Host not allowed.')
                if self.headers.get('Origin') not in (None, 'http://' + host):
                    raise RequestError(403, 'Origin not allowed.')
                route = unquote(urlsplit(self.path).path)
                if self.command == 'POST':
                    if self.headers.get('X-Editor-Token') != project.token:
                        raise RequestError(403, 'Invalid session. Reload the editor.')
                    try:
                        size = int(self.headers.get('Content-Length', '-1'))
                    except ValueError:
                        raise RequestError(400, 'Invalid request size.')
                    if not 0 <= size <= 6 * 1024 * 1024:
                        raise RequestError(413, 'Request too large or size missing.')
                    try:
                        data = json.loads(self.rfile.read(size))
                    except (ValueError, UnicodeError):
                        raise RequestError(400, 'Invalid JSON.')
                    if not isinstance(data, dict):
                        raise RequestError(400, 'Invalid request.')
                    if route == '/api/stages':
                        return self.send(200, project.save(data))
                    if route == '/api/backgrounds':
                        return self.send(200, project.upload(data))
                    raise RequestError(404, 'Operation unavailable.')
                if route == '/api/project':
                    with project.lock:
                        contents = project.target.read_bytes()
                        return self.send(200, dict(stages=json.loads(contents), revision=digest(contents), token=project.token, catalog=project.catalog()))
                if re.fullmatch(r'/api/export/[a-f0-9]{16}', route):
                    return self.send(200, project.export(route.rsplit('/', 1)[1]), 'application/zip', {'Content-Disposition': 'attachment; filename="background.zip"'})
                if re.fullmatch(r'/backgrounds/[A-Za-z0-9_]+\.png', route, re.I):
                    file = project.root / 'data/stages' / route.rsplit('/', 1)[1]
                elif re.fullmatch(r'/assets/[a-f0-9]{16}/[A-Za-z0-9_]+\.(png|tim)', route):
                    file = project.storage / route.lstrip('/')
                else:
                    name = 'index.html' if route == '/' else route.lstrip('/')
                    if name not in ('index.html', 'offline-bundle.js', 'style.css'):
                        raise RequestError(404, 'File not found.')
                    file = EDITOR / name
                content_type = {'html': 'text/html; charset=utf-8', 'js': 'text/javascript; charset=utf-8', 'css': 'text/css; charset=utf-8', 'png': 'image/png'}.get(file.suffix[1:], 'application/octet-stream')
                self.send(200, file.read_bytes(), content_type)
            except Exception as error:
                code = error.status if isinstance(error, RequestError) else 404 if isinstance(error, FileNotFoundError) else 500
                self.send(code, {'error': str(error)})

        do_GET = handle_request
        do_POST = handle_request

    server = ThreadingHTTPServer(('127.0.0.1', port), Handler)
    server.project = project
    return server


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8000)
    parser.add_argument('--root', type=Path, default=EDITOR.parent.parent)
    parser.add_argument('--storage', type=Path, default=EDITOR)
    args = parser.parse_args()
    try:
        server = create_server(args.root, args.storage, args.port)
    except OSError as error:
        parser.exit(1, 'Unable to start the server: %s. Try --port 8001.\n' % error)
    print('Crystal Fantasy editor: http://127.0.0.1:%s\nCtrl+C to stop.' % server.server_port, flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
