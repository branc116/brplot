#include "src/br_pp.h"
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"
#include "src/br_da.h"

#include <stdio.h>

#define FUNCTIONS \
"void glActiveTexture(GLenum texture)" \
"void glAttachShader(GLuint program, GLuint shader)" \
"void glBindBuffer(GLenum target, GLuint buffer)" \
"void glBindFramebuffer(GLenum target, GLuint framebuffer)" \
"void glBindRenderbuffer(GLenum target, GLuint renderbuffer)" \
"void glBindTexture(GLenum target, GLuint texture)" \
"void glBindVertexArray(GLuint array)" \
"void glBlendEquation(GLenum mode)" \
"void glBlendFunc(GLenum sfactor, GLenum dfactor)" \
"void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)" \
"void glBufferData(GLenum target, GLsizeiptr size, void const* data, GLenum usage)" \
"void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void const* data)" \
"void glClear(GLbitfield en)" \
"void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)" \
"void glCompileShader(GLuint shader)" \
"GLuint glCreateProgram(void)" \
"GLuint glCreateShader(GLenum type)" \
"void glDebugMessageCallback(glDebugProc callback, const void* userParam)" \
"void glDeleteBuffers(GLsizei n, GLuint const* buffers)" \
"void glDeleteFramebuffers(GLsizei n, GLuint* framebuffers)" \
"void glDeleteProgram(GLuint program)" \
"void glDeleteRenderbuffers(GLsizei n, GLuint* renderbuffers)" \
"void glDeleteShader(GLuint shader)" \
"void glDeleteTextures(GLsizei n, const GLuint* textures)" \
"void glDeleteVertexArrays(GLsizei n, GLuint const* arrays)" \
"void glDepthFunc(GLenum func)" \
"void glDisable(GLenum cap)" \
"void glDrawArrays(GLenum mode, GLint first, GLsizei count)" \
"void glDrawBuffers(GLsizei n, GLenum const* bufs)" \
"void glEnable(GLenum cap)" \
"void glEnableVertexAttribArray(GLuint index)" \
"void glFramebufferParameteri(GLenum target, GLenum pname, GLint param)" \
"void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)" \
"void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)" \
"void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)" \
"void glGenBuffers(GLsizei n, GLuint* buffers)" \
"void glGenFramebuffers(GLsizei n, GLuint* framebuffers)" \
"void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)" \
"void glGenTextures(GLsizei n, GLuint* textures)" \
"void glGenVertexArrays(GLsizei n, GLuint* arrays)" \
"GLint glGetAttribLocation(GLuint program, GLchar const* name)" \
"GLenum glGetError(void)" \
"void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)" \
"void glGetProgramiv(GLuint program, GLenum pname, GLint* params)" \
"void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)" \
"void glGetShaderiv(GLuint shader, GLenum pname, GLint* params)" \
"GLint glGetUniformLocation(GLuint program, GLchar const* name)" \
"void glLinkProgram(GLuint program)" \
"void glPixelStorei(GLenum pname, GLint param)" \
"void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)" \
"void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)" \
"void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)" \
"void glTexParameteri(GLenum target, GLenum pname, GLint param)" \
"void glTexParameteriv(GLenum target, GLenum pname, const GLint* params)" \
"void glUniform1fv(GLint location, GLsizei count, GLfloat const* value)" \
"void glUniform1i(GLint location, GLint v0)" \
"void glUniform2fv(GLint location, GLsizei count, GLfloat const* value)" \
"void glUniform3fv(GLint location, GLsizei count, GLfloat const* value)" \
"void glUniform4fv(GLint location, GLsizei count, GLfloat const* value)" \
"void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, GLfloat const* value)" \
"void glUseProgram(GLuint program)" \
"void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)" \
"void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)" \

typedef struct {
  br_strv_t type;
  br_strv_t name;
} fparam_t;

typedef struct {
  br_strv_t ret_type;
  br_strv_t name;
  struct { fparam_t* arr; size_t len, cap; } params;
} func_t;

typedef enum {
  s_ftype,
  s_fname,
  s_param
} s_states;

#define IS_ANUM(C) (((C) >= 'a' && (C) <= 'z') || ((C) >= 'A' && (C) <= 'Z') || ((C) >= '0' && (C) <= '9'))

