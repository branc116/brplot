#include "stdio.h"
#include "stdbool.h"
#include "raylib.h"
#include <stdlib.h>
#include "string.h"
#include "src/br_plot.h"

#define RLAPI

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

RLAPI void rlUnloadVertexBuffer(unsigned int vboId) {
  LOG("rlUnloadVertexBuffer vboId=%u\n", vboId);
}

RLAPI void rlUpdateVertexBuffer(unsigned int bufferId, const void *data, int dataSize, int offset) {
  LOG("rlUpdateVertexBuffer: bufferId=%u, data=%p, dataSize=%d, offset=%d\n", bufferId, data, dataSize, offset);
}

RLAPI bool rlEnableVertexArray(unsigned int vaoId) {
  LOG("rlEnableVertexArray: vaoId=%u\n", vaoId);
  return true;
}

RLAPI void rlEnableShader(unsigned int id) {
  LOG("rlEnableShader: id=%u\n", id);
}

RLAPI void rlEnableVertexBuffer(unsigned int id) {
  LOG("rlEnableVertexBuffer: id=%u\n", id);
}

RLAPI void rlSetVertexAttribute(unsigned int index, int compSize, int type, bool normalized, int stride, const void *pointer) {
  LOG("rlSetVertexAttribute: index=%u, compSize=%d, type=%d, normalized=%d, stride=%d, pointer=%p\n", index, compSize, type, normalized, stride, pointer);
}

RLAPI void rlEnableVertexAttribute(unsigned int index) {
  LOG("rlEnableVertexAttribute: index=%u\n", index);
}

RLAPI void rlUnloadVertexArray(unsigned int vaoId) {
  LOG("rlUnloadVertexArray: vaoId=%d\n", vaoId);
}

RLAPI void rlDrawVertexArray(int offset, int count) {
  LOG("rlDrawVertexArray: offset=%d, count=%d\n", offset, count);
}

RLAPI void rlDisableVertexArray(void) {
  LOG("rlDisableVertexArray\n");
}

RLAPI void rlDisableVertexBuffer(void) {
  LOG("rlDisableVertexBuffer\n");
}

RLAPI void rlDisableVertexBufferElement(void) {
  LOG("rlDisableVertexBufferElement\n");
}

RLAPI void rlDisableShader(void) {
  LOG("rlDisableShader\n");
}

static unsigned int rlLoadVertexArray_index = 0;
RLAPI unsigned int rlLoadVertexArray(void) {
  LOG("rlLoadVertexArray ret=%u", ++rlLoadVertexArray_index);
  return rlLoadVertexArray_index;
}

static unsigned int rlLoadVertexBuffer_index = 0;
RLAPI unsigned int rlLoadVertexBuffer(const void *buffer, int size, bool dynamic) {
  LOG("rlLoadVertexBuffer: buffer=%p, size=%d, dynamic=%d, ret=%u\n", buffer, size, dynamic, ++rlLoadVertexBuffer_index);
  return rlLoadVertexBuffer_index;
}

RLAPI void BeginDrawing(void) {
  LOG("BeginDrawing\n");
}

RLAPI void ClearBackground(Color color) {
  LOG("ClearBackground: color.r=%d, color.g=%d, color.b=%d, color.a=%d\n", color.r, color.g, color.b, color.a);
}

RLAPI void EndDrawing(void) {
  LOG("EndDrawing\n");
}

int number_of_steps = NUMBER_OF_STEPS;
#define PROB_NOT_PRESSED 0.95f
RLAPI bool WindowShouldClose(void) {
  LOGI("WindowShouldClose: number_of_steps=%d\n", --number_of_steps);
  return number_of_steps <= 0;
}

RLAPI void SetWindowState(unsigned int flags) {
  LOG("SetWindowState: flags=%xu\n", flags);
}

RLAPI void InitWindow(int width, int height, const char *title) {
  LOG("InitWindow: width=%d, height=%d, title=%s\n", width, height, title);
}

unsigned int LoadShader_id;
RLAPI Shader LoadShader(const char *vsFileName, const char *fsFileName) {
  LOG("LoadShader: vsFileName=%s, fsFileName=%s, ret.id=%u\n", vsFileName, fsFileName, ++LoadShader_id);
  Shader ret = { .id = LoadShader_id, .locs = BR_MALLOC(sizeof(int)*16) };
  return ret;
}

