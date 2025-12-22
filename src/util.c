#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include "lodepng.h"
#include "matrix.h"
#include "platform.h"
#include "util.h"

// Asset path resolver
//
// Crafthex was originally run from the repository root where relative paths like
// "textures/texture.png" and "shaders/*.glsl" exist. If you run the binary from
// a different working directory (e.g., build/), those relative paths break.
//
// We resolve asset paths by trying (in order):
//   1) the provided path as-is
//   2) $CRAFTHEX_ASSET_DIR/<path>
//   3) <exe_dir>/<path>
//   4) <exe_dir>/../<path>
//   5) <exe_dir>/../../<path>

static int file_exists(const char *path) {
    return platform_file_readable(path);
}

static void path_join(char *out, size_t out_size, const char *a, const char *b) {
    if (!out || out_size == 0) {
        return;
    }
    if (!a) a = "";
    if (!b) b = "";
    // Ensure exactly one '/'
    size_t alen = strlen(a);
    int need_slash = (alen > 0 && a[alen - 1] != '/');
    if (need_slash) {
        snprintf(out, out_size, "%s/%s", a, b);
    }
    else {
        snprintf(out, out_size, "%s%s", a, b);
    }
}

static int get_exe_dir(char *out, size_t out_size) {
    return platform_get_exe_dir(out, out_size);
}

static int resolve_asset_path(const char *in, char *out, size_t out_size) {
    if (!in || !out || out_size == 0) {
        return 0;
    }
    // 1) As-is
    if (file_exists(in)) {
        snprintf(out, out_size, "%s", in);
        return 1;
    }
    // 2) Env var root
    const char *asset_root = getenv("CRAFTHEX_ASSET_DIR");
    if (asset_root && asset_root[0]) {
        char candidate[PATH_MAX];
        path_join(candidate, sizeof(candidate), asset_root, in);
        if (file_exists(candidate)) {
            snprintf(out, out_size, "%s", candidate);
            return 1;
        }
    }
    // 3-5) Based on executable directory
    char exe_dir[PATH_MAX];
    if (get_exe_dir(exe_dir, sizeof(exe_dir))) {
        const char *prefixes[] = { "", "/..", "/../.." };
        for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++) {
            char base[PATH_MAX];
            snprintf(base, sizeof(base), "%s%s", exe_dir, prefixes[i]);
            char candidate[PATH_MAX];
            path_join(candidate, sizeof(candidate), base, in);
            if (file_exists(candidate)) {
                snprintf(out, out_size, "%s", candidate);
                return 1;
            }
        }
    }
    out[0] = '\0';
    return 0;
}

int rand_int(int n) {
    int result;
    while (n <= (result = rand() / (RAND_MAX / n)));
    return result;
}

double rand_double() {
    return (double)rand() / (double)RAND_MAX;
}

void update_fps(FPS *fps) {
    fps->frames++;
    double now = glfwGetTime();
    double elapsed = now - fps->since;
    if (elapsed >= 1) {
        fps->fps = round(fps->frames / elapsed);
        fps->frames = 0;
        fps->since = now;
    }
}

char *load_file(const char *path) {
    char resolved[PATH_MAX];
    const char *use_path = path;
    if (resolve_asset_path(path, resolved, sizeof(resolved))) {
        use_path = resolved;
    }
    FILE *file = fopen(use_path, "rb");
    if (!file) {
        char edir[PATH_MAX] = {0};
        get_exe_dir(edir, sizeof(edir));
        fprintf(stderr,
            "fopen failed for '%s' (resolved='%s'): %d %s\n"
            "Hint: run from the repo root, or set CRAFTHEX_ASSET_DIR to the project directory.\n"
            "exe_dir: %s\n",
            path,
            resolved[0] ? resolved : "(unresolved)",
            errno,
            strerror(errno),
            edir[0] ? edir : "(unknown)"
        );
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    rewind(file);
    char *data = calloc(length + 1, sizeof(char));
    fread(data, 1, length, file);
    fclose(file);
    return data;
}

GLuint gen_buffer(GLsizei size, GLfloat *data) {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

void del_buffer(GLuint buffer) {
    glDeleteBuffers(1, &buffer);
}

GLfloat *malloc_faces(int components, int faces) {
    return malloc(sizeof(GLfloat) * 6 * components * faces);
}

GLuint gen_faces(int components, int faces, GLfloat *data) {
    GLuint buffer = gen_buffer(
        sizeof(GLfloat) * 6 * components * faces, data);
    free(data);
    return buffer;
}

GLuint make_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = calloc(length, sizeof(GLchar));
        glGetShaderInfoLog(shader, length, NULL, info);
        fprintf(stderr, "glCompileShader failed:\n%s\n", info);
        free(info);
    }
    return shader;
}

GLuint load_shader(GLenum type, const char *path) {
    char *data = load_file(path);
    GLuint result = make_shader(type, data);
    free(data);
    return result;
}

