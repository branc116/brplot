#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct resampling2_t resampling2_t;
typedef struct br_data_t br_data_t;
typedef struct br_plot_t br_plot_t;


typedef enum {
  resampling_dir_null = 0ul,
  resampling_dir_left = 1ul,
  resampling_dir_right = 2ul,
  resampling_dir_up = 4ul,
  resampling_dir_down = 8ul
} resampling_dir;

resampling2_t* resampling2_malloc(void);
void resampling2_empty(resampling2_t* res);
void resampling2_free(resampling2_t* res);
void resampling2_draw(resampling2_t const* res, br_data_t const* pg, br_plot_t* rdi);
void resampling2_add_point(resampling2_t* res, br_data_t const* pg, uint32_t index);
//void    help_resampling_dir_to_str(char* buff, resampling_dir r);

#ifdef __cplusplus
}
#endif

