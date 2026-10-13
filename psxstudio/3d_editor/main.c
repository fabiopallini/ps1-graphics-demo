#include "editor.h"
static void activate(GtkApplication *app, Editor *e) {
    e->application = app;
    ui_create(e);
}
static gboolean smoke(gpointer data) {
    Editor *e = data;
    static int step = 0;
    static gboolean tested = FALSE;
    GError *error = gtk_gl_area_get_error(GTK_GL_AREA(e->area));
    if (error) {
        g_printerr("OpenGL: %s\n", error->message);
        e->exit_code = 1;
        gtk_window_destroy(e->window);
        return G_SOURCE_REMOVE;
    }
    if (!e->ready || !e->frames) {
        if (++step > 30) {
            e->exit_code = 1;
            gtk_window_destroy(e->window);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }
    if (!tested) {
        ui_smoke(e);
        tested = TRUE;
    }
    if (step < 4) {
        e->view = step++;
        editor_refresh(e);
        return G_SOURCE_CONTINUE;
    }
    if (step == 4) {
        e->view = 0;
        editor_refresh(e);
        step++;
        return G_SOURCE_CONTINUE;
    }
    GdkPaintable *paintable = gtk_widget_paintable_new(GTK_WIDGET(e->window));
    GtkSnapshot *snapshot = gtk_snapshot_new();
    int w = gtk_widget_get_width(GTK_WIDGET(e->window)),
        h = gtk_widget_get_height(GTK_WIDGET(e->window));
    gdk_paintable_snapshot(paintable, GDK_SNAPSHOT(snapshot), w, h);
    GskRenderNode *node = gtk_snapshot_free_to_node(snapshot);
    gboolean ok = FALSE;
    if (node) {
        GskRenderer *renderer = gtk_native_get_renderer(GTK_NATIVE(e->window));
        graphene_rect_t bounds = GRAPHENE_RECT_INIT(0, 0, w, h);
        GdkTexture *texture = gsk_renderer_render_texture(renderer, node, &bounds);
        ok = gdk_texture_save_to_png(texture, e->smoke_path);
        g_object_unref(texture);
        gsk_render_node_unref(node);
    }
    g_object_unref(paintable);
    if (!ok && step++ < 15)
        return G_SOURCE_CONTINUE;
    e->exit_code = ok ? 0 : 1;
    g_print("GTK/OpenGL: four views, %u rendered frames, screenshot %s\n", e->frames,
            ok ? "saved" : "failed");
    gtk_window_destroy(e->window);
    return G_SOURCE_REMOVE;
}
int main(int argc, char **argv) {
    Editor e = {0};
    e.grid = e.edges = e.ydown = TRUE;
    e.mode = 2;
    e.yaw = .65;
    e.pitch = .35;
    e.span = 3;
    e.uv_corner = -1;
    mesh_init(&e.mesh);
    e.selection = g_array_new(FALSE, FALSE, sizeof(int));
    e.undo = g_ptr_array_new_with_free_func(g_free);
    e.redo = g_ptr_array_new_with_free_func(g_free);
    mesh_primitive(&e.mesh, 0, 1, 6, (Vec){0, 0, 0}, NULL);
    const char *file = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--smoke-test") && i + 1 < argc)
            e.smoke_path = g_strdup(argv[++i]);
        else if (!strcmp(argv[i], "--help")) {
            g_print("PSX 3D Editor\nUsage: 3d_editor [model.obj | project.model.json] "
                    "[--smoke-test output.png]\n");
            return 0;
        } else
            file = argv[i];
    }
    if (file) {
        char *text = NULL;
        GError *err = NULL;
        if (g_file_get_contents(file, &text, NULL, &err)) {
            const char *p = text;
            while (g_ascii_isspace(*p))
                p++;
            gboolean ok =
                *p == '{' ? project_decode(&e, text, &err) : obj_parse(text, &e.mesh, &err);
            if (ok) {
                e.path = g_strdup(file);
                if (*p != '{')
                    editor_frame(&e);
            }
            g_free(text);
        }
        if (err) {
            g_printerr("%s\n", err->message);
            g_error_free(err);
            return 1;
        }
    }
    GtkApplication *app =
        gtk_application_new("org.psxstudio.ModelEditor", G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &e);
    if (e.smoke_path)
        g_timeout_add(300, smoke, &e);
    char *args[] = {argv[0], NULL};
    int status = g_application_run(G_APPLICATION(app), 1, args);
    g_object_unref(app);
    mesh_clear(&e.mesh);
    g_array_unref(e.selection);
    g_ptr_array_unref(e.undo);
    g_ptr_array_unref(e.redo);
    g_clear_object(&e.texture);
    g_free(e.path);
    g_free(e.texture_name);
    g_free(e.texture_data);
    g_free(e.notice);
    g_free(e.drag_snapshot);
    g_free(e.smoke_path);
    return e.exit_code ? e.exit_code : status;
}