GLuint make_program(GLuint shader1, GLuint shader2) {
    GLuint program = glCreateProgram();
    glAttachShader(program, shader1);
    glAttachShader(program, shader2);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = calloc(length, sizeof(GLchar));
        glGetProgramInfoLog(program, length, NULL, info);
        fprintf(stderr, "glLinkProgram failed: %s\n", info);
        free(info);
    }
    glDetachShader(program, shader1);
    glDetachShader(program, shader2);
    glDeleteShader(shader1);
    glDeleteShader(shader2);
    return program;
}

GLuint load_program(const char *path1, const char *path2) {
    GLuint shader1 = load_shader(GL_VERTEX_SHADER, path1);
    GLuint shader2 = load_shader(GL_FRAGMENT_SHADER, path2);
    GLuint program = make_program(shader1, shader2);
    return program;
}

void flip_image_vertical(
    unsigned char *data, unsigned int width, unsigned int height)
{
    unsigned int size = width * height * 4;
    unsigned int stride = sizeof(char) * width * 4;
    unsigned char *new_data = malloc(sizeof(unsigned char) * size);
    for (unsigned int i = 0; i < height; i++) {
        unsigned int j = height - i - 1;
        memcpy(new_data + j * stride, data + i * stride, stride);
    }
    memcpy(data, new_data, size);
    free(new_data);
}

void load_png_texture(const char *file_name) {
    unsigned int error;
    unsigned char *data = NULL;
    unsigned int width = 0, height = 0;

    char resolved[PATH_MAX];
    const char *use_path = file_name;
    if (resolve_asset_path(file_name, resolved, sizeof(resolved))) {
        use_path = resolved;
    }
    error = lodepng_decode32_file(&data, &width, &height, use_path);
    if (error) {
        // Don't hard-exit on textures. Use a visible fallback so the app can
        // still run even if assets aren't found (common when running from build/).
        fprintf(stderr,
            "load_png_texture failed for '%s' (resolved='%s'), error %u: %s\n"
            "Hint: run from the repo root, or set CRAFTHEX_ASSET_DIR to the project directory.\n",
            file_name,
            resolved[0] ? resolved : "(unresolved)",
            error,
            lodepng_error_text(error));

        // 2x2 magenta/black checker (RGBA)
        unsigned char fallback[16] = {
            255, 0, 255, 255,   0, 0, 0, 255,
            0, 0, 0, 255,       255, 0, 255, 255
        };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, fallback);
        return;
    }
    flip_image_vertical(data, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, data);
    free(data);
}

char *tokenize(char *str, const char *delim, char **key) {
    char *result;
    if (str == NULL) {
        str = *key;
    }
    str += strspn(str, delim);
    if (*str == '\0') {
        return NULL;
    }
    result = str;
    str += strcspn(str, delim);
    if (*str) {
        *str++ = '\0';
    }
    *key = str;
    return result;
}

int char_width(char input) {
    static const int lookup[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        4, 2, 4, 7, 6, 9, 7, 2, 3, 3, 4, 6, 3, 5, 2, 7,
        6, 3, 6, 6, 6, 6, 6, 6, 6, 6, 2, 3, 5, 6, 5, 7,
        8, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 5, 8, 8, 6,
        6, 7, 6, 6, 6, 6, 8,10, 8, 6, 6, 3, 6, 3, 6, 6,
        4, 7, 6, 6, 6, 6, 5, 6, 6, 2, 5, 5, 2, 9, 6, 6,
        6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 6, 4, 2, 5, 7, 0
    };
    return lookup[input];
}

int string_width(const char *input) {
    int result = 0;
    int length = strlen(input);
    for (int i = 0; i < length; i++) {
        result += char_width(input[i]);
    }
    return result;
}

int wrap(const char *input, int max_width, char *output, int max_length) {
    *output = '\0';
    char *text = malloc(sizeof(char) * (strlen(input) + 1));
    strcpy(text, input);
    int space_width = char_width(' ');
    int line_number = 0;
    char *key1, *key2;
    char *line = tokenize(text, "\r\n", &key1);
    while (line) {
        int line_width = 0;
        char *token = tokenize(line, " ", &key2);
        while (token) {
            int token_width = string_width(token);
            if (line_width) {
                if (line_width + token_width > max_width) {
                    line_width = 0;
                    line_number++;
                    strncat(output, "\n", max_length - strlen(output) - 1);
                }
                else {
                    strncat(output, " ", max_length - strlen(output) - 1);
                }
            }
            strncat(output, token, max_length - strlen(output) - 1);
            line_width += token_width + space_width;
            token = tokenize(NULL, " ", &key2);
        }
        line_number++;
        strncat(output, "\n", max_length - strlen(output) - 1);
        line = tokenize(NULL, "\r\n", &key1);
    }
    free(text);
    return line_number;
}
