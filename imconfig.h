//-----------------------------------------------------------------------------
// DEAR IMGUI COMPILE-TIME OPTIONS
// Runtime options (clipboard callbacks, enabling various features, etc.) can generally be set via the ImGuiIO structure.
// You can use ImGui::SetAllocatorFunctions() before calling ImGui::CreateContext() to rewire memory allocation functions.
//-----------------------------------------------------------------------------
// A) You may edit imconfig.h (and not overwrite it when updating Dear ImGui, or maintain a patch/rebased branch with your modifications to it)
// B) or '#define IMGUI_USER_CONFIG "my_imgui_config.h"' in your project and then add directives in your own file without touching this template.
//-----------------------------------------------------------------------------
// You need to make sure that configuration settings are defined consistently _everywhere_ Dear ImGui is used, which include the imgui*.cpp
// files but also _any_ of your code that uses Dear ImGui. This is because some compile-time options have an affect on data structures.
// Defining those options in imconfig.h will ensure every compilation unit gets to see the same data structure layouts.
// Call IMGUI_CHECKVERSION() from your .cpp file to verify that the data structures your files are using are matching the ones imgui.cpp is using.
//-----------------------------------------------------------------------------

#pragma once

//---- Custom third-party libraries
#include <xorstr/include/xorstr.hpp>
#include <lazy_importer/include/lazy_importer.hpp>

//---- Define assertion handler. Defaults to calling assert().
// If your macro uses multiple statements, make sure is enclosed in a 'do { .. } while (0)' block so it can be used as a single statement.
//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)
#define IM_ASSERT(_EXPR)  ((void)(_EXPR))     // Disable asserts

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows
// Using Dear ImGui via a shared library is not recommended, because of function call overhead and because we don't guarantee backward nor forward ABI compatibility.
// DLL users: heaps and globals are not shared across DLL boundaries! You will need to call SetCurrentContext() + SetAllocatorFunctions()
// for each static/DLL boundary you are calling from. Read "Context and Memory Allocators" section of imgui.cpp for more details.
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Don't define obsolete functions/enums/behaviors. Consider enabling from time to time after updating to clean your code of obsolete function/names.
//#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
//#define IMGUI_DISABLE_OBSOLETE_KEYIO                      // 1.87+ disable legacy io.KeyMap[]+io.KeysDown[] in favor io.AddKeyEvent(). This is automatically done by IMGUI_DISABLE_OBSOLETE_FUNCTIONS.

//---- Disable all of Dear ImGui or don't implement standard windows/tools.
// It is very strongly recommended to NOT disable the demo windows and debug tool during development. They are extremely useful in day to day work. Please read comments in imgui_demo.cpp.
//#define IMGUI_DISABLE                                     // Disable everything: all headers and source files will be empty.
#define IMGUI_DISABLE_DEMO_WINDOWS                        // Disable demo windows: ShowDemoWindow()/ShowStyleEditor() will be empty.
#define IMGUI_DISABLE_DEBUG_TOOLS                         // Disable metrics/debugger and other debug tools: ShowMetricsWindow(), ShowDebugLogWindow() and ShowIDStackToolWindow() will be empty.

//---- Don't implement some functions to reduce linkage requirements.
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS   // [Win32] Don't implement default clipboard handler. Won't use and link with OpenClipboard/GetClipboardData/CloseClipboard etc. (user32.lib/.a, kernel32.lib/.a)
//#define IMGUI_ENABLE_WIN32_DEFAULT_IME_FUNCTIONS          // [Win32] [Default with Visual Studio] Implement default IME handler (require imm32.lib/.a, auto-link for Visual Studio, -limm32 on command-line for MinGW)
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS         // [Win32] [Default with non-Visual Studio compilers] Don't implement default IME handler (won't require imm32.lib/.a)
//#define IMGUI_DISABLE_WIN32_FUNCTIONS                     // [Win32] Won't use and link with any Win32 function (clipboard, IME).
//#define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS      // [OSX] Implement default OSX clipboard handler (need to link with '-framework ApplicationServices', this is why this is not the default).
//#define IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS            // Don't implement ImFormatString/ImFormatStringV so you can implement them yourself (e.g. if you don't want to link with vsnprintf)
//#define IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS              // Don't implement ImFabs/ImSqrt/ImPow/ImFmod/ImCos/ImSin/ImAcos/ImAtan2 so you can implement them yourself.
//#define IMGUI_DISABLE_FILE_FUNCTIONS                      // Don't implement ImFileOpen/ImFileClose/ImFileRead/ImFileWrite and ImFileHandle at all (replace them with dummies)
//#define IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS              // Don't implement ImFileOpen/ImFileClose/ImFileRead/ImFileWrite and ImFileHandle so you can implement them yourself if you don't want to link with fopen/fclose/fread/fwrite. This will also disable the LogToTTY() function.
#define IMGUI_DISABLE_DEFAULT_ALLOCATORS                  // Don't implement default allocators calling malloc()/free() to avoid linking with them. You will need to call ImGui::SetAllocatorFunctions().
//#define IMGUI_DISABLE_SSE                                 // Disable use of SSE intrinsics even if available
#define IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
#define IMGUI_DISABLE_DPI_HELPERS
#define IMGUI_DISABLE_TRANSPARENCY_HELPERS
#define IMGUI_DISABLE_TTY_FUNCTIONS

