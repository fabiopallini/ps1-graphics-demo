#include "stage_editor.h"
#include <string.h>

static const float colors[3][3]={{.49f,.86f,.69f},{.94f,.61f,.54f},{.56f,.75f,.96f}};
static GLuint shader(GLenum type, const char *source, GError **error) {
    GLuint result=glCreateShader(type); glShaderSource(result,1,&source,NULL); glCompileShader(result);
    GLint ok; glGetShaderiv(result,GL_COMPILE_STATUS,&ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(result,sizeof log,NULL,log);
        g_set_error(error,G_IO_ERROR,G_IO_ERROR_FAILED,"OpenGL shader: %s",log); glDeleteShader(result); return 0;
    }
    return result;
}
static void realize(GtkGLArea *area, Studio *s) {
    gtk_gl_area_make_current(area); if (gtk_gl_area_get_error(area)) return;
    const char *vertex="#version 330 core\nlayout(location=0) in vec2 p; layout(location=1) in vec2 uv; out vec2 tex; void main(){tex=uv;gl_Position=vec4(p.x/160.0-1.0,1.0-p.y/120.0,0,1);}";
    const char *fragment="#version 330 core\nin vec2 tex; out vec4 color; uniform vec4 tint; uniform bool textured; uniform sampler2D image; void main(){color=textured?texture(image,tex)*tint:tint;}";
    GError *error=NULL; GLuint vs=shader(GL_VERTEX_SHADER,vertex,&error), fs=0;
    if (!error) fs=shader(GL_FRAGMENT_SHADER,fragment,&error);
    if (error) { gtk_gl_area_set_error(area,error); g_error_free(error); if (vs) glDeleteShader(vs); return; }
    Renderer *r=&s->renderer; r->program=glCreateProgram(); glAttachShader(r->program,vs); glAttachShader(r->program,fs); glLinkProgram(r->program);
    glDeleteShader(vs); glDeleteShader(fs); GLint ok; glGetProgramiv(r->program,GL_LINK_STATUS,&ok);
    if (!ok) {
        char log[2048]; glGetProgramInfoLog(r->program,sizeof log,NULL,log);
        error=g_error_new(G_IO_ERROR,G_IO_ERROR_FAILED,"OpenGL program: %s",log); gtk_gl_area_set_error(area,error); g_error_free(error); return;
    }
    r->tint=glGetUniformLocation(r->program,"tint"); r->textured=glGetUniformLocation(r->program,"textured");
    glGenVertexArrays(1,&r->vao); glGenBuffers(1,&r->vbo); glGenTextures(1,&r->background); glGenTextures(1,&r->labels);
    r->uploaded=NULL; r->ready=TRUE;
}
static void unrealize(GtkGLArea *area, Studio *s) {
    gtk_gl_area_make_current(area); Renderer *r=&s->renderer;
    if (!gtk_gl_area_get_error(area)) {
        glDeleteTextures(1,&r->background); glDeleteTextures(1,&r->labels); glDeleteBuffers(1,&r->vbo);
        glDeleteVertexArrays(1,&r->vao); if (r->program) glDeleteProgram(r->program);
    }
    unsigned frames=r->frames; memset(r,0,sizeof *r); r->frames=frames;
}
static void draw(Renderer *r, GLenum mode, const Screen *points, int count, const float *color, float alpha) {
    float vertices[64]; if (count>16 || count<1) return;
    for (int i=0;i<count;i++) { vertices[i*4]=points[i].x; vertices[i*4+1]=points[i].y; vertices[i*4+2]=vertices[i*4+3]=0; }
    glUniform1i(r->textured,FALSE); glUniform4f(r->tint,color[0],color[1],color[2],alpha);
    glBufferData(GL_ARRAY_BUFFER,count*4*sizeof(float),vertices,GL_STREAM_DRAW); glDrawArrays(mode,0,count);
}
static void shape(Renderer *r, Screen *p, int n, int kind, gboolean selected) {
    if (n<3) return;
    draw(r,GL_TRIANGLE_FAN,p,n,colors[kind],selected?.33f:.18f);
    const float white[]={.93f,1,.96f};
    draw(r,GL_LINE_LOOP,p,n,selected?white:colors[kind],1);
}
static void square(Renderer *r, Screen p, double size, const float *color) {
    Screen q[]={{p.x-size,p.y-size},{p.x+size,p.y-size},{p.x+size,p.y+size},{p.x-size,p.y+size}};
    draw(r,GL_TRIANGLE_FAN,q,4,color,1);
}
static void texture(Renderer *r, GLuint texture_id) {
    const float v[]={0,0,0,0,320,0,1,0,320,240,1,1,0,240,0,1};
    glBindTexture(GL_TEXTURE_2D,texture_id); glUniform1i(r->textured,TRUE); glUniform4f(r->tint,1,1,1,1);
    glBufferData(GL_ARRAY_BUFFER,sizeof v,v,GL_STREAM_DRAW); glDrawArrays(GL_TRIANGLE_FAN,0,4);
}
static void upload_background(Renderer *r, Background *bg) {
    if (r->uploaded==bg || !bg || !bg->image) return;
    GdkPixbuf *image=bg->image; int channels=gdk_pixbuf_get_n_channels(image), stride=gdk_pixbuf_get_rowstride(image);
    guint8 *data=g_malloc(320*240*4); const guint8 *pixels=gdk_pixbuf_read_pixels(image);
    for (int y=0;y<240;y++) for (int x=0;x<320;x++) {
        const guint8 *p=pixels+y*stride+x*channels; guint8 *d=data+(y*320+x)*4;
        d[0]=p[0]; d[1]=p[1]; d[2]=p[2]; d[3]=channels==4?p[3]:255;
    }
    glBindTexture(GL_TEXTURE_2D,r->background); glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,320,240,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    g_free(data); r->uploaded=bg;
}
static void label(cairo_t *cr, const char *text, Screen p, const float *color) {
    if (p.x < -30 || p.x > 350 || p.y < -20 || p.y > 260) return;
    cairo_text_extents_t ext; cairo_text_extents(cr,text,&ext);
    cairo_set_source_rgba(cr,.03,.1,.11,.85); cairo_rectangle(cr,p.x-ext.width/2-2,p.y-5,ext.width+4,8); cairo_fill(cr);
    cairo_set_source_rgb(cr,color[0],color[1],color[2]); cairo_move_to(cr,p.x-ext.width/2,p.y+.5); cairo_show_text(cr,text);
}
static gboolean render(GtkGLArea *area, GdkGLContext *context, Studio *s) {
    (void)context;
    Renderer *r=&s->renderer; if (!r->ready) return FALSE;
    int factor=gtk_widget_get_scale_factor(GTK_WIDGET(area));
    int width=gtk_widget_get_width(GTK_WIDGET(area))*factor, height=gtk_widget_get_height(GTK_WIDGET(area))*factor;
    double scale=MIN(width/320.0,height/240.0); int vw=(int)(320*scale), vh=(int)(240*scale);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_SCISSOR_TEST);
    glViewport(0,0,width,height); glClearColor(.25,.25,.25,1); glClear(GL_COLOR_BUFFER_BIT);
    glViewport((width-vw)/2,(height-vh)/2,vw,vh); glEnable(GL_SCISSOR_TEST); glScissor((width-vw)/2,(height-vh)/2,vw,vh);
    glClearColor(.063,.1,.11,1); glClear(GL_COLOR_BUFFER_BIT); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(r->program); glBindVertexArray(r->vao); glBindBuffer(GL_ARRAY_BUFFER,r->vbo);
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void *)0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void *)(2*sizeof(float)));
    glActiveTexture(GL_TEXTURE0);
    JsonObject *st=stage(&s->doc,s->sid);
    if (!st) { glDisable(GL_SCISSOR_TEST); return TRUE; }
    Background *bg=assets_background(&s->assets,st);
    if (!s->view.top && bg && bg->image) { upload_background(r,bg); texture(r,r->background); }
    cairo_surface_t *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,960,720); cairo_t *cr=cairo_create(surface);
    cairo_scale(cr,3,3); cairo_select_font_face(cr,"Sans",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_BOLD); cairo_set_font_size(cr,5);
    if (s->view.top) {
        double grid=MAX(50,pow(10,floor(log10(40/s->view.scale))));
        Point a,b; unproject(st,&s->view,(Screen){0,240},0,&a); unproject(st,&s->view,(Screen){320,0},0,&b);
        const float color[]={.2f,.27f,.24f};
        for (double x=ceil(a.x/grid)*grid;x<=b.x;x+=grid) {
            Screen p; project(st,&s->view,(Point){x,0,0},&p); Screen line[]={{p.x,0},{p.x,240}}; draw(r,GL_LINES,line,2,color,.6);
        }
        for (double z=ceil(a.z/grid)*grid;z<=b.z;z+=grid) {
            Screen p; project(st,&s->view,(Point){0,0,z},&p); Screen line[]={{0,p.y},{320,p.y}}; draw(r,GL_LINES,line,2,color,.6);
        }
        char *text=g_strdup_printf("Grid %.0f units / Z up",grid); cairo_set_source_rgb(cr,.55,.65,.6); cairo_move_to(cr,6,10); cairo_show_text(cr,text); g_free(text);
    } else if (!bg || !bg->image) {
        const float color[]={1,1,1}; label(cr,"Background unavailable: assign a PNG",(Screen){160,120},color);
    }
    for (int k=0;k<3;k++) {
        if (!s->layers[k]) continue;
        JsonArray *a=array(st,kinds[k]);
        for (unsigned i=0;i<json_array_get_length(a);i++) {
            JsonObject *o=item(&s->doc,s->sid,k,i); gboolean selected=s->kind==k && s->index==(int)i;
            Screen center;
            if (k<2) {
                Screen p[8]; int n=polygon(st,&s->view,o,p); shape(r,p,n,k,selected);
                Point c=coordinates(o); c.x+=number(o,"w")/2; c.z+=number(o,"d")/2;
                if (n && project(st,&s->view,c,&center)) {
                    char *text=k==1?g_strdup_printf("Z%u > %.0f",i,number(o,"stage_id")):g_strdup_printf("P%u",i);
                    label(cr,text,center,colors[k]); g_free(text);
                }
                if (selected) {
                    Point corners_[4]; corners(o,corners_,FALSE); const float white[]={.93f,1,.96f};
                    for (int j=0;j<4;j++) if (project(st,&s->view,corners_[j],&center)) square(r,center,2,white);
                }
            } else if (project(st,&s->view,coordinates(o),&center)) {
                Screen disk[16]; double radius=selected?3.2:2.5;
                for (int j=0;j<16;j++) disk[j]=(Screen){center.x+radius*cos(j*G_PI/8),center.y+radius*sin(j*G_PI/8)};
                draw(r,GL_TRIANGLE_FAN,disk,16,colors[k],1);
                double heading=number(o,"ry")*G_PI/2048; Point p=coordinates(o); p.x+=sin(heading)*60; p.z+=cos(heading)*60;
                Screen end; if (project(st,&s->view,p,&end)) { Screen line[]={center,end}; draw(r,GL_LINES,line,2,colors[k],1); }
                char *text=g_strdup_printf("I%u",i); center.y-=7; label(cr,text,center,colors[k]); g_free(text);
            }
        }
    }
    JsonArray *npcs=array(st,"npcs"); const float npc_color[]={.93f,.83f,.54f};
    for (unsigned i=0;npcs && i<json_array_get_length(npcs);i++) {
        JsonObject *group=json_array_get_object_element(npcs,i); GList *keys=json_object_get_members(group);
        for (GList *key=keys;key;key=key->next) {
            JsonObject *npc=json_object_get_object_member(group,key->data); Screen p;
            if (project(st,&s->view,coordinates(npc),&p)) { square(r,p,1.5,npc_color); p.y+=8; label(cr,"NPC",p,npc_color); }
        }
        g_list_free(keys);
    }
    if (s->preview) { Screen p[8]; int n=polygon(st,&s->view,json_node_get_object(s->preview),p); shape(r,p,n,s->tool-1,TRUE); }
    cairo_destroy(cr); cairo_surface_flush(surface);
    glBindTexture(GL_TEXTURE_2D,r->labels); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,960,720,0,GL_BGRA,GL_UNSIGNED_BYTE,cairo_image_surface_get_data(surface));
    /* Cairo's ARGB pixels are premultiplied. */
    glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA); texture(r,r->labels); cairo_surface_destroy(surface);
    glBindVertexArray(0); glUseProgram(0); glDisable(GL_SCISSOR_TEST); r->frames++; return TRUE;
}
void render_connect(Studio *s) {
    gtk_gl_area_set_required_version(GTK_GL_AREA(s->area),3,3);
    gtk_gl_area_set_allowed_apis(GTK_GL_AREA(s->area),GDK_GL_API_GL);
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(s->area),FALSE);
    g_signal_connect(s->area,"realize",G_CALLBACK(realize),s);
    g_signal_connect(s->area,"unrealize",G_CALLBACK(unrealize),s);
    g_signal_connect(s->area,"render",G_CALLBACK(render),s);
}
void render_queue(Studio *s) { if (s->area && !s->quitting) gtk_gl_area_queue_render(GTK_GL_AREA(s->area)); }
Screen render_pointer(Studio *s, double x, double y) {
    double w=gtk_widget_get_width(s->area),h=gtk_widget_get_height(s->area),scale=MIN(w/320,h/240);
    return (Screen){(x-(w-320*scale)/2)/scale,(y-(h-240*scale)/2)/scale};
}
