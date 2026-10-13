#include "editor.h"
static GtkWidget *box(GtkOrientation o, int n) { return gtk_box_new(o, n); }
static GtkWidget *label(const char *s) {
    GtkWidget *w = gtk_label_new(s);
    gtk_label_set_xalign(GTK_LABEL(w), 0);
    gtk_label_set_wrap(GTK_LABEL(w), TRUE);
    return w;
}
static GtkWidget *spin(double value, double min, double max, double step) {
    GtkWidget *w = gtk_spin_button_new_with_range(min, max, step);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), 3);
    gtk_editable_set_width_chars(GTK_EDITABLE(w), 5);
    gtk_editable_set_max_width_chars(GTK_EDITABLE(w), 7);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), value);
    gtk_widget_set_hexpand(w, TRUE);
    return w;
}
static GtkWidget *drop(const char *const *names) { return gtk_drop_down_new_from_strings(names); }
static double val(GtkWidget *w) { return gtk_spin_button_get_value(GTK_SPIN_BUTTON(w)); }
static void report(Editor *e, GError *err) {
    editor_notice(e, err ? err->message : "Operation failed.");
    g_clear_error(&err);
}
void editor_notice(Editor *e, const char *message) {
    g_free(e->notice);
    e->notice = g_strdup(message);
    editor_refresh(e);
}
static void selected_clear(Editor *e) { g_array_set_size(e->selection, 0); }
static gboolean has(Editor *e, int n) {
    for (guint i = 0; i < e->selection->len; i++)
        if (AT(e->selection, int, i) == n)
            return TRUE;
    return FALSE;
}
void editor_commit(Editor *e, char *before) {
    char *after = project_encode(e);
    if (strcmp(before, after)) {
        g_ptr_array_add(e->undo, before);
        if (e->undo->len > 30)
            g_ptr_array_remove_index(e->undo, 0);
        g_ptr_array_set_size(e->redo, 0);
        e->dirty = TRUE;
    } else
        g_free(before);
    g_free(after);
    editor_refresh(e);
}
void editor_history(Editor *e, gboolean redo) {
    GPtrArray *from = redo ? e->redo : e->undo, *to = redo ? e->undo : e->redo;
    if (!from->len)
        return;
    char *current = project_encode(e), *snapshot = g_ptr_array_steal_index(from, from->len - 1);
    GError *err = NULL;
    if (project_decode(e, snapshot, &err)) {
        g_ptr_array_add(to, current);
        selected_clear(e);
        e->dirty = TRUE;
    } else {
        g_free(current);
        report(e, err);
    }
    g_free(snapshot);
    editor_refresh(e);
}
void editor_refresh(Editor *e) {
    if (!e->window)
        return;
    e->refreshing = TRUE;
    gtk_editable_set_text(GTK_EDITABLE(e->name), e->mesh.name);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(e->mode_drop), e->mode);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(e->view_drop), e->view);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(e->shade_drop), e->shade);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(e->ydown_check), e->ydown);
    char *info = g_strdup_printf(
        "%u vertices / 2048\n%u UVs / 2048\n%u faces / 256 quads\n\n%s\n\n%s",
        e->mesh.vertices->len, e->mesh.uvs->len, e->mesh.faces->len,
        e->texture_name ? e->texture_name : "No texture",
        e->mesh.faces->len > 256 || e->mesh.vertices->len > 2048 || e->mesh.uvs->len > 2048
            ? "Exceeds PS1 limits. Standard OBJ export remains available."
            : "Within PS1 loader limits.");
    gtk_label_set_text(GTK_LABEL(e->info), info);
    g_free(info);
    char *status = g_strdup_printf("%s  |  %u selected  |  %s", e->notice ? e->notice : "Ready",
                                   e->mode == 2 ? e->mesh.vertices->len : e->selection->len,
                                   e->dirty ? "Modified" : "Unmodified");
    gtk_label_set_text(GTK_LABEL(e->status), status);
    g_free(status);
    gboolean vertex = e->mode == 0 && e->selection->len == 1;
    for (int j = 0; j < 3; j++) {
        gtk_widget_set_sensitive(e->vertex[j], vertex);
        if (vertex) {
            Vec p = AT(e->mesh.vertices, Vec, AT(e->selection, int, 0));
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(e->vertex[j]), j == 0   ? p.x
                                                                     : j == 1 ? p.y
                                                                              : p.z);
        }
    }
    Face *f = e->mode == 1 && e->selection->len == 1
                  ? &AT(e->mesh.faces, Face, AT(e->selection, int, 0))
                  : NULL;
    for (int j = 0; j < 4; j++)
        for (int k = 0; k < 2; k++) {
            gtk_widget_set_sensitive(e->uv_fields[j][k], f && j < f->n);
            if (f && j < f->n) {
                UV uv = AT(e->mesh.uvs, UV, f->uv[j]);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(e->uv_fields[j][k]), k ? uv.v : uv.u);
            }
        }
    e->refreshing = FALSE;
    gtk_widget_queue_draw(e->uv_area);
    render_queue(e);
}
static void setting(GObject *obj, GParamSpec *pspec, Editor *e) {
    (void)pspec;
    if (e->refreshing)
        return;
    int n = gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));
    if (GTK_WIDGET(obj) == e->mode_drop) {
        e->mode = n;
        selected_clear(e);
    } else if (GTK_WIDGET(obj) == e->view_drop)
        e->view = n;
    else if (GTK_WIDGET(obj) == e->shade_drop)
        e->shade = n;
    else if (GTK_WIDGET(obj) == e->axis_drop)
        e->axis = n;
    else if (GTK_WIDGET(obj) == e->transform_drop) {
        for (int i = 0; i < 3; i++)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(e->transform[i]), n == 2 ? 1 : 0);
    }
    editor_refresh(e);
}
static void check(GtkCheckButton *w, Editor *e) {
    if (e->refreshing)
        return;
    gboolean active = gtk_check_button_get_active(w);
    if (GTK_WIDGET(w) == e->grid_check)
        e->grid = active;
    else if (GTK_WIDGET(w) == e->edge_check)
        e->edges = active;
    else
        e->ydown = active;
    render_queue(e);
}
static void name_changed(GtkEditable *w, Editor *e) {
    if (e->refreshing)
        return;
    char *before = project_encode(e);
    g_strlcpy(e->mesh.name, gtk_editable_get_text(w), sizeof e->mesh.name);
    editor_commit(e, before);
}
static void numeric(GtkSpinButton *w, Editor *e) {
    if (e->refreshing)
        return;
    char *before = project_encode(e);
    if (e->mode == 0 && e->selection->len == 1) {
        Vec *p = &AT(e->mesh.vertices, Vec, AT(e->selection, int, 0));
        for (int j = 0; j < 3; j++)
            if (GTK_WIDGET(w) == e->vertex[j]) {
                if (j == 0)
                    p->x = val(GTK_WIDGET(w));
                else if (j == 1)
                    p->y = val(GTK_WIDGET(w));
                else
                    p->z = val(GTK_WIDGET(w));
            }
    }
    if (e->mode == 1 && e->selection->len == 1) {
        Face *f = &AT(e->mesh.faces, Face, AT(e->selection, int, 0));
        for (int j = 0; j < f->n; j++)
            for (int k = 0; k < 2; k++)
                if (GTK_WIDGET(w) == e->uv_fields[j][k]) {
                    UV *p = &AT(e->mesh.uvs, UV, f->uv[j]);
                    if (k)
                        p->v = val(GTK_WIDGET(w));
                    else
                        p->u = val(GTK_WIDGET(w));
                }
    }
    editor_commit(e, before);
}
static void action(Editor *e, int code);
static void clicked(GtkButton *b, Editor *e) {
    action(e, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "action")));
}
static GtkWidget *button(Editor *e, const char *text, int code) {
    GtkWidget *b = gtk_button_new_with_label(text);
    g_object_set_data(G_OBJECT(b), "action", GINT_TO_POINTER(code));
    g_signal_connect(b, "clicked", G_CALLBACK(clicked), e);
    return b;
}
static void discard_done(GObject *obj, GAsyncResult *result, gpointer data) {
    Editor *e = data;
    GError *err = NULL;
    int response = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(obj), result, &err);
    g_clear_error(&err);
    if (response == 1) {
        e->dirty = FALSE;
        action(e, GPOINTER_TO_INT(g_object_get_data(obj, "action")));
    }
}
static void action(Editor *e, int code) {
    if ((code == 100 || code == 101 || code == 130) && e->dirty) {
        GtkAlertDialog *d = gtk_alert_dialog_new("Discard unsaved changes?");
        const char *buttons[] = {"Cancel", "Discard", NULL};
        gtk_alert_dialog_set_buttons(d, buttons);
        gtk_alert_dialog_set_cancel_button(d, 0);
        gtk_alert_dialog_set_default_button(d, 0);
        g_object_set_data(G_OBJECT(d), "action", GINT_TO_POINTER(code));
        gtk_alert_dialog_choose(d, e->window, NULL, discard_done, e);
        g_object_unref(d);
        return;
    }
    if (code == 101) {
        ui_file(e, 0);
        return;
    }
    if (code == 102) {
        ui_file(e, 1);
        return;
    }
    if (code == 103) {
        ui_file(e, 2);
        return;
    }
    if (code == 104) {
        ui_file(e, 3);
        return;
    }
    if (code == 105) {
        ui_file(e, 4);
        return;
    }
    if (code == 110 || code == 111) {
        editor_history(e, code == 111);
        return;
    }
    if (code == 112) {
        editor_frame(e);
        return;
    }
    if (code == 113) {
        selected_clear(e);
        int count = e->mode == 0 ? (int)e->mesh.vertices->len : (int)e->mesh.faces->len;
        for (int i = 0; i < count; i++)
            g_array_append_val(e->selection, i);
        editor_refresh(e);
        return;
    }
    if (code == 114) {
        selected_clear(e);
        editor_refresh(e);
        return;
    }
    if (code == 115) {
        GArray *ids = mesh_selected(e);
        gboolean changed = TRUE;
        while (changed) {
            changed = FALSE;
            for (guint i = 0; i < e->mesh.faces->len; i++) {
                Face f = AT(e->mesh.faces, Face, i);
                gboolean hit = FALSE;
                for (int j = 0; j < f.n; j++)
                    for (guint k = 0; k < ids->len; k++)
                        if (f.v[j] == AT(ids, int, k))
                            hit = TRUE;
                if (hit)
                    for (int j = 0; j < f.n; j++) {
                        gboolean found = FALSE;
                        for (guint k = 0; k < ids->len; k++)
                            if (f.v[j] == AT(ids, int, k))
                                found = TRUE;
                        if (!found) {
                            g_array_append_val(ids, f.v[j]);
                            changed = TRUE;
                        }
                    }
            }
        }
        selected_clear(e);
        g_array_append_vals(e->selection, ids->data, ids->len);
        e->mode = 0;
        g_array_unref(ids);
        editor_refresh(e);
        return;
    }
    if (code == 116 || code == 117) {
        e->move = code == 117;
        editor_notice(e, e->move ? "Move: drag the selection; choose an axis to constrain it."
                                 : "Select: click or drag a box; Shift adds to selection.");
        return;
    }
    if (code == 120) {
        GtkAlertDialog *d = gtk_alert_dialog_new("PSX 3D Editor");
        gtk_alert_dialog_set_detail(
            d, "Click selects; Shift adds. Drag empty space for box selection. Right-drag orbits; "
               "middle-drag pans; scroll zooms.\n\nV: select, G: move, A: select all, F: frame "
               "all, Esc: deselect/cancel, Delete: remove, Ctrl+Z: undo, Ctrl+Shift+Z: redo, "
               "Ctrl+S: save project.\n\nCreate face: select 3 or 4 vertices in boundary order. "
               "Extrude: select one face. Map UV fills each selected face with the entire texture. "
               "Select a face to edit or drag its UV corners.\n\nPS1: 2048 vertices, 2048 UVs, 256 "
               "quads. Triangles repeat their last corner on export. Y down changes only the view. "
               "One pose and one PNG; no rig, animation timeline or TIM conversion.");
        gtk_alert_dialog_show(d, e->window);
        g_object_unref(d);
        return;
    }
    if (code == 130) {
        gtk_window_destroy(e->window);
        return;
    }
    char *before = project_encode(e);
    GError *err = NULL;
    gboolean ok = TRUE;
    if (code == 100) {
        mesh_clear(&e->mesh);
        mesh_init(&e->mesh);
        mesh_primitive(&e->mesh, 0, 1, 6, (Vec){0, 0, 0}, NULL);
        g_clear_object(&e->texture);
        g_clear_pointer(&e->texture_data, g_free);
        g_clear_pointer(&e->texture_name, g_free);
        g_clear_pointer(&e->path, g_free);
        selected_clear(e);
        e->mode = 2;
        editor_frame(e);
    } else if (code >= 0 && code < 6) {
        ok = mesh_primitive(&e->mesh, code, val(e->size), (int)val(e->sides),
                            (Vec){val(e->position[0]), val(e->position[1]), val(e->position[2])},
                            &err);
    } else if (code >= 10 && code <= 16)
        ok = mesh_edit(e, code - 10, &err);
    else if (code == 106) {
        g_clear_object(&e->texture);
        g_clear_pointer(&e->texture_name, g_free);
        g_clear_pointer(&e->texture_data, g_free);
        e->shade = 0;
    } else if (code == 107) {
        if (e->mode != 1 || !e->selection->len) {
            g_set_error_literal(&err, G_IO_ERROR, G_IO_ERROR_FAILED, "Select faces to map UVs.");
            ok = FALSE;
        } else {
            int plane = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(e->uv_area), "plane"));
            for (guint i = 0; i < e->selection->len; i++)
                mesh_map(&e->mesh, &AT(e->mesh.faces, Face, AT(e->selection, int, i)), plane);
            ok = mesh_validate(&e->mesh, FALSE, &err);
        }
    } else if (code == 108) {
        GArray *ids = mesh_selected(e);
        Vec pivot = {0, 0, 0};
        if (!gtk_drop_down_get_selected(GTK_DROP_DOWN(e->pivot_drop)) && ids->len) {
            for (guint i = 0; i < ids->len; i++)
                pivot = vec_add(pivot, AT(e->mesh.vertices, Vec, AT(ids, int, i)));
            pivot = vec_mul(pivot, 1.0 / ids->len);
        }
        ok = mesh_transform(
            &e->mesh, ids, gtk_drop_down_get_selected(GTK_DROP_DOWN(e->transform_drop)),
            (Vec){val(e->transform[0]), val(e->transform[1]), val(e->transform[2])}, pivot, &err);
        g_array_unref(ids);
    }
    if (ok)
        editor_commit(e, before);
    else {
        GError *restore = NULL;
        project_decode(e, before, &restore);
        g_clear_error(&restore);
        selected_clear(e);
        g_free(before);
        report(e, err);
    }
    editor_refresh(e);
}
typedef struct {
    Editor *e;
    int code;
} FileJob;
static void file_done(GObject *source, GAsyncResult *result, gpointer data) {
    FileJob *job = data;
    Editor *e = job->e;
    int code = job->code;
    GError *err = NULL;
    GFile *file = code == 0 || code == 3
                      ? gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &err)
                      : gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &err);
    g_free(job);
    if (!file) {
        g_clear_error(&err);
        return;
    }
    char *path = g_file_get_path(file);
    g_object_unref(file);
    if (!path) {
        editor_notice(e, "Choose a local file.");
        return;
    }
    if (code == 0 || code == 3) {
        char *text = NULL;
        gsize size = 0;
        if (!g_file_get_contents(path, &text, &size, &err)) {
            report(e, err);
            g_free(path);
            return;
        }
        if (size > 64 * 1024 * 1024) {
            g_free(text);
            g_free(path);
            editor_notice(e, "File exceeds the 64 MB import limit.");
            return;
        }
        char *before = project_encode(e);
        gboolean ok;
        if (code == 3) {
            char *base64 = g_base64_encode((guchar *)text, size),
                 *uri = g_strconcat("data:image/png;base64,", base64, NULL);
            GdkPixbuf *image = NULL;
            ok = texture_decode(uri, &image, &err);
            if (ok) {
                g_clear_object(&e->texture);
                e->texture = image;
                g_free(e->texture_data);
                e->texture_data = uri;
                g_free(e->texture_name);
                e->texture_name = g_path_get_basename(path);
                e->shade = 2;
            } else
                g_free(uri);
            g_free(base64);
        } else {
            const char *start = text;
            while (g_ascii_isspace(*start))
                start++;
            if (*start == '{')
                ok = project_decode(e, text, &err);
            else {
                ok = obj_parse(text, &e->mesh, &err);
                if (ok) {
                    g_clear_object(&e->texture);
                    g_clear_pointer(&e->texture_data, g_free);
                    g_clear_pointer(&e->texture_name, g_free);
                    char *name = g_path_get_basename(path);
                    char *dot = strrchr(name, '.');
                    if (dot)
                        *dot = 0;
                    g_strlcpy(e->mesh.name, name, sizeof e->mesh.name);
                    g_free(name);
                    e->shade = 0;
                    editor_frame(e);
                }
            }
        }
        if (ok) {
            if (code == 0) {
                g_ptr_array_set_size(e->undo, 0);
                g_ptr_array_set_size(e->redo, 0);
                g_free(before);
                selected_clear(e);
                g_free(e->path);
                e->path = g_strdup(path);
                e->dirty = FALSE;
                editor_notice(e, "Opened model. Load PNG separately for OBJ files.");
            } else {
                editor_commit(e, before);
                editor_notice(e, "Texture loaded.");
            }
        } else {
            g_free(before);
            report(e, err);
        }
        g_free(text);
    } else {
        char *text = NULL;
        if (code == 1)
            text = project_encode(e);
        else if (code == 2) {
            gboolean ps1 = gtk_drop_down_get_selected(GTK_DROP_DOWN(e->format_drop)) == 0;
            text = obj_export(&e->mesh, ps1, &err);
            if (text && !ps1 && e->texture) {
                char *base = g_path_get_basename(path), *dot = strrchr(base, '.');
                if (dot)
                    *dot = 0;
                char *prefix = g_strdup_printf("mtllib %s.mtl\nusemtl atlas\n", base),
                     *combined = g_strconcat(prefix, text, NULL);
                g_free(base);
                g_free(prefix);
                g_free(text);
                text = combined;
            }
        } else {
            char *name = g_strdup(e->texture_name ? e->texture_name : "TEX.png");
            for (char *p = name; *p; p++)
                if (*p == '\r' || *p == '\n' || *p == '\\' || *p == '/')
                    *p = '_';
            text = g_strdup_printf("newmtl atlas\nKa 1 1 1\nKd 1 1 1\nKs 0 0 0\nd 1\nmap_Kd %s\n",
                                   name);
            g_free(name);
        }
        if (text) {
            GFile *out = g_file_new_for_path(path);
            gboolean ok = g_file_replace_contents(out, text, strlen(text), NULL, TRUE,
                                                  G_FILE_CREATE_NONE, NULL, NULL, &err);
            g_object_unref(out);
            if (ok) {
                if (code == 1) {
                    g_free(e->path);
                    e->path = g_strdup(path);
                    e->dirty = FALSE;
                }
                editor_notice(e, code == 1 ? "Project saved." : "Export complete.");
            } else
                report(e, err);
            g_free(text);
        } else
            report(e, err);
    }
    g_free(path);
    editor_refresh(e);
}
void ui_file(Editor *e, int code) {
    GtkFileDialog *d = gtk_file_dialog_new();
    const char *titles[] = {"Open OBJ / project", "Save project", "Export OBJ", "Load PNG texture",
                            "Export MTL"};
    gtk_file_dialog_set_title(d, titles[code]);
    gtk_file_dialog_set_modal(d, TRUE);
    if (code != 0 && code != 3) {
        char *name = g_strdup_printf("%s%s", e->mesh.name,
                                     code == 1   ? ".model.json"
                                     : code == 2 ? ".obj"
                                                 : ".mtl");
        gtk_file_dialog_set_initial_name(d, name);
        g_free(name);
    }
    GtkFileFilter *filter = gtk_file_filter_new();
    if (code == 0) {
        gtk_file_filter_add_pattern(filter, "*.obj");
        gtk_file_filter_add_pattern(filter, "*.OBJ");
        gtk_file_filter_add_pattern(filter, "*.json");
    } else
        gtk_file_filter_add_pattern(filter, code == 3   ? "*.png"
                                            : code == 1 ? "*.json"
                                            : code == 2 ? "*.obj"
                                                        : "*.mtl");
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(d, G_LIST_MODEL(filters));
    g_object_unref(filters);
    g_object_unref(filter);
    FileJob *job = g_new(FileJob, 1);
    *job = (FileJob){e, code};
    if (code == 0 || code == 3)
        gtk_file_dialog_open(d, e->window, NULL, file_done, job);
    else
        gtk_file_dialog_save(d, e->window, NULL, file_done, job);
    g_object_unref(d);
}
static gboolean inside(double x, double y, Projected a, Projected b, Projected c, double *depth) {
    if (!a.visible || !b.visible || !c.visible)
        return FALSE;
    double d = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (fabs(d) < 1e-8)
        return FALSE;
    double u = ((b.y - c.y) * (x - c.x) + (c.x - b.x) * (y - c.y)) / d,
           v = ((c.y - a.y) * (x - c.x) + (a.x - c.x) * (y - c.y)) / d, w = 1 - u - v;
    if (u < 0 || v < 0 || w < 0)
        return FALSE;
    *depth = 1 / (u / a.depth + v / b.depth + w / c.depth);
    return TRUE;
}
static int pick(Editor *e, double x, double y) {
    int best = -1;
    double closest = G_MAXDOUBLE;
    if (e->mode == 0) {
        for (guint i = 0; i < e->mesh.vertices->len; i++) {
            Projected p = render_project(e, AT(e->mesh.vertices, Vec, i));
            double d = hypot(x - p.x, y - p.y);
            if (p.visible && d < 9 && d < closest) {
                closest = d;
                best = i;
            }
        }
    } else if (e->mode == 1) {
        for (guint i = 0; i < e->mesh.faces->len; i++) {
            Face f = AT(e->mesh.faces, Face, i);
            Projected p[4];
            for (int j = 0; j < f.n; j++)
                p[j] = render_project(e, AT(e->mesh.vertices, Vec, f.v[j]));
            double depth;
            if (inside(x, y, p[0], p[1], p[2], &depth) && depth < closest) {
                closest = depth;
                best = i;
            }
            if (f.n == 4 && inside(x, y, p[0], p[2], p[3], &depth) && depth < closest) {
                closest = depth;
                best = i;
            }
        }
    }
    return best;
}
static void drag_begin(GtkGestureDrag *g, double x, double y, Editor *e) {
    gtk_widget_grab_focus(e->area);
    e->start_x = e->last_x = x;
    e->start_y = e->last_y = y;
    e->drag_changed = FALSE;
    guint b = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(g));
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(g));
    e->drag_kind = b == 3 ? 1 : b == 2 ? 2 : 0;
    if (e->drag_kind)
        return;
    if (e->move && (e->mode == 2 || e->selection->len)) {
        e->drag_kind = 3;
        e->drag_snapshot = project_encode(e);
        return;
    }
    int n = pick(e, x, y);
    if (!(state & GDK_SHIFT_MASK))
        selected_clear(e);
    if (n >= 0) {
        if (!has(e, n))
            g_array_append_val(e->selection, n);
        else if (state & GDK_SHIFT_MASK)
            for (guint i = 0; i < e->selection->len; i++)
                if (AT(e->selection, int, i) == n) {
                    g_array_remove_index(e->selection, i);
                    break;
                }
    } else
        e->drag_kind = 4;
    editor_refresh(e);
}
static void drag_update(GtkGestureDrag *g, double dx, double dy, Editor *e) {
    (void)g;
    double x = e->start_x + dx, y = e->start_y + dy, mx = x - e->last_x, my = y - e->last_y;
    if (e->drag_kind == 1) {
        if (e->view == 0) {
            e->yaw -= mx * .008;
            e->pitch = CLAMP(e->pitch + my * .008, -1.5, 1.5);
        }
    } else if (e->drag_kind == 2) {
        Vec r, u, f;
        render_basis(e, &r, &u, &f);
        double scale = render_project(e, e->center).scale;
        e->center = vec_add(e->center, vec_add(vec_mul(r, -mx / scale), vec_mul(u, my / scale)));
    } else if (e->drag_kind == 3) {
        Vec r, u, f;
        render_basis(e, &r, &u, &f);
        double scale = render_project(e, e->center).scale;
        Vec delta = vec_add(vec_mul(r, mx / scale), vec_mul(u, -my / scale));
        if (e->axis) {
            int k = e->axis - 1;
            Vec axis = k == 0 ? (Vec){1, 0, 0} : k == 1 ? (Vec){0, 1, 0} : (Vec){0, 0, 1};
            double ax = vec_dot(axis, r), ay = -vec_dot(axis, u), den = ax * ax + ay * ay;
            delta =
                den > 1e-6 ? vec_mul(axis, (mx * ax + my * ay) / (scale * den)) : (Vec){0, 0, 0};
        }
        double snap = val(e->snap);
        if (snap > 0) {
            delta.x = round(delta.x / snap) * snap;
            delta.y = round(delta.y / snap) * snap;
            delta.z = round(delta.z / snap) * snap;
            if (!delta.x && !delta.y && !delta.z)
                return;
        }
        GArray *ids = mesh_selected(e);
        GError *err = NULL;
        if (mesh_transform(&e->mesh, ids, 0, delta, (Vec){0, 0, 0}, &err))
            e->drag_changed = TRUE;
        else
            report(e, err);
        g_array_unref(ids);
    }
    e->last_x = x;
    e->last_y = y;
    render_queue(e);
}
static void drag_end(GtkGestureDrag *g, double dx, double dy, Editor *e) {
    (void)g;
    if (e->drag_kind == 3 && e->drag_snapshot) {
        editor_commit(e, e->drag_snapshot);
        e->drag_snapshot = NULL;
    } else if (e->drag_kind == 4 && hypot(dx, dy) > 4) {
        double x0 = MIN(e->start_x, e->start_x + dx), x1 = MAX(e->start_x, e->start_x + dx),
               y0 = MIN(e->start_y, e->start_y + dy), y1 = MAX(e->start_y, e->start_y + dy);
        guint count = e->mode == 0 ? e->mesh.vertices->len : e->mode == 1 ? e->mesh.faces->len : 0;
        for (guint i = 0; i < count; i++) {
            Vec p = {0, 0, 0};
            if (e->mode == 0)
                p = AT(e->mesh.vertices, Vec, i);
            else {
                Face f = AT(e->mesh.faces, Face, i);
                for (int j = 0; j < f.n; j++)
                    p = vec_add(p, AT(e->mesh.vertices, Vec, f.v[j]));
                p = vec_mul(p, 1.0 / f.n);
            }
            Projected s = render_project(e, p);
            if (s.visible && s.x >= x0 && s.x <= x1 && s.y >= y0 && s.y <= y1 && !has(e, i)) {
                int n = i;
                g_array_append_val(e->selection, n);
            }
        }
    }
    e->drag_kind = 0;
    editor_refresh(e);
}
static gboolean scroll(GtkEventControllerScroll *controller, double x, double y, Editor *e) {
    (void)controller;
    (void)x;
    e->span = CLAMP(e->span * exp(y * .12), .01, 1e6);
    render_queue(e);
    return TRUE;
}
static gboolean key(GtkEventControllerKey *c, guint keyval, guint keycode, GdkModifierType state,
                    Editor *e) {
    (void)c;
    (void)keycode;
    guint k = gdk_keyval_to_lower(keyval);
    gboolean cmd = state & GDK_CONTROL_MASK;
    GtkWidget *focus = gtk_window_get_focus(e->window);
    if (cmd && k == GDK_KEY_s) {
        ui_file(e, 1);
        return TRUE;
    }
    if (focus && (GTK_IS_EDITABLE(focus) || GTK_IS_SPIN_BUTTON(focus)))
        return FALSE;
    if (cmd && k == GDK_KEY_z) {
        editor_history(e, state & GDK_SHIFT_MASK);
        return TRUE;
    }
    if (cmd && k == GDK_KEY_y) {
        editor_history(e, TRUE);
        return TRUE;
    }
    if (cmd)
        return FALSE;
    if (k == GDK_KEY_Escape) {
        if (e->drag_snapshot) {
            project_decode(e, e->drag_snapshot, NULL);
            g_clear_pointer(&e->drag_snapshot, g_free);
            e->drag_kind = 0;
        }
        action(e, 114);
    } else if (k == GDK_KEY_Delete)
        action(e, 13);
    else if (k == GDK_KEY_a)
        action(e, 113);
    else if (k == GDK_KEY_f)
        action(e, 112);
    else if (k == GDK_KEY_v)
        action(e, 116);
    else if (k == GDK_KEY_g)
        action(e, 117);
    else
        return FALSE;
    return TRUE;
}
static cairo_surface_t *texture_surface(GdkPixbuf *image) {
    int w = gdk_pixbuf_get_width(image), h = gdk_pixbuf_get_height(image),
        channels = gdk_pixbuf_get_n_channels(image), stride = gdk_pixbuf_get_rowstride(image);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    int row = cairo_image_surface_get_stride(surface);
    unsigned char *dest = cairo_image_surface_get_data(surface);
    const unsigned char *source = gdk_pixbuf_read_pixels(image);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const unsigned char *p = source + y * stride + x * channels;
            unsigned a = channels == 4 ? p[3] : 255;
            uint32_t *q = (uint32_t *)(dest + y * row + x * 4);
            *q = (a << 24) | ((p[0] * a / 255) << 16) | ((p[1] * a / 255) << 8) | (p[2] * a / 255);
        }
    cairo_surface_mark_dirty(surface);
    return surface;
}
void ui_texture_draw(GtkDrawingArea *a, cairo_t *cr, int w, int h, gpointer data) {
    (void)a;
    Editor *e = data;
    for (int y = 0; y < h; y += 16)
        for (int x = 0; x < w; x += 16) {
            double c = ((x / 16 + y / 16) % 2) ? .55 : .72;
            cairo_set_source_rgb(cr, c, c, c);
            cairo_rectangle(cr, x, y, 16, 16);
            cairo_fill(cr);
        }
    if (e->texture) {
        cairo_save(cr);
        cairo_scale(cr, (double)w / gdk_pixbuf_get_width(e->texture),
                    (double)h / gdk_pixbuf_get_height(e->texture));
        cairo_surface_t *surface = texture_surface(e->texture);
        cairo_set_source_surface(cr, surface, 0, 0);
        cairo_surface_destroy(surface);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
        cairo_paint(cr);
        cairo_restore(cr);
    }
    if (e->mode == 1 && e->selection->len == 1) {
        Face f = AT(e->mesh.faces, Face, AT(e->selection, int, 0));
        cairo_set_source_rgb(cr, 1, .3, .1);
        cairo_set_line_width(cr, 2);
        for (int j = 0; j < f.n; j++) {
            UV uv = AT(e->mesh.uvs, UV, f.uv[j]);
            if (j == 0)
                cairo_move_to(cr, uv.u * w, (1 - uv.v) * h);
            else
                cairo_line_to(cr, uv.u * w, (1 - uv.v) * h);
        }
        cairo_close_path(cr);
        cairo_stroke(cr);
        for (int j = 0; j < f.n; j++) {
            UV uv = AT(e->mesh.uvs, UV, f.uv[j]);
            cairo_arc(cr, uv.u * w, (1 - uv.v) * h, 4, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }
}
static void uv_begin(GtkGestureDrag *g, double x, double y, Editor *e) {
    (void)g;
    e->uv_corner = -1;
    if (e->mode != 1 || e->selection->len != 1)
        return;
    Face f = AT(e->mesh.faces, Face, AT(e->selection, int, 0));
    int w = gtk_widget_get_width(e->uv_area), h = gtk_widget_get_height(e->uv_area);
    for (int j = 0; j < f.n; j++) {
        UV uv = AT(e->mesh.uvs, UV, f.uv[j]);
        if (hypot(x - uv.u * w, y - (1 - uv.v) * h) < 12)
            e->uv_corner = j;
    }
    if (e->uv_corner >= 0) {
        e->start_x = x;
        e->start_y = y;
        e->drag_snapshot = project_encode(e);
    }
}
static void uv_update(GtkGestureDrag *g, double dx, double dy, Editor *e) {
    (void)g;
    if (e->uv_corner < 0)
        return;
    Face f = AT(e->mesh.faces, Face, AT(e->selection, int, 0));
    UV *uv = &AT(e->mesh.uvs, UV, f.uv[e->uv_corner]);
    uv->u = CLAMP((e->start_x + dx) / gtk_widget_get_width(e->uv_area), 0, 1);
    uv->v = CLAMP(1 - (e->start_y + dy) / gtk_widget_get_height(e->uv_area), 0, 1);
    editor_refresh(e);
}
static void uv_end(GtkGestureDrag *g, double x, double y, Editor *e) {
    (void)g;
    (void)x;
    (void)y;
    if (e->uv_corner >= 0 && e->drag_snapshot) {
        editor_commit(e, e->drag_snapshot);
        e->drag_snapshot = NULL;
    }
    e->uv_corner = -1;
}
static void uv_plane(GObject *o, GParamSpec *p, Editor *e) {
    (void)p;
    g_object_set_data(G_OBJECT(e->uv_area), "plane",
                      GINT_TO_POINTER(gtk_drop_down_get_selected(GTK_DROP_DOWN(o))));
}
static gboolean close_request(GtkWindow *w, Editor *e) {
    (void)w;
    action(e, 130);
    return TRUE;
}
static GtkWidget *section(GtkWidget *parent, const char *title) {
    GtkWidget *frame = gtk_frame_new(title), *b = box(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_start(b, 7);
    gtk_widget_set_margin_end(b, 7);
    gtk_widget_set_margin_top(b, 7);
    gtk_widget_set_margin_bottom(b, 7);
    gtk_frame_set_child(GTK_FRAME(frame), b);
    gtk_box_append(GTK_BOX(parent), frame);
    return b;
}
static void row_buttons(Editor *e, GtkWidget *parent, const char *a, int ca, const char *b,
                        int cb) {
    GtkWidget *r = box(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_append(GTK_BOX(r), button(e, a, ca));
    gtk_box_append(GTK_BOX(r), button(e, b, cb));
    gtk_box_append(GTK_BOX(parent), r);
}
static void fields(GtkWidget *parent, GtkWidget **out, double initial, double min, double max,
                   double step) {
    for (int i = 0; i < 3; i++) {
        GtkWidget *row = box(GTK_ORIENTATION_HORIZONTAL, 5);
        char axis[] = {"XYZ"[i], 0};
        gtk_box_append(GTK_BOX(row), label(axis));
        out[i] = spin(initial, min, max, step);
        gtk_box_append(GTK_BOX(row), out[i]);
        gtk_box_append(GTK_BOX(parent), row);
    }
}
void ui_create(Editor *e) {
    e->window = GTK_WINDOW(gtk_application_window_new(e->application));
    gtk_window_set_title(e->window, "PSX 3D Editor");
    gtk_window_set_default_size(e->window, 1360, 900);
    g_signal_connect(e->window, "close-request", G_CALLBACK(close_request), e);
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(
        css, "window {background:#c0c0c0;color:#000;font-family:Sans;font-size:12px;} button, "
             "dropdown, spinbutton, entry {border-radius:0;min-height:22px;} button "
             "{background:#c0c0c0;border:2px solid;border-color:#fff #555 #555 #fff;padding:3px "
             "7px;color:#000;} entry,spinbutton {background:#fff;color:#000;} frame {border:1px "
             "solid #808080;} headerbar.title {background:#000080;background-image:none;"
             "color:#fff;font-weight:bold;padding:4px 6px;border-radius:0;min-height:28px;} "
             "headerbar.title label {color:#fff;} headerbar.title button {min-height:18px;"
             "min-width:18px;padding:1px 4px;color:#000;background:#c0c0c0;} "
             "spinbutton button {min-width:14px;padding:0;border-width:1px;} .status {border:1px "
             "inset #888;padding:4px;} dropdown {color:#000;background:#ddd;}");
    gtk_style_context_add_provider_for_display(gtk_widget_get_display(GTK_WIDGET(e->window)),
                                               GTK_STYLE_PROVIDER(css),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
    GtkWidget *root = box(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(e->window, root);
    GtkWidget *title = gtk_header_bar_new();
    gtk_widget_add_css_class(title, "title");
    GtkWidget *caption = label("◆  PSX 3D Editor");
    gtk_label_set_wrap(GTK_LABEL(caption), FALSE);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(title), caption);
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(title), box(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_header_bar_set_decoration_layout(GTK_HEADER_BAR(title), ":minimize,maximize,close");
    gtk_window_set_titlebar(e->window, title);
    GtkWidget *bar = box(GTK_ORIENTATION_HORIZONTAL, 4);
    const char *names[] = {
        "New", "Open OBJ / project…", "Save project…", "Export OBJ…", "Undo", "Redo", "Help"};
    int codes[] = {100, 101, 102, 103, 110, 111, 120};
    for (int i = 0; i < 7; i++)
        gtk_box_append(GTK_BOX(bar), button(e, names[i], codes[i]));
    const char *formats[] = {"PS1 OBJ", "Standard OBJ", NULL};
    e->format_drop = drop(formats);
    gtk_box_append(GTK_BOX(bar), e->format_drop);
    gtk_box_append(GTK_BOX(root), bar);
    GtkWidget *body = box(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_vexpand(body, TRUE);
    gtk_box_append(GTK_BOX(root), body);
    GtkWidget *left = box(GTK_ORIENTATION_VERTICAL, 8), *right = box(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *ls = gtk_scrolled_window_new(), *rs = gtk_scrolled_window_new();
    gtk_widget_set_size_request(ls, 255, -1);
    gtk_widget_set_size_request(rs, 285, -1);
    gtk_widget_set_hexpand(ls, FALSE);
    gtk_widget_set_hexpand(rs, FALSE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ls), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rs), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(ls), left);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rs), right);
    gtk_box_append(GTK_BOX(body), ls);
    GtkWidget *scene = box(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_hexpand(scene, TRUE);
    gtk_box_append(GTK_BOX(body), scene);
    gtk_box_append(GTK_BOX(body), rs);
    GtkWidget *b = section(left, "Selection");
    const char *modes[] = {"Vertices", "Faces", "Model", NULL};
    e->mode_drop = drop(modes);
    gtk_box_append(GTK_BOX(b), e->mode_drop);
    row_buttons(e, b, "Select", 116, "Move", 117);
    row_buttons(e, b, "All", 113, "None", 114);
    gtk_box_append(GTK_BOX(b), button(e, "Connected", 115));
    const char *axes[] = {"Free", "X axis", "Y axis", "Z axis", NULL};
    e->axis_drop = drop(axes);
    gtk_box_append(GTK_BOX(b), label("Constraint"));
    gtk_box_append(GTK_BOX(b), e->axis_drop);
    gtk_box_append(GTK_BOX(b), label("Snap"));
    e->snap = spin(.05, 0, 10000, .01);
    gtk_box_append(GTK_BOX(b), e->snap);
    b = section(left, "Add a shape");
    row_buttons(e, b, "Cube", 0, "Plane", 1);
    row_buttons(e, b, "Pyramid", 2, "Tetrahedron", 3);
    row_buttons(e, b, "Cylinder", 4, "Vertex", 5);
    gtk_box_append(GTK_BOX(b), label("Size"));
    e->size = spin(1, .001, 100000, .1);
    gtk_box_append(GTK_BOX(b), e->size);
    gtk_box_append(GTK_BOX(b), label("Cylinder sides"));
    e->sides = spin(6, 3, 12, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(e->sides), 0);
    gtk_box_append(GTK_BOX(b), e->sides);
    gtk_box_append(GTK_BOX(b), label("Position"));
    fields(b, e->position, 0, -1e6, 1e6, .1);
    b = section(left, "Mesh");
    row_buttons(e, b, "Create face", 10, "Flip face", 11);
    row_buttons(e, b, "Duplicate", 12, "Delete", 13);
    row_buttons(e, b, "Merge vertices", 14, "Clean up", 15);
    gtk_box_append(GTK_BOX(b), label("Extrusion"));
    e->distance = spin(.2, -100000, 100000, .05);
    gtk_box_append(GTK_BOX(b), e->distance);
    gtk_box_append(GTK_BOX(b), button(e, "Extrude face", 16));
    b = section(left, "Display");
    e->grid_check = gtk_check_button_new_with_label("Grid and axes");
    e->edge_check = gtk_check_button_new_with_label("Mesh edges");
    e->ydown_check = gtk_check_button_new_with_label("Y down (PS1)");
    GtkWidget *checks[] = {e->grid_check, e->edge_check, e->ydown_check};
    for (int i = 0; i < 3; i++) {
        gtk_check_button_set_active(GTK_CHECK_BUTTON(checks[i]), TRUE);
        gtk_box_append(GTK_BOX(b), checks[i]);
        g_signal_connect(checks[i], "toggled", G_CALLBACK(check), e);
    }
    GtkWidget *views = box(GTK_ORIENTATION_HORIZONTAL, 4);
    const char *vnames[] = {"Perspective", "Front", "Side", "Top", NULL},
               *shades[] = {"Solid", "Wireframe", "Texture", NULL};
    e->view_drop = drop(vnames);
    e->shade_drop = drop(shades);
    gtk_box_append(GTK_BOX(views), e->view_drop);
    gtk_box_append(GTK_BOX(views), button(e, "Frame all", 112));
    gtk_box_append(GTK_BOX(views), e->shade_drop);
    gtk_box_append(GTK_BOX(scene), views);
    e->area = gtk_gl_area_new();
    gtk_widget_set_focusable(e->area, TRUE);
    gtk_widget_set_hexpand(e->area, TRUE);
    gtk_widget_set_vexpand(e->area, TRUE);
    gtk_widget_set_size_request(e->area, 320, 320);
    gtk_box_append(GTK_BOX(scene), e->area);
    gtk_box_append(GTK_BOX(scene),
                   label("Click: select · Shift: add · Drag empty: box select\nRight-drag: orbit · "
                         "Middle-drag: pan · Scroll: zoom"));
    render_connect(e);
    GtkGesture *drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), 0);
    g_signal_connect(drag, "drag-begin", G_CALLBACK(drag_begin), e);
    g_signal_connect(drag, "drag-update", G_CALLBACK(drag_update), e);
    g_signal_connect(drag, "drag-end", G_CALLBACK(drag_end), e);
    gtk_widget_add_controller(e->area, GTK_EVENT_CONTROLLER(drag));
    GtkEventController *wheel =
        gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(wheel, "scroll", G_CALLBACK(scroll), e);
    gtk_widget_add_controller(e->area, wheel);
    GtkEventController *keyboard = gtk_event_controller_key_new();
    g_signal_connect(keyboard, "key-pressed", G_CALLBACK(key), e);
    gtk_widget_add_controller(GTK_WIDGET(e->window), keyboard);
    b = section(right, "Properties");
    gtk_box_append(GTK_BOX(b), label("Model name"));
    e->name = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(e->name), 48);
    gtk_box_append(GTK_BOX(b), e->name);
    g_signal_connect(e->name, "changed", G_CALLBACK(name_changed), e);
    gtk_box_append(GTK_BOX(b), label("Selected vertex"));
    fields(b, e->vertex, 0, -1e6, 1e6, .01);
    for (int i = 0; i < 3; i++)
        g_signal_connect(e->vertex[i], "value-changed", G_CALLBACK(numeric), e);
    b = section(right, "Selection transform");
    const char *operations[] = {"Move (units)", "Rotate (degrees)", "Scale (factors)", NULL},
               *pivots[] = {"Selection center", "Origin 0, 0, 0", NULL};
    e->transform_drop = drop(operations);
    gtk_box_append(GTK_BOX(b), e->transform_drop);
    fields(b, e->transform, 0, -100000, 100000, .1);
    e->pivot_drop = drop(pivots);
    gtk_box_append(GTK_BOX(b), e->pivot_drop);
    gtk_box_append(GTK_BOX(b), button(e, "Apply", 108));
    b = section(right, "Texture and UVs");
    gtk_box_append(GTK_BOX(b), button(e, "Load PNG texture…", 104));
    row_buttons(e, b, "Remove", 106, "Export MTL…", 105);
    e->uv_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(e->uv_area), 240);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(e->uv_area), 200);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(e->uv_area), ui_texture_draw, e, NULL);
    gtk_box_append(GTK_BOX(b), e->uv_area);
    GtkGesture *uv = gtk_gesture_drag_new();
    g_signal_connect(uv, "drag-begin", G_CALLBACK(uv_begin), e);
    g_signal_connect(uv, "drag-update", G_CALLBACK(uv_update), e);
    g_signal_connect(uv, "drag-end", G_CALLBACK(uv_end), e);
    gtk_widget_add_controller(e->uv_area, GTK_EVENT_CONTROLLER(uv));
    const char *planes[] = {"Automatic per face", "XY plane", "XZ plane", "ZY plane", NULL};
    GtkWidget *plane = drop(planes);
    g_signal_connect(plane, "notify::selected", G_CALLBACK(uv_plane), e);
    gtk_box_append(GTK_BOX(b), plane);
    gtk_box_append(GTK_BOX(b), button(e, "Map selection UVs", 107));
    for (int j = 0; j < 4; j++) {
        GtkWidget *r = box(GTK_ORIENTATION_HORIZONTAL, 3);
        char *s = g_strdup_printf("%d U/V", j + 1);
        gtk_box_append(GTK_BOX(r), label(s));
        g_free(s);
        for (int k = 0; k < 2; k++) {
            e->uv_fields[j][k] = spin(0, -1e6, 1e6, .01);
            gtk_widget_set_size_request(e->uv_fields[j][k], 70, -1);
            gtk_box_append(GTK_BOX(r), e->uv_fields[j][k]);
            g_signal_connect(e->uv_fields[j][k], "value-changed", G_CALLBACK(numeric), e);
        }
        gtk_box_append(GTK_BOX(b), r);
    }
    b = section(right, "PS1 loader");
    e->info = label("");
    gtk_box_append(GTK_BOX(b), e->info);
    gtk_box_append(GTK_BOX(b), label("One pose, one PNG. No rig, timeline or TIM conversion. Y "
                                     "down affects only the view."));
    GtkWidget *drops[] = {e->mode_drop, e->view_drop, e->shade_drop, e->axis_drop,
                          e->transform_drop};
    for (int i = 0; i < 5; i++)
        g_signal_connect(drops[i], "notify::selected", G_CALLBACK(setting), e);
    e->status = label("");
    gtk_widget_add_css_class(e->status, "status");
    gtk_box_append(GTK_BOX(root), e->status);
    editor_refresh(e);
    gtk_window_present(e->window);
}
void ui_smoke(Editor *e) {
    char *original = project_encode(e);
    Mesh *m = &e->mesh;
    mesh_clear(m);
    mesh_init(m);
    mesh_primitive(m, 0, 1, 6, (Vec){0, 0, 0}, NULL);
    selected_clear(e);
    e->mode = 1;
    int n = 1;
    g_array_append_val(e->selection, n);
    editor_refresh(e);
    action(e, 16);
    g_assert_cmpuint(m->faces->len, ==, 10);
    g_assert_cmpuint(m->vertices->len, ==, 12);
    editor_history(e, FALSE);
    g_assert_cmpuint(m->faces->len, ==, 6);
    editor_history(e, TRUE);
    g_assert_cmpuint(m->faces->len, ==, 10);
    selected_clear(e);
    e->mode = 0;
    n = 0;
    g_array_append_val(e->selection, n);
    editor_refresh(e);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e->vertex[0]), -.8);
    g_assert_cmpfloat(AT(m->vertices, Vec, 0).x, ==, -.8);
    e->mode = 1;
    selected_clear(e);
    n = 0;
    g_array_append_val(e->selection, n);
    editor_refresh(e);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e->uv_fields[0][0]), .2);
    g_assert_cmpfloat(AT(m->uvs, UV, AT(m->faces, Face, 0).uv[0]).u, ==, .2);
    action(e, 107);
    g_assert_cmpuint(m->uvs->len, ==, 44);
    e->mode = 2;
    selected_clear(e);
    action(e, 12);
    g_assert_cmpuint(m->faces->len, ==, 20);
    action(e, 13);
    g_assert_cmpuint(m->faces->len, ==, 10);
    action(e, 15);
    g_assert_cmpuint(m->vertices->len, ==, 12);
    e->mode = 0;
    selected_clear(e);
    n = 0;
    g_array_append_val(e->selection, n);
    n = 1;
    g_array_append_val(e->selection, n);
    action(e, 14);
    g_assert_cmpuint(m->vertices->len, ==, 11);
    GError *err = NULL;
    char *obj = obj_export(m, TRUE, &err);
    g_assert_no_error(err);
    g_assert_nonnull(obj);
    Mesh copy;
    mesh_init(&copy);
    g_assert_true(obj_parse(obj, &copy, &err));
    g_assert_no_error(err);
    g_assert_cmpuint(copy.faces->len, ==, m->faces->len);
    mesh_clear(&copy);
    g_free(obj);
    g_assert_true(project_decode(e, original, &err));
    g_assert_no_error(err);
    g_free(original);
    selected_clear(e);
    e->mode = 2;
    g_ptr_array_set_size(e->undo, 0);
    g_ptr_array_set_size(e->redo, 0);
    e->dirty = FALSE;
    g_clear_pointer(&e->notice, g_free);
    editor_refresh(e);
    g_print("GTK editing: extrusion, numeric vertices/UVs, mapping, duplicate/delete, cleanup, "
            "merge, history and OBJ round-trip passed\n");
}
