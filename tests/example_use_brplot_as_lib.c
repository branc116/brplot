#include "include/brplot.h"
#include <unistd.h>

void example_v1(void) {
  br_plotter_t* plotter = br_plotter_new(br_plotter_default_ctor());
  br_plot_id plot = br_plot_new(plotter, br_plot_default_ctor());
  br_data_id data = br_data_new(plotter, br_data_default_ctor());
  br_plot_show_data(plotter, plot, data);
  br_data_add_v1(plotter, data, 10.f);
  br_data_add_v1n(plotter, data, (float[]){1.f, 2.f, 3.f}, 3);
  br_data_add_v1ns(plotter, data, (float[]){1.f, 2.f, 3.f, 4.f}, 4, 2, 1);
  // data point should be 10, 1, 2, 3, 2, 4
}

void example_v2(void) {
  br_plotter_t* plotter = br_plotter_new(br_plotter_default_ctor());
  br_plot_id plot = br_plot_new(plotter, br_plot_default_ctor());
  br_data_id data = br_data_new(plotter, br_data_default_ctor());
  br_plot_show_data(plotter, plot, data);
  br_data_add_v2n(plotter, data, (float[]){1.f, 2.f, 3.f}, 3);
  br_plotter_wait(plotter);
}

void example_simple(void) {
  br_data_id d = br_simp_plot_v1n(-1, (float[]){1.f, 2.f, 3.f}, 3);
  while (1) {
    for (int i = 0; i < 300; ++i) {
      br_simp_plot_v1n(d, (float[]){4.f, 5.f, 6.f}, 3);
    }
    sleep(1);
  }
  br_simp_wait();
}

int main(void) {
  example_v1();
  example_simple();
  return 0;
}

// gcc -I . -L bin -o example ./tests/example_use_brplot_as_lib.c -lbrplot
// gcc -I . -L bin -o example.o -c ./tests/example_use_brplot_as_lib.c && g++ -L bin example.o -lbrplot
// make BACKEND=X11 USE_CXX=NO TYPE=LIB GUI=RAYLIB CONFIG=RELEASE && gcc -ggdb -I . -L bin ./tests/example_use_brplot_as_lib.c -lbrplot_raylib_linux_debug_gcc && LD_LIBRARY_PATH="$PWD/bin" ./a.out
// LD_LIBRARY_PATH="$PWD/bin" gdb ./a.out
