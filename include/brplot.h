/*
 *  brplot - Author: Branimir Ričko
 *    The MIT License (MIT)
 *    Copyright © Branimir Ričko [branc116]
 *
 *    Permission is hereby granted, free of charge, to any person  obtaining a copy of
 *    this software and associated  documentation files (the   “Software”), to deal in
 *    the Software without  restriction,  including without  limitation  the rights to
 *    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *    the Software,  and to permit persons to whom the Software is furnished to do so,
 *    subject to the following conditions:
 *
 *    The above  copyright notice and this  permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED   “AS IS”,   WITHOUT WARRANTY OF ANY KIND,   EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   IN NO EVENT SHALL THE AUTHORS OR
 *    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER
 *    IN  AN  ACTION  OF CONTRACT,   TORT OR OTHERWISE,   ARISING FROM,   OUT OF OR IN
 *    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Brplot - Brplot is a plotting library and application.
 * There is a simple and a more complicated api that you can use.
 *
 * Example of simple API:
 * ```main.c
      #define BRPLOT_IMPLEMENTATION
      #include "brplot.c"

      int main(void) {
         for (int i = -10; i < 10; ++i)           brp_1(y_valuei*i,               0); //          y-value, group_id
         for (float i = -10.f; i < 10.f; i+=0.1f) brp_2(0.1f*sin(i), 0.1f*cos(i), 2); // x-value, y-value, group_id
         brp_wait(); // Wait until plot window is closed
      }
 * ```
 * Compile as: cc main.c
 * Functions that are part of simple api can be found by following a tag *SIMPLEAPI*
 *
 * Example of a bit more compilcate API
 * ``` main.c
       #define BRPLOT_IMPLEMENTATION
       #include "brplot.h"

       int main(void) {
         br_plotter_t* plotter = br_plotter_new(br_plotter_default_ctor());
         br_plot_id plot = br_plot_new(plotter, br_plot_default_ctor());
         br_data_id data = br_data_new(plotter, br_data_default_ctor());
         for (int i = -10; i < 10; ++i) br_data_add_v1(plotter, data, 10.f);
         br_plotter_wait(plotter);
       }
 * ```
 * Compile as: cc main.c
 * Functions that are part of a bit more complicated API can be found by following a tag *ABITMORECOMPILCATED*
 *
 * */
#pragma once
#define BR_VERSION {0, 0, 1}

#if defined(__GNUC__)
#  define BR_EXPORT __attribute__((visibility ("default")))
#else
#  define BR_EXPORT
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// --------------------------------------------- SIMPLEAPI ----------------------------------------------
/*
 * This functions behaves more or less like the more complicated api, with the difference that you don't have to pass br_plotter_t argument.
 * These functions use global instance of br_plotter_t
 * For more  details what each parameter means jump to *PARAMETER_DEFINITIONS*
 * */
typedef int br_data_id;
BR_EXPORT br_data_id brp_1   (float x,                                            br_data_id data_id);
BR_EXPORT br_data_id brp_1n  (float const* points, int n,                         br_data_id data_id);
BR_EXPORT br_data_id brp_1ns (float const* points, int n, int stride, int offset, br_data_id data_id);

BR_EXPORT br_data_id brp_2   (float x, float y,                                                                br_data_id data_id);
BR_EXPORT br_data_id brp_2n  (float const* v,                   int n,                                         br_data_id data_id);
BR_EXPORT br_data_id brp_2ns (float const* v,                   int n, int stride, int offset_x, int offset_y, br_data_id data_id);
BR_EXPORT br_data_id brp_2nd (float const* xs, float const* ys, int n,                                         br_data_id data_id);
BR_EXPORT br_data_id brp_2nds(float const* xs, float const* ys, int n, int stride, int offset_x, int offset_y, br_data_id data_id);

BR_EXPORT br_data_id brp_3   (float x, float y, float z,                                                                                      br_data_id data_id);
BR_EXPORT br_data_id brp_3n  (float const* v,                                    int n,                                                       br_data_id data_id);
BR_EXPORT br_data_id brp_3ns (float const* v,                                    int n, int stride, int offset_x, int offset_y, int offset_z, br_data_id data_id);
BR_EXPORT br_data_id brp_3nd (float const* xs, float const* ys, float const* zs, int n,                                                       br_data_id data_id);
BR_EXPORT br_data_id brp_3nds(float const* xs, float const* ys, float const* zs, int n, int stride, int offset_x, int offset_y, int offset_z, br_data_id data_id);

