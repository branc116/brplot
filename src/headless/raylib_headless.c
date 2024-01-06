#include "stdio.h"
#include "stdbool.h"
#include "raylib.h"
#include <stdlib.h>
#include "string.h"
#include "src/br_plot.h"

#define RLAPI
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

RLAPI void rlUnloadVertexBuffer(unsigned int vboId) {
  printf("rlUnloadVertexBuffer vboId=%u\n", vboId);
}

RLAPI void rlUpdateVertexBuffer(unsigned int bufferId, const void *data, int dataSize, int offset) {
  printf("rlUpdateVertexBuffer: bufferId=%u, data=%p, dataSize=%d, offset=%d\n", bufferId, data, dataSize, offset);
}

RLAPI bool rlEnableVertexArray(unsigned int vaoId) {
  printf("rlEnableVertexArray: vaoId=%u\n", vaoId);
  return true;
}

RLAPI void rlEnableShader(unsigned int id) {
  printf("rlEnableShader: id=%u\n", id);
}

RLAPI void rlEnableVertexBuffer(unsigned int id) {
  printf("rlEnableVertexBuffer: id=%u\n", id);
}

RLAPI void rlSetVertexAttribute(unsigned int index, int compSize, int type, bool normalized, int stride, const void *pointer) {
  printf("rlSetVertexAttribute: index=%u, compSize=%d, type=%d, normalized=%d, stride=%d, pointer=%p\n", index, compSize, type, normalized, stride, pointer);
}

RLAPI void rlEnableVertexAttribute(unsigned int index) {
  printf("rlEnableVertexAttribute: index=%u\n", index);
}

RLAPI void rlUnloadVertexArray(unsigned int vaoId) {
  printf("rlUnloadVertexArray: vaoId=%d\n", vaoId);
}

RLAPI void rlDrawVertexArray(int offset, int count) {
  printf("rlDrawVertexArray: offset=%d, count=%d\n", offset, count);
}

RLAPI void rlDisableVertexArray(void) {
  printf("rlDisableVertexArray\n");
}

RLAPI void rlDisableVertexBuffer(void) {
  printf("rlDisableVertexBuffer\n");
}

RLAPI void rlDisableVertexBufferElement(void) {
  printf("rlDisableVertexBufferElement\n");
}

RLAPI void rlDisableShader(void) {
  printf("rlDisableShader\n");
}

static unsigned int rlLoadVertexArray_index = 0;
RLAPI unsigned int rlLoadVertexArray(void) {
  printf("rlLoadVertexArray ret=%u", ++rlLoadVertexArray_index);
  return rlLoadVertexArray_index;
}

static unsigned int rlLoadVertexBuffer_index = 0;
RLAPI unsigned int rlLoadVertexBuffer(const void *buffer, int size, bool dynamic) {
  printf("rlLoadVertexBuffer: buffer=%p, size=%d, dynamic=%d, ret=%u\n", buffer, size, dynamic, ++rlLoadVertexBuffer_index);
  return rlLoadVertexBuffer_index;
}

RLAPI void BeginDrawing(void) {
  printf("BeginDrawing\n");
}

RLAPI void ClearBackground(Color color) {
  printf("ClearBackground: color.r=%d, color.g=%d, color.b=%d, color.a=%d\n", color.r, color.g, color.b, color.a);
}

RLAPI void EndDrawing(void) {
  printf("EndDrawing\n");
}

int number_of_steps = NUMBER_OF_STEPS;
#define PROB_NOT_PRESSED 0.95f
RLAPI bool WindowShouldClose(void) {
  printf("WindowShouldClose: number_of_steps=%d\n", --number_of_steps);
  return number_of_steps <= 0;
}

RLAPI void SetWindowState(unsigned int flags) {
  printf("SetWindowState: flags=%xu\n", flags);
}

RLAPI void InitWindow(int width, int height, const char *title) {
  printf("InitWindow: width=%d, height=%d, title=%s\n", width, height, title);
}

unsigned int LoadShader_id;
RLAPI Shader LoadShader(const char *vsFileName, const char *fsFileName) {
  printf("LoadShader: vsFileName=%s, fsFileName=%s, ret.id=%u\n", vsFileName, fsFileName, ++LoadShader_id);
  Shader ret = { .id = LoadShader_id, .locs = BR_MALLOC(sizeof(int)*16) };
  return ret;
}

int GetShaderLocation_id;
RLAPI int GetShaderLocation(Shader shader, const char *uniformName) {
  printf("GetShaderLocation: shader.id=%d, uniformName=%s, ret=%d\n", shader.id, uniformName, ++GetShaderLocation_id);
  return GetShaderLocation_id;
}

