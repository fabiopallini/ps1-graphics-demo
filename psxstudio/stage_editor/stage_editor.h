#ifndef PSX_STAGE_EDITOR_H
#define PSX_STAGE_EDITOR_H
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <epoxy/gl.h>
#include <math.h>
#include <stdint.h>

typedef struct { double x, y, z; } Point;
typedef struct { double x, y; } Screen;
typedef struct { gboolean top; double x, z, scale; } View;
typedef struct {
    JsonNode *data;
    char *path, *root, *etag, *saved;
    GPtrArray *undo, *redo;
} Document;
typedef struct { GdkPixbuf *image; char *directory; } Background;
typedef struct {
    char *root, *directory;
    GHashTable *cache, *files;
} Assets;
typedef struct {
    GLuint program, vao, vbo, background, labels;
    GLint tint, textured;
    Background *uploaded;
    gboolean ready;
    unsigned frames;
} Renderer;
typedef struct Studio Studio;
struct Studio {
    GtkApplication *application;
    GtkWidget *window, *area, *stages, *inspector, *search, *status, *messages;
    GtkWidget *title, *coordinates, *zoom_label, *viewport, *snap_widget;
    GtkWidget *tool_buttons[4], *view_buttons[2], *undo_button, *redo_button;
    Document doc;
    Assets assets;
    Renderer renderer;
    View view;
    int sid, kind, index, tool, snap;
    gboolean layers[3], refreshing, dragging, rebuilding, queued_inspector, quitting, smoke_failed;
    int drag_type, corner;
    Point start;
    Screen screen_start;
    double drag_x, drag_y, zoom;
    JsonNode *original, *preview;
    char *before, *notice, *smoke_path;
    unsigned smoke_step;
    GdkPaintable *capture;
};
extern const char *const kinds[3];
extern const int limits[3];
double number(JsonObject *o, const char *key);
const char *string(JsonObject *o, const char *key);
JsonArray *array(JsonObject *o, const char *key);
JsonObject *stage(Document *d, int sid);
JsonObject *item(Document *d, int sid, int kind, int index);
JsonNode *object_node(JsonObject *o);
JsonNode *clone_node(JsonNode *node);
char *serialize(Document *d);
gboolean restore(Document *d, const char *text, GError **error);
gboolean document_load(Document *d, const char *path, GError **error);
gboolean document_save(Document *d, GError **error);
void document_clear(Document *d);
gboolean document_dirty(Document *d);
void document_changed(Document *d, char *before);
void document_undo(Document *d, gboolean redo);
char *validate(JsonNode *data);
char *warnings(Document *d, int sid);
gboolean delete_item(Document *d, int sid, int kind, int index, GError **error);
void rotation(JsonObject *s, double r[3][3]);
Point coordinates(JsonObject *o);
gboolean project(JsonObject *s, View *v, Point p, Screen *result);
gboolean unproject(JsonObject *s, View *v, Screen p, double y, Point *result);
void corners(JsonObject *o, Point p[4], gboolean exact);
int polygon(JsonObject *s, View *v, JsonObject *o, Screen points[8]);
gboolean inside(Screen p, Screen *points, int count);
void fit_view(Document *d, int sid, View *v);
double snap_value(double value, int snap);
JsonObject *rectangle(Point a, Point b, double y, int snap);
gboolean tim_name(const char *name);
void next_names(Document *d, char **left, char **right);
char *find_root(const char *hint);

void assets_init(Assets *a, const char *root);
void assets_clear(Assets *a);
void assets_invalidate(Assets *a);
Background *assets_background(Assets *a, JsonObject *s);
void assets_add_file(Assets *a, const char *path);
gboolean assets_import(Assets *a, JsonObject *s, const char *path, GError **error);
GBytes *encode_tim(GdkPixbuf *image, int x, int y);
gboolean assets_export(Assets *a, JsonObject *s, const char *path, GError **error);

void render_connect(Studio *s);
void render_queue(Studio *s);
Screen render_pointer(Studio *s, double x, double y);
void ui_activate(GtkApplication *application, gpointer data);
void ui_refresh(Studio *s);
void ui_notice(Studio *s, const char *message);
void ui_clear(Studio *s);
#endif
