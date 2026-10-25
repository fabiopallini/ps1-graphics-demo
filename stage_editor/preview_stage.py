"""Export a stage background with projected planes/zones/spawns as standalone SVG."""
import argparse
import base64
import html
import json
import math
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]


def rotation(stage):
    x, y, z = (stage[k] * math.pi / 2048 for k in ('cam_rx', 'cam_ry', 'cam_rz'))
    sx, sy, sz = math.sin(x), math.sin(y), math.sin(z)
    cx, cy, cz = math.cos(x), math.cos(y), math.cos(z)
    return ((cy * cz, -cy * sz, sy),
            (cx * sz + sx * sy * cz, cx * cz - sx * sy * sz, -sx * cy),
            (sx * sz - cx * sy * cz, sx * cz + cx * sy * sz, cx * cy))


def camera_point(stage, point):
    v = [point[k] + stage['cam_' + k] for k in ('x', 'y', 'z')]
    return tuple(sum(a * b for a, b in zip(row, v)) for row in rotation(stage))


def screen(point):
    return 160 + 512 * point[0] / point[2], 120 + 512 * point[1] / point[2]


def project(stage, point):
    value = camera_point(stage, point)
    return screen(value) if value[2] > 1 else None


def polygon(stage, plane, near=256):
    xmin, zmax = plane['x'] - 1, plane['z'] + 1
    points = [camera_point(stage, dict(x=x, y=plane['y'], z=z)) for x, z in
              ((xmin, zmax), (plane['x'] + plane['w'], zmax),
               (plane['x'] + plane['w'], plane['z'] + plane['d']), (xmin, plane['z'] + plane['d']))]
    clipped = []
    for i, b in enumerate(points):
        a = points[i - 1]
        inside_a, inside_b = a[2] >= near, b[2] >= near
        if inside_a != inside_b:
            t = (near - a[2]) / (b[2] - a[2])
            clipped.append(tuple(a[k] + t * (b[k] - a[k]) for k in range(3)))
        if inside_b:
            clipped.append(b)
    return [screen(p) for p in clipped] if len(clipped) >= 3 else []


def png_size(path):
    raw = path.read_bytes()
    if len(raw) < 24 or raw[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('PNG non valido: %s' % path)
    return struct.unpack('>II', raw[16:24])


def image_tag(path, x, width, height):
    encoded = base64.b64encode(path.read_bytes()).decode('ascii')
    return '<image x="%s" y="0" width="%s" height="%s" href="data:image/png;base64,%s"/>' % (x, width, height, encoded)


def background_tags(root, stage, background=None):
    if background is not None:
        return [image_tag(Path(background), 0, 320, 240)]
    directory = root / 'data/stages'
    files = {p.name.upper(): p for p in directory.glob('*') if p.is_file() and p.suffix.lower() == '.png'}
    prefix = re.sub(r'_[01]\.TIM$', '', stage['tim_0'])
    full = files.get(prefix + '.PNG')
    if full and png_size(full) == (320, 240):
        return [image_tag(full, 0, 320, 240)]
    pages = [files.get(stage[k][:-4] + '.PNG') for k in ('tim_0', 'tim_1')]
    if all(pages):
        by_width = {png_size(p)[0]: p for p in pages}
        if 256 in by_width and 64 in by_width:
            return [image_tag(by_width[256], 0, 256, png_size(by_width[256])[1]),
                    image_tag(by_width[64], 255, 64, png_size(by_width[64])[1])]
    raise ValueError('Sfondo non trovato per %s: usa --background PNG o --layout-only.' % stage['name'])


def render(stage, root=ROOT, background=None, layout_only=False):
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 240" width="960" height="720">',
             '<title>%s</title>' % html.escape(stage['name']),
             '<rect width="320" height="240" fill="#101a1c"/>']
    if not layout_only:
        parts.extend(background_tags(Path(root), stage, background))
    for kind, color in (('planes', '#52ff78'), ('zones', '#ff5360')):
        for i, plane in enumerate(stage.get(kind, [])):
            poly = polygon(stage, plane)
            if not poly:
                continue
            points = ' '.join('%.2f,%.2f' % p for p in poly)
            label = '%s%s%s' % ('P' if kind == 'planes' else 'Z', i,
                                ' → %s/%s' % (plane['stage_id'], plane['spawn_id']) if kind == 'zones' else '')
            parts.append('<polygon points="%s" fill="%s" fill-opacity="0.20" stroke="%s" stroke-width="0.7"><title>%s</title></polygon>' % (points, color, color, html.escape(label)))
    for i, spawn in enumerate(stage['spawns']):
        point = project(stage, spawn)
        if point:
            parts.append('<circle cx="%.2f" cy="%.2f" r="2" fill="#39e7ff" stroke="black" stroke-width="0.6"><title>Ingresso %s</title></circle>' % (*point, i))
    parts.append('</svg>')
    return '\n'.join(parts) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', type=int, help='Indice della schermata in stages.json')
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--stages', type=Path, help='JSON alternativo, anche layout preliminare')
    parser.add_argument('--background', type=Path, help='PNG completo alternativo')
    parser.add_argument('--layout-only', action='store_true', help='Proietta il layout senza fondale')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    try:
        stages = json.loads((args.stages or root / 'stages/stages.json').read_text('utf-8'))
        if not 0 <= args.stage < len(stages):
            parser.error('Indice schermata fuori intervallo.')
        stage = stages[args.stage]
        contents = render(stage, root, args.background, args.layout_only)
        prefix = stage['tim_0'][:-4]
        output = args.output or root / 'stage_editor/previews' / (prefix + '_WALK.svg')
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(contents, encoding='utf-8')
        print(output.resolve())
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