RLAPI void UnloadShader(Shader shader) {
  BR_FREE(shader.locs);
  printf("UnloadShader: shader.id=%d\n", shader.id);
}

static float get_rand_float(void) {
  float ret = (float)rand() / (float)RAND_MAX;
  return ret;
}

RLAPI Vector2 GetMousePosition(void) {
  Vector2 r = { get_rand_float()*1280, get_rand_float()*720 };
  printf("GetMousePosition: ret.x=%f, ret.y=%f\n", r.x, r.y);
  return r;
}

RLAPI bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
  printf("CheckCollisionPointRec: point=(%f,%f), rec=(%f,%f,%f,%f)\n", point.x, point.y, rec.x, rec.y, rec.width, rec.height);
  return true;
}

RLAPI bool IsMouseButtonDown(int button) {
  bool ret = get_rand_float() > PROB_NOT_PRESSED;
  printf("IsMouseButtonDown: button=%d, ret=%d\n", button, ret);
  return ret;
}

RLAPI Vector2 GetMouseDelta(void) {
  Vector2 ret = {get_rand_float()-0.5f, get_rand_float()-0.5f};
  printf("GetMouseDelta: ret=(%f,%f)\n", ret.x, ret.y);
  return ret;
}

RLAPI float GetMouseWheelMove(void) {
  float ret = get_rand_float()-0.5f;
  printf("GetMouseWheelMove: ret=%f\n", ret);
  return ret;
}

RLAPI bool IsKeyDown(int key) {
  bool ret = get_rand_float() > PROB_NOT_PRESSED;
  printf("IsKeyDown: key=%d, ret=%d\n", key, ret);
  return ret;
}

RLAPI bool IsKeyPressed(int key) {
  bool ret = get_rand_float() > PROB_NOT_PRESSED;
  printf("IsKeyPressed: key=%d, ret=%d\n", key, ret);
  return ret;
}

RLAPI void SetShaderValue(Shader shader, int locIndex, const void *value, int uniformType) {
  printf("SetShaderValue: shader.id=%u, locIndex=%d, value=%p, uniformType=%d\n", shader.id, locIndex, value, uniformType);
}

RLAPI void BeginShaderMode(Shader shader) {
  printf("BeginShaderMode: shader.id=%d\n", shader.id);
}

RLAPI void EndShaderMode(void) {
  printf("EndShaderMode\n");
}

RLAPI void DrawRectangle(int posx, int posy, int width, int height, Color color) {
  printf("DrawRectangleRec: rec=(%d,%d,%d,%d), color=(%u,%u,%u,%u)\n", posx, posy, width, height, color.r, color.g, color.b, color.a);
}

RLAPI void DrawRectangleRec(Rectangle rec, Color color) {
  printf("DrawRectangleRec: rec=(%f,%f,%f,%f), color=(%u,%u,%u,%u)\n", rec.x, rec.y, rec.width, rec.height, color.r, color.g, color.b, color.a);
}

RLAPI void DrawRectangleV(Vector2 position, Vector2 size, Color color) {
  printf("DrawRectangleV: position=(%f,%f), size=(%f,%f), color=(%u,%u,%u,%u)\n", position.x, position.y, size.x, size.y, color.r, color.g, color.b, color.a);
}

RLAPI void CloseWindow(void) {
  printf("CloseWindow\n");
}

RLAPI int GetScreenWidth(void) {
  return 1280;
}

RLAPI int GetScreenHeight(void) {
  return 720;
}

// Calculate square distance between two vectors
float Vector2DistanceSqr(Vector2 v1, Vector2 v2)
{
  float result = ((v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y));

  return result;
}

Vector2 Vector2AddValue(Vector2 v, float add)
{
  return (Vector2){ v.x + add, v.y + add };
}

RLAPI bool CheckCollisionLines(Vector2 startPos1, Vector2 endPos1, Vector2 startPos2, Vector2 endPos2, Vector2 *collisionPoint) {
  return true;
}

RLAPI bool IsMouseButtonPressed(int button) {
  bool ret = get_rand_float() > PROB_NOT_PRESSED;
  printf("IsMouseButtonPressed: ret=%d\n", ret);
  return ret;
}

RLAPI Vector2 GetMouseWheelMoveV(void) {
  return (Vector2) { get_rand_float(), get_rand_float() };
}

RLAPI void DrawBoundingBox(BoundingBox box, Color color) {
  printf("DrawBoundingBox\n");
}