void extract_param(br_strv_t* segs, int len, fparam_t* param) {
  param->type.str = segs[0].str;
  param->type.len = len > 2 ? segs[len - 2].str - segs[0].str + segs[len - 2].len : segs[0].len;
  param->name.str = segs[len - 1].str;
  param->name.len = segs[len - 1].len;
}

static void print_declarations(FILE* file, func_t* funcs, size_t len, const char* postfix);

static void print_headless_impl(FILE* file, func_t* funcs, size_t len) {
  for (uint32_t i = 0; i < len; ++i) {
    func_t f = funcs[i];
    fprintf(file, "%.*s %.*s(", f.ret_type.len, f.ret_type.str, f.name.len, f.name.str);
    if (f.params.len == 0) {
      fprintf(file, "void");
    } else {
      for (uint32_t j = 0; j < f.params.len; ++j) {
        fparam_t param = f.params.arr[j];
        fprintf(file, "%.*s %.*s", param.type.len, param.type.str, param.name.len, param.name.str);
        if (j + 1 < f.params.len) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, ") {\n");
    bool is_void = br_strv_eq(f.ret_type, br_strv_from_literal("void"));
    for (uint32_t j = 0; j < f.params.len; ++j) {
      fparam_t param = f.params.arr[j];
      fprintf(file, "(void)%.*s;\n", param.name.len, param.name.str);
    }
    if (is_void == false) {
      fprintf(file, "return (%.*s) { 0 };\n", f.ret_type.len, f.ret_type.str);
    }
    fprintf(file, "}\n");
  }
  fprintf(file, "void brgl_load(void) {}\n");
}

static void print_tracy_impl(FILE* file, func_t* funcs, size_t len) {
  print_declarations(file, funcs, len, "internal");
  fprintf(file, "\n\n");
  for (uint32_t i = 0; i < len; ++i) {
    func_t f = funcs[i];
    fprintf(file, "%.*s %.*s(", f.ret_type.len, f.ret_type.str, f.name.len, f.name.str);
    if (f.params.len == 0) {
      fprintf(file, "void");
    } else {
      for (uint32_t j = 0; j < f.params.len; ++j) {
        fparam_t param = f.params.arr[j];
        fprintf(file, "%.*s %.*s", param.type.len, param.type.str, param.name.len, param.name.str);
        if (j + 1 < f.params.len) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, ") {\n");
    fprintf(file, "  TracyCZone(%.*s, true);\n", f.name.len, f.name.str);
    bool is_void = br_strv_eq(f.ret_type, br_strv_from_literal("void"));
    if (is_void == true) {
      fprintf(file, "  %.*s_internal(", f.name.len, f.name.str);
    } else {
      fprintf(file, "  %.*s ret = %.*s_internal(", f.ret_type.len, f.ret_type.str, f.name.len, f.name.str);
    }
    if (f.params.len == 0) {
    } else {
      for (int j = 0; j < f.params.len; ++j) {
        fparam_t param = f.params.arr[j];
        fprintf(file, "%.*s", param.name.len, param.name.str);
        if (j + 1 < f.params.len) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, ");\n");
    fprintf(file, "  TracyCZoneEnd(%.*s);\n", f.name.len, f.name.str);
    fprintf(file, "  BR_LOG_GL_ERROR(glGetError_internal());\n");
    if (is_void == false) {
      fprintf(file, "  return ret;\n");
    }
    fprintf(file, "}\n\n");
  }
}

static void print_wasm_declarations(FILE* file, func_t* funcs, size_t len) {
  fprintf(file, "\n// Declarations wasm\n\n");
  for (int i = 0; i <  len; ++i) {
    func_t f = funcs[i];
    fprintf(file, "%.*s %.*s(", f.ret_type.len, f.ret_type.str, f.name.len, f.name.str);

    if (f.params.len == 0) {
      fprintf(file, "void");
    } else {
      for (int j = 0; j < f.params.len; ++j) {
        fparam_t param = f.params.arr[j];
        fprintf(file, "%.*s %.*s", param.type.len, param.type.str, param.name.len, param.name.str);
        if (j + 1 < f.params.len) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, ");\n");
  }
}

static void print_declarations(FILE* file, func_t* funcs, size_t len, const char* postfix) {
  fprintf(file, "\n// Declarations ( %s )\n\n", postfix == NULL ? "null" : postfix);
  for (int i = 0; i <  len; ++i) {
    func_t f = funcs[i];
    if (postfix == NULL)
      fprintf(file, "static %.*s (*%.*s)(", f.ret_type.len, f.ret_type.str, f.name.len, f.name.str);
    else
      fprintf(file, "static %.*s (*%.*s_%s)(", f.ret_type.len, f.ret_type.str, f.name.len, f.name.str, postfix);

    if (f.params.len == 0) {
      fprintf(file, "void");
    } else {
      for (int j = 0; j < f.params.len; ++j) {
        fparam_t param = f.params.arr[j];
        fprintf(file, "%.*s %.*s", param.type.len, param.type.str, param.name.len, param.name.str);
        if (j + 1 < f.params.len) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, ");\n");
  }
}

void print_loader(FILE* file, func_t* funcs, size_t len, const char* postfix) {
  fprintf(file, "\n// Loader ( %s )\n", postfix == NULL ? "null" : postfix);
  fprintf(file, "void brgl_load(void) {\n");
  for (int i = 0; i <  len; ++i) {
    func_t f = funcs[i];
    if (postfix == NULL)
      fprintf(file, "  %.*s = (%.*s (*)(", f.name.len, f.name.str, f.ret_type.len, f.ret_type.str);
    else
      fprintf(file, "  %.*s_%s = (%.*s (*)(", f.name.len, f.name.str, postfix, f.ret_type.len, f.ret_type.str);
    if (f.params.len == 0) {
      fprintf(file, "void");
    } else {
      for (int j = 0; j < f.params.len; ++j) {
        fparam_t param = f.params.arr[j];
        fprintf(file, "%.*s", param.type.len, param.type.str);
        if (j + 1 < f.params.len) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, "))glfwGetProcAddress(\"%.*s\");\n", f.name.len, f.name.str);
    if (postfix) {
      fprintf(file, "  BR_ASSERT(%.*s_%s);\n", f.name.len, f.name.str, postfix);
    } else {
      fprintf(file, "  BR_ASSERT(%.*s);\n", f.name.len, f.name.str);
    }
  }
  fprintf(file, "}\n\n");
}

int do_gl_gen(void) {
  struct { func_t* arr; size_t len, cap; } funcs = { 0 };
  br_strv_t all_funcs = br_strv_from_c_str(FUNCTIONS);
  br_strv_t cur = { .str = all_funcs.str, 0 };
  func_t cur_f = { 0 };
  int line = 1;
  s_states state = s_ftype;
  int func_len = 0;
  struct { br_strv_t* arr; size_t len, cap; } param_segs = { 0 };

  for (int i = 0; i < all_funcs.len; ++i) {
    ++func_len;
    char c = all_funcs.str[i];
    switch (state) {
      case s_ftype: {
        if (c == ' ') {
          cur_f.ret_type = cur;
          cur.str = &all_funcs.str[i + 1];
          cur.len = 0;
          state = s_fname;
        } else if (IS_ANUM(c)) ++cur.len;
        else LOGF("Unexpected character in function return type: `%c`(%d) in line: %d", c, (int)c, line);
      } break;
      case s_fname: {
        if (all_funcs.str[i] == '(') {
          state = s_param;
          cur_f.name = cur;
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
        } else if (IS_ANUM(c)) ++cur.len;
        else LOGF("Unexpected character in function name: `%c`(%d) in line: %d", c, (int)c, line);
      } break;
      case s_param: {
        if (all_funcs.str[i] == ')') {
          if (cur.len > 0) {
            br_da_push(param_segs, cur);
          }
          if (param_segs.len < 2) {
            if (param_segs.len == 0) {
              LOGF("Unexpected paramter construct. Expected 2 or more tokens ( or keyword void ). Got 0 tokens. On line: %d\n"
                   "  `%.*s`", line, func_len - 1, cur_f.ret_type.str);
            }
            else if (br_strv_eq(param_segs.arr[0], br_strv_from_literal("void")) == false)
              LOGF("Unexpected paramter construct. Expected 2 or more tokens ( or keyword void ). Got a single token `%.*s`: on line: %d\n`%.*s`",
                   param_segs.arr[0].len, param_segs.arr[0].str, line, func_len - 1, cur_f.ret_type.str);
          } else {
            fparam_t param = { 0 };
            extract_param(param_segs.arr, param_segs.len, &param);
            br_da_push(cur_f.params, param);
          }

          state = s_ftype;
          ++line;
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
          br_da_push(funcs, cur_f);
          cur_f = (func_t) { 0 };
          func_len = 0;
          param_segs.len = 0;
        } else if (IS_ANUM(c) || c == '*') {
          ++cur.len;
        } else if (c == ' ') {
          if (cur.len > 0) {
            br_da_push(param_segs, cur);
          }
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
        } else if (c == ',') {
          if (cur.len > 0) {
            br_da_push(param_segs, cur);
          }
          if (param_segs.len < 2) {
            LOGF("Unexpected paramter construct. Expected 2 or more tokens. Got %zu: in line: %d\n"
                  "  `%.*s??%.5s...`", param_segs.len, line, func_len - 1, cur_f.ret_type.str, &all_funcs.str[i + 1]);
          }
          fparam_t param = { 0 };
          extract_param(param_segs.arr, param_segs.len, &param);
          br_da_push(cur_f.params, param);
          param_segs.len = 0;
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
        }
        else LOGF("Unexpected character in function paramter: `%c`(%d) in line: %d\n"
                  "  `%.*s?%c?%.5s...`", c, (int)c, line, func_len - 1, cur_f.ret_type.str, c, &all_funcs.str[i + 1]);
      } break;
    }
  }
  FILE* file = fopen(".generated/gl.c", "wb+");
  fprintf(file, "#pragma once\n");
  fprintf(file, "// Generated using tools/gl_gen.c\n");

  fprintf(file, "#if defined(__GNUC__) || defined(__clang__)\n");
  fprintf(file, "#  pragma GCC diagnostic push\n");
  fprintf(file, "#  pragma GCC diagnostic ignored \"-Wpedantic\"\n");
  fprintf(file, "#endif\n");

  fprintf(file, "#include \"src/br_gl.h\"\n");
  fprintf(file, "#include \"src/br_pp.h\"\n");
  fprintf(file, "\n");
  fprintf(file, "#include <stdbool.h>\n");
  fprintf(file, "\n");
  fprintf(file, "#include \"external/glfw/include/GLFW/glfw3.h\"");
  fprintf(file, "\n");

  fprintf(file, "#if defined(__EMSCRIPTEN__)\n");
  print_wasm_declarations(file, funcs.arr, funcs.len);
  fprintf(file, "void brgl_load(void) {}\n\n");
  fprintf(file, "#elif !defined(TRACY_ENABLE) && !defined(HEADLESS)\n");
  print_declarations(file, funcs.arr, funcs.len, NULL);
  print_loader(file, funcs.arr, funcs.len, NULL);
  fprintf(file, "#elif defined(HEADLESS) \n");
  print_headless_impl(file, funcs.arr, funcs.len);
  fprintf(file, "#else \n");
  print_tracy_impl(file, funcs.arr, funcs.len);
  print_loader(file, funcs.arr, funcs.len, "internal");
  fprintf(file, "#endif // defined(TRACY_ENABLE)\n");

  fprintf(file, "#if defined(__GNUC__) || defined(__clang__)\n");
  fprintf(file, "#  pragma GCC diagnostic pop\n");
  fprintf(file, "#endif\n");

  fclose(file);


  FILE* f = fopen(".generated/gl_cmake_is_retarded.h", "w+"); fclose(f);
  for (int i = 0; i < funcs.len; ++i) {
    br_da_free(funcs.arr[i].params);
  }
  br_da_free(funcs);
  br_da_free(param_segs);

  return 0;
}

#if !defined(BR_GL_GEN_NO_MAIN)
int main(void) {
  do_gl_gen();
}
void br_on_fatal_error(void) {}
#endif

// gcc -fsanitize=address -Wall -Wextra -Wpedantic -ggdb -I. -o bin/gl_gen tools/gl_gen.c src/str.c && ./bin/gl_gen
