#include "editor.h"
void render_basis(Editor *e, Vec *right, Vec *up, Vec *forward) {
    double yaw = e->view == 1 ? 0 : e->view == 2 ? G_PI / 2 : e->view == 3 ? 0 : e->yaw;
    double pitch = e->view == 3 ? G_PI / 2 : e->view ? 0 : e->pitch;
    *right = (Vec){cos(yaw), 0, -sin(yaw)};
    *up = (Vec){-sin(yaw) * sin(pitch), cos(pitch), -cos(yaw) * sin(pitch)};
    *forward = (Vec){sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch)};
    if (e->ydown) {
        right->y = -right->y;
        up->y = -up->y;
        forward->y = -forward->y;
    }
}
Projected render_project(Editor *e, Vec p) {
    Vec r, u, f;
    render_basis(e, &r, &u, &f);
    p = vec_sub(p, e->center);
    double h = MAX(1, gtk_widget_get_height(e->area)), w = MAX(1, gtk_widget_get_width(e->area));
    double z = e->span * 1.5 - vec_dot(p, f),
           scale = e->view ? h / e->span : h / (2 * tan(G_PI / 8) * z);
    return (Projected){w / 2 + vec_dot(p, r) * scale, h / 2 - vec_dot(p, u) * scale, z, scale,
                       e->view || z > e->span * .001};
}
static GLuint shader(GLenum type, const char *source, GError **error) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "OpenGL shader: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}
static void realize(GtkGLArea *area, Editor *e) {
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area))
        return;
    const char *vs = "#version 330 core\nlayout(location=0) in vec4 p; layout(location=1) in vec2 "
                     "uv; layout(location=2) in vec4 color; out vec2 texcoord; out vec4 tint; void "
                     "main(){gl_Position=p;texcoord=uv;tint=color;}";
    const char *fs = "#version 330 core\nin vec2 texcoord; in vec4 tint; uniform bool textured; "
                     "uniform sampler2D image; out vec4 color; void main(){vec4 "
                     "t=textured?texture(image,texcoord):vec4(1);if(t.a<0.1)discard;color=t*tint;}";
    GError *error = NULL;
    GLuint a = shader(GL_VERTEX_SHADER, vs, &error), b = 0;
    if (!error)
        b = shader(GL_FRAGMENT_SHADER, fs, &error);
    if (error) {
        gtk_gl_area_set_error(area, error);
        g_error_free(error);
        if (a)
            glDeleteShader(a);
        return;
    }
    e->program = glCreateProgram();
    glAttachShader(e->program, a);
    glAttachShader(e->program, b);
    glLinkProgram(e->program);
    glDeleteShader(a);
    glDeleteShader(b);
    GLint ok;
    glGetProgramiv(e->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(e->program, sizeof log, NULL, log);
        error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED, "OpenGL link: %s", log);
        gtk_gl_area_set_error(area, error);
        g_error_free(error);
        return;
    }
    glGenVertexArrays(1, &e->vao);
    glGenBuffers(1, &e->vbo);
    glGenTextures(1, &e->tex);
    e->textured = glGetUniformLocation(e->program, "textured");
    e->ready = TRUE;
}
static void unrealize(GtkGLArea *a, Editor *e) {
    gtk_gl_area_make_current(a);
    if (!gtk_gl_area_get_error(a)) {
        glDeleteVertexArrays(1, &e->vao);
        glDeleteBuffers(1, &e->vbo);
        glDeleteTextures(1, &e->tex);
        if (e->program)
            glDeleteProgram(e->program);
    }
    e->ready = FALSE;
}
static void vertex(Editor *e, GArray *out, Vec p, UV uv, const float color[4]) {
    Vec r, u, f;
    render_basis(e, &r, &u, &f);
    p = vec_sub(p, e->center);
    double h = MAX(1, gtk_widget_get_height(e->area)), w = MAX(1, gtk_widget_get_width(e->area));
    double x = vec_dot(p, r), y = vec_dot(p, u), z = e->span * 1.5 - vec_dot(p, f),
           near = e->span * .001, far = e->span * 100;
    float v[10];
    if (e->view) {
        v[0] = x * 2 / e->span * h / w;
        v[1] = y * 2 / e->span;
        v[2] = 2 * (z - near) / (far - near) - 1;
        v[3] = 1;
    } else {
        double focal = 1 / tan(G_PI / 8);
        v[0] = x * focal * h / w;
        v[1] = y * focal;
        v[2] = (far + near) / (far - near) * z - 2 * far * near / (far - near);
        v[3] = z;
    }
    v[4] = uv.u;
    v[5] = 1 - uv.v;
    memcpy(v + 6, color, 4 * sizeof(float));
    g_array_append_vals(out, v, 10);
}
static void draw(Editor *e, GArray *v, GLenum mode, gboolean texture) {
    glUniform1i(e->textured, texture);
    glBufferData(GL_ARRAY_BUFFER, v->len * sizeof(float), v->data, GL_STREAM_DRAW);
    glDrawArrays(mode, 0, v->len / 10);
    g_array_set_size(v, 0);
}
static gboolean selected(Editor *e, guint n) {
    if (e->mode == 2)
        return TRUE;
    for (guint i = 0; i < e->selection->len; i++)
        if (AT(e->selection, int, i) == (int)n)
            return TRUE;
    return FALSE;
}
static gboolean render(GtkGLArea *area, GdkGLContext *context, Editor *e) {
    (void)context;
    if (!e->ready)
        return FALSE;
    int factor = gtk_widget_get_scale_factor(GTK_WIDGET(area));
    glViewport(0, 0, gtk_widget_get_width(GTK_WIDGET(area)) * factor,
               gtk_widget_get_height(GTK_WIDGET(area)) * factor);
    glClearColor(.075, .12, .13, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(e->program);
    glBindVertexArray(e->vao);
    glBindBuffer(GL_ARRAY_BUFFER, e->vbo);
    for (int i = 0; i < 3; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, i == 1 ? 2 : 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                              (void *)(uintptr_t)((i == 0   ? 0
                                                   : i == 1 ? 4
                                                            : 6) *
                                                  sizeof(float)));
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->tex);
    if (e->texture) {
        int w = gdk_pixbuf_get_width(e->texture), h = gdk_pixbuf_get_height(e->texture),
            c = gdk_pixbuf_get_n_channels(e->texture),
            stride = gdk_pixbuf_get_rowstride(e->texture);
        guchar *rgba = g_malloc(w * h * 4);
        const guchar *source = gdk_pixbuf_read_pixels(e->texture);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                const guchar *p = source + y * stride + x * c;
                guchar *q = rgba + (y * w + x) * 4;
                q[0] = p[0];
                q[1] = p[1];
                q[2] = p[2];
                q[3] = c == 4 ? p[3] : 255;
            }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        g_free(rgba);
    }
    GArray *v = g_array_new(FALSE, FALSE, sizeof(float));
    UV zero = {0, 0};
    if (e->grid) {
        double step = pow(10, floor(log10(MAX(.01, e->span / 6))));
        float color[] = {.2, .3, .3, 1};
        for (int i = -10; i <= 10; i++) {
            vertex(e, v, (Vec){i * step, 0, -10 * step}, zero, color);
            vertex(e, v, (Vec){i * step, 0, 10 * step}, zero, color);
            vertex(e, v, (Vec){-10 * step, 0, i * step}, zero, color);
            vertex(e, v, (Vec){10 * step, 0, i * step}, zero, color);
        }
        draw(e, v, GL_LINES, FALSE);
        Vec axis[] = {{step * 3, 0, 0}, {0, step * 3, 0}, {0, 0, step * 3}};
        float colors[][4] = {{1, .3, .3, 1}, {.3, 1, .3, 1}, {.4, .6, 1, 1}};
        for (int i = 0; i < 3; i++) {
            vertex(e, v, (Vec){0, 0, 0}, zero, colors[i]);
            vertex(e, v, axis[i], zero, colors[i]);
        }
        draw(e, v, GL_LINES, FALSE);
    }
    if (e->shade != 1) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1, 1);
        for (guint i = 0; i < e->mesh.faces->len; i++) {
            Face f = AT(e->mesh.faces, Face, i);
            Vec n = mesh_normal(&e->mesh, &f);
            double light = .4 + .6 * fabs(vec_dot(n, vec_unit((Vec){.3, -.8, .6})));
            float color[] = {.65 * light, .78 * light, .76 * light, 1};
            if (e->mode == 1 && selected(e, i)) {
                color[0] = .95;
                color[1] = .68;
                color[2] = .3;
            }
            int tri[] = {0, 1, 2, 0, 2, 3};
            for (int j = 0; j < (f.n == 3 ? 3 : 6); j++) {
                int k = tri[j];
                vertex(e, v, AT(e->mesh.vertices, Vec, f.v[k]), AT(e->mesh.uvs, UV, f.uv[k]),
                       color);
            }
        }
        draw(e, v, GL_TRIANGLES, e->shade == 2 && e->texture);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if (e->edges || e->shade == 1) {
        float color[] = {.3, .85, .75, 1};
        for (guint i = 0; i < e->mesh.faces->len; i++) {
            Face f = AT(e->mesh.faces, Face, i);
            float highlight[] = {1, .75, .3, 1};
            const float *c = e->mode == 1 && selected(e, i) ? highlight : color;
            for (int j = 0; j < f.n; j++) {
                vertex(e, v, AT(e->mesh.vertices, Vec, f.v[j]), zero, c);
                vertex(e, v, AT(e->mesh.vertices, Vec, f.v[(j + 1) % f.n]), zero, c);
            }
        }
        draw(e, v, GL_LINES, FALSE);
    }
    if (e->mode == 0) {
        glDisable(GL_DEPTH_TEST);
        glPointSize(6 * factor);
        for (guint i = 0; i < e->mesh.vertices->len; i++) {
            float color[] = {.75, .85, .85, 1};
            if (selected(e, i)) {
                color[0] = 1;
                color[1] = .6;
                color[2] = .2;
            }
            vertex(e, v, AT(e->mesh.vertices, Vec, i), zero, color);
        }
        draw(e, v, GL_POINTS, FALSE);
    }
    if (e->drag_kind == 4) {
        glDisable(GL_DEPTH_TEST);
        double w = MAX(1, gtk_widget_get_width(e->area)),
               h = MAX(1, gtk_widget_get_height(e->area));
        double corners[][2] = {{e->start_x, e->start_y},
                               {e->last_x, e->start_y},
                               {e->last_x, e->last_y},
                               {e->start_x, e->last_y}};
        for (int i = 0; i < 4; i++) {
            float p[] = {
                corners[i][0] * 2 / w - 1, 1 - corners[i][1] * 2 / h, 0, 1, 0, 0, 1, .8, .3, 1};
            g_array_append_vals(v, p, 10);
        }
        draw(e, v, GL_LINE_LOOP, FALSE);
    }
    g_array_unref(v);
    glBindVertexArray(0);
    e->frames++;
    return TRUE;
}
void render_connect(Editor *e) {
    gtk_gl_area_set_required_version(GTK_GL_AREA(e->area), 3, 3);
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(e->area), TRUE);
    gtk_gl_area_set_allowed_apis(GTK_GL_AREA(e->area), GDK_GL_API_GL);
    g_signal_connect(e->area, "realize", G_CALLBACK(realize), e);
    g_signal_connect(e->area, "unrealize", G_CALLBACK(unrealize), e);
    g_signal_connect(e->area, "render", G_CALLBACK(render), e);
}
void render_queue(Editor *e) {
    if (e->area)
        gtk_gl_area_queue_render(GTK_GL_AREA(e->area));
}
void editor_frame(Editor *e) {
    Vec min = {G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE},
        max = {-G_MAXDOUBLE, -G_MAXDOUBLE, -G_MAXDOUBLE};
    if (!e->mesh.vertices->len) {
        e->center = (Vec){0, 0, 0};
        e->span = 3;
        return;
    }
    for (guint i = 0; i < e->mesh.vertices->len; i++) {
        Vec p = AT(e->mesh.vertices, Vec, i);
        min = (Vec){MIN(min.x, p.x), MIN(min.y, p.y), MIN(min.z, p.z)};
        max = (Vec){MAX(max.x, p.x), MAX(max.y, p.y), MAX(max.z, p.z)};
    }
    e->center = vec_mul(vec_add(min, max), .5);
    double radius = 0;
    for (guint i = 0; i < e->mesh.vertices->len; i++) {
        Vec p = vec_sub(AT(e->mesh.vertices, Vec, i), e->center);
        radius = MAX(radius, sqrt(vec_dot(p, p)));
    }
    e->span = MAX(.1, radius * 3);
    render_queue(e);
}
