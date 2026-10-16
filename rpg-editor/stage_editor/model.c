#include "stage_editor.h"
#include <string.h>

const char *const kinds[3] = {"planes", "zones", "spawns"};
const int limits[3] = {10, 5, 5};
static GQuark error_domain(void) { return g_quark_from_static_string("psx-stage-editor"); }
double number(JsonObject *o, const char *k) {
    JsonNode *n = o ? json_object_get_member(o, k) : NULL;
    return n && JSON_NODE_HOLDS_VALUE(n) &&
        (json_node_get_value_type(n) == G_TYPE_INT64 || json_node_get_value_type(n) == G_TYPE_DOUBLE)
        ? json_node_get_double(n) : 0;
}
const char *string(JsonObject *o, const char *k) {
    JsonNode *n = o ? json_object_get_member(o, k) : NULL;
    return n && JSON_NODE_HOLDS_VALUE(n) && json_node_get_value_type(n) == G_TYPE_STRING
        ? json_node_get_string(n) : "";
}
JsonArray *array(JsonObject *o, const char *k) {
    JsonNode *n = o ? json_object_get_member(o, k) : NULL;
    return n && JSON_NODE_HOLDS_ARRAY(n) ? json_node_get_array(n) : NULL;
}
JsonObject *stage(Document *d, int sid) {
    if (!d->data || !JSON_NODE_HOLDS_ARRAY(d->data) || sid < 0) return NULL;
    JsonArray *a = json_node_get_array(d->data);
    if ((unsigned)sid >= json_array_get_length(a)) return NULL;
    JsonNode *n = json_array_get_element(a, sid);
    return JSON_NODE_HOLDS_OBJECT(n) ? json_node_get_object(n) : NULL;
}
JsonObject *item(Document *d, int sid, int kind, int index) {
    if (kind < 0 || kind > 2 || index < 0) return NULL;
    JsonArray *a = array(stage(d, sid), kinds[kind]);
    if (!a || (unsigned)index >= json_array_get_length(a)) return NULL;
    JsonNode *n = json_array_get_element(a, index);
    return JSON_NODE_HOLDS_OBJECT(n) ? json_node_get_object(n) : NULL;
}
JsonNode *object_node(JsonObject *o) {
    JsonNode *n = json_node_new(JSON_NODE_OBJECT);
    json_node_take_object(n, o);
    return n;
}
JsonNode *clone_node(JsonNode *node) {
    char *text=json_to_string(node,FALSE);
    JsonNode *copy=json_from_string(text,NULL); g_free(text); return copy;
}
char *serialize(Document *d) {
    JsonGenerator *g = json_generator_new();
    json_generator_set_root(g, d->data);
    json_generator_set_pretty(g, TRUE);
    char *text = json_generator_to_data(g, NULL);
    char *result = g_strconcat(text, "\n", NULL);
    g_free(text); g_object_unref(g);
    return result;
}
gboolean restore(Document *d, const char *text, GError **error) {
    JsonParser *p = json_parser_new();
    if (!json_parser_load_from_data(p, text, -1, error)) { g_object_unref(p); return FALSE; }
    if (d->data) json_node_unref(d->data);
    d->data = json_node_copy(json_parser_get_root(p));
    g_object_unref(p);
    return TRUE;
}
static gboolean integer(JsonObject *o, const char *key, double low, double high) {
    JsonNode *n = o ? json_object_get_member(o, key) : NULL;
    if (!n || !JSON_NODE_HOLDS_VALUE(n)) return FALSE;
    GType t = json_node_get_value_type(n);
    if (t != G_TYPE_INT64 && t != G_TYPE_DOUBLE) return FALSE;
    double v = json_node_get_double(n);
    return isfinite(v) && v == floor(v) && v >= low && v <= high;
}
gboolean tim_name(const char *name) {
    return name && g_regex_match_simple("^[A-Z0-9_]{1,5}\\.TIM$", name, 0, 0);
}
static void bad(GString *errors, int sid, const char *message) {
    if (errors->len) g_string_append_c(errors, '\n');
    g_string_append_printf(errors, "Area %d: %s", sid, message);
}
char *validate(JsonNode *data) {
    GString *e = g_string_new(NULL);
    if (!data || !JSON_NODE_HOLDS_ARRAY(data) || !json_array_get_length(json_node_get_array(data))) {
        g_string_append(e, "At least one stage is required.");
        return g_string_free(e, FALSE);
    }
    Document d = {.data = data};
    unsigned count = json_array_get_length(json_node_get_array(data));
    for (unsigned sid = 0; sid < count; sid++) {
        JsonObject *s = stage(&d, sid);
        if (!s) { bad(e, sid, "Invalid stage object."); continue; }
        if (!*string(s, "name")) bad(e, sid, "A name is required.");
        if (!tim_name(string(s, "tim_0")) || !tim_name(string(s, "tim_1")))
            bad(e, sid, "Use uppercase TIM names, maximum five characters before .TIM.");
        const char *camera[] = {"cam_x", "cam_y", "cam_z", "cam_rx", "cam_ry", "cam_rz"};
        for (int j = 0; j < 6; j++)
            if (!integer(s, camera[j], j < 3 ? INT32_MIN : INT16_MIN, j < 3 ? INT32_MAX : INT16_MAX))
                bad(e, sid, "Invalid camera coordinate or rotation.");
        for (int k = 0; k < 3; k++) {
            JsonArray *a = array(s, kinds[k]);
            if (!a || json_array_get_length(a) > (unsigned)limits[k] || (k == 2 && !json_array_get_length(a))) {
                bad(e, sid, "Platform/zone/spawn limit exceeded, or missing spawn point."); continue;
            }
            for (unsigned j = 0; j < json_array_get_length(a); j++) {
                JsonObject *o = item(&d, sid, k, j);
                const char *rect[] = {"x", "y", "z", "w", "h", "d"};
                const char *spawn[] = {"x", "y", "z", "rx", "ry", "rz"};
                for (int f = 0; f < 6; f++)
                    if (!integer(o, k == 2 ? spawn[f] : rect[f], k == 2 && f >= 3 ? INT16_MIN : INT32_MIN,
                                 k == 2 && f >= 3 ? INT16_MAX : INT32_MAX)) bad(e, sid, "Invalid object coordinate.");
                if (k != 2 && (number(o, "w") <= 0 || number(o, "d") >= 0)) bad(e, sid, "Width must be positive and depth negative.");
                if (k == 1) {
                    gboolean valid_target=integer(o,"stage_id",0,count-1);
                    int target = valid_target ? (int)number(o, "stage_id") : -1;
                    JsonArray *sp = array(stage(&d, target), "spawns");
                    if (!integer(o, "stage_id", 0, count - 1) || !sp ||
                        !integer(o, "spawn_id", 0, (double)json_array_get_length(sp) - 1)) bad(e, sid, "Invalid zone destination.");
                }
            }
        }
        JsonArray *npcs = array(s, "npcs");
        if (json_object_has_member(s, "npcs") && !npcs) bad(e, sid, "Invalid NPC list.");
        unsigned npc_count = 0;
        for (unsigned j = 0; npcs && j < json_array_get_length(npcs); j++) {
            JsonNode *group = json_array_get_element(npcs, j);
            if (!JSON_NODE_HOLDS_OBJECT(group)) { bad(e, sid, "Invalid NPC group."); continue; }
            JsonObject *go = json_node_get_object(group);
            GList *keys = json_object_get_members(go);
            for (GList *key = keys; key; key = key->next) {
                npc_count++;
                JsonNode *nn = json_object_get_member(go, key->data);
                if (!JSON_NODE_HOLDS_OBJECT(nn)) { bad(e, sid, "Invalid NPC."); continue; }
                JsonObject *npc = json_node_get_object(nn);
                const char *fields[] = {"x", "y", "z", "rx", "ry", "rz"};
                for (int f = 0; f < 6; f++) if (!integer(npc, fields[f], f < 3 ? INT32_MIN : INT16_MIN,
                    f < 3 ? INT32_MAX : INT16_MAX)) bad(e, sid, "Invalid NPC coordinate.");
                JsonArray *talk = array(npc, "talk_chars");
                if (!talk || !json_array_get_length(talk)) bad(e, sid, "Missing NPC dialogue.");
                for (unsigned t = 0; talk && t < json_array_get_length(talk); t++) {
                    JsonNode *page = json_array_get_element(talk, t);
                    if (!JSON_NODE_HOLDS_VALUE(page) || json_node_get_value_type(page) != G_TYPE_STRING ||
                        strlen(json_node_get_string(page)) > 77) bad(e, sid, "NPC dialogue must be at most 77 UTF-8 bytes per page.");
                }
            }
            g_list_free(keys);
        }
        if (npc_count > 5) bad(e, sid, "Maximum five NPCs.");
    }
    return g_string_free(e, FALSE);
}
gboolean document_load(Document *d, const char *path, GError **error) {
    GFile *file = g_file_new_for_path(path);
    char *text = NULL, *etag = NULL;
    gboolean ok = g_file_load_contents(file, NULL, &text, NULL, &etag, error);
    g_object_unref(file);
    if (!ok) return FALSE;
    Document temp = {0};
    ok = restore(&temp, text, error);
    g_free(text);
    if (ok) {
        char *checks = validate(temp.data);
        if (*checks) { g_set_error(error, error_domain(), 1, "%s", checks); ok = FALSE; }
        g_free(checks);
    }
    if (!ok) { if (temp.data) json_node_unref(temp.data); g_free(etag); return FALSE; }
    if (d->data) json_node_unref(d->data);
    d->data = temp.data;
    g_free(d->path); d->path = g_canonicalize_filename(path, NULL);
    g_free(d->etag); d->etag = etag;
    g_free(d->saved); d->saved = serialize(d);
    if (!d->undo) d->undo = g_ptr_array_new_with_free_func(g_free);
    if (!d->redo) d->redo = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_set_size(d->undo, 0); g_ptr_array_set_size(d->redo, 0);
    return TRUE;
}
gboolean document_dirty(Document *d) {
    if (!d->data) return FALSE;
    char *current = serialize(d);
    gboolean dirty = g_strcmp0(current, d->saved) != 0;
    g_free(current); return dirty;
}
gboolean document_save(Document *d, GError **error) {
    char *checks = validate(d->data);
    if (*checks) { g_set_error(error, error_domain(), 1, "%s", checks); g_free(checks); return FALSE; }
    g_free(checks);
    char *text = serialize(d), *etag = NULL;
    GFile *file = g_file_new_for_path(d->path);
    /* GIO atomically replaces the file, checks its ETag and keeps a ~ backup. */
    gboolean ok = g_file_replace_contents(file, text, strlen(text), d->etag, TRUE,
        G_FILE_CREATE_NONE, &etag, NULL, error);
    g_object_unref(file);
    if (ok) { g_free(d->etag); d->etag = etag; g_free(d->saved); d->saved = text; }
    else { g_free(text); g_free(etag); }
    return ok;
}
void document_changed(Document *d, char *before) {
    char *after = serialize(d);
    if (g_strcmp0(before, after)) {
        g_ptr_array_add(d->undo, before);
        if (d->undo->len > 60) g_ptr_array_remove_index(d->undo, 0);
        g_ptr_array_set_size(d->redo, 0);
    } else g_free(before);
    g_free(after);
}
void document_undo(Document *d, gboolean redo) {
    GPtrArray *from = redo ? d->redo : d->undo, *to = redo ? d->undo : d->redo;
    if (!from || !from->len) return;
    g_ptr_array_add(to, serialize(d));
    char *text = g_ptr_array_steal_index(from, from->len - 1);
    restore(d, text, NULL); g_free(text);
}
void document_clear(Document *d) {
    if (d->data) json_node_unref(d->data);
    if (d->undo) g_ptr_array_unref(d->undo);
    if (d->redo) g_ptr_array_unref(d->redo);
    g_free(d->path); g_free(d->root); g_free(d->etag); g_free(d->saved);
    memset(d, 0, sizeof *d);
}
gboolean delete_item(Document *d, int sid, int k, int index, GError **error) {
    if (!item(d, sid, k, index)) return FALSE;
    JsonArray *a = array(stage(d, sid), kinds[k]);
    if (k == 2) {
        if (json_array_get_length(a) == 1) { g_set_error_literal(error, error_domain(), 1, "Every stage needs at least one spawn point."); return FALSE; }
        unsigned count = json_array_get_length(json_node_get_array(d->data));
        for (unsigned s = 0; s < count; s++) {
            JsonArray *zones = array(stage(d, s), "zones");
            for (unsigned i = 0; i < json_array_get_length(zones); i++) {
                JsonObject *z = item(d, s, 1, i);
                if (number(z, "stage_id") == sid && number(z, "spawn_id") == index) {
                    g_set_error_literal(error, error_domain(), 1, "This spawn is referenced by a zone. Change the destination first."); return FALSE;
                }
            }
        }
        for (unsigned s = 0; s < count; s++) {
            JsonArray *zones = array(stage(d, s), "zones");
            for (unsigned i = 0; i < json_array_get_length(zones); i++) {
                JsonObject *z = item(d, s, 1, i);
                if (number(z, "stage_id") == sid && number(z, "spawn_id") > index)
                    json_object_set_int_member(z, "spawn_id", number(z, "spawn_id") - 1);
            }
        }
    }
    json_array_remove_element(a, index); return TRUE;
}
Point coordinates(JsonObject *o) { return (Point){number(o, "x"), number(o, "y"), number(o, "z")}; }
void rotation(JsonObject *s, double r[3][3]) {
    double x = number(s, "cam_rx") * G_PI / 2048, y = number(s, "cam_ry") * G_PI / 2048, z = number(s, "cam_rz") * G_PI / 2048;
    double sx = sin(x), sy = sin(y), sz = sin(z), cx = cos(x), cy = cos(y), cz = cos(z);
    double m[3][3] = {{cy*cz, -cy*sz, sy}, {cx*sz+sx*sy*cz, cx*cz-sx*sy*sz, -sx*cy},
        {sx*sz-cx*sy*cz, sx*cz+cx*sy*sz, cx*cy}};
    memcpy(r, m, sizeof m);
}
static Point camera(JsonObject *s, Point p) {
    double r[3][3]; rotation(s, r);
    double q[] = {p.x + number(s, "cam_x"), p.y + number(s, "cam_y"), p.z + number(s, "cam_z")};
    return (Point){r[0][0]*q[0]+r[0][1]*q[1]+r[0][2]*q[2], r[1][0]*q[0]+r[1][1]*q[1]+r[1][2]*q[2],
        r[2][0]*q[0]+r[2][1]*q[1]+r[2][2]*q[2]};
}
gboolean project(JsonObject *s, View *v, Point p, Screen *out) {
    if (v->top) { *out = (Screen){160 + (p.x-v->x)*v->scale, 120 - (p.z-v->z)*v->scale}; return TRUE; }
    p = camera(s, p);
    if (p.z <= 1) return FALSE;
    *out = (Screen){160 + 512*p.x/p.z, 120 + 512*p.y/p.z}; return TRUE;
}
gboolean unproject(JsonObject *s, View *v, Screen p, double y, Point *out) {
    if (v->top) { *out = (Point){v->x+(p.x-160)/v->scale, y, v->z-(p.y-120)/v->scale}; return TRUE; }
    double r[3][3], ray[] = {(p.x-160)/512, (p.y-120)/512, 1}, d[3] = {0};
    rotation(s, r);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) d[i] += r[j][i]*ray[j];
    if (fabs(d[1]) < 1e-8) return FALSE;
    double t = (y + number(s, "cam_y"))/d[1];
    if (!isfinite(t) || t <= 0) return FALSE;
    *out = (Point){-number(s,"cam_x")+t*d[0], y, -number(s,"cam_z")+t*d[2]}; return TRUE;
}
void corners(JsonObject *o, Point p[4], gboolean exact) {
    Point a = coordinates(o);
    double right = a.x + number(o, "w"), bottom = a.z + number(o, "d");
    if (exact) { a.x--; a.z++; }
    p[0] = a; p[1] = (Point){right,a.y,a.z}; p[2] = (Point){right,a.y,bottom}; p[3] = (Point){a.x,a.y,bottom};
}
int polygon(JsonObject *s, View *v, JsonObject *o, Screen out[8]) {
    Point p[4], clipped[8]; corners(o, p, TRUE);
    if (v->top) { for (int i = 0; i < 4; i++) project(s, v, p[i], &out[i]); return 4; }
    for (int i = 0; i < 4; i++) p[i] = camera(s, p[i]);
    int n = 0;
    for (int i = 0; i < 4; i++) {
        Point a = p[i], b = p[(i+1)%4];
        gboolean ia = a.z >= 256, ib = b.z >= 256;
        if (ia) clipped[n++] = a;
        if (ia != ib) {
            double t = (256-a.z)/(b.z-a.z);
            clipped[n++] = (Point){a.x+t*(b.x-a.x),a.y+t*(b.y-a.y),256};
        }
    }
    if (n < 3) return 0;
    for (int i = 0; i < n; i++) out[i] = (Screen){160+512*clipped[i].x/clipped[i].z,120+512*clipped[i].y/clipped[i].z};
    return n;
}
gboolean inside(Screen p, Screen *points, int count) {
    gboolean hit = FALSE;
    for (int i = 0, j = count - 1; i < count; j = i++) {
        Screen a = points[i], b = points[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x) hit = !hit;
    }
    return hit;
}
void fit_view(Document *d, int sid, View *v) {
    double xmin=-100, xmax=100, zmin=-100, zmax=100;
    for (int k = 0; k < 3; k++) {
        JsonArray *a = array(stage(d,sid),kinds[k]);
        for (unsigned i = 0; a && i < json_array_get_length(a); i++) {
            JsonObject *o = item(d,sid,k,i); Point p[4];
            if (k == 2) p[0] = coordinates(o); else corners(o,p,FALSE);
            for (int j = 0; j < (k == 2 ? 1 : 4); j++) {
                xmin=MIN(xmin,p[j].x); xmax=MAX(xmax,p[j].x); zmin=MIN(zmin,p[j].z); zmax=MAX(zmax,p[j].z);
            }
        }
    }
    v->x=(xmin+xmax)/2; v->z=(zmin+zmax)/2; v->scale=MIN(280/(xmax-xmin),200/(zmax-zmin));
}
double snap_value(double v, int snap) { return floor(v/snap+0.5)*snap; }
JsonObject *rectangle(Point a, Point b, double y, int snap) {
    double xmin=snap_value(MIN(a.x,b.x),snap), xmax=snap_value(MAX(a.x,b.x),snap);
    double zmin=snap_value(MIN(a.z,b.z),snap), zmax=snap_value(MAX(a.z,b.z),snap);
    JsonObject *o = json_object_new();
    json_object_set_int_member(o,"x",xmin); json_object_set_int_member(o,"y",y); json_object_set_int_member(o,"z",zmax);
    json_object_set_int_member(o,"w",MAX(snap,xmax-xmin)); json_object_set_int_member(o,"h",0); json_object_set_int_member(o,"d",-MAX(snap,zmax-zmin));
    return o;
}
char *warnings(Document *d, int sid) {
    GString *w = g_string_new(NULL);
    JsonArray *sp = array(stage(d,sid),"spawns"), *planes = array(stage(d,sid),"planes"), *zones = array(stage(d,sid),"zones");
    for (unsigned i=0; sp && i<json_array_get_length(sp); i++) {
        Point p = coordinates(item(d,sid,2,i)); gboolean walk=FALSE, hit=FALSE;
        for (unsigned j=0; planes && j<json_array_get_length(planes); j++) {
            JsonObject *o=item(d,sid,0,j); Point a=coordinates(o);
            if (p.x>a.x-1 && p.x<a.x+number(o,"w") && p.z>a.z+number(o,"d") && p.z<a.z+1) walk=TRUE;
        }
        for (unsigned j=0; zones && j<json_array_get_length(zones); j++) {
            JsonObject *o=item(d,sid,1,j); Point a=coordinates(o);
            if (p.x<=a.x+number(o,"w") && p.x+100>=a.x && p.z<=a.z && p.z+100>=a.z+number(o,"d")) hit=TRUE;
        }
        if (!walk) g_string_append_printf(w,"Spawn %u is outside the platforms.\n",i);
        if (hit) g_string_append_printf(w,"Spawn %u immediately activates a zone.\n",i);
    }
    return g_string_free(w,FALSE);
}
void next_names(Document *d, char **left, char **right) {
    unsigned count=json_array_get_length(json_node_get_array(d->data));
    for (unsigned n=count;n<10000;n++) {
        char *a=n<10?g_strdup_printf("BG%u_0.TIM",n):n<100?g_strdup_printf("B%u_0.TIM",n):g_strdup_printf("N%uA.TIM",n);
        char *b=n<10?g_strdup_printf("BG%u_1.TIM",n):n<100?g_strdup_printf("B%u_1.TIM",n):g_strdup_printf("N%uB.TIM",n);
        gboolean used=!tim_name(a)||!tim_name(b);
        for (unsigned s=0;s<count;s++) for (int i=0;i<2;i++) {
            const char *old=string(stage(d,s),i?"tim_1":"tim_0");
            if (!strcmp(old,a)||!strcmp(old,b)) used=TRUE;
        }
        if (!used) { *left=a; *right=b; return; }
        g_free(a); g_free(b);
    }
    *left=g_strdup(""); *right=g_strdup("");
}
char *find_root(const char *hint) {
    char *dir=g_canonicalize_filename(hint,NULL);
    while (TRUE) {
        char *path=g_build_filename(dir,"stages","stages.json",NULL);
        gboolean found=g_file_test(path,G_FILE_TEST_IS_REGULAR); g_free(path);
        if (found) return dir;
        char *parent=g_path_get_dirname(dir);
        if (!strcmp(parent,dir)) { g_free(parent); g_free(dir); return NULL; }
        g_free(dir); dir=parent;
    }
}
