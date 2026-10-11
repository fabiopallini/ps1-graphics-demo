/* The runtime uses RotMatrix (Rx * Ry * Rz), R * (point + camera.pos),
 * SetGeomOffset(160,120) and SetGeomScreen(512). Angles use 4096 units/turn.
 * Reference: PSn00bSDK libpsn00b/include/psxgte.h, RotMatrix documentation.
 * Browser preview uses floating point: GTE fixed-point rounding may differ ~1px. */
export const LIMITS = { planes: 10, zones: 5, spawns: 5, npcs: 5 };
export const clone = value => JSON.parse(JSON.stringify(value));
export function rotation(stage) {
  const [x, y, z] = ['cam_rx', 'cam_ry', 'cam_rz'].map(k => stage[k] * Math.PI / 2048);
  const [sx, sy, sz] = [Math.sin(x), Math.sin(y), Math.sin(z)];
  const [cx, cy, cz] = [Math.cos(x), Math.cos(y), Math.cos(z)];
  return [
    [cy * cz, -cy * sz, sy],
    [cx * sz + sx * sy * cz, cx * cz - sx * sy * sz, -sx * cy],
    [sx * sz - cx * sy * cz, sx * cz + cx * sy * sz, cx * cy]
  ];
}
export function project(stage, point) {
  const r = rotation(stage);
  const v = [point.x + stage.cam_x, point.y + stage.cam_y, point.z + stage.cam_z];
  const p = r.map(row => row.reduce((sum, a, i) => sum + a * v[i], 0));
  if (p[2] <= 1) return null;
  return { x: 160 + 512 * p[0] / p[2], y: 120 + 512 * p[1] / p[2], depth: p[2] };
}
export function projectPolygon(stage, points, near = 256) {
  const r = rotation(stage);
  let polygon = points.map(point => {
    const v = [point.x + stage.cam_x, point.y + stage.cam_y, point.z + stage.cam_z];
    return r.map(row => row.reduce((sum, a, i) => sum + a * v[i], 0));
  });
  const clipped = [];
  for (let i = 0; i < polygon.length; i++) {
    const a = polygon[i], b = polygon[(i + 1) % polygon.length];
    const aInside = a[2] >= near, bInside = b[2] >= near;
    if (aInside) clipped.push(a);
    if (aInside !== bInside) {
      const t = (near - a[2]) / (b[2] - a[2]);
      clipped.push(a.map((v, axis) => v + t * (b[axis] - v)));
    }
  }
  return clipped.length >= 3 ? clipped.map(p => ({ x:160 + 512*p[0]/p[2], y:120 + 512*p[1]/p[2] })) : null;
}
export function unproject(stage, screen, groundY = 0) {
  const r = rotation(stage), ray = [(screen.x - 160) / 512, (screen.y - 120) / 512, 1];
  const direction = [0, 1, 2].map(i => r.reduce((sum, row, j) => sum + row[i] * ray[j], 0));
  if (Math.abs(direction[1]) < 1e-8) return null;
  const t = (groundY + stage.cam_y) / direction[1];
  if (t <= 0 || !Number.isFinite(t)) return null;
  return { x: -stage.cam_x + t * direction[0], y: groundY, z: -stage.cam_z + t * direction[2] };
}
export function corners(p, exact = false) {
  const xmin = p.x - (exact ? 1 : 0), zmax = p.z + (exact ? 1 : 0);
  return [{ x: xmin, y: p.y, z: zmax }, { x: p.x + p.w, y: p.y, z: zmax },
    { x: p.x + p.w, y: p.y, z: p.z + p.d }, { x: xmin, y: p.y, z: p.z + p.d }];
}
export function rectangle(a, b, groundY = 0, snap = 5) {
  const round = n => Math.round(n / snap) * snap;
  const xmin = round(Math.min(a.x, b.x)), xmax = round(Math.max(a.x, b.x));
  const zmin = round(Math.min(a.z, b.z)), zmax = round(Math.max(a.z, b.z));
  return { x: xmin, y: groundY, z: zmax, w: Math.max(snap, xmax - xmin), h: 0, d: -Math.max(snap, zmax - zmin) };
}
export function inside(point, polygon) {
  let hit = false;
  for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i++) {
    const a = polygon[i], b = polygon[j];
    if ((a.y > point.y) !== (b.y > point.y) && point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x) hit = !hit;
  }
  return hit;
}
export function walkable(stage, x, z) {
  return stage.planes.some(p => x > p.x - 1 && x < p.x + p.w && z > p.z + p.d && z < p.z + 1);
}
export function zoneHit(p, x, z, size = 100) {
  return x <= p.x + p.w && x + size >= p.x && z <= p.z && z + size >= p.z + p.d;
}
export function validate(stages) {
  const errors = [], warnings = [];
  const integer = (v, min = -2147483648, max = 2147483647) => Number.isInteger(v) && v >= min && v <= max;
  if (!Array.isArray(stages) || !stages.length) return { errors: ['Serve almeno una schermata.'], warnings };
  stages.forEach((s, sid) => {
    const prefix = `Area ${sid}`;
    if (!s || typeof s !== 'object' || Array.isArray(s)) { errors.push(`${prefix}: dati non validi.`); return; }
    if (typeof s.name !== 'string' || !s.name.trim()) errors.push(`${prefix}: nome mancante.`);
    for (const k of ['cam_x', 'cam_y', 'cam_z', 'cam_rx', 'cam_ry', 'cam_rz']) {
      if (!integer(s[k], k.includes('cam_r') ? -32768 : -2147483648, k.includes('cam_r') ? 32767 : 2147483647)) errors.push(`${prefix}: ${k} non valido.`);
    }
    for (const k of ['tim_0', 'tim_1']) {
      if (typeof s[k] !== 'string' || !/^[A-Z0-9_]{1,5}\.TIM$/.test(s[k])) errors.push(`${prefix}: ${k} deve essere un nome TIM maiuscolo di massimo 9 caratteri (es. BG8_0.TIM).`);
    }
    for (const [kind, capacity] of Object.entries(LIMITS)) {
      const items = s[kind] ?? (kind === 'npcs' ? [] : undefined);
      if (!Array.isArray(items) || items.length > capacity || (kind === 'spawns' && !items.length)) {
        errors.push(`${prefix}: ${kind}, limite ${capacity}${kind === 'spawns' ? ', almeno un ingresso' : ''}.`); continue;
      }
      if (kind === 'npcs') {
        for (const group of items) {
          if (!group || typeof group !== 'object' || Array.isArray(group)) { errors.push(`${prefix}: NPC non valido.`); continue; }
          for (const npc of Object.values(group)) {
            if (!npc || !['x','y','z','rx','ry','rz'].every(k => integer(npc[k], k.length === 2 ? -32768 : -2147483648, k.length === 2 ? 32767 : 2147483647)) || !Array.isArray(npc.talk_chars) || !npc.talk_chars.length || npc.talk_chars.some(t => typeof t !== 'string' || t.includes('\0') || new TextEncoder().encode(t).length > 77)) errors.push(`${prefix}: NPC o dialoghi non compatibili (max 77 byte/pagina).`);
          }
        }
        if (items.reduce((n, group) => n + Object.keys(group || {}).length, 0) > capacity) errors.push(`${prefix}: troppi NPC.`);
        continue;
      }
      items.forEach((p, i) => {
        if (!p || typeof p !== 'object' || Array.isArray(p)) { errors.push(`${prefix}: ${kind} ${i} non valido.`); return; }
        const fields = kind === 'spawns' ? ['x','y','z','rx','ry','rz'] : ['x','y','z','w','h','d'];
        for (const k of fields) if (!integer(p[k], k.length === 2 ? -32768 : -2147483648, k.length === 2 ? 32767 : 2147483647)) errors.push(`${prefix}: ${kind} ${i}, ${k} non valido.`);
        if (kind !== 'spawns' && (!(p.w > 0) || !(p.d < 0))) errors.push(`${prefix}: ${kind} ${i}, larghezza positiva e profondità negativa richieste.`);
        if (kind === 'zones' && (!integer(p.stage_id, 0, stages.length - 1) || !integer(p.spawn_id, 0, (stages[p.stage_id]?.spawns?.length ?? 0) - 1))) errors.push(`${prefix}: zona ${i}, destinazione non valida.`);
      });
    }
    if (Array.isArray(s.planes) && s.planes.every(p => p && typeof p === 'object') && Array.isArray(s.spawns) && Array.isArray(s.zones) && s.zones.every(p => p && typeof p === 'object')) {
      s.spawns.forEach((sp, i) => {
        if (!sp) return;
        if (!walkable(s, sp.x, sp.z)) warnings.push(`${prefix}: ingresso ${i} fuori dalle piattaforme.`);
        if (s.zones.some(p => zoneHit(p, sp.x, sp.z))) warnings.push(`${prefix}: ingresso ${i} attiva subito una zona.`);
      });
    }
  });
  return { errors, warnings };
}
export function encodeTim(rgba, width, height, vramX, vramY) {
  if (rgba.length !== width * height * 4) throw new Error('Dimensioni pixel TIM non valide.');
  const buffer = new ArrayBuffer(20 + width * height * 2), view = new DataView(buffer);
  view.setUint32(0, 0x10, true); view.setUint32(4, 2, true);
  view.setUint32(8, 12 + width * height * 2, true);
  [vramX, vramY, width, height].forEach((v, i) => view.setUint16(12 + 2 * i, v, true));
  for (let i = 0; i < width * height; i++) {
    const offset = i * 4;
    let pixel = (rgba[offset] >> 3) | ((rgba[offset + 1] >> 3) << 5) | ((rgba[offset + 2] >> 3) << 10);
    if (rgba[offset + 3] < 128) pixel = 0;
    else if (pixel === 0) pixel = 0x8000; // Opaque black, not PS1 transparent black.
    view.setUint16(20 + i * 2, pixel, true);
  }
  return new Uint8Array(buffer);
}
export function serialize(stages) {
  // Compact coordinate rows, as in the existing hand-written stages.json.
  return JSON.stringify(stages, null, '\t').replace(/\{\n(?:\t+"(?:x|y|z|w|h|d|rx|ry|rz|stage_id|spawn_id)": -?\d+,?\n)+\t+\}/g, block =>
    '{' + block.slice(1, -1).trim().split('\n').map(line => line.trim()).join(' ') + '}') + '\n';
}
