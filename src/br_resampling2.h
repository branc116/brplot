#pragma once
#include "br_data.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct resampling2_t resampling2_t;
typedef struct br_plot_t br_plot_t;

void br_resampling2_construct(void);
resampling2_t* resampling2_malloc(br_data_kind_t kind);
void resampling2_empty(resampling2_t* res);
void resampling2_free(resampling2_t* res);
void resampling2_draw(resampling2_t* res, br_data_t const* pg, br_plot_t* rdi, br_shaders_t* shaders);
// TODO: index should be size_t...
void resampling2_add_point(resampling2_t* res, br_data_t const* pg, uint32_t index);
void resampling2_reset(resampling2_t* res);
void resampling2_change_something(br_datas_t pg);
// In seconds
double br_resampling2_get_draw_time(resampling2_t* res);
float br_resampling2_get_something(resampling2_t* res);
float br_resampling2_get_something2(resampling2_t* res);

#ifdef __cplusplus
}
#endif

