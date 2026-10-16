#include "editor.h"
#include <errno.h>
static gboolean bad(GError **err, const char *message) {
    g_set_error_literal(err, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, message);
    return FALSE;
}
static gboolean idx(const char *s, guint count, int *out) {
    char *end;
    long n = strtol(s, &end, 10);
    if (end == s || *end || !n)
        return FALSE;
    n = n > 0 ? n - 1 : (long)count + n;
    if (n < 0 || n >= count)
        return FALSE;
    *out = n;
    return TRUE;
}
gboolean obj_parse(const char *text, Mesh *dest, GError **error) {
    Mesh m;
    mesh_init(&m);
    char **lines = g_strsplit(text, "\n", -1);
    gboolean ok = TRUE;
    char group[49] = "";
    for (int l = 0; lines[l] && ok; l++) {
        char *line = lines[l], *hash = strchr(line, '#');
        if (hash)
            *hash = 0;
        g_strstrip(line);
        if (l == 0 && g_str_has_prefix(line, "\357\273\277"))
            line += 3;
        char **raw = g_strsplit_set(line, " \t\r", -1);
        GPtrArray *t = g_ptr_array_new();
        for (int i = 0; raw[i]; i++)
            if (*raw[i])
                g_ptr_array_add(t, raw[i]);
        if (t->len) {
            const char *kind = g_ptr_array_index(t, 0);
            if (!strcmp(kind, "v") || !strcmp(kind, "vt")) {
                gboolean vertex = !strcmp(kind, "v");
                double p[3] = {0, 0, 0};
                int count = vertex ? 3 : 2;
                if (t->len < (guint)count + 1)
                    ok = FALSE;
                else
                    for (int j = 0; j < count; j++) {
                        char *end;
                        const char *s = g_ptr_array_index(t, j + 1);
                        p[j] = g_ascii_strtod(s, &end);
                        if (!vertex && isnan(p[j]))
                            p[j] = 0;
                        if (*end || end == s || !isfinite(p[j]))
                            ok = FALSE;
                    }
                if (ok) {
                    if (vertex) {
                        Vec v = {p[0], p[1], p[2]};
                        g_array_append_val(m.vertices, v);
                    } else {
                        UV uv = {p[0], p[1]};
                        g_array_append_val(m.uvs, uv);
                    }
                }
            } else if (!strcmp(kind, "g") || !strcmp(kind, "o")) {
                g_strlcpy(group, t->len > 1 ? g_ptr_array_index(t, 1) : "", sizeof group);
            } else if (!strcmp(kind, "f")) {
                if (t->len < 4 || t->len > 5) {
                    ok = FALSE;
                } else {
                    Face f = {.n = (int)t->len - 1};
                    g_strlcpy(f.group, group, sizeof f.group);
                    gboolean map = FALSE;
                    for (int j = 0; j < f.n; j++) {
                        char **pair = g_strsplit(g_ptr_array_index(t, j + 1), "/", -1);
                        if (!idx(pair[0], m.vertices->len, &f.v[j]))
                            ok = FALSE;
                        if (!pair[1] || !*pair[1]) {
                            map = TRUE;
                            f.uv[j] = -1;
                        } else if (!idx(pair[1], m.uvs->len, &f.uv[j]))
                            ok = FALSE;
                        g_strfreev(pair);
                    }
                    if (ok) {
                        if (map)
                            mesh_map(&m, &f, 0);
                        g_array_append_val(m.faces, f);
                    }
                }
            }
        }
        if (m.vertices->len > LIMIT || m.faces->len > LIMIT || m.uvs->len > UV_LIMIT)
            ok = FALSE;
        g_ptr_array_unref(t);
        g_strfreev(raw);
        if (!ok) {
            char *message =
                g_strdup_printf("OBJ line %d: invalid data or polygon with more than four corners. "
                                "Triangulate larger polygons before importing.",
                                l + 1);
            bad(error, message);
            g_free(message);
        }
    }
    g_strfreev(lines);
    if (ok && !m.vertices->len)
        ok = bad(error, "OBJ contains no vertices.");
    if (ok)
        ok = mesh_validate(&m, FALSE, error);
    if (ok) {
        mesh_clear(dest);
        *dest = m;
    } else
        mesh_clear(&m);
    return ok;
}
static gboolean number_text(double value, gboolean ps1, char out[64]) {
    for (int precision = 6; precision >= 0; precision--) {
        char format[16];
        g_snprintf(format, sizeof format, "%%.%df", precision);
        g_ascii_formatd(out, 64, format, value);
        char *dot = strchr(out, '.');
        if (dot) {
            char *end = out + strlen(out) - 1;
            while (end > dot && *end == '0')
                *end-- = 0;
            if (end == dot)
                *end = 0;
        }
        if (!strcmp(out, "-0"))
            strcpy(out, "0");
        if (!ps1 || strlen(out) <= 9)
            return TRUE;
    }
    return FALSE;
}
static char *safe_name(const char *name) {
    char *s = g_strdup(name);
    for (char *p = s; *p; p++)
        if (!g_ascii_isalnum(*p) && *p != '_' && *p != '-')
            *p = '_';
    return s;
}
char *obj_export(const Mesh *m, gboolean ps1, GError **error) {
    if (!mesh_validate(m, ps1, error))
        return NULL;
    if (!m->faces->len) {
        bad(error, "Add a face before exporting.");
        return NULL;
    }
    char *name = safe_name(m->name);
    GString *out = g_string_new("# PSX 3D Editor\n");
    g_string_append_printf(out, "o %s\n", name);
    g_free(name);
    for (int k = 0; k < 2; k++) {
        guint count = k ? m->uvs->len : m->vertices->len;
        for (guint i = 0; i < count; i++) {
            double p[3];
            int n = k ? 2 : 3;
            if (k) {
                UV uv = AT(m->uvs, UV, i);
                p[0] = uv.u;
                p[1] = uv.v;
            } else {
                Vec v = AT(m->vertices, Vec, i);
                p[0] = v.x;
                p[1] = v.y;
                p[2] = v.z;
            }
            g_string_append(out, k ? "vt" : "v");
            for (int j = 0; j < n; j++) {
                char value[64];
                if (!number_text(p[j], ps1, value)) {
                    bad(error, "Coordinate exceeds PS1's 9-character limit.");
                    g_string_free(out, TRUE);
                    return NULL;
                }
                g_string_append_printf(out, " %s", value);
            }
            g_string_append_c(out, '\n');
        }
    }
    char group[49] = "";
    for (guint i = 0; i < m->faces->len; i++) {
        Face f = AT(m->faces, Face, i);
        if (!ps1 && *f.group && strcmp(group, f.group)) {
            g_strlcpy(group, f.group, sizeof group);
            char *s = safe_name(group);
            g_string_append_printf(out, "g %s\n", s);
            g_free(s);
        }
        g_string_append(out, "f");
        for (int j = 0; j < (ps1 ? 4 : f.n); j++) {
            int k = MIN(j, f.n - 1);
            g_string_append_printf(out, " %d/%d", f.v[k] + 1, f.uv[k] + 1);
        }
        g_string_append_c(out, '\n');
    }
    return g_string_free(out, FALSE);
}
static JsonArray *triple(Vec p) {
    JsonArray *a = json_array_new();
    json_array_add_double_element(a, p.x);
    json_array_add_double_element(a, p.y);
    json_array_add_double_element(a, p.z);
    return a;
}
char *project_encode(Editor *e) {
    JsonObject *root = json_object_new(), *model = json_object_new(), *camera = json_object_new();
    json_object_set_string_member(root, "format", "psx3dstudio-1");
    json_object_set_string_member(model, "name", e->mesh.name);
    JsonArray *vertices = json_array_new(), *uvs = json_array_new(), *faces = json_array_new();
    for (guint i = 0; i < e->mesh.vertices->len; i++)
        json_array_add_array_element(vertices, triple(AT(e->mesh.vertices, Vec, i)));
    for (guint i = 0; i < e->mesh.uvs->len; i++) {
        UV p = AT(e->mesh.uvs, UV, i);
        JsonArray *a = json_array_new();
        json_array_add_double_element(a, p.u);
        json_array_add_double_element(a, p.v);
        json_array_add_array_element(uvs, a);
    }
    for (guint i = 0; i < e->mesh.faces->len; i++) {
        Face f = AT(e->mesh.faces, Face, i);
        JsonObject *o = json_object_new();
        JsonArray *v = json_array_new(), *uv = json_array_new();
        for (int j = 0; j < f.n; j++) {
            json_array_add_int_element(v, f.v[j]);
            json_array_add_int_element(uv, f.uv[j]);
        }
        json_object_set_array_member(o, "v", v);
        json_object_set_array_member(o, "uv", uv);
        json_object_set_string_member(o, "group", f.group);
        json_array_add_object_element(faces, o);
    }
    json_object_set_array_member(model, "vertices", vertices);
    json_object_set_array_member(model, "uvs", uvs);
    json_object_set_array_member(model, "faces", faces);
    json_object_set_object_member(root, "model", model);
    if (e->texture_data) {
        JsonObject *t = json_object_new();
        json_object_set_string_member(t, "name", e->texture_name ? e->texture_name : "TEX.png");
        json_object_set_string_member(t, "data", e->texture_data);
        json_object_set_object_member(root, "texture", t);
    } else
        json_object_set_null_member(root, "texture");
    json_object_set_double_member(camera, "yaw", e->yaw);
    json_object_set_double_member(camera, "pitch", e->pitch);
    json_object_set_double_member(camera, "span", e->span);
    json_object_set_double_member(camera, "distance", e->span * 1.5);
    json_object_set_array_member(camera, "center", triple(e->center));
    json_object_set_object_member(root, "camera", camera);
    const char *views[] = {"perspective", "front", "side", "top"};
    json_object_set_string_member(root, "view", views[CLAMP(e->view, 0, 3)]);
    json_object_set_boolean_member(root, "yDown", e->ydown);
    JsonNode *node = json_node_new(JSON_NODE_OBJECT);
    json_node_take_object(node, root);
    JsonGenerator *g = json_generator_new();
    json_generator_set_root(g, node);
    json_generator_set_pretty(g, TRUE);
    char *text = json_generator_to_data(g, NULL);
    g_object_unref(g);
    json_node_free(node);
    return text;
}
static JsonArray *arr(JsonObject *o, const char *name) {
    JsonNode *n = json_object_get_member(o, name);
    return n && JSON_NODE_HOLDS_ARRAY(n) ? json_node_get_array(n) : NULL;
}
static JsonObject *object(JsonObject *o, const char *name) {
    JsonNode *n = json_object_get_member(o, name);
    return n && JSON_NODE_HOLDS_OBJECT(n) ? json_node_get_object(n) : NULL;
}
static const char *str(JsonObject *o, const char *name) {
    JsonNode *n = json_object_get_member(o, name);
    return n && JSON_NODE_HOLDS_VALUE(n) && json_node_get_value_type(n) == G_TYPE_STRING
               ? json_node_get_string(n)
               : NULL;
}
static gboolean num(JsonNode *n, double *value) {
    if (!n || !JSON_NODE_HOLDS_VALUE(n))
        return FALSE;
    GType t = json_node_get_value_type(n);
    if (t != G_TYPE_INT64 && t != G_TYPE_DOUBLE)
        return FALSE;
    *value = json_node_get_double(n);
    return isfinite(*value) && fabs(*value) <= 1e6;
}
static gboolean values(JsonArray *a, int n, double *p) {
    if (!a || json_array_get_length(a) != (guint)n)
        return FALSE;
    for (int i = 0; i < n; i++)
        if (!num(json_array_get_element(a, i), p + i))
            return FALSE;
    return TRUE;
}
gboolean texture_decode(const char *data, GdkPixbuf **image, GError **error) {
    const char *prefix = "data:image/png;base64,";
    if (!g_str_has_prefix(data, prefix) || strlen(data) > 32 * 1024 * 1024)
        return bad(error, "Invalid or oversized PNG data.");
    gsize size;
    guchar *bytes = g_base64_decode(data + strlen(prefix), &size);
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type("png", error);
    if (!loader) {
        g_free(bytes);
        return FALSE;
    }
    gboolean ok = gdk_pixbuf_loader_write(loader, bytes, size, error);
    if (ok)
        ok = gdk_pixbuf_loader_close(loader, error);
    else
        gdk_pixbuf_loader_close(loader, NULL);
    if (ok) {
        GdkPixbuf *p = gdk_pixbuf_loader_get_pixbuf(loader);
        if (!p || gdk_pixbuf_get_width(p) > 2048 || gdk_pixbuf_get_height(p) > 2048)
            ok = bad(error, "Texture limit: 2048 x 2048 pixels.");
        else
            *image = g_object_ref(p);
    }
    g_object_unref(loader);
    g_free(bytes);
    return ok;
}
gboolean project_decode(Editor *e, const char *text, GError **error) {
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, text, -1, error)) {
        g_object_unref(parser);
        return FALSE;
    }
    JsonNode *node = json_parser_get_root(parser);
    JsonObject *root = JSON_NODE_HOLDS_OBJECT(node) ? json_node_get_object(node) : NULL;
    const char *format = root ? str(root, "format") : NULL;
    JsonObject *model = root ? object(root, "model") : NULL;
    if (!format || (strcmp(format, "psx3dstudio-1") && strcmp(format, "crystal-modeler-1")) ||
        !model) {
        g_object_unref(parser);
        return bad(error, "Not a PSX 3D Studio project.");
    }
    Mesh m;
    mesh_init(&m);
    const char *name = str(model, "name");
    if (name)
        g_strlcpy(m.name, name, sizeof m.name);
    gboolean ok = TRUE;
    for (int k = 0; k < 2 && ok; k++) {
        JsonArray *a = arr(model, k ? "uvs" : "vertices");
        if (!a || json_array_get_length(a) > (guint)(k ? UV_LIMIT : LIMIT)) {
            ok = FALSE;
            break;
        }
        for (guint i = 0; i < json_array_get_length(a) && ok; i++) {
            JsonNode *n = json_array_get_element(a, i);
            double p[3];
            if (!JSON_NODE_HOLDS_ARRAY(n) || !values(json_node_get_array(n), k ? 2 : 3, p)) {
                ok = FALSE;
                break;
            }
            if (k) {
                UV uv = {p[0], p[1]};
                g_array_append_val(m.uvs, uv);
            } else {
                Vec v = {p[0], p[1], p[2]};
                g_array_append_val(m.vertices, v);
            }
        }
    }
    JsonArray *faces = arr(model, "faces");
    if (!faces || json_array_get_length(faces) > LIMIT)
        ok = FALSE;
    for (guint i = 0; ok && i < json_array_get_length(faces); i++) {
        JsonNode *n = json_array_get_element(faces, i);
        if (!JSON_NODE_HOLDS_OBJECT(n)) {
            ok = FALSE;
            break;
        }
        JsonObject *o = json_node_get_object(n);
        JsonArray *v = arr(o, "v"), *uv = arr(o, "uv");
        Face f = {0};
        if (!v || !uv || (f.n = json_array_get_length(v)) < 3 || f.n > 4 ||
            json_array_get_length(uv) != (guint)f.n) {
            ok = FALSE;
            break;
        }
        for (int j = 0; j < f.n; j++) {
            double a, b;
            if (!num(json_array_get_element(v, j), &a) || !num(json_array_get_element(uv, j), &b) ||
                a != floor(a) || b != floor(b)) {
                ok = FALSE;
                break;
            }
            f.v[j] = (int)a;
            f.uv[j] = (int)b;
        }
        const char *group = str(o, "group");
        if (group)
            g_strlcpy(f.group, group, sizeof f.group);
        if (ok)
            g_array_append_val(m.faces, f);
    }
    if (!ok)
        bad(error, "Invalid project geometry.");
    else
        ok = mesh_validate(&m, FALSE, error);
    GdkPixbuf *image = NULL;
    char *data = NULL, *texname = NULL;
    JsonNode *tn = json_object_get_member(root, "texture");
    if (ok && tn && !JSON_NODE_HOLDS_NULL(tn)) {
        JsonObject *t = object(root, "texture");
        const char *d = t ? str(t, "data") : NULL;
        if (!d)
            ok = bad(error, "Invalid project texture.");
        else if ((ok = texture_decode(d, &image, error))) {
            data = g_strdup(d);
            texname = g_strdup(str(t, "name") ? str(t, "name") : "TEX.png");
        }
    }
    if (ok) {
        mesh_clear(&e->mesh);
        e->mesh = m;
        g_clear_object(&e->texture);
        e->texture = image;
        g_free(e->texture_data);
        e->texture_data = data;
        g_free(e->texture_name);
        e->texture_name = texname;
        e->shade = image ? 2 : 0;
        JsonNode *yd = json_object_get_member(root, "yDown");
        e->ydown = yd && JSON_NODE_HOLDS_VALUE(yd) && json_node_get_value_type(yd) == G_TYPE_BOOLEAN
                       ? json_node_get_boolean(yd)
                       : TRUE;
        const char *view = str(root, "view");
        e->view = view && !strcmp(view, "front")  ? 1
                  : view && !strcmp(view, "side") ? 2
                  : view && !strcmp(view, "top")  ? 3
                                                  : 0;
        JsonObject *cam = object(root, "camera");
        e->yaw = .65;
        e->pitch = .35;
        e->span = 3;
        e->center = (Vec){0, 0, 0};
        if (cam) {
            double value, p[3];
            if (num(json_object_get_member(cam, "yaw"), &value))
                e->yaw = value;
            if (num(json_object_get_member(cam, "pitch"), &value))
                e->pitch = CLAMP(value, -1.5, 1.5);
            if (num(json_object_get_member(cam, "span"), &value))
                e->span = CLAMP(value, .01, 1e6);
            if (values(arr(cam, "center"), 3, p))
                e->center = (Vec){p[0], p[1], p[2]};
        }
    } else {
        mesh_clear(&m);
        g_clear_object(&image);
        g_free(data);
        g_free(texname);
    }
    g_object_unref(parser);
    return ok;
}