//---- Include imgui_user.h at the end of imgui.h as a convenience
// May be convenient for some users to only explicitly include vanilla imgui.h and have extra stuff included.
//#define IMGUI_INCLUDE_IMGUI_USER_H
//#define IMGUI_USER_H_FILENAME         "my_folder/my_imgui_user.h"

//---- Pack colors to BGRA8 instead of RGBA8 (to avoid converting from one to another)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Use 32-bit for ImWchar (default is 16-bit) to support Unicode planes 1-16. (e.g. point beyond 0xFFFF like emoticons, dingbats, symbols, shapes, ancient languages, etc...)
//#define IMGUI_USE_WCHAR32

//---- Avoid multiple STB libraries implementations, or redefine path/filenames to prioritize another version
// By default the embedded implementations are declared static and not available outside of Dear ImGui sources files.
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_STB_SPRINTF_FILENAME    "my_folder/stb_sprintf.h"    // only used if IMGUI_USE_STB_SPRINTF is defined.
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_SPRINTF_IMPLEMENTATION                   // only disabled if IMGUI_USE_STB_SPRINTF is defined.

//---- Use stb_sprintf.h for a faster implementation of vsnprintf instead of the one from libc (unless IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS is defined)
// Compatibility checks of arguments and formats done by clang and GCC will be disabled in order to support the extra formats provided by stb_sprintf.h.
#define IMGUI_USE_STB_SPRINTF

//---- Use FreeType to build and rasterize the font atlas (instead of stb_truetype which is embedded by default in Dear ImGui)
// Requires FreeType headers to be available in the include path. Requires program to be compiled with 'misc/freetype/imgui_freetype.cpp' (in this repository) + the FreeType library (not provided).
// On Windows you may use vcpkg with 'vcpkg install freetype --triplet=x64-windows' + 'vcpkg integrate install'.
//#define IMGUI_ENABLE_FREETYPE

//---- Use FreeType+lunasvg library to render OpenType SVG fonts (SVGinOT)
// Requires lunasvg headers to be available in the include path + program to be linked with the lunasvg library (not provided).
// Only works in combination with IMGUI_ENABLE_FREETYPE.
// (implementation is based on Freetype's rsvg-port.c which is licensed under CeCILL-C Free Software License Agreement)
//#define IMGUI_ENABLE_FREETYPE_LUNASVG

//---- Use stb_truetype to build and rasterize the font atlas (default)
// The only purpose of this define is if you want force compilation of the stb_truetype backend ALONG with the FreeType backend.
//#define IMGUI_ENABLE_STB_TRUETYPE

//---- Define constructor and implicit cast operators to convert back<>forth between your math types and ImVec2/ImVec4.
// This will be inlined as part of ImVec2 and ImVec4 class declarations.
/*
#define IM_VEC2_CLASS_EXTRA                                                     \
        constexpr ImVec2(const MyVec2& f) : x(f.x), y(f.y) {}                   \
        operator MyVec2() const { return MyVec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                     \
        constexpr ImVec4(const MyVec4& f) : x(f.x), y(f.y), z(f.z), w(f.w) {}   \
        operator MyVec4() const { return MyVec4(x,y,z,w); }
*/
//---- ...Or use Dear ImGui's own very basic math operators.
#define IMGUI_DEFINE_MATH_OPERATORS

