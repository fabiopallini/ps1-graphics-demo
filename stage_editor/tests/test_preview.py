import importlib.util
import json
import math
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

EDITOR = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('preview_stage', EDITOR / 'preview_stage.py')
preview = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preview)


class PreviewTests(unittest.TestCase):
    def setUp(self):
        self.stages = json.loads((EDITOR.parent / 'stages/stages.json').read_text('utf-8'))

    def test_all_existing_backgrounds_render(self):
        for stage in self.stages:
            with self.subTest(stage=stage['name']):
                xml = ET.fromstring(preview.render(stage))
                self.assertEqual(xml.tag, '{http://www.w3.org/2000/svg}svg')
                self.assertTrue(xml.findall('{http://www.w3.org/2000/svg}image'))

    def test_sanctuary_projection(self):
        stage = self.stages[8]
        x, y = preview.project(stage, stage['spawns'][0])
        self.assertAlmostEqual(x, 160)
        self.assertAlmostEqual(y, 193.18219162)

    def test_three_rotations_and_behind_camera(self):
        stage = dict(cam_x=0, cam_y=0, cam_z=0, cam_rx=1024, cam_ry=1024, cam_rz=1024)
        rotated = preview.camera_point(stage, dict(x=1, y=2, z=3))
        for actual, expected in zip(rotated, (3, -2, 1)):
            self.assertAlmostEqual(actual, expected)
        stage.update(cam_rx=0, cam_ry=0, cam_rz=0)
        self.assertIsNone(preview.project(stage, dict(x=0, y=0, z=-10)))

    def test_near_clipping_keeps_partly_visible_floor(self):
        stage = self.stages[6]
        poly = preview.polygon(stage, stage['planes'][0])
        self.assertGreaterEqual(len(poly), 3)
        self.assertTrue(all(math.isfinite(c) for point in poly for c in point))

    def test_layout_without_assets(self):
        xml = ET.fromstring(preview.render(self.stages[8], root=Path('/nonexistent'), layout_only=True))
        self.assertFalse(xml.findall('{http://www.w3.org/2000/svg}image'))
        self.assertEqual(len(xml.findall('{http://www.w3.org/2000/svg}polygon')), 3)


if __name__ == '__main__':
    unittest.main()
