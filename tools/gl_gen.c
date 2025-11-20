#include "src/br_pp.h"
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"
#include "src/br_da.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

struct {
  const char* name;
  const char* name_upper;
  const char* load_func;
  const char* functions;
  const char* prefix;
} modules[] = {
  {
    .name = "gl",
    .name_upper = "GL",
	  .load_func = "brpl_load_gl",
    .functions =
      "void glBegin(GLenum mode)"
      "void glColor3f(GLfloat r, GLfloat g, GLfloat b)"
      "void glVertex3f(GLfloat x, GLfloat y, GLfloat z)"
      "void glEnd(void)"
      "void glFlush(void)"
      "void glActiveTexture(GLenum texture)"
      "void glAttachShader(GLuint program, GLuint shader)"
      "void glBindBuffer(GLenum target, GLuint buffer)"
      "void glBindFramebuffer(GLenum target, GLuint framebuffer)"
      "void glBindRenderbuffer(GLenum target, GLuint renderbuffer)"
      "void glBindTexture(GLenum target, GLuint texture)"
      "void glBindVertexArray(GLuint array)"
      "void glBlendEquation(GLenum mode)"
      "void glBlendFunc(GLenum sfactor, GLenum dfactor)"
      "void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)"
      "void glBufferData(GLenum target, GLsizeiptr size, void const* data, GLenum usage)"
      "void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void const* data)"
      "void glClear(GLbitfield en)"
      "void glFinish(void)"
      "void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)"
      "void glCompileShader(GLuint shader)"
      "GLuint glCreateProgram(void)"
      "GLuint glCreateShader(GLenum type)"
      "void glDeleteBuffers(GLsizei n, GLuint const* buffers)"
      "void glDeleteFramebuffers(GLsizei n, GLuint* framebuffers)"
      "void glDeleteProgram(GLuint program)"
      "void glDeleteRenderbuffers(GLsizei n, GLuint* renderbuffers)"
      "void glDeleteShader(GLuint shader)"
      "void glDeleteTextures(GLsizei n, const GLuint* textures)"
      "void glDeleteVertexArrays(GLsizei n, GLuint const* arrays)"
      "void glDepthFunc(GLenum func)"
      "void glDisable(GLenum cap)"
      "void glDrawArrays(GLenum mode, GLint first, GLsizei count)"
      "void glDrawBuffers(GLsizei n, GLenum const* bufs)"
      "void glEnable(GLenum cap)"
      "void glEnableVertexAttribArray(GLuint index)"
      "void glFramebufferParameteri(GLenum target, GLenum pname, GLint param)"
      "void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)"
      "void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)"
      "void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)"
      "void glGenBuffers(GLsizei n, GLuint* buffers)"
      "void glGenFramebuffers(GLsizei n, GLuint* framebuffers)"
      "void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)"
      "void glGenTextures(GLsizei n, GLuint* textures)"
      "void glGenVertexArrays(GLsizei n, GLuint* arrays)"
      "GLint glGetAttribLocation(GLuint program, GLchar const* name)"
      "GLenum glGetError(void)"
      "void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)"
      "void glGetProgramiv(GLuint program, GLenum pname, GLint* params)"
      "void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)"
      "void glGetShaderiv(GLuint shader, GLenum pname, GLint* params)"
      "GLint glGetUniformLocation(GLuint program, GLchar const* name)"
      "void glLinkProgram(GLuint program)"
      "void glPixelStorei(GLenum pname, GLint param)"
      "void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)"
      "void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)"
      "void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)"
      "void glTexParameteri(GLenum target, GLenum pname, GLint param)"
      "void glTexParameteriv(GLenum target, GLenum pname, const GLint* params)"
      "void glUniform1fv(GLint location, GLsizei count, GLfloat const* value)"
      "void glUniform1i(GLint location, GLint v0)"
      "void glUniform2fv(GLint location, GLsizei count, GLfloat const* value)"
      "void glUniform3fv(GLint location, GLsizei count, GLfloat const* value)"
      "void glUniform4fv(GLint location, GLsizei count, GLfloat const* value)"
      "void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, GLfloat const* value)"
      "void glUseProgram(GLuint program)"
      "void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)"
      "void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)"
  },
  {
    .name = "glx",
    .name_upper = "GLX",
    .functions =
      "ccharp_t glXGetClientString(brpl_x11_Display* dpy, int name)"
      "brpl_x11_Bool glXQueryExtension(brpl_x11_Display* dpy, int* errorb, int* event)"
      "brpl_x11_Bool glXQueryVersion(brpl_x11_Display* display, int* major, int* minor)"
      "void glXDestroyContext(brpl_x11_Display* dpy, GLXContext ctx)"
      "ccharp_t glXQueryExtensionsString(brpl_x11_Display* dpy, int screen)"
      "void glXDestroyWindow(brpl_x11_Display* dpy, GLXWindow window)"
      "XVisualInfop glXGetVisualFromFBConfig(brpl_x11_Display* dpy, GLXFBConfig config)"
      "GLXContext glXCreateNewContext(brpl_x11_Display* dpy, GLXFBConfig config, int renderType, void* shareList, brpl_x11_Bool direct)"
      "GLXContext glXCreateContext(brpl_x11_Display* dpy, brpl_x11_XVisualInfo* visual, void* shareList, brpl_x11_Bool direct)"
      "GLXContext glXCreateContextAttribsARB(brpl_x11_Display* dpy, GLXFBConfig config, GLXContext share_context, brpl_x11_Bool direct, const int* attrib_list)"
      "GLXFBConfigs glXGetFBConfigs(brpl_x11_Display* dpy, int screen, int* nitems)"
      "int glXGetFBConfigAttrib(brpl_x11_Display* display, GLXFBConfig config, int attriib, int* value)"
      "void glXSwapBuffers(brpl_x11_Display* display, GLXWindow w)"
      "GLXWindow glXCreateWindow(brpl_x11_Display* display, GLXFBConfig native_config, brpl_x11_Window x11_window_handle, const int* attribList)"
      "GLXWindow glXMakeCurrent(brpl_x11_Display* display, GLXWindow glx_window, void* glx_ctx)"
      "GLXFBConfigs glXChooseFBConfig(brpl_x11_Display* d, int screen, const int* attribList, int* nitems)"
      "?funcptr_t glXGetProcAddressARB(ccharp_t vp)"
      "?funcptr_t glXGetProcAddress(ccharp_t procName)"
  },
  {
    .name = "x11",
    .name_upper = "X11",
    .prefix = "brpl_x11_",
    .functions =
      "brpl_x11_Status XInitThreads(void)"
      "void XrmInitialize(void)"
      "brpl_x11_Displayp XOpenDisplay(const char* name)"
      "int XDefaultScreen(brpl_x11_Display* d)"
      "brpl_x11_Window XRootWindow(brpl_x11_Display* d, int screen)"
      "brpl_x11_Visualp XDefaultVisual(brpl_x11_Display* d, int screen)"
      "int XDefaultDepth(brpl_x11_Display* d, int screen)"
      "int XCloseDisplay(brpl_x11_Display* d)"
      "brpl_x11_XErrorHandler XSetErrorHandler(brpl_x11_XErrorHandler handler)"
      "int XGetErrorText(brpl_x11_Display* d, int code, char* bufer_return, int length)"
      "brpl_x11_Status XSetWMProtocols(brpl_x11_Display* d, brpl_x11_Window w, brpl_x11_Atom* protocols, int count)"
      "brpl_x11_XrmQuark XrmUniqueQuark(void)"
      "brpl_x11_Window XCreateWindow(brpl_x11_Display* display, brpl_x11_Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, brpl_x11_Visual* visual, unsigned long valuemask, brpl_x11_XSetWindowAttributes* attributes)"
      "int XDestroyWindow(brpl_x11_Display* d, brpl_x11_Window w)"
      "brpl_x11_Atom XInternAtom(brpl_x11_Display* display, const char* atom_name, brpl_x11_Bool only_if_exists)"
      "void Xutf8SetWMProperties(brpl_x11_Display* display, brpl_x11_Window w, const char* window_name, const char* icon_name, char** argv, int argc, brpl_x11_XSizeHints* normal_hints, brpl_x11_XWMHints* wm_hints, brpl_x11_XClassHint* class_hints)"
      "int XChangeProperty(brpl_x11_Display* display, brpl_x11_Window w, brpl_x11_Atom property, brpl_x11_Atom type, int format, int mode, const unsigned char* data, int nelements)"
      "int XMapWindow(brpl_x11_Display* display, brpl_x11_Window w)"
      "int XPending(brpl_x11_Display* display)"
      "int XQLength(brpl_x11_Display* display)"
      "int XNextEvent(brpl_x11_Display* display, brpl_x11_XEvent* ev)"
      "int XFlush(brpl_x11_Display* display)"
      "int XFree(void* data)"
      "int XLookupString(brpl_x11_XKeyEvent* event_struct, char* buffer_return, int bytes_buffer, brpl_x11_KeySym* keysym_return, brpl_x11_XComposeStatus* status_in_out)"
      "brpl_x11_XIM XOpenIM(brpl_x11_Display* display, void* rdb, char* res_name, char* res_class)"
      "brpl_x11_XIC XCreateIC(brpl_x11_XIM im, ...)"
      "int Xutf8LookupString(brpl_x11_XIC ic, brpl_x11_XKeyPressedEvent* event, char* buffer_return, int bytes_buffer , brpl_x11_KeySym* keysym_return, brpl_x11_Status* status_return)"
      "int XKeycodeToKeysym(brpl_x11_Display* display, uint32_t keycode, int index)"
      "int XLookupKeysym(brpl_x11_XKeyEvent* key_event, int index)"
      "int XQueryExtension(brpl_x11_Display* d, const char* name, int* major, int* minor, int* error)"
      "brpl_x11_Bool XGetEventData(brpl_x11_Display* display, brpl_x11_XGenericEventCookie* cookie)"
      "void XFreeEventData(brpl_x11_Display* display, brpl_x11_XGenericEventCookie* cookie)"
  }, {
    .name = "xi",
    .name_upper = "XI",
    .functions =
      "int XISelectEvents(brpl_x11_Display* d, brpl_x11_Window win, brpl_x11_XIEventMask* masks, int num_masks)"
  }, {
    .name = "glfw",
    .name_upper = "GLFW",
    .functions =
      "int glfwInit(void)"
      "void glfwInitHint(int hint, int value)"
      "int glfwDefaultWindowHints(void)"
      "void glfwSwapBuffers(GLFWwindowp win)"
      "GLFWwindowp glfwCreateWindow(int width, int height, const char* title, void* bs1, void* bs2)"
      "GLFWerrorfun glfwSetErrorCallback(GLFWerrorfun callback)"
      "void glfwMakeContextCurrent(GLFWwindowp window)"
      "void glfwSetWindowSize(GLFWwindowp glfw, int width, int height)"
      "void glfwPollEvents(void)"
      "GLFWwindowsizefun glfwSetWindowSizeCallback(GLFWwindow* window, GLFWwindowsizefun callback)"
      "GLFWwindowclosefun glfwSetWindowCloseCallback(GLFWwindow* window, GLFWwindowclosefun callback)"
      "GLFWwindowfocusfun glfwSetWindowFocusCallback(GLFWwindow* window, GLFWwindowfocusfun callback)"
      "GLFWwindowcontentscalefun glfwSetWindowContentScaleCallback(GLFWwindow* window, GLFWwindowcontentscalefun callback)"
      "GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow* window, GLFWmousebuttonfun callback)"
      "GLFWcursorposfun glfwSetCursorPosCallback(GLFWwindow* window, GLFWcursorposfun callback)"
      "GLFWscrollfun glfwSetScrollCallback(GLFWwindow* window, GLFWscrollfun callback)"
      "GLFWkeyfun glfwSetKeyCallback(GLFWwindow* window, GLFWkeyfun callback)"
      "GLFWcharfun glfwSetCharCallback(GLFWwindow* window, GLFWcharfun callback)"
      "voidp glfwGetWindowUserPointer(GLFWwindow* window)"
      "void glfwSetWindowUserPointer(GLFWwindow* window, void* pointer)"
  }, {
    .name = "wgl",
    .name_upper = "WGL",
    .prefix = "br_",
    .functions =
      "HGLRC wglCreateContext(HDC hdc)"
      "void wglMakeCurrent(HDC hdc, HGLRC hglrc)"
      "voidp wglGetProcAddress(const char* str)"
  }, {
    .name = "gdi",
    .name_upper = "GDI",
    .prefix = "br_",
    .functions =
      "int ChoosePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR* pfd)"
      "int SetPixelFormat(HDC hdc, int format, PIXELFORMATDESCRIPTOR* pfd)"
      "void SwapBuffers(HDC hdc)"
  }
};

