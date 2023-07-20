#include "stdio.h"
#include "plotter.h"
#include <math.h>
#include <stdlib.h>
#include "quick_benchmark.h"

int quad_tree_count_nodes(quad_tree_root_t* r);
float quad_tree_average_goodness(quad_tree_root_t* r);
float quad_tree_average_depth(quad_tree_root_t* r);

void gen_rand_points(Vector2* data, int len) {
  for (int i = 0 ; i < len; ++i) {
    float x = (float)rand() / (float)RAND_MAX;
    float y = (float)rand() / (float)RAND_MAX;
    data[i] = (Vector2){x, y};
  }
}

void gen_sin_points(Vector2* data, int len) {
  for (int i = 0 ; i < len; ++i) {
    float x = i;
    float y = sinf(x/100);
    data[i] = (Vector2){x, y};
  }
}

void insert_data_ordered(Vector2* data, int data_len) {
  quad_tree_root_t* root = quad_tree_malloc();
  for (int i = 0 ; i < data_len; ++i) {
    quad_tree_add_point(root, data, data[i], i);
  }
  printf("nodes = %d ", quad_tree_count_nodes(root));
  printf("goodnes = %.4f ", quad_tree_average_goodness(root));
  printf("depth = %.4f ", quad_tree_average_depth(root));
  quad_tree_free(root);
}

void insert_data_reversed(Vector2* data, int data_len) {
  quad_tree_root_t* root = quad_tree_malloc();
  for (int i = 0 ; i < data_len; ++i) {
    quad_tree_add_point(root, data, data[data_len - i - 1], data_len - i - 1);
  }
  printf("nodes = %d ", quad_tree_count_nodes(root));
  printf("goodnes = %.4f ", quad_tree_average_goodness(root));
  printf("depth = %.4f ", quad_tree_average_depth(root));
  quad_tree_free(root);
}

void insert_data_out_in(Vector2* data, int data_len) {
  quad_tree_root_t* root = quad_tree_malloc();
  for (int i = 0 ; i < data_len/2; i++) {
    quad_tree_add_point(root, data, data[data_len - i - 1], data_len - i - 1);
    quad_tree_add_point(root, data, data[i], i);
  }
  printf("nodes = %d ", quad_tree_count_nodes(root));
  printf("goodnes = %.4f ", quad_tree_average_goodness(root));
  printf("depth = %.4f ", quad_tree_average_depth(root));
  quad_tree_free(root);
}
#define len 1025*8*2
void bench1(FILE* data_out) {
  Vector2 *data = malloc(sizeof(Vector2)*len);
  gen_rand_points(data, len);
  Vector2 *data_sin = malloc(sizeof(Vector2)*len);
  gen_sin_points(data_sin, len);
  QB_BENCH_BEGIN(stdout, 1, 1);
      QB_BENCH_ADD(insert_data_ordered, data, len/16);
      QB_BENCH_ADD(insert_data_ordered, data, len/4);
      QB_BENCH_ADD(insert_data_ordered, data, len);
      QB_BENCH_ADD(insert_data_reversed, data, len/16);
      QB_BENCH_ADD(insert_data_reversed, data, len/4);
      QB_BENCH_ADD(insert_data_reversed, data, len);
      QB_BENCH_ADD(insert_data_out_in, data, len/16);
      QB_BENCH_ADD(insert_data_out_in, data, len/4);
      QB_BENCH_ADD(insert_data_out_in, data, len);
      QB_BENCH_ADD(insert_data_ordered, data_sin, len/16);
      QB_BENCH_ADD(insert_data_ordered, data_sin, len/4);
      QB_BENCH_ADD(insert_data_ordered, data_sin, len);
      QB_BENCH_ADD(insert_data_reversed, data_sin, len/16);
      QB_BENCH_ADD(insert_data_reversed, data_sin, len/4);
      QB_BENCH_ADD(insert_data_reversed, data_sin, len);
      QB_BENCH_ADD(insert_data_out_in, data_sin, len/16);
      QB_BENCH_ADD(insert_data_out_in, data_sin, len/4);
      QB_BENCH_ADD(insert_data_out_in, data_sin, len);
  QB_BENCH_END();
  free(data);
  free(data_sin);
}
void fun(quad_tree_root_t* root, Vector2* data, int data_len) {
  for (int i = 0 ; i < data_len; i++) {
    quad_tree_add_point(root, data, data[i], i);
  }
}
void balanc_bench(FILE* data_out) {
  Vector2 *data_sin = malloc(sizeof(Vector2)*len);
  fprintf(data_out, "nodes, #balanc, goodnes, depth, max_bad, max_els, min_els\n");
  QB_BENCH_BEGIN(stdout, 1, 1);
    for (int k = 20; k < 65; k+=5) {
      for (int j = 1; j < 8; ++j) {
        for (float i = 0.55; i <= .7; i+=0.02) { 
          gen_sin_points(data_sin, len);
          quad_tree_root_t* root = quad_tree_malloc();
          root->balanc_enable = true;
          root->balanc_max_baddness = i;
          root->balanc_max_elements = k*QUAD_TREE_SPLIT_COUNT;
          root->balanc_min_elements = j*QUAD_TREE_SPLIT_COUNT;
          root->balanc_count = 0;
          QB_BENCH_ADD(fun, root, data_sin, len);
          fprintf(data_out, "%d,%d,%.4f,%.4f,%.3f,%d,%d\n", quad_tree_count_nodes(root),
           root->balanc_count,
           quad_tree_average_goodness(root),
           quad_tree_average_depth(root),
           root->balanc_max_baddness, root->balanc_max_elements, root->balanc_min_elements);
          quad_tree_free(root);
          fflush(data_out);
        }
      }
    }
  QB_BENCH_END();
  free(data_sin);
}

typedef void (*bench)(FILE*);
bench all_bechs[] = {bench1, balanc_bench};

int main(int argc, char ** argv) {
  FILE* data_out = fopen("bench/data3.csv", "w");
  fprintf(data_out, "SHITFUCK\n");

  if (argc <= 1) {
    for (int i = 0; i < sizeof(all_bechs)/sizeof(bench); ++i) {
      all_bechs[i](data_out);
    }
  } else {
    int i = atoi(argv[1]);
    all_bechs[i](data_out);
  }
  fclose(data_out);
  return 0;
}

