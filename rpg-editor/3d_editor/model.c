#include "editor.h"
static gboolean fail(GError **error, const char *message) {
    g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, message);
    return FALSE;
}
Vec vec_add(Vec a, Vec b) { return (Vec){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec vec_sub(Vec a, Vec b) { return (Vec){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec vec_mul(Vec a, double k) { return (Vec){a.x * k, a.y * k, a.z * k}; }
double vec_dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec vec_cross(Vec a, Vec b) {
    return (Vec){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec vec_unit(Vec a) {
    double n = sqrt(vec_dot(a, a));
    return n > 1e-12 ? vec_mul(a, 1 / n) : (Vec){0, 0, 0};
}
void mesh_init(Mesh *m) {
    memset(m, 0, sizeof *m);
    g_strlcpy(m->name, "MODEL", sizeof m->name);
    m->vertices = g_array_new(FALSE, FALSE, sizeof(Vec));
    m->uvs = g_array_new(FALSE, FALSE, sizeof(UV));
    m->faces = g_array_new(FALSE, FALSE, sizeof(Face));
}
void mesh_clear(Mesh *m) {
    g_clear_pointer(&m->vertices, g_array_unref);
    g_clear_pointer(&m->uvs, g_array_unref);
    g_clear_pointer(&m->faces, g_array_unref);
}
gboolean mesh_validate(const Mesh *m, gboolean ps1, GError **error) {
    if (m->vertices->len > LIMIT || m->faces->len > LIMIT || m->uvs->len > UV_LIMIT)
        return fail(error, "Model exceeds editor limits.");
    if (ps1 && (m->vertices->len > 2048 || m->uvs->len > 2048 || m->faces->len > 256))
        return fail(error, "PS1 limits: 2048 vertices, 2048 UVs, 256 quads.");
    for (guint i = 0; i < m->vertices->len; i++) {
        Vec p = AT(m->vertices, Vec, i);
        if (!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z) || fabs(p.x) > 1e6 ||
            fabs(p.y) > 1e6 || fabs(p.z) > 1e6)
            return fail(error, "Invalid vertex coordinates.");
    }
    for (guint i = 0; i < m->uvs->len; i++) {
        UV p = AT(m->uvs, UV, i);
        if (!isfinite(p.u) || !isfinite(p.v) || fabs(p.u) > 1e6 || fabs(p.v) > 1e6)
            return fail(error, "Invalid UV coordinates.");
    }
    for (guint i = 0; i < m->faces->len; i++) {
        Face *f = &AT(m->faces, Face, i);
        if (f->n < 3 || f->n > 4)
            return fail(error, "Faces must have three or four corners.");
        int unique = 0;
        for (int j = 0; j < f->n; j++) {
            if (f->v[j] < 0 || (guint)f->v[j] >= m->vertices->len || f->uv[j] < 0 ||
                (guint)f->uv[j] >= m->uvs->len)
                return fail(error, "Face index out of range.");
            gboolean seen = FALSE;
            for (int k = 0; k < j; k++)
                if (f->v[j] == f->v[k])
                    seen = TRUE;
            unique += !seen;
        }
        if (unique < 3)
            return fail(error, "Face has fewer than three distinct vertices.");
    }
    return TRUE;
}
Vec mesh_normal(const Mesh *m, const Face *f) {
    Vec n = {0, 0, 0};
    for (int j = 0; j < f->n; j++) {
        Vec a = AT(m->vertices, Vec, f->v[j]), b = AT(m->vertices, Vec, f->v[(j + 1) % f->n]);
        n.x += (a.y - b.y) * (a.z + b.z);
        n.y += (a.z - b.z) * (a.x + b.x);
        n.z += (a.x - b.x) * (a.y + b.y);
    }
    return vec_unit(n);
}
static double component(Vec v, int i) { return i == 0 ? v.x : i == 1 ? v.y : v.z; }
void mesh_map(Mesh *m, Face *f, int plane) {
    int a = 0, b = 1;
    if (plane == 0) {
        Vec n = mesh_normal(m, f);
        if (fabs(n.x) >= fabs(n.y) && fabs(n.x) >= fabs(n.z)) {
            a = 2;
            b = 1;
        } else if (fabs(n.y) >= fabs(n.z)) {
            a = 0;
            b = 2;
        }
    } else if (plane == 2) {
        a = 0;
        b = 2;
    } else if (plane == 3) {
        a = 2;
        b = 1;
    }
    double lo[2] = {G_MAXDOUBLE, G_MAXDOUBLE}, hi[2] = {-G_MAXDOUBLE, -G_MAXDOUBLE};
    for (int j = 0; j < f->n; j++) {
        Vec p = AT(m->vertices, Vec, f->v[j]);
        double v[] = {component(p, a), component(p, b)};
        for (int k = 0; k < 2; k++) {
            lo[k] = MIN(lo[k], v[k]);
            hi[k] = MAX(hi[k], v[k]);
        }
    }
    for (int j = 0; j < f->n; j++) {
        Vec p = AT(m->vertices, Vec, f->v[j]);
        UV uv = {(component(p, a) - lo[0]) / MAX(1e-12, hi[0] - lo[0]),
                 (component(p, b) - lo[1]) / MAX(1e-12, hi[1] - lo[1])};
        f->uv[j] = m->uvs->len;
        g_array_append_val(m->uvs, uv);
    }
}
static void add_face(Mesh *m, int base, int n, int a, int b, int c, int d) {
    Face f = {.n = n, .v = {base + a, base + b, base + c, base + d}};
    mesh_map(m, &f, 0);
    g_array_append_val(m->faces, f);
}
gboolean mesh_primitive(Mesh *m, int kind, double size, int sides, Vec offset, GError **error) {
    if (!isfinite(size) || size <= 0)
        return fail(error, "Size must be positive.");
    if (m->vertices->len + 26 > LIMIT || m->faces->len + 36 > LIMIT || m->uvs->len + 144 > UV_LIMIT)
        return fail(error, "No room for another primitive.");
    double h = size / 2;
    int base = m->vertices->len;
    if (kind == 5) {
        g_array_append_val(m->vertices, offset);
        return TRUE;
    }
    Vec cube[] = {{-h, -h, -h}, {h, -h, -h}, {h, h, -h}, {-h, h, -h},
                  {-h, -h, h},  {h, -h, h},  {h, h, h},  {-h, h, h}};
    if (kind == 0) {
        for (int i = 0; i < 8; i++) {
            Vec p = vec_add(cube[i], offset);
            g_array_append_val(m->vertices, p);
        }
        int f[][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3},
                      {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2}};
        for (int i = 0; i < 6; i++)
            add_face(m, base, 4, f[i][0], f[i][1], f[i][2], f[i][3]);
    } else if (kind == 1) {
        Vec v[] = {{-h, 0, -h}, {h, 0, -h}, {h, 0, h}, {-h, 0, h}};
        for (int i = 0; i < 4; i++) {
            Vec p = vec_add(v[i], offset);
            g_array_append_val(m->vertices, p);
        }
        add_face(m, base, 4, 0, 1, 2, 3);
    } else if (kind == 2 || kind == 3) {
        Vec v[] = {{-h, h, -h}, {h, h, -h}, {h, h, h}, {-h, h, h}, {0, -h, 0}};
        if (kind == 2) {
            for (int i = 0; i < 5; i++) {
                Vec p = vec_add(v[i], offset);
                g_array_append_val(m->vertices, p);
            }
            add_face(m, base, 4, 0, 3, 2, 1);
            for (int i = 0; i < 4; i++)
                add_face(m, base, 3, i, (i + 1) % 4, 4, 4);
        } else {
            Vec t[] = {{0, -h, 0}, {-h, h, -h}, {h, h, -h}, {0, h, h}};
            for (int i = 0; i < 4; i++) {
                Vec p = vec_add(t[i], offset);
                g_array_append_val(m->vertices, p);
            }
            add_face(m, base, 3, 1, 3, 2, 2);
            add_face(m, base, 3, 0, 1, 2, 2);
            add_face(m, base, 3, 0, 2, 3, 3);
            add_face(m, base, 3, 0, 3, 1, 1);
        }
    } else if (kind == 4) {
        sides = CLAMP(sides, 3, 12);
        for (int y = 0; y < 2; y++)
            for (int i = 0; i < sides; i++) {
                double a = i * 2 * G_PI / sides;
                Vec p = vec_add((Vec){cos(a) * h, y ? h : -h, sin(a) * h}, offset);
                g_array_append_val(m->vertices, p);
            }
        Vec p = vec_add((Vec){0, -h, 0}, offset);
        g_array_append_val(m->vertices, p);
        p = vec_add((Vec){0, h, 0}, offset);
        g_array_append_val(m->vertices, p);
        for (int i = 0; i < sides; i++) {
            int j = (i + 1) % sides;
            add_face(m, base, 4, i, i + sides, j + sides, j);
            add_face(m, base, 3, 2 * sides, i, j, j);
            add_face(m, base, 3, 2 * sides + 1, j + sides, i + sides, i + sides);
        }
    } else
        return fail(error, "Unknown primitive.");
    return TRUE;
}
static gboolean has(GArray *ids, int n) {
    for (guint i = 0; i < ids->len; i++)
        if (AT(ids, int, i) == n)
            return TRUE;
    return FALSE;
}
GArray *mesh_selected(Editor *e) {
    GArray *ids = g_array_new(FALSE, FALSE, sizeof(int));
    if (e->mode == 2) {
        for (guint i = 0; i < e->mesh.vertices->len; i++) {
            int n = i;
            g_array_append_val(ids, n);
        }
    } else if (e->mode == 0) {
        g_array_append_vals(ids, e->selection->data, e->selection->len);
    } else
        for (guint i = 0; i < e->selection->len; i++) {
            Face *f = &AT(e->mesh.faces, Face, AT(e->selection, int, i));
            for (int j = 0; j < f->n; j++)
                if (!has(ids, f->v[j]))
                    g_array_append_val(ids, f->v[j]);
        }
    return ids;
}
gboolean mesh_transform(Mesh *m, GArray *ids, int type, Vec values, Vec pivot, GError **error) {
    if (!isfinite(values.x) || !isfinite(values.y) || !isfinite(values.z))
        return fail(error, "Invalid transform.");
    GArray *points = g_array_new(FALSE, FALSE, sizeof(Vec));
    for (guint i = 0; i < ids->len; i++) {
        int n = AT(ids, int, i);
        if (n < 0 || (guint)n >= m->vertices->len) {
            g_array_unref(points);
            return fail(error, "Invalid selection.");
        }
        Vec p = AT(m->vertices, Vec, n), q = vec_sub(p, pivot);
        if (type == 0)
            p = vec_add(p, values);
        else {
            if (type == 2)
                q = (Vec){q.x * values.x, q.y * values.y, q.z * values.z};
            else {
                double a = values.x * G_PI / 180, b = values.y * G_PI / 180,
                       c = values.z * G_PI / 180;
                q = (Vec){q.x, q.y * cos(a) - q.z * sin(a), q.y * sin(a) + q.z * cos(a)};
                q = (Vec){q.x * cos(b) + q.z * sin(b), q.y, -q.x * sin(b) + q.z * cos(b)};
                q = (Vec){q.x * cos(c) - q.y * sin(c), q.x * sin(c) + q.y * cos(c), q.z};
            }
            p = vec_add(q, pivot);
        }
        if (!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z) || fabs(p.x) > 1e6 ||
            fabs(p.y) > 1e6 || fabs(p.z) > 1e6) {
            g_array_unref(points);
            return fail(error, "Transform exceeds coordinate limits.");
        }
        g_array_append_val(points, p);
    }
    for (guint i = 0; i < ids->len; i++)
        AT(m->vertices, Vec, AT(ids, int, i)) = AT(points, Vec, i);
    g_array_unref(points);
    return TRUE;
}
static void remove_vertices(Mesh *m, GArray *ids) {
    int *map = g_new(int, m->vertices->len);
    guint out = 0;
    for (guint i = 0; i < m->vertices->len; i++) {
        map[i] = has(ids, i) ? -1 : (int)out;
        if (map[i] >= 0)
            AT(m->vertices, Vec, out++) = AT(m->vertices, Vec, i);
    }
    for (guint i = 0; i < m->faces->len;) {
        Face *f = &AT(m->faces, Face, i);
        gboolean bad = FALSE;
        for (int j = 0; j < f->n; j++)
            if (map[f->v[j]] < 0)
                bad = TRUE;
        if (bad)
            g_array_remove_index(m->faces, i);
        else {
            for (int j = 0; j < f->n; j++)
                f->v[j] = map[f->v[j]];
            i++;
        }
    }
    g_array_set_size(m->vertices, out);
    g_free(map);
}
gboolean mesh_edit(Editor *e, int action, GError **error) {
    Mesh *m = &e->mesh;
    GArray *ids = mesh_selected(e);
    gboolean ok = TRUE;
    if (action == 0) {
        if (e->mode != 0 || ids->len < 3 || ids->len > 4)
            ok = fail(error, "Select 3 or 4 vertices in boundary order.");
        else {
            Face f = {.n = (int)ids->len};
            for (guint i = 0; i < ids->len; i++)
                f.v[i] = AT(ids, int, i);
            mesh_map(m, &f, 0);
            g_array_append_val(m->faces, f);
        }
    } else if (action == 1) {
        if (e->mode != 1 || !e->selection->len)
            ok = fail(error, "Select faces to flip.");
        else
            for (guint i = 0; i < e->selection->len; i++) {
                Face *f = &AT(m->faces, Face, AT(e->selection, int, i));
                for (int j = 0; j < f->n / 2; j++) {
                    int k = f->n - 1 - j, t = f->v[j];
                    f->v[j] = f->v[k];
                    f->v[k] = t;
                    t = f->uv[j];
                    f->uv[j] = f->uv[k];
                    f->uv[k] = t;
                }
            }
    } else if (action == 2) {
        int *map = g_new(int, m->vertices->len);
        for (guint i = 0; i < m->vertices->len; i++)
            map[i] = -1;
        for (guint i = 0; i < ids->len; i++) {
            int n = AT(ids, int, i);
            Vec p = AT(m->vertices, Vec, n);
            map[n] = m->vertices->len;
            g_array_append_val(m->vertices, p);
        }
        guint count = m->faces->len;
        for (guint i = 0; i < count; i++)
            if (e->mode == 2 || (e->mode == 1 && has(e->selection, i))) {
                Face f = AT(m->faces, Face, i);
                for (int j = 0; j < f.n; j++) {
                    f.v[j] = map[f.v[j]];
                    UV uv = AT(m->uvs, UV, f.uv[j]);
                    f.uv[j] = m->uvs->len;
                    g_array_append_val(m->uvs, uv);
                }
                g_array_append_val(m->faces, f);
            }
        g_array_set_size(e->selection, 0);
        for (guint i = 0; i < ids->len; i++) {
            int n = map[AT(ids, int, i)];
            g_array_append_val(e->selection, n);
        }
        e->mode = 0;
        g_free(map);
    } else if (action == 3) {
        if (e->mode == 1) {
            for (int i = (int)m->faces->len - 1; i >= 0; i--)
                if (has(e->selection, i))
                    g_array_remove_index(m->faces, i);
        } else
            remove_vertices(m, ids);
        g_array_set_size(e->selection, 0);
    } else if (action == 4) {
        if (e->mode != 0 || ids->len < 2)
            ok = fail(error, "Select at least two vertices to merge.");
        else {
            int keep = AT(ids, int, 0);
            Vec c = {0, 0, 0};
            for (guint i = 0; i < ids->len; i++)
                c = vec_add(c, AT(m->vertices, Vec, AT(ids, int, i)));
            AT(m->vertices, Vec, keep) = vec_mul(c, 1.0 / ids->len);
            for (guint i = 0; i < m->faces->len;) {
                Face *f = &AT(m->faces, Face, i);
                int n = 0;
                for (int j = 0; j < f->n; j++) {
                    int v = has(ids, f->v[j]) ? keep : f->v[j];
                    gboolean seen = FALSE;
                    for (int k = 0; k < n; k++)
                        if (f->v[k] == v)
                            seen = TRUE;
                    if (!seen) {
                        f->v[n] = v;
                        f->uv[n++] = f->uv[j];
                    }
                }
                f->n = n;
                if (n < 3)
                    g_array_remove_index(m->faces, i);
                else
                    i++;
            }
            g_array_remove_index(ids, 0);
            remove_vertices(m, ids);
            g_array_set_size(e->selection, 0);
        }
    } else if (action == 5) {
        GArray *unused = g_array_new(FALSE, FALSE, sizeof(int));
        gboolean *used = g_new0(gboolean, m->vertices->len);
        for (guint i = 0; i < m->faces->len; i++) {
            Face *f = &AT(m->faces, Face, i);
            for (int j = 0; j < f->n; j++)
                used[f->v[j]] = TRUE;
        }
        for (guint i = 0; i < m->vertices->len; i++)
            if (!used[i]) {
                int n = i;
                g_array_append_val(unused, n);
            }
        remove_vertices(m, unused);
        g_free(used);
        g_array_unref(unused);
        int *map = g_new(int, m->uvs->len);
        for (guint i = 0; i < m->uvs->len; i++)
            map[i] = -1;
        GArray *uvs = g_array_new(FALSE, FALSE, sizeof(UV));
        for (guint i = 0; i < m->faces->len; i++) {
            Face *f = &AT(m->faces, Face, i);
            for (int j = 0; j < f->n; j++) {
                int n = f->uv[j];
                if (map[n] < 0) {
                    map[n] = uvs->len;
                    UV uv = AT(m->uvs, UV, n);
                    g_array_append_val(uvs, uv);
                }
                f->uv[j] = map[n];
            }
        }
        g_array_unref(m->uvs);
        m->uvs = uvs;
        g_free(map);
        g_array_set_size(e->selection, 0);
    } else if (action == 6) {
        if (e->mode != 1 || e->selection->len != 1)
            ok = fail(error, "Select one face to extrude.");
        else {
            int index = AT(e->selection, int, 0);
            Face f = AT(m->faces, Face, index);
            Vec normal = mesh_normal(m, &f);
            double d = gtk_spin_button_get_value(GTK_SPIN_BUTTON(e->distance));
            if (vec_dot(normal, normal) < .5 || fabs(d) < 1e-8)
                ok = fail(error, "Face has no normal, or extrusion is zero.");
            else {
                Face top = f;
                for (int j = 0; j < f.n; j++) {
                    Vec p = vec_add(AT(m->vertices, Vec, f.v[j]), vec_mul(normal, d));
                    top.v[j] = m->vertices->len;
                    g_array_append_val(m->vertices, p);
                }
                AT(m->faces, Face, index) = top;
                for (int j = 0; j < f.n; j++) {
                    int k = (j + 1) % f.n;
                    Face side = {.n = 4, .v = {f.v[j], f.v[k], top.v[k], top.v[j]}};
                    mesh_map(m, &side, 0);
                    g_array_append_val(m->faces, side);
                }
            }
        }
    }
    g_array_unref(ids);
    return ok && mesh_validate(m, FALSE, error);
}