bool g_gen_tracy = true;

typedef struct {
  br_strv_t type;
  br_strv_t name;
} fparam_t;

typedef struct {
  br_strv_t ret_type;
  br_strv_t name;
  struct { fparam_t* arr; size_t len, cap; } params;
  bool is_optional;
} func_t;

typedef enum {
  s_ftype,
  s_fname,
  s_param
} s_states;

#define IS_ANUM(C) (((C) >= 'a' && (C) <= 'z') || ((C) >= 'A' && (C) <= 'Z') || ((C) >= '0' && (C) <= '9') || ((C) == '_'))

void extract_param(br_strv_t* segs, int len, fparam_t* param) {
  param->type.str = segs[0].str;
  param->type.len = len > 2 ? segs[len - 2].str - segs[0].str + segs[len - 2].len : segs[0].len;
  param->name.str = segs[len - 1].str;
  param->name.len = segs[len - 1].len;
}

static void print_declarations(FILE* file, func_t* funcs, size_t len, const char* prefix, const char* postfix, const char* modifier);

static void print_headless_impl(FILE* file, const char* module_name, func_t* funcs, size_t len, const char* prefix) {
  if (NULL == prefix) prefix = "";

  for (uint32_t i = 0; i < len; ++i) {
    func_t f = funcs[i];
    fprintf(file, "%.*s %s%.*s(", f.ret_type.len, f.ret_type.str, prefix, f.name.len, f.name.str);
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
      if (param.type.str[0] == '.') continue;
      fprintf(file, "(void)%.*s;\n", param.name.len, param.name.str);
    }
    if (is_void == false) {
      fprintf(file, "return (%.*s) { 0 };\n", f.ret_type.len, f.ret_type.str);
    }
    fprintf(file, "}\n");
  }
  fprintf(file, "bool br_%s_load(void) { return true; }\n", module_name);
}

