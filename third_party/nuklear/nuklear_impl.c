/* Isolated Nuklear v4.13.4 implementation unit (MIT / Public Domain) */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wnull-pointer-subtraction"
#else
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_BUTTON_TRIGGER_ON_RELEASE
#define NK_IMPLEMENTATION
#include "nuklear.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