int GetShaderLocation_id;
RLAPI int GetShaderLocation(Shader shader, const char *uniformName) {
  LOG("GetShaderLocation: shader.id=%d, uniformName=%s, ret=%d\n", shader.id, uniformName, ++GetShaderLocation_id);
  return GetShaderLocation_id;
}

RLAPI void UnloadShader(Shader shader) {
  BR_FREE(shader.locs);
  LOG("UnloadShader: shader.id=%d\n", shader.id);
}

static float get_rand_float(void) {
  float ret = (float)rand() / (float)RAND_MAX;
  return ret;
}

RLAPI Vector2 GetMousePosition(void) {
  Vector2 r = { get_rand_float()*1280, get_rand_float()*720 };
  LOG("GetMousePosition: ret.x=%f, ret.y=%f\n", r.x, r.y);
  return r;
}

RLAPI bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
  LOG("CheckCollisionPointRec: point=(%f,%f), rec=(%f,%f,%f,%f)\n", point.x, point.y, rec.x, rec.y, rec.width, rec.height);
  return true;
}

RLAPI bool IsMouseButtonDown(int button) {
  bool ret = get_rand_float() > PROB_NOT_PRESSED;
  LOG("IsMouseButtonDown: button=%d, ret=%d\n", button, ret);
  return ret;
}

RLAPI Vector2 GetMouseDelta(void) {
  Vector2 ret = {get_rand_float()-0.5f, get_rand_float()-0.5f};
  LOG("GetMouseDelta: ret=(%f,%f)\n", ret.x, ret.y);
  return ret;
}

RLAPI float GetMouseWheelMove(void) {
  float ret = get_rand_float()-0.5f;
  LOG("GetMouseWheelMove: ret=%f\n", ret);
  return ret;
}

RLAPI bool IsKeyDown(int key) {
  bool ret = get_rand_float() > PROB_NOT_PRESSED;
  LOG("IsKeyDown: key=%d, ret=%d\n", key, ret);
  return ret;
}

RLAPI bool IsKeyPressed(int key) {
  bool ret = get_rand_float() > PROB_NOT_PRESSED;
  LOG("IsKeyPressed: key=%d, ret=%d\n", key, ret);
  return ret;
}

RLAPI void SetShaderValue(Shader shader, int locIndex, const void *value, int uniformType) {
  LOG("SetShaderValue: shader.id=%u, locIndex=%d, value=%p, uniformType=%d\n", shader.id, locIndex, value, uniformType);
}

RLAPI void BeginShaderMode(Shader shader) {
  LOG("BeginShaderMode: shader.id=%d\n", shader.id);
}

RLAPI void EndShaderMode(void) {
  LOG("EndShaderMode\n");
}

RLAPI void DrawRectangle(int posx, int posy, int width, int height, Color color) {
  LOG("DrawRectangleRec: rec=(%d,%d,%d,%d), color=(%u,%u,%u,%u)\n", posx, posy, width, height, color.r, color.g, color.b, color.a);
}

RLAPI void DrawRectangleRec(Rectangle rec, Color color) {
  LOG("DrawRectangleRec: rec=(%f,%f,%f,%f), color=(%u,%u,%u,%u)\n", rec.x, rec.y, rec.width, rec.height, color.r, color.g, color.b, color.a);
}

RLAPI void DrawRectangleV(Vector2 position, Vector2 size, Color color) {
  LOG("DrawRectangleV: position=(%f,%f), size=(%f,%f), color=(%u,%u,%u,%u)\n", position.x, position.y, size.x, size.y, color.r, color.g, color.b, color.a);
}

RLAPI void CloseWindow(void) {
  LOG("CloseWindow\n");
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
  LOG("IsMouseButtonPressed: ret=%d\n", ret);
  return ret;
}

RLAPI Vector2 GetMouseWheelMoveV(void) {
  return (Vector2) { get_rand_float(), get_rand_float() };
}

RLAPI void DrawBoundingBox(BoundingBox box, Color color) {
  LOG("DrawBoundingBox\n");
}

RLAPI void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
  LOG("DrawTextEx: %s\n", text);
}