static void print_tracy_impl(FILE* file, func_t* funcs, size_t len, const char* prefix) {
  print_declarations(file, funcs, len, prefix, "_internal", "static");
  fprintf(file, "\n\n");
  for (uint32_t i = 0; i < len; ++i) {
    func_t f = funcs[i];
    fprintf(file, "%.*s %s%.*s(", f.ret_type.len, f.ret_type.str, prefix, f.name.len, f.name.str);
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
    if (is_void == false) {
      fprintf(file, "  %.*s ret =", f.ret_type.len, f.ret_type.str);
    }
    fprintf(file, "  %.*s_internal(", f.name.len, f.name.str);
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
    if (is_void == false) {
      fprintf(file, "  return ret;\n");
    }
    fprintf(file, "}\n\n");
  }
}

static void print_static_declarations(FILE* file, func_t* funcs, size_t len, const char* prefix) {
  if (NULL == prefix) prefix = "";
  for (int i = 0; i <  len; ++i) {
    func_t f = funcs[i];

    fprintf(file, "%.*s %s%.*s(", f.ret_type.len, f.ret_type.str, prefix, f.name.len, f.name.str);

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

static void print_declarations(FILE* file, func_t* funcs, size_t len, const char* prefix, const char* postfix, const char* modifier) {
  fprintf(file, "\n// Declarations ( %s )\n\n", postfix == NULL ? "null" : postfix);
  if (NULL == prefix) prefix = "";
  if (NULL == postfix) postfix = "";
  for (int i = 0; i <  len; ++i) {
    func_t f = funcs[i];
    fprintf(file, "%s %.*s (*%s%.*s%s)(", modifier, f.ret_type.len, f.ret_type.str, prefix, f.name.len, f.name.str, postfix);

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

void print_loader(FILE* file, const char* module_name, func_t* funcs, size_t len, const char* load_func, const char* prefix, const char* postfix) {
  if (NULL == load_func) load_func = "brpl_load_symbol";
  if (NULL == prefix) prefix = "";
  if (NULL == postfix) postfix = "";

  fprintf(file, "\n// Loader ( %s )\n", postfix == NULL ? "null" : postfix);
  fprintf(file, "bool br_%s_load(void) {\n", module_name);
  fprintf(file, "  void* module = NULL;\n");
  fprintf(file, "  for (size_t i = 0; module == NULL && i < BR_ARR_LEN(br_%s_library_names); ++i) module = brpl_load_library(br_%s_library_names[i]);\n", module_name, module_name);
  fprintf(file, "  if (module == NULL) return false;\n");
  for (int i = 0; i <  len; ++i) {
    func_t f = funcs[i];
    fprintf(file, "  %s%.*s%s = (%.*s (*)(", prefix, f.name.len, f.name.str, postfix, f.ret_type.len, f.ret_type.str);
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
    fprintf(file, "))%s(module, \"%.*s\");\n", load_func, f.name.len, f.name.str);
    fprintf(file, "  if (NULL == %s%.*s%s) {\n", prefix, f.name.len, f.name.str, postfix);
    fprintf(file, "    LOGW(\"Failed to load %.*s from shared library %s.\");\n", f.name.len, f.name.str, module_name);
    fprintf(file, "  }\n");
  }
  fprintf(file, "  return true;\n");
  fprintf(file, "}\n\n");
}

int do_gl_gen(const char* module, const char* module_upper, const char* load_func, const char* prefix, const char* functions, FILE* file_impl, FILE* file_header) {
  struct { func_t* arr; size_t len, cap; } funcs = { 0 };
  br_strv_t all_funcs = br_strv_from_c_str(functions);
  br_strv_t cur = { .str = all_funcs.str, 0 };
  func_t cur_f = { 0 };
  int line = 1;
  s_states state = s_ftype;
  int func_len = 0;
  bool cur_is_optional = false;
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
        else if (cur.len == 0 && c == '?') { cur_is_optional = true; cur.str += 1; }
        else LOGF("[%s] Unexpected character in function return type: `%c`(%d) in line: %d", module, c, (int)c, line);
      } break;
      case s_fname: {
        if (all_funcs.str[i] == '(') {
          state = s_param;
          cur_f.name = cur;
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
        } else if (IS_ANUM(c)) ++cur.len;
        else LOGF("[%s] Unexpected character in function name: `%c`(%d) in line: %d", module, c, (int)c, line);
      } break;
      case s_param: {
        if (all_funcs.str[i] == ')') {
          if (cur.len > 0) {
            br_da_push(param_segs, cur);
          }
          if (param_segs.len < 2) {
            if (param_segs.len == 0) {
              LOGF("[%s] Unexpected paramter construct. Expected 2 or more tokens ( or keyword void ). Got 0 tokens. On line: %d\n"
                   "  `%.*s`", module, line, func_len - 1, cur_f.ret_type.str);
            }
            else if (br_strv_eq(param_segs.arr[0], br_strv_from_literal("void")) == false)
              LOGF("[%s] Unexpected paramter construct. Expected 2 or more tokens ( or keyword void ). Got a single token `%.*s`: on line: %d\n`%.*s`",
                   module, param_segs.arr[0].len, param_segs.arr[0].str, line, func_len - 1, cur_f.ret_type.str);
          } else {
            fparam_t param = { 0 };
            extract_param(param_segs.arr, param_segs.len, &param);
            br_da_push(cur_f.params, param);
          }

          state = s_ftype;
          ++line;
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
          cur_f.is_optional = cur_is_optional;
          br_da_push(funcs, cur_f);
          cur_f = (func_t) { 0 };
          cur_is_optional = false;
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
            LOGF("[%s] Unexpected paramter construct. Expected 2 or more tokens. Got %zu: in line: %d\n"
                  "  `%.*s??%.5s...`", module, param_segs.len, line, func_len - 1, cur_f.ret_type.str, &all_funcs.str[i + 1]);
          }
          fparam_t param = { 0 };
          extract_param(param_segs.arr, param_segs.len, &param);
          br_da_push(cur_f.params, param);
          param_segs.len = 0;
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
        } else if (c == '.') {
          char c1 = all_funcs.str[i + 1];
          char c2 = all_funcs.str[i + 2];
          if (c1 == '.' && c2 == '.') {
            fparam_t param = {
              .type = BR_STRL("..."),
              .name = BR_STRL("")
            };
            br_da_push(cur_f.params, param);
          }
          i += 3;
          char c3 = all_funcs.str[i + 1];
          BR_ASSERTF(c3 == ')', "Expect end of function declaration after '...', but got %c(%d)", c3, c3);
          state = s_ftype;
          ++line;
          cur.len = 0;
          cur.str = &all_funcs.str[i + 1];
          cur_f.is_optional = cur_is_optional;
          br_da_push(funcs, cur_f);
          cur_f = (func_t) { 0 };
          cur_is_optional = false;
          func_len = 0;
          param_segs.len = 0;
        }
        else LOGF("[%s] Unexpected character in function paramter: `%c`(%d) in line: %d\n"
                  "  `%.*s?%c?%.5s...`", module, c, (int)c, line, func_len - 1, cur_f.ret_type.str, c, &all_funcs.str[i + 1]);
      } break;
    }
  }

  {
    fprintf(file_impl, "#if defined(BR_HAS_%s) && defined(BR_WANTS_%s)\n", module_upper, module_upper);

    fprintf(file_impl, "#if defined(BR_%s_STATIC)\n", module_upper);
    fprintf(file_impl, "bool br_%s_load(void) { return true; }\n\n", module);

    fprintf(file_impl, "#elif defined(HEADLESS)\n");
    print_headless_impl(file_impl, module, funcs.arr, funcs.len, prefix);

    if (g_gen_tracy) {
      fprintf(file_impl, "#elif defined(TRACY_ENABLE)\n");
      print_tracy_impl(file_impl, funcs.arr, funcs.len, prefix);
      print_loader(file_impl, module, funcs.arr, funcs.len, load_func, prefix, "internal");
    }

    fprintf(file_impl, "#else // Normal mode\n");
    print_declarations(file_impl, funcs.arr, funcs.len, prefix, NULL, "");
    print_loader(file_impl, module, funcs.arr, funcs.len, load_func, prefix, NULL);

    fprintf(file_impl, "#endif // STATIC/HEADLESS/TRACY/NORMAL\n");

    fprintf(file_impl, "#endif // defined(BR_HAS_%s) && defined(BR_WANTS_%s)\n", module_upper, module_upper);
  }

  {
    fprintf(file_header, "#if defined(BR_HAS_%s) && defined(BR_WANTS_%s)\n", module_upper, module_upper);
    fprintf(file_header, "bool br_%s_load(void);\n\n", module);

    fprintf(file_header, "#if defined(TRACY_ENABLE) || defined(HEADLESS) || defined(BR_%s_STATIC)\n", module_upper);
    print_static_declarations(file_header, funcs.arr, funcs.len, prefix);

    fprintf(file_header, "#else // NORMAL\n");
    print_declarations(file_header, funcs.arr, funcs.len, prefix, NULL, "extern");

    fprintf(file_header, "#endif\n");

    fprintf(file_header, "#endif // defined(BR_HAS_%s) && defined(BR_WANTS_%s)\n", module_upper, module_upper);
  }

  for (int i = 0; i < funcs.len; ++i) {
    br_da_free(funcs.arr[i].params);
  }
  br_da_free(funcs);
  br_da_free(param_segs);

  return 0;
}

