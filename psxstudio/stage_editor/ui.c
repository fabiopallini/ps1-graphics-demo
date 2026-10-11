#include "stage_editor.h"
#include <string.h>
#include <errno.h>

enum { A_OPEN=1,A_RELOAD,A_SAVE,A_EXPORT,A_UNDO,A_REDO,A_NEW,A_PNGS,A_ASSIGN,A_ZIP,A_DELETE,A_DUPLICATE,A_FIT,A_ZOOM_IN,A_ZOOM_OUT,A_CLOSE };
typedef struct { Studio *s; const char *key; gboolean object; } Field;
typedef struct { Studio *s; int action; } Pending;
typedef struct { Studio *s; GtkWidget *window,*name,*left,*right,*file_label,*error; char *path; } NewStage;
static void action(Studio *s,int code);
static void cancel_drag(Studio *s);
static void update_inspector(Studio *s);
static void refresh_list(Studio *s);
static void refresh(Studio *s,gboolean inspector);
static gboolean refresh_idle(gpointer data) {
    Studio *s=data; gboolean inspector=s->queued_inspector; s->queued_inspector=FALSE; s->rebuilding=FALSE;
    if (!s->quitting) refresh(s,inspector);
    return G_SOURCE_REMOVE;
}
static void defer_refresh(Studio *s,gboolean inspector) {
    s->queued_inspector|=inspector;
    if (!s->rebuilding) { s->rebuilding=TRUE; g_idle_add(refresh_idle,s); }
}
static void box_clear(GtkWidget *box) {
    GtkWidget *child;
    while ((child=gtk_widget_get_first_child(box))) gtk_box_remove(GTK_BOX(box),child);
}
static GtkWidget *box(GtkOrientation direction,int spacing) { return gtk_box_new(direction,spacing); }
static GtkWidget *text(const char *value) {
    GtkWidget *w=gtk_label_new(value); gtk_label_set_xalign(GTK_LABEL(w),0); gtk_label_set_wrap(GTK_LABEL(w),TRUE); return w;
}
static void margins(GtkWidget *w,int n) {
    gtk_widget_set_margin_start(w,n); gtk_widget_set_margin_end(w,n); gtk_widget_set_margin_top(w,n); gtk_widget_set_margin_bottom(w,n);
}
static void button_clicked(GtkButton *b,Studio *s) { action(s,GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b),"action"))); }
static GtkWidget *button(Studio *s,const char *name,int code) {
    GtkWidget *w=gtk_button_new_with_label(name); g_object_set_data(G_OBJECT(w),"action",GINT_TO_POINTER(code));
    g_signal_connect(w,"clicked",G_CALLBACK(button_clicked),s); return w;
}
static void add_button(GtkWidget *parent,Studio *s,const char *name,int code) { gtk_box_append(GTK_BOX(parent),button(s,name,code)); }
void ui_notice(Studio *s,const char *message) {
    g_free(s->notice); s->notice=g_strdup(message);
    if (s->status) gtk_label_set_text(GTK_LABEL(s->status),message);
}
static void report(Studio *s,GError *error) { if (error) { ui_notice(s,error->message); g_error_free(error); } }
static void changed(Studio *s,char *before) {
    document_changed(&s->doc,before); g_clear_pointer(&s->notice,g_free); ui_refresh(s);
}
static void choose_tool(Studio *s,int tool) {
    cancel_drag(s); s->tool=tool;
    for (int i=0;i<4;i++) {
        if (i==tool) gtk_widget_add_css_class(s->tool_buttons[i],"active"); else gtk_widget_remove_css_class(s->tool_buttons[i],"active");
    }
    gtk_widget_set_cursor_from_name(s->area,tool?"crosshair":"default");
}
static void tool_clicked(GtkButton *b,Studio *s) { choose_tool(s,GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b),"tool"))); }
static void view_clicked(GtkButton *b,Studio *s) {
    cancel_drag(s); s->view.top=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b),"top"));
    for (int i=0;i<2;i++) {
        if (s->view.top==i) gtk_widget_add_css_class(s->view_buttons[i],"active"); else gtk_widget_remove_css_class(s->view_buttons[i],"active");
    }
    render_queue(s);
}
static void snap_changed(GObject *obj,GParamSpec *pspec,Studio *s) {
    (void)pspec; const int values[]={1,5,10,25}; unsigned i=gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));
    s->snap=values[MIN(i,3)];
}
static void layer_changed(GtkCheckButton *b,Studio *s) {
    int i=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b),"layer")); s->layers[i]=gtk_check_button_get_active(b); render_queue(s);
}
static void stage_clicked(GtkButton *b,Studio *s) {
    cancel_drag(s); s->sid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b),"sid")); s->kind=s->index=-1;
    fit_view(&s->doc,s->sid,&s->view); ui_refresh(s);
}
static void object_clicked(GtkButton *b,Studio *s) {
    cancel_drag(s); s->kind=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b),"kind")); s->index=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b),"index")); ui_refresh(s);
}
static void search_changed(GtkSearchEntry *entry,Studio *s) { (void)entry; refresh_list(s); }
static void refresh_list(Studio *s) {
    if (!s->stages) return;
    box_clear(s->stages); if (!s->doc.data) return;
    char *term=g_utf8_casefold(gtk_editable_get_text(GTK_EDITABLE(s->search)),-1);
    unsigned count=json_array_get_length(json_node_get_array(s->doc.data));
    for (unsigned i=0;i<count;i++) {
        JsonObject *st=stage(&s->doc,i); char *name=g_strdup_printf("%02u  %s",i,string(st,"name")); char *fold=g_utf8_casefold(name,-1);
        gboolean visible=strstr(fold,term)!=NULL; g_free(fold);
        if (!visible) { g_free(name); continue; }
        GtkWidget *b=gtk_button_new(), *row=box(GTK_ORIENTATION_HORIZONTAL,6), *labels=box(GTK_ORIENTATION_VERTICAL,2);
        Background *bg=assets_background(&s->assets,st);
        if (bg && bg->image) {
            GBytes *pixels=gdk_pixbuf_read_pixel_bytes(bg->image);
            GdkTexture *texture=gdk_memory_texture_new(320,240,gdk_pixbuf_get_has_alpha(bg->image)?GDK_MEMORY_R8G8B8A8:GDK_MEMORY_R8G8B8,pixels,gdk_pixbuf_get_rowstride(bg->image));
            g_bytes_unref(pixels); GtkWidget *picture=gtk_picture_new_for_paintable(GDK_PAINTABLE(texture));
            gtk_picture_set_content_fit(GTK_PICTURE(picture),GTK_CONTENT_FIT_CONTAIN); gtk_widget_set_size_request(picture,64,48); gtk_widget_set_hexpand(picture,FALSE);
            gtk_box_append(GTK_BOX(row),picture); g_object_unref(texture);
        }
        GtkWidget *label=text(name); gtk_label_set_ellipsize(GTK_LABEL(label),PANGO_ELLIPSIZE_END); gtk_label_set_wrap(GTK_LABEL(label),FALSE);
        gtk_label_set_max_width_chars(GTK_LABEL(label),18); gtk_box_append(GTK_BOX(labels),label);
        char *counts=g_strdup_printf("%u platforms / %u zones",json_array_get_length(array(st,"planes")),json_array_get_length(array(st,"zones")));
        label=text(counts); gtk_widget_add_css_class(label,"small"); gtk_box_append(GTK_BOX(labels),label);
        gtk_box_append(GTK_BOX(row),labels); gtk_button_set_child(GTK_BUTTON(b),row);
        if (s->sid==(int)i) gtk_widget_add_css_class(b,"active");
        g_object_set_data(G_OBJECT(b),"sid",GINT_TO_POINTER(i)); g_signal_connect(b,"clicked",G_CALLBACK(stage_clicked),s); gtk_box_append(GTK_BOX(s->stages),b);
        g_free(name); g_free(counts);
    }
    g_free(term);
}
static void field_commit(GtkWidget *widget,Field *field) {
    Studio *s=field->s; if (s->refreshing || s->quitting) return;
    JsonObject *o=field->object?item(&s->doc,s->sid,s->kind,s->index):stage(&s->doc,s->sid); if (!o) return;
    const char *key=field->key, *value=gtk_editable_get_text(GTK_EDITABLE(widget));
    gboolean numeric=strcmp(key,"name") && strcmp(key,"tim_0") && strcmp(key,"tim_1");
    gint64 n=0; char *normalized=NULL;
    if (numeric) {
        char *end=NULL; errno=0; n=g_ascii_strtoll(value,&end,10);
        gboolean rotation_=!strncmp(key,"cam_r",5) || !strcmp(key,"rx") || !strcmp(key,"ry") || !strcmp(key,"rz");
        if (!*value || *end || errno || n<(rotation_?INT16_MIN:INT32_MIN) || n>(rotation_?INT16_MAX:INT32_MAX)) {
            char *old=g_strdup_printf("%.0f",number(o,key)); gtk_editable_set_text(GTK_EDITABLE(widget),old); g_free(old);
            ui_notice(s,"Enter an integer within the PS1 coordinate/rotation range."); defer_refresh(s,FALSE); return;
        }
        if (number(o,key)==n) return;
    } else {
        normalized=g_strdup(value); g_strstrip(normalized);
        if (g_str_has_prefix(key,"tim_")) { char *upper=g_ascii_strup(normalized,-1); g_free(normalized); normalized=upper; }
        if (!strcmp(string(o,key),normalized)) { g_free(normalized); return; }
    }
    char *before=serialize(&s->doc);
    if (numeric) json_object_set_int_member(o,key,n);
    else { json_object_set_string_member(o,key,normalized); gtk_editable_set_text(GTK_EDITABLE(widget),normalized); }
    g_free(normalized); document_changed(&s->doc,before); g_clear_pointer(&s->notice,g_free);
    if (g_str_has_prefix(key,"tim_")) s->renderer.uploaded=NULL;
    render_queue(s); defer_refresh(s,FALSE);
}
static void field_activate(GtkEntry *entry,Field *field) { field_commit(GTK_WIDGET(entry),field); }
static void field_leave(GtkEventControllerFocus *controller,Field *field) { field_commit(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller)),field); }
static void commit_focus(Studio *s) {
    GtkWidget *focus=gtk_window_get_focus(GTK_WINDOW(s->window));
    GtkWidget *entry=focus?(GTK_IS_ENTRY(focus)?focus:gtk_widget_get_ancestor(focus,GTK_TYPE_ENTRY)):NULL;
    Field *field=entry?g_object_get_data(G_OBJECT(entry),"field"):NULL;
    if (field) field_commit(entry,field);
}
static gboolean dismissed(GError *error) {
    return g_error_matches(error,GTK_DIALOG_ERROR,GTK_DIALOG_ERROR_CANCELLED) ||
        g_error_matches(error,GTK_DIALOG_ERROR,GTK_DIALOG_ERROR_DISMISSED);
}
static GtkWidget *entry_field(Studio *s,const char *label,const char *key,JsonObject *o,gboolean object) {
    GtkWidget *row=box(GTK_ORIENTATION_VERTICAL,2), *entry=gtk_entry_new(); gtk_box_append(GTK_BOX(row),text(label)); gtk_box_append(GTK_BOX(row),entry);
    gboolean numeric=strcmp(key,"name") && strcmp(key,"tim_0") && strcmp(key,"tim_1");
    char *value=numeric?g_strdup_printf("%.0f",number(o,key)):g_strdup(string(o,key));
    gtk_editable_set_text(GTK_EDITABLE(entry),value); g_free(value); gtk_editable_set_width_chars(GTK_EDITABLE(entry),10);
    if (numeric) gtk_entry_set_input_purpose(GTK_ENTRY(entry),GTK_INPUT_PURPOSE_NUMBER);
    Field *field=g_new0(Field,1); *field=(Field){s,key,object}; g_object_set_data_full(G_OBJECT(entry),"field",field,g_free);
    g_signal_connect(entry,"activate",G_CALLBACK(field_activate),field);
    GtkEventController *focus=gtk_event_controller_focus_new(); g_signal_connect(focus,"leave",G_CALLBACK(field_leave),field); gtk_widget_add_controller(entry,focus);
    return row;
}
static GtkWidget *fields(Studio *s,JsonObject *o,const char *const *names,int count,gboolean object) {
    GtkWidget *grid=gtk_grid_new(); gtk_grid_set_column_spacing(GTK_GRID(grid),8); gtk_grid_set_row_spacing(GTK_GRID(grid),6);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid),TRUE);
    for (int i=0;i<count;i++) { GtkWidget *f=entry_field(s,names[i],names[i],o,object); gtk_grid_attach(GTK_GRID(grid),f,i%2,i/2,1,1); }
    return grid;
}
static GtkWidget *section(Studio *s,const char *name,gboolean open) {
    GtkWidget *expander=gtk_expander_new(name), *content=box(GTK_ORIENTATION_VERTICAL,6); margins(content,6);
    gtk_expander_set_child(GTK_EXPANDER(expander),content); gtk_expander_set_expanded(GTK_EXPANDER(expander),open);
    gtk_box_append(GTK_BOX(s->inspector),expander); return content;
}
static void destination_changed(GObject *obj,GParamSpec *pspec,Field *f) {
    (void)pspec; Studio *s=f->s; if (s->refreshing) return;
    JsonObject *o=item(&s->doc,s->sid,s->kind,s->index); if (!o) return;
    unsigned value=gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));
    if (value==GTK_INVALID_LIST_POSITION || number(o,f->key)==value) return;
    char *before=serialize(&s->doc); json_object_set_int_member(o,f->key,value);
    if (!strcmp(f->key,"stage_id")) json_object_set_int_member(o,"spawn_id",0);
    document_changed(&s->doc,before); g_clear_pointer(&s->notice,g_free); render_queue(s); defer_refresh(s,TRUE);
}
static void destination(Studio *s,GtkWidget *parent,JsonObject *o,gboolean spawn) {
    GtkStringList *list=gtk_string_list_new(NULL);
    JsonArray *a=spawn?array(stage(&s->doc,(int)number(o,"stage_id")),"spawns"):json_node_get_array(s->doc.data);
    for (unsigned i=0;a && i<json_array_get_length(a);i++) {
        char *label=spawn?g_strdup_printf("%u / X %.0f, Z %.0f",i,number(item(&s->doc,(int)number(o,"stage_id"),2,i),"x"),number(item(&s->doc,(int)number(o,"stage_id"),2,i),"z")):
            g_strdup_printf("%u / %s",i,string(stage(&s->doc,i),"name"));
        gtk_string_list_append(list,label); g_free(label);
    }
    GtkWidget *drop=gtk_drop_down_new(G_LIST_MODEL(list),NULL); const char *key=spawn?"spawn_id":"stage_id";
    gtk_drop_down_set_selected(GTK_DROP_DOWN(drop),(unsigned)number(o,key));
    Field *f=g_new0(Field,1); *f=(Field){s,key,TRUE}; g_object_set_data_full(G_OBJECT(drop),"field",f,g_free);
    g_signal_connect(drop,"notify::selected",G_CALLBACK(destination_changed),f);
    gtk_box_append(GTK_BOX(parent),text(spawn?"Destination spawn":"Destination stage")); gtk_box_append(GTK_BOX(parent),drop);
}
static void update_inspector(Studio *s) {
    gboolean settings_open=TRUE,camera_open=FALSE,objects_open=TRUE;
    for (GtkWidget *child=gtk_widget_get_first_child(s->inspector);child;child=gtk_widget_get_next_sibling(child)) {
        if (!GTK_IS_EXPANDER(child)) continue;
        const char *label=gtk_expander_get_label(GTK_EXPANDER(child)); gboolean open=gtk_expander_get_expanded(GTK_EXPANDER(child));
        if (!strcmp(label,"Stage settings")) settings_open=open;
        if (!strcmp(label,"Game camera")) camera_open=open;
        if (!strcmp(label,"Platforms, zones and spawns")) objects_open=open;
    }
    box_clear(s->inspector); JsonObject *st=stage(&s->doc,s->sid); if (!st) return;
    JsonObject *o=item(&s->doc,s->sid,s->kind,s->index);
    if (!o) { s->kind=s->index=-1; gtk_box_append(GTK_BOX(s->inspector),text("Select a shape on the background to edit it. Drag two corners to draw a platform or zone.")); }
    else {
        const char *titles[]={"Platform","Zone","Spawn point"}; char *title=g_strdup_printf("%s %d",titles[s->kind],s->index);
        gtk_box_append(GTK_BOX(s->inspector),text(title)); g_free(title);
        const char *rect[]={"x","y","z","w","h","d"}, *spawn[]={"x","y","z","rx","ry","rz"};
        gtk_box_append(GTK_BOX(s->inspector),fields(s,o,s->kind==2?spawn:rect,6,TRUE));
        if (s->kind==1) { destination(s,s->inspector,o,FALSE); destination(s,s->inspector,o,TRUE); }
        GtkWidget *actions=box(GTK_ORIENTATION_HORIZONTAL,6); add_button(actions,s,"Duplicate",A_DUPLICATE); add_button(actions,s,"Delete",A_DELETE); gtk_box_append(GTK_BOX(s->inspector),actions);
        if (s->kind!=2) gtk_box_append(GTK_BOX(s->inspector),text("Positive W / negative D. Drag the corner handles to resize."));
    }
    GtkWidget *settings=section(s,"Stage settings",settings_open);
    gtk_box_append(GTK_BOX(settings),entry_field(s,"Name","name",st,FALSE));
    const char *tim[]={"tim_0","tim_1"}; gtk_box_append(GTK_BOX(settings),fields(s,st,tim,2,FALSE));
    add_button(settings,s,"Assign PNG background",A_ASSIGN);
    Background *bg=assets_background(&s->assets,st); if (bg && bg->directory) add_button(settings,s,"Export PNG + TIM package",A_ZIP);
    GtkWidget *camera=section(s,"Game camera",camera_open); gtk_box_append(GTK_BOX(camera),text("PS1 angles: 4096 units per turn."));
    const char *cam[]={"cam_x","cam_y","cam_z","cam_rx","cam_ry","cam_rz"}; gtk_box_append(GTK_BOX(camera),fields(s,st,cam,6,FALSE));
    GtkWidget *objects=section(s,"Platforms, zones and spawns",objects_open);
    for (int k=0;k<3;k++) {
        JsonArray *a=array(st,kinds[k]); char *count=g_strdup_printf("%s / %u of %d",kinds[k],json_array_get_length(a),limits[k]);
        gtk_box_append(GTK_BOX(objects),text(count)); g_free(count);
        for (unsigned i=0;i<json_array_get_length(a);i++) {
            JsonObject *p=item(&s->doc,s->sid,k,i); char *name=k==1?g_strdup_printf("Zone %u > %.0f / %.0f",i,number(p,"stage_id"),number(p,"spawn_id")):
                g_strdup_printf("%s %u / %.0f, %.0f",k==0?"Platform":"Spawn",i,number(p,"x"),number(p,"z"));
            GtkWidget *b=gtk_button_new_with_label(name); g_free(name); g_object_set_data(G_OBJECT(b),"kind",GINT_TO_POINTER(k)); g_object_set_data(G_OBJECT(b),"index",GINT_TO_POINTER(i));
            if (s->kind==k && s->index==(int)i) gtk_widget_add_css_class(b,"active");
            g_signal_connect(b,"clicked",G_CALLBACK(object_clicked),s); gtk_box_append(GTK_BOX(objects),b);
        }
    }
    gtk_box_append(GTK_BOX(s->inspector),text("NPCs and dialogues are preserved.\n\nAfter saving: make run in stages, then bash build.sh from the project root."));
}
static void refresh(Studio *s,gboolean inspector) {
    if (!s->window || s->quitting) return;
    s->refreshing=TRUE;
    if (s->doc.data) s->sid=CLAMP(s->sid,0,(int)json_array_get_length(json_node_get_array(s->doc.data))-1);
    JsonObject *st=stage(&s->doc,s->sid); char *title=st?g_strdup_printf("AREA %02d / %s",s->sid,string(st,"name")):g_strdup("Open stages.json to start");
    gtk_label_set_text(GTK_LABEL(s->title),title); g_free(title);
    refresh_list(s); if (inspector) update_inspector(s);
    if (s->doc.data) {
        char *errors=validate(s->doc.data), *warn=warnings(&s->doc,s->sid);
        Background *bg=assets_background(&s->assets,st);
        char *message=g_strconcat(errors,*errors?"\n":"",warn,bg && bg->directory?"Imported background: export PNG/TIM and register CD entries before building.":"",NULL);
        gtk_label_set_text(GTK_LABEL(s->messages),message); g_free(message); g_free(errors); g_free(warn);
    }
    gtk_widget_set_sensitive(s->undo_button,s->doc.undo && s->doc.undo->len); gtk_widget_set_sensitive(s->redo_button,s->doc.redo && s->doc.redo->len);
    if (!s->notice) {
        char *status=g_strdup_printf("%s / %s",document_dirty(&s->doc)?"Unsaved changes":"All changes saved",s->doc.path?s->doc.path:"No document loaded");
        gtk_label_set_text(GTK_LABEL(s->status),status); g_free(status);
    } else gtk_label_set_text(GTK_LABEL(s->status),s->notice);
    s->refreshing=FALSE; render_queue(s);
}
void ui_refresh(Studio *s) { refresh(s,TRUE); }
static void cancel_drag(Studio *s) {
    if (s->before) { restore(&s->doc,s->before,NULL); g_clear_pointer(&s->before,g_free); }
    g_clear_pointer(&s->original,json_node_unref); g_clear_pointer(&s->preview,json_node_unref); s->dragging=FALSE;
    render_queue(s);
}
static int pick(Studio *s,Screen point,int *kind,int *index) {
    JsonObject *st=stage(&s->doc,s->sid), *selected=item(&s->doc,s->sid,s->kind,s->index);
    if (selected && s->kind<2 && s->layers[s->kind]) {
        Point p[4]; corners(selected,p,FALSE);
        for (int i=0;i<4;i++) {
            Screen q; if (project(st,&s->view,p[i],&q) && hypot(q.x-point.x,q.y-point.y)<5) { *kind=s->kind; *index=s->index; return i; }
        }
    }
    for (int k=2;k>=0;k--) {
        if (!s->layers[k]) continue;
        JsonArray *a=array(st,kinds[k]);
        for (int i=(int)json_array_get_length(a)-1;i>=0;i--) {
            JsonObject *o=item(&s->doc,s->sid,k,i); Screen q,p[8]; gboolean hit;
            if (k==2) hit=project(st,&s->view,coordinates(o),&q) && hypot(q.x-point.x,q.y-point.y)<5;
            else { int n=polygon(st,&s->view,o,p); hit=inside(point,p,n); }
            if (hit) { *kind=k; *index=i; return -1; }
        }
    }
    *kind=*index=-1; return -1;
}
static void drag_begin(GtkGestureDrag *gesture,double x,double y,Studio *s) {
    (void)gesture; if (!stage(&s->doc,s->sid)) return;
    gtk_widget_grab_focus(s->area); cancel_drag(s); s->drag_x=x; s->drag_y=y;
    Screen screen=render_pointer(s,x,y); if (screen.x<0 || screen.x>320 || screen.y<0 || screen.y>240) return;
    s->screen_start=screen;
    if (!s->tool) {
        s->corner=pick(s,screen,&s->kind,&s->index); ui_refresh(s);
        JsonObject *o=item(&s->doc,s->sid,s->kind,s->index); if (!o) return;
        if (!unproject(stage(&s->doc,s->sid),&s->view,screen,number(o,"y"),&s->start)) { ui_notice(s,"No ground intersection. Use Top view."); return; }
        s->original=clone_node(json_array_get_element(array(stage(&s->doc,s->sid),kinds[s->kind]),s->index));
        s->drag_type=s->corner>=0?2:1;
    } else {
        int k=s->tool-1; JsonArray *a=array(stage(&s->doc,s->sid),kinds[k]);
        if (json_array_get_length(a)>=(unsigned)limits[k]) { ui_notice(s,"PS1 object limit reached for this stage."); return; }
        if (!unproject(stage(&s->doc,s->sid),&s->view,screen,0,&s->start)) { ui_notice(s,"No ground intersection. Use Top view."); return; }
        if (k==2) {
            char *before=serialize(&s->doc); JsonObject *spawn=json_object_new();
            json_object_set_int_member(spawn,"x",snap_value(s->start.x,s->snap)); json_object_set_int_member(spawn,"y",0); json_object_set_int_member(spawn,"z",snap_value(s->start.z,s->snap));
            json_object_set_int_member(spawn,"rx",0); json_object_set_int_member(spawn,"ry",0); json_object_set_int_member(spawn,"rz",0);
            json_array_add_object_element(a,spawn); s->kind=k; s->index=json_array_get_length(a)-1; choose_tool(s,0); changed(s,before); return;
        }
        s->drag_type=0;
    }
    s->before=serialize(&s->doc); s->dragging=TRUE;
}
static void drag_update(GtkGestureDrag *gesture,double dx,double dy,Studio *s) {
    (void)gesture; if (!s->dragging) return;
    Screen screen=render_pointer(s,s->drag_x+dx,s->drag_y+dy); Point world;
    double y=s->original?number(json_node_get_object(s->original),"y"):0;
    if (!unproject(stage(&s->doc,s->sid),&s->view,screen,y,&world)) return;
    if (!s->drag_type) { g_clear_pointer(&s->preview,json_node_unref); s->preview=object_node(rectangle(s->start,world,0,s->snap)); }
    else {
        JsonObject *o=item(&s->doc,s->sid,s->kind,s->index), *original=json_node_get_object(s->original);
        if (s->drag_type==1) {
            json_object_set_int_member(o,"x",number(original,"x")+snap_value(world.x-s->start.x,s->snap));
            json_object_set_int_member(o,"z",number(original,"z")+snap_value(world.z-s->start.z,s->snap));
        } else {
            Point p[4]; corners(original,p,FALSE); JsonObject *r=rectangle(p[(s->corner+2)%4],world,y,s->snap);
            const char *keys[]={"x","z","w","d"}; for (int i=0;i<4;i++) json_object_set_int_member(o,keys[i],number(r,keys[i])); json_object_unref(r);
        }
    }
    render_queue(s);
}
static void drag_end(GtkGestureDrag *gesture,double dx,double dy,Studio *s) {
    if (!s->dragging) return;
    drag_update(gesture,dx,dy,s);
    if (!s->drag_type) {
        Screen end=render_pointer(s,s->drag_x+dx,s->drag_y+dy);
        if (!s->preview || hypot(end.x-s->screen_start.x,end.y-s->screen_start.y)<2) { cancel_drag(s); ui_refresh(s); return; }
        int k=s->tool-1; JsonObject *o=json_node_get_object(s->preview);
        if (k==1) {
            unsigned count=json_array_get_length(json_node_get_array(s->doc.data));
            json_object_set_int_member(o,"stage_id",s->sid?s->sid-1:(int)MIN(1u,count-1)); json_object_set_int_member(o,"spawn_id",0);
        }
        JsonArray *a=array(stage(&s->doc,s->sid),kinds[k]); json_array_add_element(a,s->preview); s->preview=NULL; s->kind=k; s->index=json_array_get_length(a)-1;
    }
    char *before=s->before; s->before=NULL; s->dragging=FALSE; g_clear_pointer(&s->original,json_node_unref); choose_tool(s,0); changed(s,before);
}
static void gesture_cancel(GtkGesture *gesture,GdkEventSequence *sequence,Studio *s) { (void)gesture; (void)sequence; cancel_drag(s); ui_refresh(s); }
static void motion(GtkEventControllerMotion *controller,double x,double y,Studio *s) {
    (void)controller; JsonObject *st=stage(&s->doc,s->sid); if (!st) return;
    Point p; JsonObject *o=item(&s->doc,s->sid,s->kind,s->index);
    gboolean ok=unproject(st,&s->view,render_pointer(s,x,y),number(o,"y"),&p);
    char *value=ok?g_strdup_printf("X %.0f / Z %.0f",p.x,p.z):g_strdup("No ground intersection");
    gtk_label_set_text(GTK_LABEL(s->coordinates),value); g_free(value);
}
static gboolean key_pressed(GtkEventControllerKey *controller,guint key,guint keycode,GdkModifierType state_,Studio *s) {
    (void)controller; (void)keycode;
    gboolean command=state_&(GDK_CONTROL_MASK|GDK_META_MASK);
    if (command && (key==GDK_KEY_s || key==GDK_KEY_S)) { action(s,A_SAVE); return TRUE; }
    GtkWidget *focus=gtk_window_get_focus(GTK_WINDOW(s->window));
    if (focus && (GTK_IS_EDITABLE(focus)||GTK_IS_TEXT_VIEW(focus)||gtk_widget_get_ancestor(focus,GTK_TYPE_DROP_DOWN))) return FALSE;
    if (command && (key==GDK_KEY_z || key==GDK_KEY_Z)) { action(s,state_&GDK_SHIFT_MASK?A_REDO:A_UNDO); return TRUE; }
    if (command && key==GDK_KEY_y) { action(s,A_REDO); return TRUE; }
    if (key==GDK_KEY_Escape) { cancel_drag(s); s->kind=s->index=-1; choose_tool(s,0); ui_refresh(s); return TRUE; }
    if (key==GDK_KEY_Delete || key==GDK_KEY_BackSpace) { action(s,A_DELETE); return TRUE; }
    if (!command) {
        guint keys[]={GDK_KEY_v,GDK_KEY_p,GDK_KEY_z,GDK_KEY_i};
        for (int i=0;i<4;i++) if (gdk_keyval_to_lower(key)==keys[i]) { choose_tool(s,i); return TRUE; }
    }
    return FALSE;
}
static gboolean save(Studio *s) {
    if (!s->doc.data) { ui_notice(s,"Open stages.json first."); return FALSE; }
    commit_focus(s);
    cancel_drag(s); GError *error=NULL;
    if (!document_save(&s->doc,&error)) { report(s,error); return FALSE; }
    ui_notice(s,"Saved stages.json / previous version kept in stages.json~"); ui_refresh(s); return TRUE;
}
static void set_asset_root(Studio *s,const char *path) {
    char *parent=g_path_get_dirname(path), *root=find_root(parent); g_free(parent);
    if (root && g_strcmp0(root,s->doc.root)) {
        s->renderer.uploaded=NULL; assets_clear(&s->assets); assets_init(&s->assets,root); g_free(s->doc.root); s->doc.root=root;
    } else g_free(root);
}
static GtkFileDialog *file_dialog(Studio *s,const char *title,const char *pattern) {
    GtkFileDialog *dialog=gtk_file_dialog_new(); gtk_file_dialog_set_title(dialog,title); gtk_file_dialog_set_modal(dialog,TRUE);
    GFile *folder=g_file_new_for_path(s->doc.root); gtk_file_dialog_set_initial_folder(dialog,folder); g_object_unref(folder);
    if (pattern) {
        GtkFileFilter *filter=gtk_file_filter_new(); gtk_file_filter_set_name(filter,pattern); gtk_file_filter_add_pattern(filter,pattern);
        char *upper=g_ascii_strup(pattern,-1); gtk_file_filter_add_pattern(filter,upper); g_free(upper);
        GListStore *filters=g_list_store_new(GTK_TYPE_FILE_FILTER); g_list_store_append(filters,filter);
        gtk_file_dialog_set_filters(dialog,G_LIST_MODEL(filters)); gtk_file_dialog_set_default_filter(dialog,filter); g_object_unref(filter); g_object_unref(filters);
    }
    return dialog;
}
static void file_done(GObject *object,GAsyncResult *result,gpointer data) {
    Pending *job=data; Studio *s=job->s; int code=job->action; GError *error=NULL;
    GFile *file=code==A_EXPORT || code==A_ZIP?gtk_file_dialog_save_finish(GTK_FILE_DIALOG(object),result,&error):gtk_file_dialog_open_finish(GTK_FILE_DIALOG(object),result,&error);
    if (!file) { if (error && !dismissed(error)) report(s,error); else g_clear_error(&error); g_free(job); return; }
    char *path=g_file_get_path(file); g_object_unref(file);
    if (!path) { ui_notice(s,"Choose a local file."); g_free(job); return; }
    if (code==A_OPEN) {
        if (document_load(&s->doc,path,&error)) { set_asset_root(s,path); s->sid=0; s->kind=s->index=-1; fit_view(&s->doc,s->sid,&s->view); g_clear_pointer(&s->notice,g_free); }
    } else if (code==A_EXPORT) {
        char *canonical=g_canonicalize_filename(path,NULL);
        if (!g_strcmp0(canonical,s->doc.path)) ui_notice(s,"Choose a different file for the copy, or use Save JSON.");
        else {
            char *value=serialize(&s->doc);
            if (g_file_set_contents_full(path,value,-1,G_FILE_SET_CONTENTS_CONSISTENT,0644,&error)) ui_notice(s,"JSON copy exported. Use Save JSON to save the open document.");
            g_free(value);
        }
        g_free(canonical);
    } else if (code==A_ASSIGN) {
        if (assets_import(&s->assets,stage(&s->doc,s->sid),path,&error)) { s->renderer.uploaded=NULL; ui_notice(s,"Background imported. Export its PNG/TIM package before building the game."); }
    } else if (code==A_ZIP) {
        if (assets_export(&s->assets,stage(&s->doc,s->sid),path,&error)) ui_notice(s,"PNG/TIM package exported.");
    }
    g_free(path); report(s,error); ui_refresh(s); g_free(job);
}
static void pngs_done(GObject *object,GAsyncResult *result,gpointer data) {
    Studio *s=data; GError *error=NULL; GListModel *files=gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(object),result,&error);
    if (!files) { if (error && !dismissed(error)) report(s,error); else g_clear_error(&error); return; }
    for (unsigned i=0;i<g_list_model_get_n_items(files);i++) {
        GFile *file=g_list_model_get_item(files,i); char *path=g_file_get_path(file);
        if (path) assets_add_file(&s->assets,path);
        g_free(path); g_object_unref(file);
    }
    g_object_unref(files); s->renderer.uploaded=NULL; assets_invalidate(&s->assets); ui_notice(s,"PNG backgrounds loaded."); ui_refresh(s);
}
static void new_free(gpointer data) { NewStage *n=data; g_free(n->path); g_free(n); }
static void new_cancel(GtkButton *button_,NewStage *n) { (void)button_; gtk_window_destroy(GTK_WINDOW(n->window)); }
static void new_png_done(GObject *object,GAsyncResult *result,gpointer data) {
    GtkWindow *window=data; NewStage *n=g_object_get_data(G_OBJECT(window),"new-stage"); GError *error=NULL;
    GFile *file=gtk_file_dialog_open_finish(GTK_FILE_DIALOG(object),result,&error);
    if (file) {
        g_free(n->path); n->path=g_file_get_path(file); g_object_unref(file);
        gtk_label_set_text(GTK_LABEL(n->file_label),n->path?n->path:"Choose a local PNG file.");
    } else if (error && !dismissed(error)) gtk_label_set_text(GTK_LABEL(n->error),error->message);
    g_clear_error(&error); g_object_unref(window);
}
static void new_png(GtkButton *button_,NewStage *n) {
    (void)button_; GtkFileDialog *dialog=file_dialog(n->s,"Choose background PNG","*.png");
    gtk_file_dialog_open(dialog,GTK_WINDOW(n->window),NULL,new_png_done,g_object_ref(n->window)); g_object_unref(dialog);
}
static void new_create(GtkButton *button_,NewStage *n) {
    (void)button_; Studio *s=n->s;
    char *name=g_strdup(gtk_editable_get_text(GTK_EDITABLE(n->name))), *a=g_ascii_strup(gtk_editable_get_text(GTK_EDITABLE(n->left)),-1), *b=g_ascii_strup(gtk_editable_get_text(GTK_EDITABLE(n->right)),-1);
    g_strstrip(name); g_strstrip(a); g_strstrip(b);
    const char *problem=NULL;
    if (!*name || !tim_name(a) || !tim_name(b) || !strcmp(a,b)) problem="Enter a name and two different uppercase TIM names (e.g. BG9_0.TIM).";
    if (!n->path) problem="Choose a PNG background.";
    unsigned count=json_array_get_length(json_node_get_array(s->doc.data));
    for (unsigned i=0;i<count;i++) for (int j=0;j<2;j++) {
        const char *old=string(stage(&s->doc,i),j?"tim_1":"tim_0"); if (!strcmp(old,a)||!strcmp(old,b)) problem="These TIM names are already used. Choose unused names.";
    }
    if (problem) { gtk_label_set_text(GTK_LABEL(n->error),problem); g_free(name); g_free(a); g_free(b); return; }
    JsonObject *current=stage(&s->doc,s->sid), *st=json_object_new();
    json_object_set_string_member(st,"name",name); json_object_set_string_member(st,"tim_0",a); json_object_set_string_member(st,"tim_1",b);
    const char *cam[]={"cam_x","cam_y","cam_z","cam_rx","cam_ry","cam_rz"};
    for (int i=0;i<6;i++) json_object_set_int_member(st,cam[i],number(current,cam[i]));
    json_object_set_array_member(st,"planes",json_array_new()); json_object_set_array_member(st,"zones",json_array_new()); json_object_set_array_member(st,"npcs",json_array_new());
    JsonArray *spawn=json_array_new(); json_array_add_element(spawn,clone_node(json_array_get_element(array(current,"spawns"),0))); json_object_set_array_member(st,"spawns",spawn);
    GError *error=NULL;
    if (!assets_import(&s->assets,st,n->path,&error)) { gtk_label_set_text(GTK_LABEL(n->error),error->message); g_error_free(error); json_object_unref(st); }
    else {
        char *before=serialize(&s->doc); json_array_add_object_element(json_node_get_array(s->doc.data),st); s->sid=count; s->kind=s->index=-1;
        s->renderer.uploaded=NULL; fit_view(&s->doc,s->sid,&s->view); changed(s,before); gtk_window_destroy(GTK_WINDOW(n->window));
    }
    g_free(name); g_free(a); g_free(b);
}
static void new_stage(Studio *s) {
    NewStage *n=g_new0(NewStage,1); n->s=s; n->window=gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(n->window),"New stage"); gtk_window_set_transient_for(GTK_WINDOW(n->window),GTK_WINDOW(s->window));
    gtk_window_set_modal(GTK_WINDOW(n->window),TRUE); gtk_window_set_default_size(GTK_WINDOW(n->window),460,400);
    g_object_set_data_full(G_OBJECT(n->window),"new-stage",n,new_free);
    GtkWidget *content=box(GTK_ORIENTATION_VERTICAL,8); margins(content,12); gtk_window_set_child(GTK_WINDOW(n->window),content);
    gtk_box_append(GTK_BOX(content),text("Add a background, then draw platforms and transition zones. The current camera and first spawn are copied."));
    const char *labels[]={"Stage name","First TIM","Second TIM"}; GtkWidget **entries[]={&n->name,&n->left,&n->right};
    for (int i=0;i<3;i++) { gtk_box_append(GTK_BOX(content),text(labels[i])); *entries[i]=gtk_entry_new(); gtk_box_append(GTK_BOX(content),*entries[i]); }
    char *a,*b; next_names(&s->doc,&a,&b); char *name=g_strdup_printf("%u_new_area",json_array_get_length(json_node_get_array(s->doc.data)));
    gtk_editable_set_text(GTK_EDITABLE(n->name),name); gtk_editable_set_text(GTK_EDITABLE(n->left),a); gtk_editable_set_text(GTK_EDITABLE(n->right),b); g_free(a); g_free(b); g_free(name);
    GtkWidget *png=gtk_button_new_with_label("Choose PNG background"); g_signal_connect(png,"clicked",G_CALLBACK(new_png),n); gtk_box_append(GTK_BOX(content),png);
    n->file_label=text("No PNG selected"); gtk_label_set_ellipsize(GTK_LABEL(n->file_label),PANGO_ELLIPSIZE_MIDDLE); gtk_box_append(GTK_BOX(content),n->file_label);
    n->error=text(""); gtk_widget_add_css_class(n->error,"error"); gtk_box_append(GTK_BOX(content),n->error);
    GtkWidget *actions=box(GTK_ORIENTATION_HORIZONTAL,8), *cancel=gtk_button_new_with_label("Cancel"), *create=gtk_button_new_with_label("Create stage");
    g_signal_connect(cancel,"clicked",G_CALLBACK(new_cancel),n); g_signal_connect(create,"clicked",G_CALLBACK(new_create),n); gtk_box_append(GTK_BOX(actions),cancel); gtk_box_append(GTK_BOX(actions),create); gtk_box_append(GTK_BOX(content),actions);
    gtk_window_present(GTK_WINDOW(n->window));
}
static void perform(Studio *s,int code) {
    if (code==A_CLOSE) { s->quitting=TRUE; gtk_window_destroy(GTK_WINDOW(s->window)); return; }
    if (code==A_RELOAD) {
        GError *error=NULL; char *path=g_strdup(s->doc.path);
        if (document_load(&s->doc,path,&error)) { s->renderer.uploaded=NULL; assets_invalidate(&s->assets); s->kind=s->index=-1; g_clear_pointer(&s->notice,g_free); fit_view(&s->doc,s->sid,&s->view); }
        g_free(path); report(s,error); ui_refresh(s); return;
    }
    const char *title=code==A_OPEN?"Open stages.json":code==A_EXPORT?"Export JSON copy":code==A_ZIP?"Export PNG/TIM package":"Assign PNG background";
    GtkFileDialog *dialog=file_dialog(s,title,code==A_OPEN || code==A_EXPORT?"*.json":code==A_ZIP?"*.zip":"*.png");
    Pending *job=g_new0(Pending,1); *job=(Pending){s,code};
    if (code==A_EXPORT || code==A_ZIP) {
        gtk_file_dialog_set_initial_name(dialog,code==A_EXPORT?"stages-copy.json":"stage-assets.zip"); gtk_file_dialog_save(dialog,GTK_WINDOW(s->window),NULL,file_done,job);
    } else gtk_file_dialog_open(dialog,GTK_WINDOW(s->window),NULL,file_done,job);
    g_object_unref(dialog);
}
static void confirm_done(GObject *object,GAsyncResult *result,gpointer data) {
    Pending *job=data; GError *error=NULL; int choice=gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(object),result,&error);
    if (!error && (choice==1 || (choice==2 && save(job->s)))) perform(job->s,job->action);
    g_clear_error(&error); g_free(job);
}
static void confirm_action(Studio *s,int code) {
    commit_focus(s);
    cancel_drag(s);
    if (!document_dirty(&s->doc)) { perform(s,code); return; }
    GtkAlertDialog *dialog=gtk_alert_dialog_new("Save changes before continuing?");
    gtk_alert_dialog_set_detail(dialog,"Changes to stages.json are saved only when you choose Save JSON. Imported assets are already stored in the editor's assets folder.");
    const char *buttons[]={"Cancel","Discard changes","Save",NULL}; gtk_alert_dialog_set_buttons(dialog,buttons); gtk_alert_dialog_set_cancel_button(dialog,0); gtk_alert_dialog_set_default_button(dialog,2);
    Pending *job=g_new0(Pending,1); *job=(Pending){s,code}; gtk_alert_dialog_choose(dialog,GTK_WINDOW(s->window),NULL,confirm_done,job); g_object_unref(dialog);
}
static gboolean close_requested(GtkWindow *window,Studio *s) { (void)window; if (!s->quitting) confirm_action(s,A_CLOSE); return TRUE; }
static void zoom(Studio *s,double value) {
    s->zoom=CLAMP(value,.5,6); gtk_widget_set_size_request(s->area,(int)(320*s->zoom),(int)(240*s->zoom));
    char *label=g_strdup_printf("%.0f%%",s->zoom*100); gtk_label_set_text(GTK_LABEL(s->zoom_label),label); g_free(label); render_queue(s);
}
static void action(Studio *s,int code) {
    if (code==A_RELOAD && !s->doc.path) { ui_notice(s,"Open stages.json first."); return; }
    if (code==A_OPEN || code==A_RELOAD || code==A_CLOSE) { confirm_action(s,code); return; }
    if (code==A_SAVE) { save(s); return; }
    if (code==A_UNDO || code==A_REDO) { cancel_drag(s); document_undo(&s->doc,code==A_REDO); s->kind=s->index=-1; g_clear_pointer(&s->notice,g_free); ui_refresh(s); return; }
    if (code==A_FIT) {
        fit_view(&s->doc,s->sid,&s->view); double w=gtk_widget_get_width(s->viewport)-8,h=gtk_widget_get_height(s->viewport)-8; zoom(s,MAX(.5,MIN(w/320,h/240))); return;
    }
    if (code==A_ZOOM_IN || code==A_ZOOM_OUT) { zoom(s,s->zoom+(code==A_ZOOM_IN?.25:-.25)); return; }
    if (code==A_PNGS) { GtkFileDialog *dialog=file_dialog(s,"Load PNG backgrounds","*.png"); gtk_file_dialog_open_multiple(dialog,GTK_WINDOW(s->window),NULL,pngs_done,s); g_object_unref(dialog); return; }
    if (!s->doc.data) { ui_notice(s,"Open stages.json first."); return; }
    cancel_drag(s);
    if (code==A_NEW) { new_stage(s); return; }
    if (code==A_EXPORT || code==A_ASSIGN || code==A_ZIP) { perform(s,code); return; }
    JsonObject *o=item(&s->doc,s->sid,s->kind,s->index); if (!o) return;
    char *before=serialize(&s->doc);
    if (code==A_DELETE) {
        GError *error=NULL;
        if (!delete_item(&s->doc,s->sid,s->kind,s->index,&error)) { g_free(before); report(s,error); return; }
        s->kind=s->index=-1;
    } else if (code==A_DUPLICATE) {
        JsonArray *a=array(stage(&s->doc,s->sid),kinds[s->kind]);
        if (json_array_get_length(a)>=(unsigned)limits[s->kind]) { g_free(before); ui_notice(s,"PS1 object limit reached."); return; }
        JsonNode *copy=clone_node(json_array_get_element(a,s->index)); JsonObject *obj=json_node_get_object(copy);
        json_object_set_int_member(obj,"x",number(obj,"x")+s->snap*2); json_object_set_int_member(obj,"z",number(obj,"z")+s->snap*2);
        json_array_add_element(a,copy); s->index=json_array_get_length(a)-1;
    }
    changed(s,before);
}
static GtkWidget *find_field(GtkWidget *widget,const char *key) {
    Field *field=g_object_get_data(G_OBJECT(widget),"field");
    if (field && !strcmp(field->key,key)) return widget;
    for (GtkWidget *child=gtk_widget_get_first_child(widget);child;child=gtk_widget_get_next_sibling(child)) {
        GtkWidget *found=find_field(child,key); if (found) return found;
    }
    return NULL;
}
static Screen widget_point(Studio *s,Point p) {
    Screen result; g_assert_true(project(stage(&s->doc,s->sid),&s->view,p,&result));
    double w=gtk_widget_get_width(s->area),h=gtk_widget_get_height(s->area),scale=MIN(w/320,h/240);
    return (Screen){result.x*scale+(w-320*scale)/2,result.y*scale+(h-240*scale)/2};
}
static void test_drag(Studio *s,Point from,Point to) {
    Screen a=widget_point(s,from),b=widget_point(s,to); drag_begin(NULL,a.x,a.y,s); drag_end(NULL,b.x-a.x,b.y-a.y,s);
}
static void test_interactions(Studio *s) {
    /* Exercise GTK handlers in memory; never save or import into the real project. */
    char *original=serialize(&s->doc); int original_sid=s->sid; gboolean original_view=s->view.top;
    s->sid=0; s->view.top=TRUE; fit_view(&s->doc,0,&s->view); ui_refresh(s);
    unsigned plane_count=json_array_get_length(array(stage(&s->doc,0),"planes"));
    choose_tool(s,1); test_drag(s,(Point){-100,0,-50},(Point){-150,0,-100});
    g_assert_cmpuint(json_array_get_length(array(stage(&s->doc,0),"planes")),==,plane_count+1);
    g_assert_cmpfloat(number(item(&s->doc,0,0,plane_count),"w"),==,50);
    action(s,A_DUPLICATE); g_assert_cmpuint(json_array_get_length(array(stage(&s->doc,0),"planes")),==,plane_count+2);
    action(s,A_DELETE); g_assert_cmpuint(json_array_get_length(array(stage(&s->doc,0),"planes")),==,plane_count+1);
    s->kind=0; s->index=plane_count;
    test_drag(s,(Point){-125,0,-75},(Point){-105,0,-65});
    g_assert_cmpfloat(number(item(&s->doc,0,0,plane_count),"x"),==,-130);
    g_assert_cmpfloat(number(item(&s->doc,0,0,plane_count),"z"),==,-40);
    test_drag(s,(Point){-80,0,-90},(Point){-30,0,-150});
    g_assert_cmpfloat(number(item(&s->doc,0,0,plane_count),"w"),==,100);
    g_assert_cmpfloat(number(item(&s->doc,0,0,plane_count),"d"),==,-110);
    action(s,A_UNDO); g_assert_cmpfloat(number(item(&s->doc,0,0,plane_count),"w"),==,50);
    action(s,A_REDO); g_assert_cmpfloat(number(item(&s->doc,0,0,plane_count),"w"),==,100);
    unsigned spawn_count=json_array_get_length(array(stage(&s->doc,0),"spawns"));
    choose_tool(s,3); Screen p=widget_point(s,(Point){-140,0,-120}); drag_begin(NULL,p.x,p.y,s);
    g_assert_cmpuint(json_array_get_length(array(stage(&s->doc,0),"spawns")),==,spawn_count+1);
    action(s,A_DELETE); g_assert_cmpuint(json_array_get_length(array(stage(&s->doc,0),"spawns")),==,spawn_count);
    unsigned zone_count=json_array_get_length(array(stage(&s->doc,0),"zones"));
    choose_tool(s,2); test_drag(s,(Point){-180,0,-160},(Point){-130,0,-200});
    g_assert_cmpuint(json_array_get_length(array(stage(&s->doc,0),"zones")),==,zone_count+1);
    GtkWidget *drop=find_field(s->inspector,"stage_id"); g_assert_nonnull(drop); gtk_drop_down_set_selected(GTK_DROP_DOWN(drop),8);
    g_assert_cmpfloat(number(item(&s->doc,0,1,zone_count),"stage_id"),==,8);
    g_assert_cmpfloat(number(item(&s->doc,0,1,zone_count),"spawn_id"),==,0);
    GtkWidget *name=find_field(s->inspector,"name"); g_assert_nonnull(name);
    gtk_editable_set_text(GTK_EDITABLE(name),"0_native_test"); g_signal_emit_by_name(name,"activate");
    g_assert_cmpstr(string(stage(&s->doc,0),"name"),==,"0_native_test");
    g_assert_true(gtk_widget_grab_focus(name)); gtk_editable_set_text(GTK_EDITABLE(name),"0_pending_field"); commit_focus(s);
    g_assert_cmpstr(string(stage(&s->doc,0),"name"),==,"0_pending_field");
    char *checks=validate(s->doc.data); g_assert_cmpstr(checks,==,""); g_free(checks);
    choose_tool(s,1); Screen a=widget_point(s,(Point){-10,0,-10}); drag_begin(NULL,a.x,a.y,s); drag_update(NULL,50,50,s);
    cancel_drag(s); g_assert_cmpuint(json_array_get_length(array(stage(&s->doc,0),"planes")),==,plane_count+1);
    restore(&s->doc,original,NULL); g_free(original); g_ptr_array_set_size(s->doc.undo,0); g_ptr_array_set_size(s->doc.redo,0);
    s->sid=original_sid; s->view.top=original_view; s->kind=s->index=-1; choose_tool(s,0); fit_view(&s->doc,s->sid,&s->view); ui_refresh(s);
    g_print("GTK interactions: draw, move, resize, duplicate, delete, spawn, destination, properties, undo/redo, cancel passed\n");
}
static gboolean smoke(gpointer data) {
    Studio *s=data;
    const GError *error=gtk_gl_area_get_error(GTK_GL_AREA(s->area));
    if (error) { g_printerr("OpenGL test failed: %s\n",error->message); s->smoke_failed=TRUE; s->quitting=TRUE; g_application_quit(G_APPLICATION(s->application)); return G_SOURCE_REMOVE; }
    unsigned count=s->doc.data?json_array_get_length(json_node_get_array(s->doc.data)):0;
    if (s->smoke_step<count*2) {
        s->sid=s->smoke_step/2; s->view.top=s->smoke_step%2; s->smoke_step++; fit_view(&s->doc,s->sid,&s->view); ui_refresh(s); return G_SOURCE_CONTINUE;
    }
    if (!s->renderer.frames) { g_printerr("No OpenGL frame rendered.\n"); s->smoke_failed=TRUE; s->quitting=TRUE; g_application_quit(G_APPLICATION(s->application)); return G_SOURCE_REMOVE; }
    s->view.top=FALSE;
    if (s->smoke_step==count*2) {
        test_interactions(s); s->smoke_step++; ui_refresh(s); s->capture=gtk_widget_paintable_new(s->window); return G_SOURCE_CONTINUE;
    }
    GtkSnapshot *snapshot=gtk_snapshot_new();
    int w=gtk_widget_get_width(s->window),h=gtk_widget_get_height(s->window);
    gdk_paintable_snapshot(s->capture,GDK_SNAPSHOT(snapshot),w,h);
    GskRenderNode *node=gtk_snapshot_free_to_node(snapshot); GskRenderer *renderer=gtk_native_get_renderer(GTK_NATIVE(s->window));
    if (node) {
        graphene_rect_t bounds=GRAPHENE_RECT_INIT(0,0,w,h); GdkTexture *texture_=gsk_renderer_render_texture(renderer,node,&bounds);
        gboolean ok=gdk_texture_save_to_png(texture_,s->smoke_path); s->smoke_failed=!ok;
        g_print("GTK/OpenGL smoke test: %u stages, %u frames, screenshot %s\n",count,s->renderer.frames,ok?"saved":"FAILED");
        g_object_unref(texture_); gsk_render_node_unref(node);
    } else if (s->smoke_step++<count*2+20) return G_SOURCE_CONTINUE;
    else { g_printerr("GTK screenshot unavailable.\n"); s->smoke_failed=TRUE; }
    g_clear_object(&s->capture); s->quitting=TRUE; gtk_window_destroy(GTK_WINDOW(s->window)); return G_SOURCE_REMOVE;
}
void ui_activate(GtkApplication *application,gpointer data) {
    Studio *s=data; s->application=application;
    if (s->window) { gtk_window_present(GTK_WINDOW(s->window)); return; }
    s->window=gtk_application_window_new(application); gtk_window_set_title(GTK_WINDOW(s->window),"PSX Stage Editor"); gtk_window_set_default_size(GTK_WINDOW(s->window),1360,880);
    g_signal_connect(s->window,"close-request",G_CALLBACK(close_requested),s);
    GtkCssProvider *css=gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        "window { background:#c0c0c0; color:#000; font-family:Tahoma,Arial,sans-serif; font-size:12px; }"
        "button { border-radius:0; background:#c0c0c0; color:#000; border:2px solid; border-color:#fff #444 #444 #fff; box-shadow:none; padding:5px 7px; min-height:16px; }"
        "button:active,button.active { border-color:#444 #fff #fff #444; background:#a9a9a9; } button:disabled { color:#777; }"
        "entry,dropdown { border-radius:0; background:#fff; color:#000; border:2px solid; border-color:#777 #fff #fff #777; min-height:22px; }"
        "entry { padding:2px 4px; } entry selection { background:#000080; color:white; }"
        "scrolledwindow { border:2px solid; border-color:#777 #fff #fff #777; }"
        ".titlebar { background:#000080; color:white; padding:5px 8px; font-weight:bold; }"
        ".small { font-size:10px; } .error { color:#900; } expander title { font-weight:bold; }"
        "checkbutton { color:black; } separator { background:#808080; min-height:1px; min-width:1px; } tooltip { color:black; background:#ffffdf; }"
        ".status { border:1px solid; border-color:#777 #fff #fff #777; padding:3px; }");
    gtk_style_context_add_provider_for_display(gtk_widget_get_display(s->window),GTK_STYLE_PROVIDER(css),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); g_object_unref(css);
    GtkWidget *main=box(GTK_ORIENTATION_VERTICAL,6); margins(main,6); gtk_window_set_child(GTK_WINDOW(s->window),main);
    GtkWidget *brand=text("PSX STAGE EDITOR"); gtk_label_set_wrap(GTK_LABEL(brand),FALSE); gtk_widget_add_css_class(brand,"titlebar"); gtk_box_append(GTK_BOX(main),brand);
    GtkWidget *toolbar=box(GTK_ORIENTATION_HORIZONTAL,6); gtk_box_append(GTK_BOX(main),toolbar);
    add_button(toolbar,s,"Open JSON",A_OPEN); add_button(toolbar,s,"Reload",A_RELOAD); add_button(toolbar,s,"Save JSON",A_SAVE); add_button(toolbar,s,"Export JSON copy",A_EXPORT);
    s->undo_button=button(s,"Undo",A_UNDO); s->redo_button=button(s,"Redo",A_REDO); gtk_box_append(GTK_BOX(toolbar),s->undo_button); gtk_box_append(GTK_BOX(toolbar),s->redo_button);
    GtkWidget *paned=gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); gtk_widget_set_vexpand(paned,TRUE); gtk_box_append(GTK_BOX(main),paned);
    GtkWidget *left=box(GTK_ORIENTATION_VERTICAL,6); gtk_widget_set_size_request(left,225,-1); gtk_paned_set_start_child(GTK_PANED(paned),left); gtk_paned_set_resize_start_child(GTK_PANED(paned),FALSE);
    gtk_box_append(GTK_BOX(left),text("STAGES")); s->search=gtk_search_entry_new(); gtk_widget_set_tooltip_text(s->search,"Search stages"); gtk_box_append(GTK_BOX(left),s->search);
    g_signal_connect(s->search,"search-changed",G_CALLBACK(search_changed),s);
    GtkWidget *stage_scroll=gtk_scrolled_window_new(); gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(stage_scroll),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(stage_scroll,TRUE); s->stages=box(GTK_ORIENTATION_VERTICAL,5); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stage_scroll),s->stages); gtk_box_append(GTK_BOX(left),stage_scroll);
    add_button(left,s,"New stage",A_NEW); add_button(left,s,"Load PNG backgrounds",A_PNGS);
    GtkWidget *right_pane=gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); gtk_paned_set_end_child(GTK_PANED(paned),right_pane); gtk_paned_set_position(GTK_PANED(paned),235);
    GtkWidget *center=box(GTK_ORIENTATION_VERTICAL,6); margins(center,6); gtk_paned_set_start_child(GTK_PANED(right_pane),center); gtk_paned_set_resize_end_child(GTK_PANED(right_pane),FALSE);
    s->title=text(""); gtk_label_set_wrap(GTK_LABEL(s->title),FALSE); gtk_label_set_ellipsize(GTK_LABEL(s->title),PANGO_ELLIPSIZE_END); gtk_widget_add_css_class(s->title,"titlebar"); gtk_box_append(GTK_BOX(center),s->title);
    GtkWidget *view=box(GTK_ORIENTATION_HORIZONTAL,5); gtk_box_append(GTK_BOX(center),view);
    for (int i=0;i<2;i++) {
        s->view_buttons[i]=gtk_button_new_with_label(i?"Top view":"Background"); g_object_set_data(G_OBJECT(s->view_buttons[i]),"top",GINT_TO_POINTER(i));
        g_signal_connect(s->view_buttons[i],"clicked",G_CALLBACK(view_clicked),s); gtk_box_append(GTK_BOX(view),s->view_buttons[i]); if (!i) gtk_widget_add_css_class(s->view_buttons[i],"active");
    }
    GtkWidget *tools=box(GTK_ORIENTATION_HORIZONTAL,4); gtk_box_append(GTK_BOX(center),tools);
    const char *names[]={"Select","Platform","Zone","Spawn"}, *tips[]={"Select/move/resize (V)","Draw two platform corners (P)","Draw two zone corners (Z)","Add a spawn point (I)"};
    for (int i=0;i<4;i++) {
        s->tool_buttons[i]=gtk_button_new_with_label(names[i]); gtk_widget_set_tooltip_text(s->tool_buttons[i],tips[i]); g_object_set_data(G_OBJECT(s->tool_buttons[i]),"tool",GINT_TO_POINTER(i));
        g_signal_connect(s->tool_buttons[i],"clicked",G_CALLBACK(tool_clicked),s); gtk_box_append(GTK_BOX(tools),s->tool_buttons[i]);
    }
    gtk_widget_add_css_class(s->tool_buttons[0],"active"); gtk_box_append(GTK_BOX(tools),text("Snap"));
    const char *snaps[]={"1","5","10","25",NULL}; s->snap_widget=gtk_drop_down_new_from_strings(snaps); gtk_drop_down_set_selected(GTK_DROP_DOWN(s->snap_widget),1);
    g_signal_connect(s->snap_widget,"notify::selected",G_CALLBACK(snap_changed),s); gtk_box_append(GTK_BOX(tools),s->snap_widget);
    s->viewport=gtk_scrolled_window_new(); gtk_widget_set_vexpand(s->viewport,TRUE); gtk_widget_set_hexpand(s->viewport,TRUE); gtk_box_append(GTK_BOX(center),s->viewport);
    s->area=gtk_gl_area_new(); gtk_widget_set_focusable(s->area,TRUE); gtk_widget_set_hexpand(s->area,TRUE); gtk_widget_set_vexpand(s->area,TRUE); gtk_widget_set_size_request(s->area,640,480);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s->viewport),s->area); render_connect(s);
    GtkGesture *drag=gtk_gesture_drag_new(); gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag),1);
    g_signal_connect(drag,"drag-begin",G_CALLBACK(drag_begin),s); g_signal_connect(drag,"drag-update",G_CALLBACK(drag_update),s); g_signal_connect(drag,"drag-end",G_CALLBACK(drag_end),s);
    g_signal_connect(drag,"cancel",G_CALLBACK(gesture_cancel),s); gtk_widget_add_controller(s->area,GTK_EVENT_CONTROLLER(drag));
    GtkEventController *motion_=gtk_event_controller_motion_new(); g_signal_connect(motion_,"motion",G_CALLBACK(motion),s); gtk_widget_add_controller(s->area,motion_);
    GtkWidget *footer=box(GTK_ORIENTATION_HORIZONTAL,5); s->coordinates=text("X — / Z —"); gtk_widget_set_hexpand(s->coordinates,TRUE); gtk_box_append(GTK_BOX(footer),s->coordinates);
    add_button(footer,s,"−",A_ZOOM_OUT); s->zoom_label=text("200%"); gtk_box_append(GTK_BOX(footer),s->zoom_label); add_button(footer,s,"+",A_ZOOM_IN); add_button(footer,s,"Fit",A_FIT); gtk_box_append(GTK_BOX(center),footer);
    GtkWidget *layers=box(GTK_ORIENTATION_HORIZONTAL,5); const char *layer_names[]={"Platforms","Zones","Spawns"};
    for (int i=0;i<3;i++) {
        GtkWidget *check=gtk_check_button_new_with_label(layer_names[i]); gtk_check_button_set_active(GTK_CHECK_BUTTON(check),TRUE); g_object_set_data(G_OBJECT(check),"layer",GINT_TO_POINTER(i));
        g_signal_connect(check,"toggled",G_CALLBACK(layer_changed),s); gtk_box_append(GTK_BOX(layers),check);
    }
    gtk_box_append(GTK_BOX(center),layers);
    GtkWidget *message_scroll=gtk_scrolled_window_new(); gtk_widget_set_size_request(message_scroll,-1,75); gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(message_scroll),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
    s->messages=text(""); margins(s->messages,5); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(message_scroll),s->messages); gtk_box_append(GTK_BOX(center),message_scroll);
    GtkWidget *inspect_scroll=gtk_scrolled_window_new(); gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(inspect_scroll),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC); gtk_widget_set_size_request(inspect_scroll,295,-1);
    s->inspector=box(GTK_ORIENTATION_VERTICAL,10); margins(s->inspector,8); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(inspect_scroll),s->inspector);
    gtk_paned_set_end_child(GTK_PANED(right_pane),inspect_scroll); gtk_paned_set_position(GTK_PANED(right_pane),780);
    s->status=text(""); gtk_label_set_wrap(GTK_LABEL(s->status),FALSE); gtk_label_set_ellipsize(GTK_LABEL(s->status),PANGO_ELLIPSIZE_MIDDLE); gtk_widget_add_css_class(s->status,"status"); gtk_box_append(GTK_BOX(main),s->status);
    GtkEventController *keys=gtk_event_controller_key_new(); gtk_event_controller_set_propagation_phase(keys,GTK_PHASE_CAPTURE); g_signal_connect(keys,"key-pressed",G_CALLBACK(key_pressed),s); gtk_widget_add_controller(s->window,keys);
    ui_refresh(s); gtk_window_present(GTK_WINDOW(s->window));
    if (s->smoke_path) g_timeout_add(250,smoke,s);
}
void ui_clear(Studio *s) { cancel_drag(s); g_clear_object(&s->capture); g_free(s->notice); g_free(s->smoke_path); assets_clear(&s->assets); document_clear(&s->doc); }
