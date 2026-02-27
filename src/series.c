#include "src/br_series.h"
#include "src/br_da.h"
#include "src/br_filesystem.h"
#include "src/br_free_list.h"

static BR_THREAD_LOCAL struct {
  br_serieses_t* s;
} br_serieses;

bool br_series_push(br_series_t* s, double value) {
  bool success = true;

  if (s->len > 0) {
    s->min = fmin(value, s->min);
    s->max = fmax(value, s->max);
    double next_denom = 1.0/(double)(s->len + 1);
    s->average = ((double)(s->len)*next_denom*s->average + value*next_denom);
  } else {
    s->offset = 0;
    s->scale = 1.0;
    s->min = value;
    s->max = value;
    s->average = value;
  }

  if (s->len < BR_SERIES_SUPPORT_CAP) {
    if (!s->support) {
      BR_MALLOCE(s->support, BR_SERIES_SUPPORT_CAP);
      br_da_reserve(*s, 2*BR_SERIES_SUPPORT_CAP);
    }
    s->support[s->len++] = value;
    return true;
  } else if (s->len == BR_SERIES_SUPPORT_CAP) {
    br_da_reserve(*s, 2*BR_SERIES_SUPPORT_CAP);
    double center = (s->min + s->max) / 2.0;
    double iscale = (s->max - s->min);
    if (iscale > 1e-5) iscale = 2.0/iscale;
    else               iscale = 1e5;
    s->len = 0;
    for (size_t i = 0; i < BR_SERIES_SUPPORT_CAP; ++i) {
      br_da_push(*s, (float)((s->support[i] - center)*iscale));
    }
    s->offset = center;
    s->scale = 1/iscale;
  }

  br_da_push(*s, br_series_to_local(*s, value));
error:
  return success;
}

float br_series(br_series_t s, br_u64 index) {
  BR_ASSERTF(index < s.len, "index = %llu, s.len = %llu", index, s.len);
  if (s.len <= BR_SERIES_SUPPORT_CAP) return (float)s.support[index];
  else return (br_da_get(s, index));
}

br_series_view_t br_series_view(br_series_t s, br_u64 index, br_u64 len) {
  BR_ASSERTF(index + len <= s.len, "index = %llu, len = %llu, s.len = %llu", index, len, s.len);
  BR_ASSERTF(index + len >= index, "index = %llu, len = %llu", index, len);
  if (s.len > BR_SERIES_SUPPORT_CAP) return (br_series_view_t) { .arr = s.arr + index, .len = len };
  else {
    for (size_t i = 0; i < s.len; ++i) {
      s.arr[i] = (float)s.support[index + i];
    }
    return (br_series_view_t) { .arr = s.arr, .len = len };
  }
}

double br_series_zero(br_series_t s, br_u64 index) {
  BR_ASSERTF(index < s.len, "index = %llu, len = %llu, s.len = %llu", index, s.len, s.len);

  if (s.len > BR_SERIES_SUPPORT_CAP) return ((double)br_da_get(s, index))*s.scale + s.offset;
  else return s.support[index];
}

br_series_view_zero_t br_series_view_zero(br_series_t s, br_u64 index, br_u64 len) {
  BR_ASSERTF(index + len <= s.len, "index = %llu, len = %llu, s.len = %llu", index, len, s.len);
  BR_ASSERTF(index + len >= index, "index = %llu, len = %llu", index, len);
  BR_ASSERTF(len <= BR_SERIES_SUPPORT_CAP, "len = %llu", len);

  if (s.len > BR_SERIES_SUPPORT_CAP) {
    for (size_t i = 0; i < s.len; ++i) {
      s.support[i] = (double)s.support[index + i]*s.scale + s.offset;
    }
    return (br_series_view_zero_t) { .arr = s.support, .len = len };
  } else {
    return (br_series_view_zero_t) { .arr = s.support + index, .len = len };
  }
}

float br_series_to_local(br_series_t s, double value) {
  return (float)((value - s.offset)/s.scale);
}

double br_series_to_zero(br_series_t s, float value) {
  return (double)value*s.scale + s.offset;
}

size_t br_series_len(br_series_t s) {
  return s.len;
}

bool br_series_write(BR_FILE* file, br_series_t s) {
  bool success = true;
  int value = 0;
  LOGI("Writing the series");

  if (s.len > BR_SERIES_SUPPORT_CAP) {
    value = 1;
    BR_FS_WRITE1(file, value);
    br_da_write(file, s);
  } else {
    value = 0;
    BR_FS_WRITE1(file, value);
    br_da_write_header(file, s);
    if (s.len > 0) {
      if (s.len != BR_FWRITE(s.support, sizeof(s.support[0]), s.len, file)) BR_ERRORE("Failed to write the support");
    }
  }

error:
  return success;
}

bool br_series_read(BR_FILE* file, br_series_t* s) {
  bool success = true;
  int value = 0;

  *s = (br_series_t){0};

  LOGI("Pos=%ld 0x%lX", ftell(file), ftell(file));
  BR_FS_READ1(file, value);
  if (value == 0) {
    br_da_read_head(file, *s);
    BR_MALLOCE(s->support, BR_SERIES_SUPPORT_CAP);
    if (s->len > 0) {
      if (s->len != BR_FREAD(s->support, sizeof(s->support[0]), s->len, file)) BR_ERRORE("Failed to read the support");
    }
  } else if (value == 1) {
    br_da_read(file, *s);
    BR_MALLOCE(s->support, BR_SERIES_SUPPORT_CAP);
  } else BR_ERROR("Value %d is not a good value...", value);

error:
  return success;
}

bool br_serieses_write(BR_FILE* file, br_serieses_t s) {
  int error = 0;
  bool success = true;
  LOGI("Writing the serieses");

  brfl_write(file, s, error);
  if (error) BR_ERROR("Failed to write serieses");
  brfl_foreach(i, s) {
    if (false == br_series_write(file, br_da_get(s, i))) BR_ERROR("Failed to write series %d", i);
  }

error:
  return success;
}
bool br_serieses_read(BR_FILE* file, br_serieses_t* s) {
  bool success = true;
  int error = 0;

  brfl_read(file, *s, error);
  if (error) BR_ERROR("Failed to read serieses..");
  brfl_foreach(i, *s) {
    if (false == br_series_read(file, br_da_getp(*s, i))) BR_ERROR("Failed to read series %d", i);
  }

error:
  return success;
}

void br_serieses_construct(br_serieses_t* s) {
  br_serieses.s = s;
}

void br_serieses_push(int handle, double value) {
  br_series_t* series = br_da_getp(*br_serieses.s, handle);
  br_series_push(series, value);
}

void br_serieses_push_len(int handle) {
  br_series_t* series = br_da_getp(*br_serieses.s, handle);
  double value = (double)series->len;
  br_series_push(series, value);
}

double br_serieses_zero(int handle, size_t index) {
  br_series_t series = br_da_get(*br_serieses.s, handle);
  double value = br_series_zero(series, index);
  return value;
}

br_u64 br_serieses_len(int handle) {
  br_series_t series = br_da_get(*br_serieses.s, handle);
  return br_series_len(series);
}

void br_serieses_empty(int handle) {
  br_series_t* series = br_da_getp(*br_serieses.s, handle);
  series->len = 0;
}

void br_serieses_release(int handle) {
  br_series_t* series = br_da_getp(*br_serieses.s, handle);
  BR_FREE(series->arr);

  brfl_remove(*br_serieses.s, handle);
}

br_series_t br_serieses_get(int handle) {
  br_series_t series = br_da_get(*br_serieses.s, handle);
  return series;
}