RLAPI void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
  printf("DrawTextEx: %s\n", text);
}

RLAPI Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing) {
  printf("MeasureTextEx\n");
  return (Vector2){ (float)strlen(text)*get_rand_float()*3.f, get_rand_float()*42.f };
}

RLAPI Image GenImageFontAtlas(const GlyphInfo *chars, Rectangle **recs, int glyphCount, int fontSize, int padding, int packMethod) {
  printf("GenImageFontAtlas\n");
  return (Image) { 0 };
}

RLAPI Texture2D LoadTextureFromImage(Image image) {
  printf("LoadTextureFromImage\n");
  return (Texture2D) { .id = 69 };
}

RLAPI void SetTextureFilter(Texture2D texture, int filter) {
  printf("SetTextureFilter\n");
}

RLAPI void UnloadImage(Image image) {
  printf("UnloadImage\n");
}

RLAPI int GetFPS(void) {
  return 69;
}

RLAPI void SetTraceLogLevel(int logLevel) {
  printf("SetTraceLogLevel: logLevel=%d\n", logLevel);
}

RLAPI Shader LoadShaderFromMemory(const char *vsCode, const char *fsCode) {
  printf("LoadShaderFromMemory: vsCode=%s, fsCode=%s\n", vsCode, fsCode);
  Shader ret = { .id = LoadShader_id, .locs = BR_MALLOC(sizeof(int)*16) };
  return ret;
}

RLAPI double GetTime(void) {
  return 69.0;
}

void BeginScissorMode(int x, int y, int width, int height) {
  printf("BeginScissorMode: %d, %d, %d, %d\n", x, y, width, height);
}

RenderTexture2D LoadRenderTexture(int width, int height) {
  return (RenderTexture2D){ .id = 69 };
}

void EndScissorMode(void) {
  printf("EndScissorMode\n");
}

void BeginTextureMode(RenderTexture2D target) {
  printf("BeginTextureMode");
}

void EndTextureMode(void) {
  printf("EndTextureMode\n");
}

Image LoadImageFromTexture(Texture2D texture) {
  return (Image){0};
}

RLAPI void UnloadRenderTexture(RenderTexture2D target) {
  printf("UnloadRenderTexture");
}

void ImageFlipVertical(Image *image) {
  printf("ImageFlipVertical\n");
}

bool ExportImage(Image image, const char *fileName) {
  printf("ExportImage: fileName=%s", fileName);
  return true;
}

FilePathList LoadDirectoryFiles(const char *dirPath) {
  return (FilePathList){0};
}

void UnloadDirectoryFiles(FilePathList files) {
}

bool DirectoryExists(const char *dirPath) {
  return true;
}

void SetExitKey(int key) {
}

void SetWindowSize(int width, int height) {
}

// String pointer reverse break: returns right-most occurrence of charset in s
static const char *strprbrk(const char *s, const char *charset) {
    const char *latestMatch = NULL;
    for (; s = strpbrk(s, charset), s != NULL; latestMatch = s++) { }
    return latestMatch;
}

// Get pointer to filename for a path string
const char *GetFileName(const char *filePath) {
    const char *fileName = NULL;
    if (filePath != NULL) fileName = strprbrk(filePath, "\\/");

    if (!fileName) return filePath;

    return fileName + 1;
}

const char *TextFormat(const char *text, ...) {
  return text;
}

Vector2 Vector2Scale(Vector2 v, float scale)
{
    Vector2 result = { v.x*scale, v.y*scale };

    return result;
}

// Subtract two vectors (v1 - v2)
Vector2 Vector2Subtract(Vector2 v1, Vector2 v2)
{
    Vector2 result = { v1.x - v2.x, v1.y - v2.y };

    return result;
}

Vector2 Vector2Add(Vector2 v1, Vector2 v2)
{
    Vector2 result = { v1.x + v2.x, v1.y + v2.y };

    return result;
}

void rlSetBlendMode(int mode) {
  printf("rlSetBlendMode: mode=%d\n", mode);
}
void rlSetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation) {
  printf("rlSetBlendFactors: glSrcFactor=%d, glDstFactor=%d, glEquation=%d\n", glSrcFactor, glDstFactor, glEquation);
}


#include "math.h"
Vector2 Vector2Normalize(Vector2 v)
{
    Vector2 result = { 0 };
    float length = sqrtf((v.x*v.x) + (v.y*v.y));

    if (length > 0)
    {
        float ilength = 1.0f/length;
        result.x = v.x*ilength;
        result.y = v.y*ilength;
    }

    return result;
}