BR_EXPORT void brp_wait(void);

/*
 * This macro is used to extend structures so that backward compatibility is
 * preserved. So Each _ctor structure is exactly 128 bytes. That should be
 * more than enough for constructing every object that can be constructed from
 * now until eternity.
 **/
#define PADDED_STRUCT(name, internal_name) typedef struct name { \
  int version; \
  union { \
    struct internal_name ctor; \
    unsigned long long internal__padding[128 / 8]; \
  }; \
} name



typedef struct br_plotter_t br_plotter_t;
typedef int br_plot_id;

typedef enum {
  br_plotter_ui_kind_minimal = 0,
} br_plotter_ui_kind_t;
#define BR_PLOTTER_UI_KIND_T

#if !defined(BR_PLOT_KIND_T)
typedef enum {
  br_plot_kind_2d,
  br_plot_kind_3d
} br_plot_kind_t;
#define BR_PLOT_KIND_T
#endif

#if !defined(BR_DATA_KIND_T)
typedef enum {
  br_data_kind_2d,
  br_data_kind_3d
} br_data_kind_t;
#define BR_DATA_KIND_T
#endif

struct br_plotter_ctor_int_t {
  int width, height;
  br_plotter_ui_kind_t kind;
  int use_permastate;
  int use_stdin;
};

struct br_plot_ctor_int_t {
  br_plot_kind_t kind;
  float width; // From 0 to 1
  float height; // From 0 to 1
  int rearange_others;
};

struct br_data_ctor_int_t {
  br_data_kind_t kind;
};

PADDED_STRUCT(br_plotter_ctor_t, br_plotter_ctor_int_t);
PADDED_STRUCT(br_plot_ctor_t, br_plot_ctor_int_t);
PADDED_STRUCT(br_data_ctor_t, br_data_ctor_int_t);


// --------------------------------------------- ABITMORECOMPILCATED ---------------------------------------------

BR_EXPORT br_plotter_ctor_t* br_plotter_default_ctor(void);
BR_EXPORT br_plotter_t* br_plotter_new(br_plotter_ctor_t const* ctor);
BR_EXPORT void br_plotter_wait(br_plotter_t const* plotter);
BR_EXPORT void br_plotter_free(br_plotter_t* plotter);

BR_EXPORT br_plot_ctor_t* br_plot_default_ctor(void);
BR_EXPORT br_plot_id br_plot_new(br_plotter_t* plotter, br_plot_ctor_t const* ctor);
BR_EXPORT void br_plot_free(br_plotter_t* plotter, br_plot_id plot);

BR_EXPORT br_data_ctor_t* br_data_default_ctor(void);
BR_EXPORT br_data_id br_data_new(br_plotter_t* plotter, br_data_ctor_t const* ctor);

