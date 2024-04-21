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
void resampling2_draw(resampling2_t const* res, br_data_t const* pg, br_plot_t* rdi);
void resampling2_add_point(resampling2_t* res, br_data_t const* pg, uint32_t index);

#ifdef __cplusplus
}
#endif

