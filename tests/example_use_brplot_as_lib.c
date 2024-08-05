#include "include/brplot.h"

void example_vebose(void) {
  br_plotter_t* plotter = br_plotter_new(br_plotter_default_ctor());
  br_plot_id plot = br_plot_new(plotter, br_plot_default_ctor());
  br_data_id data = br_data_new(plotter, br_data_default_ctor());
  br_plot_show_data(plotter, plot, data);
  br_data_add_v1n(plotter, data, (float[]){1.f, 2.f, 3.f}, 3);
  br_plotter_wait(plotter);
}

void example_simple(void) {
  //br_data_id data = br_simp_plot_v1n(-1, (float[]){1.f, 2.f, 3.f}, 3);
  //br_simp_plot_v1n(data, (float[]){4.f, 5.f, 6.f}, 3);
  //br_simp_wait();
}

int main(void) {
  example_vebose();
  return 0;
}

// gcc -I . -L bin -o example ./tests/example_use_brplot_as_lib.c -lbrplot
// gcc -I . -L bin -o example.o -c ./tests/example_use_brplot_as_lib.c && g++ -L bin example.o -lbrplot
// make USE_CXX=NO TYPE=LIB GUI=RAYLIB CONFIG=DEBUG && gcc -ggdb -I . -L bin ./tests/example_use_brplot_as_lib.c -lbrplot && LD_LIBRARY_PATH="$PWD/bin" ./a.out
