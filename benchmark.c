#include "stdio.h"
#include "plotter.h"
#include <stdlib.h>
#include "quick_benchmark.h"

int quad_tree_count_nodes(quad_tree_root_t* r);
float quad_tree_average_goodness(quad_tree_root_t* r);

void gen_rand_points(Vector2* data, int len) {
  for (int i = 0 ; i < len; ++i) {
    float x = (float)rand() / (float)RAND_MAX;
    float y = (float)rand() / (float)RAND_MAX;
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
  quad_tree_free(root);
}

void insert_data_reversed(Vector2* data, int data_len) {
  quad_tree_root_t* root = quad_tree_malloc();
  for (int i = 0 ; i < data_len; ++i) {
    quad_tree_add_point(root, data, data[data_len - i - 1], data_len - i - 1);
  }
  printf("nodes = %d ", quad_tree_count_nodes(root));
  printf("goodnes = %.4f ", quad_tree_average_goodness(root));
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
  quad_tree_free(root);
}

int main(void) {
//  FILE* fd = fopen("quad_bench.csv", "rw+");
//  srand(42069);
#define len 1025*8*2
  Vector2 *data = malloc(sizeof(Vector2)*len);
  gen_rand_points(data, len);
  QB_BENCH_BEGIN(stdout, 5, 2);
      QB_BENCH_ADD(insert_data_ordered, data, len/16);
      QB_BENCH_ADD(insert_data_ordered, data, len/8);
      QB_BENCH_ADD(insert_data_ordered, data, len/4);
      QB_BENCH_ADD(insert_data_ordered, data, len/2);
      QB_BENCH_ADD(insert_data_ordered, data, len);
      QB_BENCH_ADD(insert_data_reversed, data, len/16);
      QB_BENCH_ADD(insert_data_reversed, data, len/8);
      QB_BENCH_ADD(insert_data_reversed, data, len/4);
      QB_BENCH_ADD(insert_data_reversed, data, len/2);
      QB_BENCH_ADD(insert_data_reversed, data, len);
      QB_BENCH_ADD(insert_data_out_in, data, len/16);
      QB_BENCH_ADD(insert_data_out_in, data, len/8);
      QB_BENCH_ADD(insert_data_out_in, data, len/4);
      QB_BENCH_ADD(insert_data_out_in, data, len/2);
      QB_BENCH_ADD(insert_data_out_in, data, len);
  QB_BENCH_END();
  return 0;
}

