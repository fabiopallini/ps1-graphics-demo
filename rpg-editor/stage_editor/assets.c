#include "stage_editor.h"
#include <string.h>
#include <glib/gstdio.h>

static void background_free(gpointer data) {
    Background *b=data;
    g_clear_object(&b->image); g_free(b->directory); g_free(b);
}
void assets_add_file(Assets *a, const char *path) {
    char *base=g_path_get_basename(path), *upper=g_ascii_strup(base,-1);
    g_hash_table_replace(a->files,upper,g_strdup(path)); g_free(base);
}
static void scan(Assets *a, const char *dir) {
    GDir *d=g_dir_open(dir,0,NULL); if (!d) return;
    const char *name;
    while ((name=g_dir_read_name(d))) {
        char *upper=g_ascii_strup(name,-1);
        if (g_str_has_suffix(upper,".PNG")) {
            char *path=g_build_filename(dir,name,NULL); assets_add_file(a,path); g_free(path);
        }
        g_free(upper);
    }
    g_dir_close(d);
}
void assets_init(Assets *a, const char *root) {
    a->root=g_strdup(root); a->directory=g_build_filename(root,"rpg-editor","stage_editor","assets",NULL);
    a->cache=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,background_free);
    a->files=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
    char *dir=g_build_filename(root,"data","stages",NULL); scan(a,dir); g_free(dir);
}
void assets_clear(Assets *a) {
    if (a->cache) g_hash_table_unref(a->cache);
    if (a->files) g_hash_table_unref(a->files);
    g_free(a->root); g_free(a->directory); memset(a,0,sizeof *a);
}
void assets_invalidate(Assets *a) { g_hash_table_remove_all(a->cache); }
static char *asset_key(JsonObject *s) { return g_strdup_printf("%s|%s",string(s,"tim_0"),string(s,"tim_1")); }
static JsonNode *manifest(const char *dir) {
    char *path=g_build_filename(dir,"index.json",NULL);
    JsonParser *p=json_parser_new(); JsonNode *n=NULL;
    if (json_parser_load_from_file(p,path,NULL) && JSON_NODE_HOLDS_ARRAY(json_parser_get_root(p))) n=json_node_copy(json_parser_get_root(p));
    g_object_unref(p); g_free(path);
    if (!n) { n=json_node_new(JSON_NODE_ARRAY); json_node_take_array(n,json_array_new()); }
    return n;
}
static char *imported_directory(const char *dir, JsonObject *s) {
    JsonNode *n=manifest(dir); JsonArray *a=json_node_get_array(n); char *result=NULL;
    for (int i=(int)json_array_get_length(a)-1;i>=0;i--) {
        JsonNode *item=json_array_get_element(a,i);
        if (!JSON_NODE_HOLDS_OBJECT(item)) continue;
        JsonObject *o=json_node_get_object(item);
        const char *id=string(o,"id");
        if (!*id || strchr(id,'/') || strchr(id,'\\') || strstr(id,"..")) continue;
        if (!strcmp(string(o,"tim0"),string(s,"tim_0")) && !strcmp(string(o,"tim1"),string(s,"tim_1"))) {
            char *path=g_build_filename(dir,id,"preview.png",NULL);
            if (g_file_test(path,G_FILE_TEST_IS_REGULAR)) result=g_build_filename(dir,id,NULL);
            g_free(path); if (result) break;
        }
    }
    json_node_unref(n); return result;
}
static GdkPixbuf *preview(GdkPixbuf *source) {
    int w=gdk_pixbuf_get_width(source), h=gdk_pixbuf_get_height(source);
    if (w==320 && h>=240) {
        GdkPixbuf *sub=gdk_pixbuf_new_subpixbuf(source,0,0,320,240), *copy=gdk_pixbuf_copy(sub);
        g_object_unref(sub); return copy;
    }
    return gdk_pixbuf_scale_simple(source,320,240,GDK_INTERP_BILINEAR);
}
Background *assets_background(Assets *a, JsonObject *s) {
    if (!s) return NULL;
    char *key=asset_key(s); Background *b=g_hash_table_lookup(a->cache,key);
    if (b) { g_free(key); return b; }
    b=g_new0(Background,1);
    b->directory=imported_directory(a->directory,s);
    if (b->directory) {
        char *path=g_build_filename(b->directory,"preview.png",NULL);
        GdkPixbuf *source=gdk_pixbuf_new_from_file(path,NULL);
        if (source) { b->image=preview(source); g_object_unref(source); }
        g_free(path);
    }
    if (!b->image) {
        const char *tim=string(s,"tim_0");
        char *prefix=g_strdup(tim); char *suffix=strrchr(prefix,'_');
        if (suffix && (!strcmp(suffix,"_0.TIM")||!strcmp(suffix,"_1.TIM"))) *suffix=0;
        else { suffix=strrchr(prefix,'.'); if (suffix) *suffix=0; }
        char *full=g_strconcat(prefix,".PNG",NULL);
        const char *path=g_hash_table_lookup(a->files,full);
        if (path) {
            GdkPixbuf *source=gdk_pixbuf_new_from_file(path,NULL);
            if (source && gdk_pixbuf_get_width(source)==320 && gdk_pixbuf_get_height(source)>=240) b->image=preview(source);
            g_clear_object(&source);
        }
        g_free(prefix); g_free(full);
    }
    if (!b->image) {
        GdkPixbuf *parts[2]={NULL,NULL};
        for (int i=0;i<2;i++) {
            char *name=g_strdup(string(s,i?"tim_1":"tim_0")); char *ext=strrchr(name,'.');
            if (ext) strcpy(ext,".PNG");
            const char *path=g_hash_table_lookup(a->files,name);
            if (path) parts[i]=gdk_pixbuf_new_from_file(path,NULL);
            g_free(name);
        }
        GdkPixbuf *left=NULL,*right=NULL;
        for (int i=0;i<2;i++) if (parts[i] && gdk_pixbuf_get_height(parts[i])>=240) {
            if (gdk_pixbuf_get_width(parts[i])==256) left=parts[i];
            if (gdk_pixbuf_get_width(parts[i])==64) right=parts[i];
        }
        if (left && right) {
            b->image=gdk_pixbuf_new(GDK_COLORSPACE_RGB,FALSE,8,320,240); gdk_pixbuf_fill(b->image,0x000000ff);
            gdk_pixbuf_composite(left,b->image,0,0,256,240,0,0,1,1,GDK_INTERP_NEAREST,255);
            gdk_pixbuf_composite(right,b->image,255,0,64,240,255,0,1,1,GDK_INTERP_NEAREST,255);
        }
        for (int i=0;i<2;i++) g_clear_object(&parts[i]);
    }
    g_hash_table_insert(a->cache,key,b); return b;
}
static void u16(GByteArray *b, unsigned v) {
    guint8 p[2]={(guint8)v,(guint8)(v>>8)}; g_byte_array_append(b,p,2);
}
static void u32(GByteArray *b, uint32_t v) {
    guint8 p[4]={(guint8)v,(guint8)(v>>8),(guint8)(v>>16),(guint8)(v>>24)}; g_byte_array_append(b,p,4);
}
GBytes *encode_tim(GdkPixbuf *image, int x, int y) {
    int w=gdk_pixbuf_get_width(image), h=gdk_pixbuf_get_height(image), stride=gdk_pixbuf_get_rowstride(image), channels=gdk_pixbuf_get_n_channels(image);
    GByteArray *bytes=g_byte_array_new(); u32(bytes,0x10); u32(bytes,2); u32(bytes,12+w*h*2);
    u16(bytes,x); u16(bytes,y); u16(bytes,w); u16(bytes,h);
    const guint8 *pixels=gdk_pixbuf_read_pixels(image);
    for (int j=0;j<h;j++) for (int i=0;i<w;i++) {
        const guint8 *p=pixels+j*stride+i*channels;
        unsigned v=(p[0]>>3)|((p[1]>>3)<<5)|((p[2]>>3)<<10);
        if (channels==4 && p[3]<128) v=0; else if (!v) v=0x8000;
        u16(bytes,v);
    }
    return g_byte_array_free_to_bytes(bytes);
}
static gboolean write_bytes(const char *path, GBytes *bytes, GError **error) {
    gsize size; const char *data=g_bytes_get_data(bytes,&size);
    return g_file_set_contents_full(path,data,size,G_FILE_SET_CONTENTS_CONSISTENT,0644,error);
}
static void remove_package(const char *dir) {
    GDir *d=g_dir_open(dir,0,NULL); const char *name;
    while (d && (name=g_dir_read_name(d))) { char *p=g_build_filename(dir,name,NULL); g_unlink(p); g_free(p); }
    if (d) g_dir_close(d);
    g_rmdir(dir);
}
gboolean assets_import(Assets *a, JsonObject *s, const char *path, GError **error) {
    if (!tim_name(string(s,"tim_0")) || !tim_name(string(s,"tim_1")) || !strcmp(string(s,"tim_0"),string(s,"tim_1"))) {
        g_set_error_literal(error,G_IO_ERROR,G_IO_ERROR_INVALID_ARGUMENT,"Use two different valid TIM names before importing."); return FALSE;
    }
    char *format=NULL; GdkPixbufFormat *f=gdk_pixbuf_get_file_info(path,NULL,NULL);
    if (f) format=gdk_pixbuf_format_get_name(f);
    gboolean png=!g_strcmp0(format,"png"); g_free(format);
    if (!png) { g_set_error_literal(error,G_IO_ERROR,G_IO_ERROR_INVALID_ARGUMENT,"Choose a PNG image."); return FALSE; }
    GdkPixbuf *source=gdk_pixbuf_new_from_file(path,error); if (!source) return FALSE;
    GdkPixbuf *scaled;
    int height=gdk_pixbuf_get_width(source)==320 && gdk_pixbuf_get_height(source)==256?256:240;
    scaled=gdk_pixbuf_scale_simple(source,320,height,GDK_INTERP_BILINEAR); g_object_unref(source);
    GdkPixbuf *full=gdk_pixbuf_new(GDK_COLORSPACE_RGB,FALSE,8,320,256); gdk_pixbuf_fill(full,0x000000ff);
    /* Composite alpha onto opaque black; reflect floor rows to fill PAL's 256 lines. */
    gdk_pixbuf_composite(scaled,full,0,0,320,height,0,0,1,1,GDK_INTERP_NEAREST,255);
    g_object_unref(scaled);
    if (height==240) for (int y=240;y<256;y++) gdk_pixbuf_copy_area(full,0,479-y,320,1,full,0,y);
    char *uuid=g_uuid_string_random(), *dir=g_build_filename(a->directory,uuid,NULL);
    if (g_mkdir_with_parents(dir,0755)) {
        g_set_error_literal(error,G_IO_ERROR,G_IO_ERROR_FAILED,"Cannot create the imported asset directory.");
        g_object_unref(full); g_free(uuid); g_free(dir); return FALSE;
    }
    GdkPixbuf *prev=preview(full); char *out=g_build_filename(dir,"preview.png",NULL);
    gboolean ok=gdk_pixbuf_save(prev,out,"png",error,NULL); g_free(out); g_object_unref(prev);
    JsonArray *files=json_array_new();
    for (int i=0;i<2 && ok;i++) {
        int width=i?64:256, offset=i?255:0;
        GdkPixbuf *part=gdk_pixbuf_new_subpixbuf(full,offset,0,width,256);
        const char *tim=string(s,i?"tim_1":"tim_0");
        char *name=g_strdup(tim); strcpy(strrchr(name,'.'),".png");
        out=g_build_filename(dir,name,NULL); ok=gdk_pixbuf_save(part,out,"png",error,NULL); g_free(out);
        json_array_add_string_element(files,name); g_free(name);
        name=g_ascii_strdown(tim,-1); out=g_build_filename(dir,name,NULL);
        GBytes *bytes=encode_tim(part,i?576:320,0);
        if (ok) ok=write_bytes(out,bytes,error);
        g_bytes_unref(bytes); g_free(out); json_array_add_string_element(files,name); g_free(name); g_object_unref(part);
    }
    g_object_unref(full);
    if (ok) {
        JsonNode *index=manifest(a->directory); JsonObject *entry=json_object_new();
        json_object_set_string_member(entry,"id",uuid); json_object_set_string_member(entry,"name",string(s,"name"));
        json_object_set_string_member(entry,"tim0",string(s,"tim_0")); json_object_set_string_member(entry,"tim1",string(s,"tim_1"));
        json_object_set_array_member(entry,"files",files); files=NULL;
        json_array_add_object_element(json_node_get_array(index),entry);
        JsonGenerator *g=json_generator_new(); json_generator_set_root(g,index); json_generator_set_pretty(g,TRUE);
        char *text=json_generator_to_data(g,NULL); out=g_build_filename(a->directory,"index.json",NULL);
        ok=g_file_set_contents_full(out,text,-1,G_FILE_SET_CONTENTS_CONSISTENT,0644,error);
        g_free(out); g_free(text); g_object_unref(g); json_node_unref(index);
    }
    if (!ok) remove_package(dir);
    if (files) json_array_unref(files);
    g_free(uuid); g_free(dir); if (ok) assets_invalidate(a); return ok;
}
typedef struct { char *name; GBytes *bytes; uint32_t crc, offset; } ZipItem;
static uint32_t crc32_bytes(GBytes *b) {
    gsize size; const guint8 *p=g_bytes_get_data(b,&size); uint32_t crc=0xffffffff;
    for (gsize i=0;i<size;i++) { crc^=p[i]; for (int k=0;k<8;k++) crc=(crc>>1)^((crc&1)?0xedb88320:0); }
    return crc^0xffffffff;
}
static void zip_item_free(gpointer data) { ZipItem *i=data; g_free(i->name); g_bytes_unref(i->bytes); g_free(i); }
static void zip_add(GPtrArray *a, const char *name, GBytes *bytes) {
    ZipItem *i=g_new0(ZipItem,1); i->name=g_strdup(name); i->bytes=g_bytes_ref(bytes); i->crc=crc32_bytes(bytes); g_ptr_array_add(a,i);
}
static GBytes *zip_pack(GPtrArray *items) {
    GByteArray *b=g_byte_array_new();
    for (unsigned j=0;j<items->len;j++) {
        ZipItem *i=g_ptr_array_index(items,j); gsize size; const guint8 *data=g_bytes_get_data(i->bytes,&size); i->offset=b->len;
        u32(b,0x04034b50); u16(b,20); u16(b,0); u16(b,0); u16(b,0); u16(b,33);
        u32(b,i->crc); u32(b,size); u32(b,size); u16(b,strlen(i->name)); u16(b,0);
        g_byte_array_append(b,(const guint8 *)i->name,strlen(i->name)); g_byte_array_append(b,data,size);
    }
    uint32_t start=b->len;
    for (unsigned j=0;j<items->len;j++) {
        ZipItem *i=g_ptr_array_index(items,j); gsize size=g_bytes_get_size(i->bytes);
        u32(b,0x02014b50); u16(b,20); u16(b,20); u16(b,0); u16(b,0); u16(b,0); u16(b,33);
        u32(b,i->crc); u32(b,size); u32(b,size); u16(b,strlen(i->name)); u16(b,0); u16(b,0);
        u16(b,0); u16(b,0); u32(b,0); u32(b,i->offset); g_byte_array_append(b,(const guint8 *)i->name,strlen(i->name));
    }
    uint32_t length=b->len-start;
    u32(b,0x06054b50); u16(b,0); u16(b,0); u16(b,items->len); u16(b,items->len); u32(b,length); u32(b,start); u16(b,0);
    return g_byte_array_free_to_bytes(b);
}
gboolean assets_export(Assets *a, JsonObject *s, const char *path, GError **error) {
    Background *bg=assets_background(a,s);
    if (!bg || !bg->directory) { g_set_error_literal(error,G_IO_ERROR,G_IO_ERROR_NOT_FOUND,"Assign a PNG background before exporting its package."); return FALSE; }
    GPtrArray *items=g_ptr_array_new_with_free_func(zip_item_free); gboolean ok=TRUE;
    GString *cti=g_string_new(NULL);
    for (int i=0;i<2 && ok;i++) {
        const char *tim=string(s,i?"tim_1":"tim_0"); char *stem=g_strndup(tim,strlen(tim)-4);
        for (int k=0;k<2 && ok;k++) {
            char *name=k?g_ascii_strdown(tim,-1):g_strconcat(stem,".png",NULL);
            char *source=g_build_filename(bg->directory,name,NULL), *data=NULL; gsize size=0;
            if (k && !g_file_test(source,G_FILE_TEST_IS_REGULAR)) {
                g_free(name); name=g_strconcat(stem,".tim",NULL); g_free(source); source=g_build_filename(bg->directory,name,NULL);
            }
            ok=g_file_get_contents(source,&data,&size,error);
            if (ok) {
                GBytes *bytes=g_bytes_new_take(data,size); char *destination=g_strconcat("data/stages/",name,NULL);
                zip_add(items,destination,bytes); g_bytes_unref(bytes); g_free(destination);
            }
            g_free(source); g_free(name);
        }
        char *lower=g_ascii_strdown(tim,-1), *legacy=g_strconcat(stem,".tim",NULL), *check=g_build_filename(bg->directory,lower,NULL);
        g_string_append_printf(cti,"File %s\n    XAFileAttributes Form1 Data\n    Source [root]\\data\\stages\\%s\nEndFile\n\n",tim,g_file_test(check,G_FILE_TEST_IS_REGULAR)?lower:legacy);
        g_free(check); g_free(legacy);
        g_free(lower); g_free(stem);
    }
    if (ok) {
        GBytes *bytes=g_bytes_new(cti->str,cti->len); zip_add(items,"CONF_ENTRIES.TXT",bytes); g_bytes_unref(bytes);
        const char *readme="Copy data/stages files into the project. Add CONF_ENTRIES.TXT entries at the root of cdrom/CONF.CTI without duplicates.\nSave stages.json manually, run make build in stages, then bash build.sh from the project root.\n16-bit TIM pages: 256x256 at (320,0), 64x256 at (576,0); right page starts at source column 255.\nPAL backgrounds contain 256 opaque rows. Generated names must not overlap other CD entries.\n";
        bytes=g_bytes_new_static(readme,strlen(readme)); zip_add(items,"README.txt",bytes); g_bytes_unref(bytes);
        bytes=zip_pack(items); ok=write_bytes(path,bytes,error); g_bytes_unref(bytes);
    }
    g_string_free(cti,TRUE); g_ptr_array_unref(items); return ok;
}