RLAPI Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing) {
  LOG("MeasureTextEx\n");
  return (Vector2){ (float)strlen(text)*get_rand_float()*3.f, get_rand_float()*42.f };
}

RLAPI Image GenImageFontAtlas(const GlyphInfo *chars, Rectangle **recs, int glyphCount, int fontSize, int padding, int packMethod) {
  LOG("GenImageFontAtlas\n");
  return (Image) { 0 };
}

RLAPI Texture2D LoadTextureFromImage(Image image) {
  LOG("LoadTextureFromImage\n");
  return (Texture2D) { .id = 69 };
}

RLAPI void SetTextureFilter(Texture2D texture, int filter) {
  LOG("SetTextureFilter\n");
}

RLAPI void UnloadImage(Image image) {
  LOG("UnloadImage\n");
}

RLAPI int GetFPS(void) {
  return 69;
}

RLAPI void SetTraceLogLevel(int logLevel) {
  LOG("SetTraceLogLevel: logLevel=%d\n", logLevel);
}

RLAPI Shader LoadShaderFromMemory(const char *vsCode, const char *fsCode) {
  LOG("LoadShaderFromMemory: vsCode=%s, fsCode=%s\n", vsCode, fsCode);
  Shader ret = { .id = LoadShader_id, .locs = BR_MALLOC(sizeof(int)*16) };
  return ret;
}

static double cur = 0.0;
RLAPI double GetTime(void) {
  return cur += .001;
}

void BeginScissorMode(int x, int y, int width, int height) {
  LOG("BeginScissorMode: %d, %d, %d, %d\n", x, y, width, height);
}

RenderTexture2D LoadRenderTexture(int width, int height) {
  return (RenderTexture2D){ .id = 69 };
}

void EndScissorMode(void) {
  LOG("EndScissorMode\n");
}

void BeginTextureMode(RenderTexture2D target) {
  LOG("BeginTextureMode");
}

void EndTextureMode(void) {
  LOG("EndTextureMode\n");
}

Image LoadImageFromTexture(Texture2D texture) {
  return (Image){0};
}

RLAPI void UnloadRenderTexture(RenderTexture2D target) {
  LOG("UnloadRenderTexture");
}

void ImageFlipVertical(Image *image) {
  LOG("ImageFlipVertical\n");
}

bool ExportImage(Image image, const char *fileName) {
  LOG("ExportImage: fileName=%s", fileName);
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
  LOG("rlSetBlendMode: mode=%d\n", mode);
}

void rlSetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation) {
  LOG("rlSetBlendFactors: glSrcFactor=%d, glDstFactor=%d, glEquation=%d\n", glSrcFactor, glDstFactor, glEquation);
}

float GetFrameTime(void) {
  return 0.16f;
}

Matrix MatrixPerspective(double fovY, double aspect, double nearPlane, double farPlane) {
  return (Matrix) {0};
}

Matrix MatrixLookAt(Vector3 eye, Vector3 target, Vector3 up) {
  return (Matrix) {0};
}

Matrix MatrixMultiply(Matrix left, Matrix right) {
  return (Matrix) {0};
}

void rlSetUniformMatrix(int locIndex, Matrix mat) {
}

int rlGetLocationUniform(unsigned int shaderId, const char *uniformName) {
  return 69;
}

int rlGetLocationAttrib(unsigned int shaderId, const char *attribName) {
  return 69;
}

unsigned int rlLoadShaderCode(const char *vsCode, const char *fsCode) {
  return 69;
}

char *LoadFileText(const char *fileName) {
  return NULL;
}

void rlSetUniform(int locIndex, const void *value, int uniformType, int count) {
  return;
}

void rlUnloadShaderProgram(unsigned int id) {
  return;
}

void rlViewport(int x, int y, int width, int height) {
}

#include "math.h"
Vector2 Vector2Normalize(Vector2 v) {
    Vector2 result = { 0 };
    float length = sqrtf((v.x*v.x) + (v.y*v.y));

    if (length > 0) {
        float ilength = 1.0f/length;
        result.x = v.x*ilength;
        result.y = v.y*ilength;
    }

    return result;
}

Vector3 Vector3Scale(Vector3 v, float scalar) {
    Vector3 result = { v.x*scalar, v.y*scalar, v.z*scalar };
    return result;
}

