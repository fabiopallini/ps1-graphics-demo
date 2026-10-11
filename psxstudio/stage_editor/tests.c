#include "stage_editor.h"
#include <glib/gstdio.h>
#include <string.h>

static char *project_root;
static Document fixture(void) {
    Document d={0}; GError *error=NULL;
    char *path=g_build_filename(project_root,"stages","stages.json",NULL);
    g_assert_true(document_load(&d,path,&error)); g_assert_no_error(error); g_free(path); return d;
}
static void remove_tree(const char *dir) {
    GDir *d=g_dir_open(dir,0,NULL); const char *name;
    while (d && (name=g_dir_read_name(d))) {
        char *path=g_build_filename(dir,name,NULL);
        if (g_file_test(path,G_FILE_TEST_IS_DIR)) remove_tree(path); else g_unlink(path);
        g_free(path);
    }
    if (d) g_dir_close(d);
    g_rmdir(dir);
}
static void projection_test(void) {
    Document d=fixture(); View v={0};
    unsigned count=json_array_get_length(json_node_get_array(d.data));
    for (unsigned i=0;i<count;i++) {
        JsonObject *s=stage(&d,i); JsonArray *a=array(s,"spawns");
        for (unsigned j=0;j<json_array_get_length(a);j++) {
            Point p=coordinates(item(&d,i,2,j)),q; Screen screen;
            g_assert_true(project(s,&v,p,&screen)); g_assert_true(unproject(s,&v,screen,p.y,&q));
            g_assert_cmpfloat_with_epsilon(p.x,q.x,1e-7); g_assert_cmpfloat_with_epsilon(p.z,q.z,1e-7);
        }
        fit_view(&d,i,&v); v.top=TRUE; Point p={123,37,-829},q; Screen screen;
        g_assert_true(project(s,&v,p,&screen)); g_assert_true(unproject(s,&v,screen,p.y,&q));
        g_assert_cmpfloat_with_epsilon(p.x,q.x,1e-7); g_assert_cmpfloat_with_epsilon(p.z,q.z,1e-7); v.top=FALSE;
    }
    Screen reference; g_assert_true(project(stage(&d,8),&v,coordinates(item(&d,8,2,0)),&reference));
    g_assert_cmpfloat_with_epsilon(reference.x,160,1e-6); g_assert_cmpfloat_with_epsilon(reference.y,193.18219162,1e-6);
    document_clear(&d);
}
static void clipping_test(void) {
    JsonObject *s=json_object_new(); const char *keys[]={"cam_x","cam_y","cam_z","cam_rx","cam_ry","cam_rz"};
    for (int i=0;i<6;i++) json_object_set_int_member(s,keys[i],0);
    JsonObject *o=rectangle((Point){-50,0,100},(Point){50,0,500},40,5); View v={0}; Screen p[8];
    int n=polygon(s,&v,o,p); g_assert_cmpint(n,==,4);
    for (int i=0;i<n;i++) { g_assert_true(isfinite(p[i].x)); g_assert_true(isfinite(p[i].y)); g_assert_cmpfloat(p[i].y,<=,200); }
    json_object_set_int_member(o,"z",100); g_assert_cmpint(polygon(s,&v,o,p),==,0);
    json_object_unref(o); json_object_unref(s);
}
static void rectangle_test(void) {
    g_assert_cmpfloat(snap_value(-2.5,5),==,0); g_assert_cmpfloat(snap_value(-7.5,5),==,-5);
    JsonObject *r=rectangle((Point){19,0,-31},(Point){-4,0,4},10,5);
    g_assert_cmpfloat(number(r,"x"),==,-5); g_assert_cmpfloat(number(r,"z"),==,5);
    g_assert_cmpfloat(number(r,"w"),==,25); g_assert_cmpfloat(number(r,"d"),==,-35);
    g_assert_cmpfloat(number(r,"y"),==,10); json_object_unref(r);
}
static void history_test(void) {
    Document d=fixture(); JsonObject *s=stage(&d,0);
    json_object_set_string_member(s,"unknown_metadata","keep me");
    g_free(d.saved); d.saved=serialize(&d);
    char *before=serialize(&d); json_object_set_int_member(item(&d,0,0,0),"x",123); document_changed(&d,before);
    g_assert_true(document_dirty(&d)); g_assert_cmpuint(d.undo->len,==,1);
    document_undo(&d,FALSE); g_assert_false(document_dirty(&d));
    g_assert_cmpstr(string(stage(&d,0),"unknown_metadata"),==,"keep me");
    document_undo(&d,TRUE); g_assert_cmpfloat(number(item(&d,0,0,0),"x"),==,123);
    JsonArray *a=array(stage(&d,0),"planes"); JsonNode *copy=clone_node(json_array_get_element(a,0));
    json_object_set_int_member(json_node_get_object(copy),"x",999);
    g_assert_cmpfloat(number(item(&d,0,0,0),"x"),==,123); json_node_unref(copy);
    for (int i=0;i<70;i++) { before=serialize(&d); json_object_set_int_member(item(&d,0,0,0),"x",i); document_changed(&d,before); }
    g_assert_cmpuint(d.undo->len,==,60); document_clear(&d);
}
static void validation_test(void) {
    Document d=fixture(); char *e=validate(d.data); g_assert_cmpstr(e,==,""); g_free(e);
    JsonObject *zone=item(&d,0,1,0); json_object_set_int_member(zone,"spawn_id",999);
    e=validate(d.data); g_assert_nonnull(strstr(e,"Invalid zone destination")); g_free(e);
    json_object_set_int_member(zone,"spawn_id",0); json_object_set_double_member(zone,"stage_id",1e100);
    e=validate(d.data); g_assert_nonnull(strstr(e,"Invalid zone destination")); g_free(e);
    g_assert_true(tim_name("BG9_0.TIM")); g_assert_false(tim_name("TOOLONG.TIM")); g_assert_false(tim_name("bg0.tim"));
    document_clear(&d);
}
static void spawn_delete_test(void) {
    Document d=fixture(); GError *error=NULL;
    g_assert_false(delete_item(&d,1,2,0,&error)); g_assert_nonnull(error); g_clear_error(&error);
    g_assert_false(delete_item(&d,0,2,1,&error)); g_assert_nonnull(error); g_clear_error(&error);
    JsonArray *a=array(stage(&d,0),"spawns");
    json_array_add_element(a,clone_node(json_array_get_element(a,0))); json_array_add_element(a,clone_node(json_array_get_element(a,0)));
    JsonObject *zone=item(&d,1,1,0); json_object_set_int_member(zone,"spawn_id",4);
    g_assert_true(delete_item(&d,0,2,3,&error)); g_assert_no_error(error);
    g_assert_cmpfloat(number(zone,"spawn_id"),==,3); document_clear(&d);
}
static void file_test(void) {
    Document source=fixture(); char *dir=g_dir_make_tmp("psxstudio-files-XXXXXX",NULL), *path=g_build_filename(dir,"stages.json",NULL), *original=serialize(&source);
    g_assert_true(g_file_set_contents(path,original,-1,NULL)); Document d={0}; GError *error=NULL;
    g_assert_true(document_load(&d,path,&error));
    json_object_set_string_member(stage(&d,0),"name","0_test"); g_assert_true(document_save(&d,&error)); g_assert_no_error(error); g_assert_false(document_dirty(&d));
    char *backup=g_strconcat(path,"~",NULL), *text=NULL; g_assert_true(g_file_get_contents(backup,&text,NULL,&error)); g_assert_cmpstr(text,==,original); g_free(text);
    g_assert_true(g_file_set_contents(path,original,-1,&error));
    GFile *file=g_file_new_for_path(path); g_assert_true(g_file_set_attribute_uint64(file,G_FILE_ATTRIBUTE_TIME_MODIFIED,4102444800,G_FILE_QUERY_INFO_NONE,NULL,&error)); g_object_unref(file);
    json_object_set_string_member(stage(&d,0),"name","0_would_overwrite"); g_assert_false(document_save(&d,&error)); g_assert_error(error,G_IO_ERROR,G_IO_ERROR_WRONG_ETAG); g_clear_error(&error);
    g_assert_true(g_file_get_contents(path,&text,NULL,&error)); g_assert_cmpstr(text,==,original); g_free(text);
    g_free(original); g_free(backup); g_free(path); document_clear(&d); document_clear(&source); remove_tree(dir); g_free(dir);
}
static unsigned little16(const guint8 *p) { return p[0]|p[1]<<8; }
static unsigned little32(const guint8 *p) { return p[0]|p[1]<<8|p[2]<<16|(unsigned)p[3]<<24; }
static void tim_test(void) {
    GdkPixbuf *image=gdk_pixbuf_new(GDK_COLORSPACE_RGB,TRUE,8,3,1); guint8 *p=gdk_pixbuf_get_pixels(image);
    const guint8 values[]={0,0,0,255,255,0,0,255,0,255,0,0}; memcpy(p,values,sizeof values);
    GBytes *b=encode_tim(image,320,256); gsize size; const guint8 *data=g_bytes_get_data(b,&size);
    g_assert_cmpuint(size,==,26); g_assert_cmpuint(little32(data),==,0x10); g_assert_cmpuint(little32(data+4),==,2);
    g_assert_cmpuint(little16(data+12),==,320); g_assert_cmpuint(little16(data+14),==,256);
    g_assert_cmpuint(little16(data+20),==,0x8000); g_assert_cmpuint(little16(data+22),==,31); g_assert_cmpuint(little16(data+24),==,0);
    g_bytes_unref(b); g_object_unref(image);
}
static uint32_t test_crc(const guint8 *p,gsize size) {
    uint32_t c=0xffffffff; for (gsize i=0;i<size;i++) { c^=p[i]; for (int k=0;k<8;k++) c=(c>>1)^((c&1)?0xedb88320:0); } return c^0xffffffff;
}
static void assets_test(void) {
    GError *error=NULL;
    char *dir=g_dir_make_tmp("psxstudio-assets-XXXXXX",NULL), *png=g_build_filename(dir,"input.png",NULL), *zip=g_build_filename(dir,"package.zip",NULL);
    GdkPixbuf *image=gdk_pixbuf_new(GDK_COLORSPACE_RGB,TRUE,8,320,240); gdk_pixbuf_fill(image,0x204060ff);
    guint8 *pixels=gdk_pixbuf_get_pixels(image); int stride=gdk_pixbuf_get_rowstride(image);
    for (int y=0;y<240;y++) { guint8 *p=pixels+y*stride+255*4; p[0]=248; p[1]=y; p[2]=0; }
    gboolean saved=gdk_pixbuf_save(image,png,"png",&error,NULL); g_assert_no_error(error); g_assert_true(saved); g_object_unref(image);
    Assets a={0}; assets_init(&a,dir); JsonObject *s=json_object_new();
    json_object_set_string_member(s,"name","test"); json_object_set_string_member(s,"tim_0","TEST0.TIM"); json_object_set_string_member(s,"tim_1","TEST1.TIM");
    g_assert_true(assets_import(&a,s,png,&error)); g_assert_no_error(error);
    Background *bg=assets_background(&a,s); g_assert_nonnull(bg->image); g_assert_nonnull(bg->directory);
    char *path=g_build_filename(bg->directory,"test1.tim",NULL), *data=NULL; gsize size;
    g_assert_true(g_file_get_contents(path,&data,&size,&error)); g_free(path);
    g_assert_cmpuint(size,==,20+64*256*2); g_assert_cmpuint(little16((guint8 *)data+12),==,576);
    g_assert_cmpuint(little16((guint8 *)data+20),==,31);
    g_assert_cmpuint(little16((guint8 *)data+20+240*64*2),==,little16((guint8 *)data+20+239*64*2));
    g_assert_cmpuint(little16((guint8 *)data+20+255*64*2),==,little16((guint8 *)data+20+224*64*2));
    g_free(data); g_assert_true(assets_export(&a,s,zip,&error)); g_assert_no_error(error);
    g_assert_true(g_file_get_contents(zip,&data,&size,&error));
    unsigned entries=0; gsize pos=0; guint8 *p=(guint8 *)data;
    while (pos+30<size && little32(p+pos)==0x04034b50) {
        gsize length=little32(p+pos+18), names=little16(p+pos+26), extra=little16(p+pos+28), start=pos+30+names+extra;
        g_assert_cmpuint(start+length,<=,size); g_assert_cmpuint(test_crc(p+start,length),==,little32(p+pos+14));
        pos=start+length; entries++;
    }
    g_assert_cmpuint(entries,==,6); g_assert_cmpuint(little32(p+pos),==,0x02014b50); g_assert_cmpuint(little32(p+size-22),==,0x06054b50);
    g_free(data); json_object_unref(s); assets_clear(&a); g_free(png); g_free(zip); remove_tree(dir); g_free(dir);
}
static void project_backgrounds_test(void) {
    Document d=fixture(); Assets a={0}; assets_init(&a,project_root);
    for (unsigned i=0;i<json_array_get_length(json_node_get_array(d.data));i++) {
        Background *b=assets_background(&a,stage(&d,i)); g_assert_nonnull(b); g_assert_nonnull(b->image);
        g_assert_cmpint(gdk_pixbuf_get_width(b->image),==,320); g_assert_cmpint(gdk_pixbuf_get_height(b->image),==,240);
        const guint8 *p=gdk_pixbuf_read_pixels(b->image); int stride=gdk_pixbuf_get_rowstride(b->image),channels=gdk_pixbuf_get_n_channels(b->image); guint64 sum=0;
        for (int y=0;y<240;y++) for (int x=0;x<320;x++) for (int c=0;c<3;c++) sum+=p[y*stride+x*channels+c];
        g_assert_cmpuint(sum,>,0);
    }
    assets_clear(&a); document_clear(&d);
}
int main(int argc,char **argv) {
    g_test_init(&argc,&argv,NULL); char *cwd=g_get_current_dir(); project_root=find_root(cwd); g_free(cwd); g_assert_nonnull(project_root);
    g_test_add_func("/stage/projection-roundtrip",projection_test); g_test_add_func("/stage/near-clipping",clipping_test);
    g_test_add_func("/stage/rectangle-snapping",rectangle_test); g_test_add_func("/document/history-preservation",history_test);
    g_test_add_func("/document/validation",validation_test); g_test_add_func("/document/spawn-references",spawn_delete_test);
    g_test_add_func("/document/atomic-save-conflict-backup",file_test); g_test_add_func("/assets/tim-alpha",tim_test); g_test_add_func("/assets/import-pal-zip",assets_test);
    g_test_add_func("/assets/project-backgrounds",project_backgrounds_test);
    int result=g_test_run(); g_free(project_root); return result;
}
