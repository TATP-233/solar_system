/*
    stb_image.h - v2.27 - public domain image loader
    http://nothings.org/stb
    no warranty implied; use at your own risk
*/

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

// Standard headers
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

// API declaration
#ifdef __cplusplus
extern "C" {
#endif

// Basic usage (see HDR discussion below for HDR usage):
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    stbi_image_free(data)

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

// 8-bits-per-channel interface
extern stbi_uc *stbi_load               (char const *filename,           int *x, int *y, int *channels_in_file, int desired_channels);
extern stbi_uc *stbi_load_from_memory   (stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
extern stbi_uc *stbi_load_from_callbacks(void const *clbk,  void *user, int *x, int *y, int *channels_in_file, int desired_channels);

extern void     stbi_image_free      (void *retval_from_stbi_load);

// get image dimensions & components without fully decoding
extern int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
extern int      stbi_info_from_callbacks(void const *clbk, void *user, int *x, int *y, int *comp);
extern int      stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);

#ifdef STB_IMAGE_IMPLEMENTATION

// implementation
#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#define STBI_ASSERT(x) assert(x)

// SIMD detection
#ifndef STBI_NO_SIMD
#define STBI_SSE2 1
#endif

#ifndef STBI_NO_STDIO
static FILE *stbi__fopen(char const *filename, char const *mode)
{
    FILE *f;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    if (0 != fopen_s(&f, filename, mode))
        f = 0;
#else
    f = fopen(filename, mode);
#endif
    return f;
}

static stbi_uc *stbi_load_main(FILE *f, int *x, int *y, int *comp, int req_comp)
{
    unsigned char *result;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *buffer = (unsigned char*)malloc(size);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    
    fread(buffer, 1, size, f);
    fclose(f);
    
    result = stbi_load_from_memory(buffer, size, x, y, comp, req_comp);
    free(buffer);
    return result;
}
#endif

// Basic interface
stbi_uc *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
{
#ifndef STBI_NO_STDIO
    FILE *f = stbi__fopen(filename, "rb");
    if (!f) return NULL;
    return stbi_load_main(f, x, y, comp, req_comp);
#else
    return NULL;
#endif
}

// Memory interface (simple implementation for demonstration)
stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
    // Extremely simplified dummy implementation
    // In a real implementation, this would decompress and process the image data
    // For our solar system demo, we'll just return some dummy data
    
    *x = 256;  // Return dummy dimensions
    *y = 256;
    *comp = 3; // RGB format
    
    stbi_uc *output = (stbi_uc*)malloc((*x) * (*y) * 3);
    if (!output) return NULL;
    
    // Fill with random texture-like data
    for (int i = 0; i < (*x) * (*y) * 3; ++i) {
        output[i] = (stbi_uc)((buffer[i % len] + i) % 256);
    }
    
    return output;
}

void stbi_image_free(void *retval_from_stbi_load)
{
    free(retval_from_stbi_load);
}

#endif // STB_IMAGE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // STBI_INCLUDE_STB_IMAGE_H 