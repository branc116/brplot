#ifdef __cplusplus
extern "C" {
#endif

#include "graph.h"

// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(graph_values_t* gv);

#ifdef __cplusplus
}
#endif
