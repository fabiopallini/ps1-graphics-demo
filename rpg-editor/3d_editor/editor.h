#ifndef PSX_MODEL_EDITOR_H
#define PSX_MODEL_EDITOR_H
#include <epoxy/gl.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <math.h>
#include <string.h>
#define LIMIT 20000
#define UV_LIMIT 80000
#define AT(a, t, i) g_array_index((a), t, (i))
typedef struct {
    double x, y, z;
} Vec;
typedef struct {
    double u, v;
} UV;
typedef struct {
    int n, v[4], uv[4];
    char group[49];
} Face;
typedef struct {
    char name[49];
    GArray *vertices, *uvs, *faces;
} Mesh;
typedef struct {
    double x, y, depth, scale;
    gboolean visible;
} Projected;
typedef struct Editor Editor;
struct Editor {
    GtkApplication *application;
    GtkWindow *window;
    GtkWidget *area, *status, *info, *uv_area, *name;
    GtkWidget *mode_drop, *view_drop, *shade_drop, *axis_drop, *transform_drop, *pivot_drop;
    GtkWidget *snap, *size, *sides, *distance, *position[3], *transform[3], *vertex[3],
        *uv_fields[4][2];
    GtkWidget *grid_check, *edge_check, *ydown_check, *format_drop;
    GtkWidget *tool_buttons[2], *undo_button, *redo_button;
    Mesh mesh;
    GArray *selection;
    GPtrArray *undo, *redo;
    char *path, *texture_name, *texture_data, *notice, *smoke_path;
    GdkPixbuf *texture;
    gboolean dirty, refreshing, ready, grid, edges, ydown, move, drag_changed;
    int mode, view, shade, axis, drag_kind, uv_corner;
    double yaw, pitch, span;
    Vec center;
    double start_x, start_y, last_x, last_y;
    char *drag_snapshot;
    GLuint program, vao, vbo, tex;
    GLint textured;
    unsigned frames;
    int exit_code;
};
Vec vec_add(Vec a, Vec b);
Vec vec_sub(Vec a, Vec b);
Vec vec_mul(Vec a, double k);
double vec_dot(Vec a, Vec b);
Vec vec_cross(Vec a, Vec b);
Vec vec_unit(Vec a);
void mesh_init(Mesh *m);
void mesh_clear(Mesh *m);
gboolean mesh_validate(const Mesh *m, gboolean ps1, GError **error);
Vec mesh_normal(const Mesh *m, const Face *f);
void mesh_map(Mesh *m, Face *f, int plane);
gboolean mesh_primitive(Mesh *m, int kind, double size, int sides, Vec offset, GError **error);
GArray *mesh_selected(Editor *e);
gboolean mesh_transform(Mesh *m, GArray *ids, int type, Vec values, Vec pivot, GError **error);
gboolean mesh_edit(Editor *e, int action, GError **error);
/* Actions: create=0, flip=1, duplicate=2, delete=3, merge=4, cleanup=5, extrude=6. */
gboolean obj_parse(const char *text, Mesh *m, GError **error);
char *obj_export(const Mesh *m, gboolean ps1, GError **error);
char *project_encode(Editor *e);
gboolean project_decode(Editor *e, const char *text, GError **error);
gboolean texture_decode(const char *data, GdkPixbuf **image, GError **error);
void editor_notice(Editor *e, const char *message);
void editor_refresh(Editor *e);
void editor_commit(Editor *e, char *before);
void editor_history(Editor *e, gboolean redo);
void editor_frame(Editor *e);
void render_connect(Editor *e);
void render_queue(Editor *e);
Projected render_project(Editor *e, Vec p);
void render_basis(Editor *e, Vec *right, Vec *up, Vec *forward);
void ui_create(Editor *e);
void ui_smoke(Editor *e);
void ui_texture_draw(GtkDrawingArea *a, cairo_t *cr, int w, int h, gpointer data);
void ui_file(Editor *e, int action); /* open=0, save=1, OBJ=2, PNG=3, MTL=4 */
#endif
