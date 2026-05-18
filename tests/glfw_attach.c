#include <GL/gl.h>
#define BRPLOT_IMPLEMENTATION
#include "include/brplot.h"

#include <GLFW/glfw3.h>
#include <GL/gl.h>

const char* glErrorToString(GLenum error) {
  switch (error) {
    case GL_NO_ERROR: return "No error has been recorded. The value of this symbolic constant is guaranteed to be 0.";
    case GL_INVALID_ENUM: return "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";
    case GL_INVALID_VALUE: return "A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";
    case GL_INVALID_OPERATION: return "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag.";
    case GL_OUT_OF_MEMORY: return "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
    case GL_STACK_UNDERFLOW: return "An attempt has been made to perform an operation that would cause an internal stack to underflow.";
    case GL_STACK_OVERFLOW: return "An attempt has been made to perform an operation that would cause an internal stack to overflow.";
    default: return "Unknown error";
  }
}

void checkGlError_(const char* file_name, int line) {
  return;
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) printf("[%s:%d]Error = %d (%s)\n", file_name, line, error, glErrorToString(error));
}
#define checkGlError(...) __VA_ARGS__; checkGlError_(__FILE__, __LINE__)

int main(void) {
  GLFWwindow* window = NULL;
  br_plotter_t* plotter = NULL;
  int width = 800, height = 600;

  // Initialize the glfw context...
  glfwInit();
  window = glfwCreateWindow(width, height, "Brplot attach to the window", NULL, NULL);
  glfwMakeContextCurrent(window);
  checkGlError(glDisable(GL_DEPTH_TEST));
  checkGlError(glDisable(GL_CULL_FACE));
  checkGlError(glEnable(GL_SCISSOR_TEST));
  checkGlError(glViewport(0, 0, 800, 600));


  // Initialize the brplot
  plotter = br_plotter_malloc();
  plotter->uiw.pl.kind = brpl_window_glfw_attach;
  plotter->uiw.pl.viewport = BR_EXTENTI(10, 0, 400, 300);
  br_plotter_init(plotter);

  int diffx = 1, diffy = 1, diff_sizex = 1, diff_sizey = 1;
  while (glfwWindowShouldClose(window) == false) {
    glfwPollEvents();
    glfwGetWindowSize(window, &width, &height);

    // Draw the main scene
    checkGlError(glScissor(0, 0, width, height));
    checkGlError(glViewport(0, 0, width, height));
    checkGlError(glClearColor(0.5f, 0, 0, 0.1f));
    checkGlError(glClear(GL_COLOR_BUFFER_BIT));

    int rect_size = width/10;
    checkGlError(glScissor(rect_size,rect_size,rect_size,rect_size));
    checkGlError(glViewport(rect_size,rect_size,rect_size,rect_size));
    checkGlError(glClearColor(0.1f, 0, 0.5f, 0.2f));
    checkGlError(glClear(GL_COLOR_BUFFER_BIT));


    checkGlError(glColor3f(0.0f, 0.5f, 0.0f));
    checkGlError(glBegin(GL_TRIANGLES));
      checkGlError(glVertex4f( 0.00f,  0.75f, 0.f, 1.f));
      checkGlError(glVertex4f(-0.75f, -0.75f, 0.f, 1.f));
      checkGlError(glVertex4f( 0.75f, -0.75f, 0.f, 1.f));
    checkGlError(glEnd());

    // Draw the brplot
    br_plotter_one_iter(plotter);
    br_data_push_xy(&plotter->groups, plotter->uiw.pl.viewport.x, plotter->uiw.pl.viewport.y, 0);
    plotter->uiw.pl.viewport.x      += diffx;
    plotter->uiw.pl.viewport.y      += diffy;
    plotter->uiw.pl.viewport.width  += diff_sizex;
    plotter->uiw.pl.viewport.height += diff_sizey;
    if (plotter->uiw.pl.viewport.x + plotter->uiw.pl.viewport.width > width) diffx = -1;
    if (plotter->uiw.pl.viewport.x < 0) diffx = 1;
    if (plotter->uiw.pl.viewport.y + plotter->uiw.pl.viewport.height > height) diffy = -1;
    if (plotter->uiw.pl.viewport.y < 0) diffy = 1;
    if (plotter->uiw.pl.viewport.x + plotter->uiw.pl.viewport.width > width) diff_sizex = -1;
    if (plotter->uiw.pl.viewport.y + plotter->uiw.pl.viewport.height > height) diff_sizey = -1;
    if (plotter->uiw.pl.viewport.width  < 200) diff_sizex = 1;
    if (plotter->uiw.pl.viewport.height < 200) diff_sizey = 1;

    glfwSwapBuffers(window);
  }
}
// tcc -I. -o bin/glfw_attach tests/glfw_attach.c -lglfw -lGL
