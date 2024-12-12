#include <stdio.h>
#include <errno.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb_rect_pack.h"

// Created icons using Pixelorama
int max_size(int n, int* arr) {
  if (n == 0) return 0;
  int m = 0;
  for (int i = 0; i < n; ++i) {
    if (arr[i] > m) m = arr[i];
  }
  return m;
}

int log2i(int n) {
  if (n <= 0) return 0;

  int res = 0;
  while (n > 0) n >>= 1, res++;
  return res;
}


void sizes_to_rects(int n, const int* sizes, stbrp_rect* rects) {
  for (int i = 0; i < n; ++i) {
    rects[i] = (stbrp_rect) { .h = sizes[i], .w = sizes[i] };
  }
}

int pack(int n, int* sizes, stbrp_rect** out_rects) {
  int sz = max_size(n, sizes);
  int log_sz = log2i(sz);
  int cur = 1 << log_sz;

  stbrp_context rc = { 0 };
  stbrp_node* nodes = calloc(n, sizeof(stbrp_node));
  stbrp_rect* rects = calloc(n, sizeof(stbrp_rect));

  stbrp_init_target(&rc, cur, cur, nodes, n);
  sizes_to_rects(n, sizes, rects);
  int packed = stbrp_pack_rects(&rc, rects, n);
  while (packed == 0) {
    cur <<= 1;
    rc = (stbrp_context){ 0 };
    sizes_to_rects(n, sizes, rects);
    stbrp_init_target(&rc, cur, cur, nodes, n);
    packed = stbrp_pack_rects(&rc, rects, n);
  }
  *out_rects = rects;
  free(nodes);
  return cur;
}

#define STATIC_ARRAY_INT(...) .arr = (int[]){ __VA_ARGS__ }, .len = sizeof((char[]) { __VA_ARGS__ })
#define STATIC_ARRAY_SIZE(arr) (sizeof((arr))/sizeof((arr)[0]))


typedef struct {
  const char* img;
  const int* arr;
  size_t len;
}image_t;

image_t imgs[] = {
  {
    .img = "menu",
    STATIC_ARRAY_INT(16, 32)
  },
//  {
//    .img = "close",
//    STATIC_ARRAY_INT(16, 32)
//  }
};

int* image_sizes(int n, image_t* imgs, int* out_len) {
  int out_cap = 16;
  int* out_sizes = malloc(out_cap * sizeof(int));
  *out_len = 0;
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < imgs[i].len; ++j) {
      out_sizes[(*out_len)++] = imgs[i].arr[j];
    }
  }
  return out_sizes;
}

typedef struct {
  int x, y, w, h;
  char* name;


  unsigned char* data;
} output_struct;

void print_img(unsigned char* img, int w, int h, int c) {
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; ++j) {
      printf("%c", img[(c - 1) + j * c + i * (w*c)] > 254 ? '#' : '.');
    }
    printf("\n");
  }
}

void write_to_atlas(unsigned char* atlas, int as, stbrp_rect rect, unsigned char* img) {
  for (int i = 0; i < rect.h; ++i) {
    for (int j = 0; j < rect.w; ++j) {
      atlas[rect.x + j + (rect.y + i) * as] = img[j + rect.w * i];
    }
  }
}

int main(void) {
  stbrp_rect* rects = NULL;
  int len = 0;
  int* sizes = image_sizes(STATIC_ARRAY_SIZE(imgs), imgs, &len);
  int sz = pack(len, sizes, &rects);
  unsigned char* atlas = calloc(sz*sz, 1);
  int rect_index = 0;
  for (int i = 0; i < STATIC_ARRAY_SIZE(imgs); ++i) {
    char path[4096] = { 0 };
    for (int j = 0; j < imgs[i].len; ++j) {
      sprintf(path, "content/%s_%d.png", imgs[i].img, imgs[i].arr[j]);
      FILE* file = fopen(path, "rb");
      if (file == 0) {
        fprintf(stderr, "Failed to open `%s`: %s\n", path, strerror(errno));
        return 1;
      }
      int x, y, c;
      stbi_uc* d = stbi_load_from_file(file, &x, &y, &c, 1);
      if (c != 1) {
        fprintf(stderr, "ERROR: Expeceted png to have only one channle but %s has %d. Consider using tools/extract_alpha.c tool.\n", path, c);
        return 1;
      }
      print_img(d, x, y, c);
      write_to_atlas(atlas, sz, rects[rect_index], d);
      ++rect_index;
      free(d);
    }
  }
  stbi_write_png("atlas.png", sz, sz, 1, atlas, sz);
  free(sizes);
  free(atlas);
  free(rects);
  return 0;
}
// gcc -I. -o bin/pack_icons tools/pack_icons.c -lm