// --------------------------------------------- PARAMETER_DEFINITIONS ---------------------------------------------
/*
 * br_data_add_ is a set of functions that serve the same purpose.
 *              add points to data object. Form of all functions in
 *              the is is: br_data_add_v[dimensions][n[d][s]]
 *              dimensions can be: 1, 2, 3 - what is the dimension of the data
 *              n -> Flag - There is N data points.
 *              d -> Flag ( Only if n flag is on ) - Data points are in [D]ifferent addresses.
 *              s -> Flag ( Only if n flag is on ) - Data points have non standard [S]tride and offset.
 * All functions in this set are:
 *   br_data_add_v1, br_data_add_v1n, br_data_add_v1ns
 *   br_data_add_v2, br_data_add_v2n, br_data_add_v2ns, br_data_add_v2nd, br_data_add_v2nds
 *   br_data_add_v3, br_data_add_v3n, br_data_add_v3ns, br_data_add_v3nd, br_data_add_v3nds
 * Each br_data_add_ function returns number of element inserted into data object.
 *
 * Examples:
 * This is array x
 * |------------------------------------------------------------------------------------|
 * |    x1    |    x2    |    x3    |   x4    |   x5    |   x6    |    x7    |    x8    |
 * |------------------------------------------------------------------------------------|
 * n=8
 *
 * br_data_add_v1n: Add each element to the data object.
 *   E.G.:
 *     br_data_add_v1n(data, x, 8) =>  [x1, x2, x3, x4, x5, x6, x7, x8]
 *
 * br_data_add_v1ns: stride and offset is taken into account. E.G.:
 *   If stride=1, offset=0 => same as br_data_add_v1n(data, x, 8)
 *   If stride=2, offset=0 => Data inserted=[x1, x3, x5, x7]
 *   If stride=2, offset=1 => Data inserted=[x2, x4, x6, x8]
 *   If stride=3, offset=0 => Data inserted=[x1, x4] NOTE: Not inserting x7 because it's
 *                                                         not filling the entire stride
 *   If stride=3, offset=2 => Data inserted=[x3, x6]
 * NOTE: offset < stride !!!!
 * NOTE: stride != 0 !!!!
 *
 * br_data_add_v2n: added points will be: [ [x1, x2], [x3, x4], [x5, x6], [x7, x8] ]
 *
 * br_data_add_v2ns: stide and offset are taken into account. E.G.:
 *   stride=2, offset_x=0, offset_y=1 => same as br_data_add_v2n
 *   stride=1, offset_x=0, offset_y=0 => [ [x1, x1], [x2, x2], [x3, x3], [x4, x4], [x5, x5], [x6, x6], [x7, x7], [x8, x8] ]
 *
 *
 * br_data_add_v2nd: For this variand y array needs to be defined. It may be same as array x.
 *     If This is array y:
 *     |------------------------------------------------------------------------------------|
 *     |    y1    |    y2    |    y3    |   y4    |   y5    |   y6    |    y7    |    y8    |
 *     |------------------------------------------------------------------------------------|
 *     E.G.:
 *       br_data_add_v2nd(data, xs=x, ys=y, n=8) =>
 *         [ [x1, y1], [x2, y2], [x3, y3], [x4, y4], [x5, y5], [x6, y6], [x7, y7], [x8, y8] ]
 *
 * br_data_add_v2nds: For this variand y array needs to be defined. It may be same as array x.
 *     br_data_add_v2nds(data, xs=x, ys=x, n=8, stride=2, offset_x=0, offset_y=1) => same as br_data_add_v2n(data, x, 8)
 *     br_data_add_v2nds(data, xs=x, ys=y, n=8, stride=1, offset_x=0, offset_y=0) => same as br_data_add_v2nd(data, xs=x, ys=y, n=8)
 *
 * */
BR_EXPORT int br_data_add_v1   (br_plotter_t* plotter, float x, br_data_id data);
BR_EXPORT int br_data_add_v1n  (br_plotter_t* plotter, float const* x, int n, br_data_id data);
BR_EXPORT int br_data_add_v1ns (br_plotter_t* plotter, float const* x, int n, int stride, int offset, br_data_id data);
BR_EXPORT int br_data_add_v2   (br_plotter_t* plotter, float x, float y, br_data_id data);
BR_EXPORT int br_data_add_v2n  (br_plotter_t* plotter, float const* v, int n, br_data_id data);
BR_EXPORT int br_data_add_v2ns (br_plotter_t* plotter, float const* v, int n, int stride, int offset_x, int offset_y, br_data_id data);
BR_EXPORT int br_data_add_v2nd (br_plotter_t* plotter, float const* xs, float const* ys, int n, br_data_id data);
BR_EXPORT int br_data_add_v2nds(br_plotter_t* plotter, float const* xs, float const* ys, int n, int stride, int offset_x, int offset_y, br_data_id data);
BR_EXPORT int br_data_add_v3   (br_plotter_t* plotter, float x, float y, float z, br_data_id data);
BR_EXPORT int br_data_add_v3n  (br_plotter_t* plotter, float const* v, int n, br_data_id data);
BR_EXPORT int br_data_add_v3ns (br_plotter_t* plotter, float const* v, int n, int stride, int offset_x, int offset_y, int offset_z, br_data_id data);
BR_EXPORT int br_data_add_v3nd (br_plotter_t* plotter, float const* xs, float const* ys, float const* zs, int n, br_data_id data);
BR_EXPORT int br_data_add_v3nds(br_plotter_t* plotter, float const* xs, float const* ys, float const* zs, int n, int stride, int offset_x, int offset_y, int offset_z, br_data_id data);

BR_EXPORT void br_data_free(br_data_id data);

BR_EXPORT void br_plot_show_data(br_plotter_t* plotter, br_plot_id plot, br_data_id data);
BR_EXPORT void br_plot_hide_data(br_plotter_t* plotter, br_plot_id plot, br_data_id data);

#if defined(__cplusplus)
}
#endif

// If you just wanna build brplot as an app you have to define this
#if defined(BRPLOT_IMPLEMENTATION) || defined(BRPLOT_APP)
#  include "src/br_pp.h"
#  include "tools/unity/brplot.c"
#endif