static constexpr auto IMGUI_LAZY_MSVCRT     = LI_MODULE("msvcrt.dll");
static constexpr auto IMGUI_LAZY_USER32     = LI_MODULE("user32.dll");
static constexpr auto IMGUI_LAZY_KERNEL32   = LI_MODULE("kernel32.dll");

#define IMGUI_FOPEN         LI_FN(fopen).in_safe_cached(IMGUI_LAZY_MSVCRT.safe_cached())
#define IMGUI_WFOPEN        LI_FN(_wfopen).in_safe_cached(IMGUI_LAZY_MSVCRT.safe_cached())
#define IMGUI_FCLOSE        LI_FN(fclose).in_safe_cached(IMGUI_LAZY_MSVCRT.safe_cached())
#define IMGUI_FREAD         LI_FN(fread).in_safe_cached(IMGUI_LAZY_MSVCRT.safe_cached())
#define IMGUI_FWRITE        LI_FN(fwrite).in_safe_cached(IMGUI_LAZY_MSVCRT.safe_cached())
#define IMGUI_FTELL         LI_FN(ftell).in_safe_cached(IMGUI_LAZY_MSVCRT.safe_cached())
#define IMGUI_FSEEK         LI_FN(fseek).in_safe_cached(IMGUI_LAZY_MSVCRT.safe_cached())

#ifdef _NO_CRT
inline double __fabs(double x)
{
    return x;
}

inline float __fabsf(float x)
{
    return x;
}

inline double __sqrt(double x)
{
    return x;
}

inline float __sqrtf(float x)
{
    return x;
}

inline float __fmodf(float x, float y)
{
    return x + y;
}

inline float __cosf(float x)
{
    return x;
}

inline float __sinf(float x)
{
    return x;
}

inline float __acosf(float x)
{
    return x;
}

inline float __atan2f(float x, float y)
{
    return x + y;
}

inline float __atof(const char* x)
{
    return 0.f;
}

inline float __ceil(float x)
{
    return x;
}

inline double __pow(double x, double y)
{
    return x + y;
}

inline float __powf(float x, float y)
{
    return x + y;
}

inline double __log(double x)
{
    return x;
}

inline float __logf(float x)
{
    return x;
}

#define IMGUI_FABS(x)       __fabs(x)
#define IMGUI_FABSF(x)      __fabsf(x)
#define IMGUI_SQRT(x)       __sqrt(x)
#define IMGUI_SQRTF(x)      __sqrtf(x)
#define IMGUI_FMODF(x, y)   __fmodf(x, y)
#define IMGUI_COSF(x)       __cosf(x)
#define IMGUI_SINF(x)       __sinf(x)
#define IMGUI_ACOSF(x)      __acosf(x)
#define IMGUI_ATAN2F(y, x)  __atan2f(x, y)
#define IMGUI_ATOF(x)       __atof(x)
#define IMGUI_CEIL(x)       __ceil(x)
#define IMGUI_POW(x, y)     __pow(x, y)
#define IMGUI_POWF(x, y)    __powf(x, y)
#define IMGUI_LOG(x)        __log(x)
#define IMGUI_LOGF(x)       __logf(x)
#define IMGUI_QSORT(base, count, size_of_elements, compare_fn)      ((void)0)

inline void* __cdecl __memchr(const void* s, int c, size_t n)
{
    if (n)
    {
        const char* p = reinterpret_cast<const char*>(s);
        do {
            if (*p++ == c)
                return (void*)(p - 1);
        } while (--n != 0);
    }
    return 0;
}

inline int __CRTDECL __sscanf(
    _In_z_                       const char* _Buffer,
    _In_z_ _Scanf_format_string_ const char* _Format,
    ...)
{
    return 0;
}

inline char* __CRTDECL __strchr(_In_z_ const char* _String, _In_ int const _Ch)
{
    return nullptr;
}

inline int __cdecl __strcmp(
    _In_z_ char const* _Str1,
    _In_z_ char const* _Str2
)
{
    return 0;
}

inline char*
__strncpy(char* dst, const char* src, size_t n)
{
    if (n != 0) {
        char* d = dst;
        const char* s = src;

        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                    *d++ = 0;
                break;
            }
        } while (--n != 0);
    }
    return (dst);
}

