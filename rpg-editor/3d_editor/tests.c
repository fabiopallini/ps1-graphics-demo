#include "editor.h"
static void roundtrip(void) {
    for (int kind = 0; kind < 5; kind++) {
        Mesh m, n;
        mesh_init(&m);
        mesh_init(&n);
        GError *error = NULL;
        g_assert_true(mesh_primitive(&m, kind, 1, 6, (Vec){0, 0, 0}, &error));
        g_assert_no_error(error);
        char *obj = obj_export(&m, TRUE, &error);
        g_assert_nonnull(obj);
        g_assert_no_error(error);
        g_assert_true(obj_parse(obj, &n, &error));
        g_assert_no_error(error);
        g_assert_cmpuint(m.vertices->len, ==, n.vertices->len);
        g_assert_cmpuint(m.faces->len, ==, n.faces->len);
        for (guint i = 0; i < m.faces->len; i++) {
            Face a = AT(m.faces, Face, i), b = AT(n.faces, Face, i);
            g_assert_cmpint(b.n, ==, 4);
            for (int j = 0; j < 4; j++) {
                int k = MIN(j, a.n - 1);
                g_assert_cmpint(a.v[k], ==, b.v[j]);
                g_assert_cmpint(a.uv[k], ==, b.uv[j]);
            }
        }
        g_free(obj);
        mesh_clear(&m);
        mesh_clear(&n);
    }
}
static void parsing(void) {
    Mesh m;
    mesh_init(&m);
    GError *e = NULL;
    g_assert_true(obj_parse("v 0 0 0\nv 1 0 0\nv 0 1 0\nvt nan nan\nf -3/1 -2/1 -1/1\n", &m, &e));
    g_assert_no_error(e);
    g_assert_cmpfloat(AT(m.uvs, UV, 0).u, ==, 0);
    g_assert_false(obj_parse("v 0 0 0\nf 1 2 3", &m, &e));
    g_clear_error(&e);
    g_assert_cmpuint(m.vertices->len, ==, 3);
    mesh_clear(&m);
}
static void project(void) {
    Editor a = {0}, b = {0};
    mesh_init(&a.mesh);
    mesh_init(&b.mesh);
    a.ydown = TRUE;
    a.yaw = .65;
    a.pitch = .35;
    a.span = 3;
    mesh_primitive(&a.mesh, 0, 1, 6, (Vec){2, 0, 0}, NULL);
    char *text = project_encode(&a);
    GError *err = NULL;
    g_assert_true(project_decode(&b, text, &err));
    g_assert_no_error(err);
    g_assert_cmpuint(b.mesh.faces->len, ==, 6);
    g_assert_true(b.ydown);
    g_assert_cmpfloat(b.span, ==, 3);
    char *out = project_encode(&b);
    g_assert_cmpstr(text, ==, out);
    g_assert_false(project_decode(
        &b,
        "{\"format\":\"psx3dstudio-1\",\"model\":{\"vertices\":[],\"uvs\":[],\"faces\":[null]}}",
        &err));
    g_clear_error(&err);
    g_assert_cmpuint(b.mesh.faces->len, ==, 6);
    g_free(text);
    g_free(out);
    mesh_clear(&a.mesh);
    mesh_clear(&b.mesh);
}
static void transform(void) {
    Mesh m;
    mesh_init(&m);
    mesh_primitive(&m, 0, 1, 6, (Vec){0, 0, 0}, NULL);
    GArray *ids = g_array_new(FALSE, FALSE, sizeof(int));
    int n = 0;
    g_array_append_val(ids, n);
    GError *err = NULL;
    g_assert_true(mesh_transform(&m, ids, 0, (Vec){1, 2, 3}, (Vec){0, 0, 0}, &err));
    Vec p = AT(m.vertices, Vec, 0);
    g_assert_cmpfloat(p.x, ==, .5);
    g_assert_cmpfloat(p.y, ==, 1.5);
    g_assert_cmpfloat(p.z, ==, 2.5);
    g_assert_false(mesh_transform(&m, ids, 2, (Vec){1e7, 1e7, 1e7}, (Vec){0, 0, 0}, &err));
    g_clear_error(&err);
    g_assert_cmpfloat(AT(m.vertices, Vec, 0).x, ==, .5);
    g_array_unref(ids);
    mesh_clear(&m);
}
static void edits(void) {
    Editor e = {0};
    mesh_init(&e.mesh);
    mesh_primitive(&e.mesh, 0, 1, 6, (Vec){0, 0, 0}, NULL);
    e.selection = g_array_new(FALSE, FALSE, sizeof(int));
    e.mode = 2;
    GError *err = NULL;
    g_assert_true(mesh_edit(&e, 2, &err));
    g_assert_no_error(err);
    g_assert_cmpuint(e.mesh.faces->len, ==, 12);
    g_assert_cmpuint(e.mesh.vertices->len, ==, 16);
    g_assert_cmpint(e.mode, ==, 0);
    g_assert_true(mesh_edit(&e, 3, &err));
    g_assert_cmpuint(e.mesh.vertices->len, ==, 8);
    g_assert_cmpuint(e.mesh.faces->len, ==, 6);
    g_assert_true(mesh_edit(&e, 5, &err));
    g_assert_cmpuint(e.mesh.uvs->len, ==, 24);
    g_array_unref(e.selection);
    mesh_clear(&e.mesh);
}
static void limits(void) {
    Mesh m;
    mesh_init(&m);
    for (int i = 0; i < 43; i++)
        mesh_primitive(&m, 0, 1, 6, (Vec){i * 2, 0, 0}, NULL);
    GError *err = NULL;
    char *obj = obj_export(&m, TRUE, &err);
    g_assert_null(obj);
    g_assert_nonnull(err);
    g_clear_error(&err);
    obj = obj_export(&m, FALSE, &err);
    g_assert_nonnull(obj);
    g_assert_no_error(err);
    g_free(obj);
    mesh_clear(&m);
}
static void game_assets(void) {
    const char *dir = "../../data/chars/char1";
    if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
        dir = "data/chars/char1";
    GError *err = NULL;
    GDir *d = g_dir_open(dir, 0, &err);
    g_assert_no_error(err);
    g_assert_nonnull(d);
    const char *name;
    int count = 0;
    while ((name = g_dir_read_name(d))) {
        if (!g_str_has_suffix(name, ".obj"))
            continue;
        char *path = g_build_filename(dir, name, NULL), *text = NULL;
        g_assert_true(g_file_get_contents(path, &text, NULL, &err));
        g_assert_no_error(err);
        Mesh m, n;
        mesh_init(&m);
        mesh_init(&n);
        g_assert_true(obj_parse(text, &m, &err));
        g_assert_no_error(err);
        char *out = obj_export(&m, TRUE, &err);
        g_assert_no_error(err);
        g_assert_nonnull(out);
        g_assert_true(obj_parse(out, &n, &err));
        g_assert_no_error(err);
        g_assert_cmpuint(m.faces->len, ==, n.faces->len);
        for (guint i = 0; i < m.faces->len; i++) {
            Face a = AT(m.faces, Face, i), b = AT(n.faces, Face, i);
            for (int j = 0; j < a.n; j++) {
                g_assert_cmpint(a.v[j], ==, b.v[j]);
                g_assert_cmpint(a.uv[j], ==, b.uv[j]);
            }
        }
        mesh_clear(&m);
        mesh_clear(&n);
        g_free(out);
        g_free(text);
        g_free(path);
        count++;
    }
    g_dir_close(d);
    g_assert_cmpint(count, >, 0);
    g_test_message("Checked %d real character OBJ frames", count);
}
int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/obj/primitive-roundtrip", roundtrip);
    g_test_add_func("/obj/negative-nan-errors", parsing);
    g_test_add_func("/project/compatibility-atomic-load", project);
    g_test_add_func("/mesh/transform-atomic", transform);
    g_test_add_func("/mesh/duplicate-delete-cleanup", edits);
    g_test_add_func("/obj/ps1-limits", limits);
    g_test_add_func("/obj/game-animation-frames", game_assets);
    return g_test_run();
}
