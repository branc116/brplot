#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>

// TODO remove this. Use pipeing from nc or something to achive this efect.
// This doesn't follow unix philosophy.
void udp_main(void* ptr);
void add_point_callback(void* gv, char* buffer, size_t s);

#ifdef __cplusplus
}
#endif