inline char* __strstr(const char*, const char*)
{
    return nullptr;
}

inline int __cdecl __strncmp(
    _In_reads_or_z_(_MaxCount) char const* _Str1,
    _In_reads_or_z_(_MaxCount) char const* _Str2,
    _In_                       size_t      _MaxCount
)
{
    return 0;
}

#define IMGUI_MEMCPY                    memcpy
#define IMGUI_MEMCMP                    memcmp
#define IMGUI_MEMSET                    memset
#define IMGUI_MEMMOVE                   memmove
#define IMGUI_MEMCHR                    __memchr
#define IMGUI_ZEROMEMORY(p, s)          IMGUI_MEMSET(p, 0, s);

#define IMGUI_STRCHR                    __strchr
#define IMGUI_STRCMP                    __strcmp
#define IMGUI_STRLEN                    strlen
#define IMGUI_STRNCPY                   __strncpy
#define IMGUI_STRNCMP                   __strncmp
#define IMGUI_STRSTR                    __strstr
#define IMGUI_SCANF                     __sscanf
#else
#define IMGUI_FABS(x)                   fabs(x)
#define IMGUI_FABSF(x)                  fabsf(x)
#define IMGUI_SQRT(x)                   sqrt(x)
#define IMGUI_SQRTF(x)                  sqrtf(x)
#define IMGUI_FMODF(x, y)               fmodf(x, y)
#define IMGUI_COSF(x)                   cosf(x)
#define IMGUI_SINF(x)                   sinf(x)
#define IMGUI_ACOSF(x)                  acosf(x)
#define IMGUI_ATAN2F(x, y)              atan2f(x, y)
#define IMGUI_ATOF(x)                   atof(x)
#define IMGUI_CEIL(x)                   ceilf(x)
#define IMGUI_POW(x, y)                 pow(x, y)
#define IMGUI_POWF(x, y)                powf(x, y)
#define IMGUI_LOG(x)                    log(x)
#define IMGUI_LOGF(x)                   logf(x)

#define IMGUI_MEMCPY                    memcpy
#define IMGUI_MEMCMP                    memcmp
#define IMGUI_MEMSET                    memset
#define IMGUI_MEMMOVE                   memmove
#define IMGUI_MEMCHR                    memchr
#define IMGUI_ZEROMEMORY(p, s)          IMGUI_MEMSET(p, 0, s);

#define IMGUI_STRCHR                    strchr
#define IMGUI_STRCMP                    strcmp
#define IMGUI_STRLEN                    strlen
#define IMGUI_STRNCPY                   strncpy
#define IMGUI_STRNCMP                   strncmp
#define IMGUI_STRSTR                    strstr
#define IMGUI_SCANF                     sscanf

#define IMGUI_QSORT(base, count, sizeOfElements, compareFn)      qsort(base, count, sizeOfElements, compareFn)
#endif

//---- Use 32-bit vertex indices (default is 16-bit) is one way to allow large meshes with more than 64K vertices.
// Your renderer backend will need to support it (most example renderer backends support both 16/32-bit indices).
// Another way to allow large meshes while keeping 16-bit indices is to handle ImDrawCmd::VtxOffset in your renderer.
// Read about ImGuiBackendFlags_RendererHasVtxOffset for details.
//#define ImDrawIdx unsigned int

//---- Override ImDrawCallback signature (will need to modify renderer backends accordingly)
//struct ImDrawList;
//struct ImDrawCmd;
//typedef void (*MyImDrawCallback)(const ImDrawList* draw_list, const ImDrawCmd* cmd, void* my_renderer_user_data);
//#define ImDrawCallback MyImDrawCallback

//---- Debug Tools: Macro to break in Debugger (we provide a default implementation of this in the codebase)
// (use 'Metrics->Tools->Item Picker' to pick widgets with the mouse and break into them for easy debugging.)
//#define IM_DEBUG_BREAK  IM_ASSERT(0)
//#define IM_DEBUG_BREAK  __debugbreak()

//---- Debug Tools: Enable slower asserts
//#define IMGUI_DEBUG_PARANOID

//---- Tip: You can add extra functions within the ImGui:: namespace from anywhere (e.g. your own sources/header files)
/*
namespace ImGui
{
    void MyFunction(const char* name, MyMatrix44* mtx);
}
*/