Vector3 Vector3Add(Vector3 v1, Vector3 v2) {
    Vector3 result = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
    return result;
}

Vector3 Vector3Normalize(Vector3 v) {
    Vector3 result = v;

    float length = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (length != 0.0f) {
        float ilength = 1.0f/length;

        result.x *= ilength;
        result.y *= ilength;
        result.z *= ilength;
    }

    return result;
}

Vector2 Vector2Divide(Vector2 v1, Vector2 v2) {
    Vector2 result = { v1.x/v2.x, v1.y/v2.y };
    return result;
}

float Vector3Length(const Vector3 v) {
    float result = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    return result;
}

Vector3 Vector3Subtract(Vector3 v1, Vector3 v2) {
    Vector3 result = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
    return result;
}

float Vector3DotProduct(Vector3 v1, Vector3 v2) {
    float result = (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z);
    return result;
}

Vector3 Vector3RotateByAxisAngle(Vector3 v, Vector3 axis, float angle) {
    // Using Euler-Rodrigues Formula
    // Ref.: https://en.wikipedia.org/w/index.php?title=Euler%E2%80%93Rodrigues_formula

    Vector3 result = v;

    // Vector3Normalize(axis);
    float length = sqrtf(axis.x*axis.x + axis.y*axis.y + axis.z*axis.z);
    if (length == 0.0f) length = 1.0f;
    float ilength = 1.0f / length;
    axis.x *= ilength;
    axis.y *= ilength;
    axis.z *= ilength;

    angle /= 2.0f;
    float a = sinf(angle);
    float b = axis.x*a;
    float c = axis.y*a;
    float d = axis.z*a;
    a = cosf(angle);
    Vector3 w = { b, c, d };

    // Vector3CrossProduct(w, v)
    Vector3 wv = { w.y*v.z - w.z*v.y, w.z*v.x - w.x*v.z, w.x*v.y - w.y*v.x };

    // Vector3CrossProduct(w, wv)
    Vector3 wwv = { w.y*wv.z - w.z*wv.y, w.z*wv.x - w.x*wv.z, w.x*wv.y - w.y*wv.x };

    // Vector3Scale(wv, 2*a)
    a *= 2;
    wv.x *= a;
    wv.y *= a;
    wv.z *= a;

    // Vector3Scale(wwv, 2)
    wwv.x *= 2;
    wwv.y *= 2;
    wwv.z *= 2;

    result.x += wv.x;
    result.y += wv.y;
    result.z += wv.z;

    result.x += wwv.x;
    result.y += wwv.y;
    result.z += wwv.z;

    return result;
}

Vector2 Vector2Multiply(Vector2 v1, Vector2 v2) {
    Vector2 result = { v1.x*v2.x, v1.y*v2.y };
    return result;
}

Vector3 Vector3CrossProduct(Vector3 v1, Vector3 v2) {
    Vector3 result = { v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x };
    return result;
}

void SetConfigFlags(unsigned int flags) {
  (void)flags;
}

void rlDisableBackfaceCulling(void) {}

void rlEnableBackfaceCulling(void) {}

float Vector3Distance(Vector3 v1, Vector3 v2) {
    float result = 0.0f;

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    result = sqrtf(dx*dx + dy*dy + dz*dz);

    return result;
}

Vector3 Vector3Perpendicular(Vector3 v) {
    Vector3 result = { 0 };

    float min = (float) fabs(v.x);
    Vector3 cardinalAxis = {1.0f, 0.0f, 0.0f};

    if (fabsf(v.y) < min) {
        min = (float) fabs(v.y);
        Vector3 tmp = {0.0f, 1.0f, 0.0f};
        cardinalAxis = tmp;
    }

    if (fabsf(v.z) < min) {
        Vector3 tmp = {0.0f, 0.0f, 1.0f};
        cardinalAxis = tmp;
    }

    // Cross product between vectors
    result.x = v.y*cardinalAxis.z - v.z*cardinalAxis.y;
    result.y = v.z*cardinalAxis.x - v.x*cardinalAxis.z;
    result.z = v.x*cardinalAxis.y - v.y*cardinalAxis.x;

    return result;
}

bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2)
{
    bool collision = false;

    if ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
        (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y)) collision = true;

    return collision;
}

void rlDisableDepthTest(void) {}
void rlEnableDepthTest(void) {}