#if !defined(BR_GL_GEN_NO_MAIN)
int main(int argc, char** argv) {
  if (argc == 2) {
    if (0 == strcmp(argv[1], "--no-tracy")) {
      argv += 1;
      argc -= 1;
      g_gen_tracy = false;
      LOGI("Disable tracy");
    } else {
      LOGF("Unknown flag `%s`", argv[0]);
    }
  } else if (argc > 2) {
    LOGF("Unknown flags");
    LOGF("USAGE: %s [--no-tracy]", argv[0]);
  }
  FILE* file_impl = fopen(".generated/gl.c", "wb+");
  if (NULL == file_impl) {
    LOGF("Failed to open output files: %s", strerror(errno));
  }
  FILE* file_header = fopen(".generated/gl.h", "wb+");
  if (NULL == file_header) {
    LOGF("Failed to open output files: %s", strerror(errno));
  }
  fprintf(file_impl, "// Generated using %s\n", __FILE__);
  fprintf(file_impl, "#if defined(__GNUC__) || defined(__clang__)\n");
  fprintf(file_impl, "#  pragma GCC diagnostic push\n");
  fprintf(file_impl, "#  pragma GCC diagnostic ignored \"-Wpedantic\"\n");
  fprintf(file_impl, "#endif\n");

  fprintf(file_header, "// Generated using %s\n", __FILE__);
  fprintf(file_header, "#pragma once\n");

  for (int i = 0; i < BR_ARR_LEN(modules); ++i) {
    do_gl_gen(modules[i].name, modules[i].name_upper, modules[i].load_func, modules[i].prefix, modules[i].functions, file_impl, file_header);
  }

  fprintf(file_impl, "#if defined(__GNUC__) || defined(__clang__)\n");
  fprintf(file_impl, "#  pragma GCC diagnostic pop\n");
  fprintf(file_impl, "#endif\n");
  fclose(file_impl);

  fclose(file_header);
}
void br_on_fatal_error(void) {}
void brgui_push_log_line(const char* fmt, ...) {}
#endif

// gcc -fsanitize=address -Wall -Wextra -Wpedantic -ggdb -I. -o bin/gl_gen tools/gl_gen.c src/str.c && ./bin/gl_gen
