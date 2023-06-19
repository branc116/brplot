#pragma once
//Maximum number of points that can be shown by this plotter.
//If you try to show more, this program will crash....
#ifdef PLATFORM_WEB
#define POINTS_CAP (16 * 1024 * 1024)
#else
#define POINTS_CAP (64 * 1024 * 1024)
#endif
#define SMOL_MESHES_CAP 1024

// This is the size of one group of points that will be drawn with one draw call.
// TODO: make this dynamic.
#define PTOM_COUNT (1<<15)
