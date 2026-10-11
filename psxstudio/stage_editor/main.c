#include "stage_editor.h"
#include <stdio.h>

int main(int argc,char **argv) {
    char *root=NULL,*path=NULL,*smoke_path=NULL,*validate_path=NULL;
    GOptionEntry options[]={
        {"root",'r',0,G_OPTION_ARG_FILENAME,&root,"Project root directory","DIRECTORY"},
        {"stages",'s',0,G_OPTION_ARG_FILENAME,&path,"Open a stages JSON file","FILE"},
        {"validate",0,0,G_OPTION_ARG_FILENAME,&validate_path,"Validate JSON without opening GTK","FILE"},
        {"smoke-test",0,0,G_OPTION_ARG_FILENAME,&smoke_path,"Exercise all stage views and save a screenshot","PNG"},
        {NULL,0,0,0,NULL,NULL,NULL}
    };
    GOptionContext *context=g_option_context_new("— native PS1 stage editor"); g_option_context_add_main_entries(context,options,NULL);
    GError *error=NULL;
    if (!g_option_context_parse(context,&argc,&argv,&error)) { g_printerr("%s\n",error->message); g_error_free(error); g_option_context_free(context); return 1; }
    g_option_context_free(context);
    if (validate_path) {
        Document doc={0}; gboolean ok=document_load(&doc,validate_path,&error);
        if (ok) g_print("Valid stages.json: %u stages\n",json_array_get_length(json_node_get_array(doc.data)));
        else { g_printerr("%s\n",error->message); g_error_free(error); }
        document_clear(&doc); g_free(root); g_free(path); g_free(smoke_path); g_free(validate_path); return ok?0:1;
    }
    if (root) { char *resolved=g_canonicalize_filename(root,NULL); g_free(root); root=resolved; }
    else {
        char *cwd=g_get_current_dir(); root=find_root(cwd); g_free(cwd);
        if (!root) {
            char *executable=g_file_read_link("/proc/self/exe",NULL);
            if (executable) { char *dir=g_path_get_dirname(executable); root=find_root(dir); g_free(dir); g_free(executable); }
        }
    }
    if (!root) { g_printerr("Project root not found. Run inside the project or use --root DIRECTORY.\n"); g_free(path); g_free(smoke_path); return 1; }
    Studio studio={0}; studio.sid=0; studio.kind=studio.index=-1; studio.snap=5; studio.zoom=2; studio.view.scale=.1;
    for (int i=0;i<3;i++) studio.layers[i]=TRUE;
    studio.doc.root=root; studio.smoke_path=smoke_path;
    if (!path) path=g_build_filename(root,"stages","stages.json",NULL);
    if (!document_load(&studio.doc,path,&error)) { studio.notice=g_strdup(error->message); g_error_free(error); }
    g_free(path); assets_init(&studio.assets,root); fit_view(&studio.doc,0,&studio.view);
    GtkApplication *app=gtk_application_new("org.psxstudio.StageEditor",G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app,"activate",G_CALLBACK(ui_activate),&studio);
    int status=g_application_run(G_APPLICATION(app),1,argv);
    if (studio.smoke_path && (studio.smoke_failed || !studio.renderer.frames)) status=1;
    studio.quitting=TRUE; ui_clear(&studio); g_object_unref(app); return status;
}
