// dear imgui, v1.90.5
// (drawing and font code)

/*

Index of this file:

// [SECTION] STB libraries implementation
// [SECTION] Style functions
// [SECTION] ImDrawList
// [SECTION] ImTriangulator, ImDrawList concave polygon fill
// [SECTION] ImDrawListSplitter
// [SECTION] ImDrawData
// [SECTION] Helpers ShadeVertsXXX functions
// [SECTION] ImFontConfig
// [SECTION] ImFontAtlas
// [SECTION] ImFontAtlas glyph ranges helpers
// [SECTION] ImFontGlyphRangesBuilder
// [SECTION] ImFont
// [SECTION] ImGui Internal Render Helpers
// [SECTION] Decompression code
// [SECTION] Default font data (ProggyClean.ttf)

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif

#include <stdio.h>      // vsnprintf, sscanf, printf

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4505)     // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#pragma warning (disable: 26451)    // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#pragma warning (disable: 26812)    // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3). [MSVC Static Analyzer)
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors"            // warning: declaration requires a global destructor         // similar to above, not sure what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wcomma"                          // warning: possible misuse of comma operator here
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wreserved-identifier"            // warning: identifier '_Xxx' is reserved because it starts with '_' followed by a capital letter
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wstack-protector"          // warning: stack protector not protecting local variables: variable length buffer
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif

//-------------------------------------------------------------------------
// [SECTION] STB libraries implementation (for stb_truetype and stb_rect_pack)
//-------------------------------------------------------------------------

// Compile time options:
//#define IMGUI_STB_NAMESPACE           ImStb
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#pragma warning (disable: 6011)                             // (stb_rectpack) Dereferencing NULL pointer 'cur->next'.
#pragma warning (disable: 6385)                             // (stb_truetype) Reading invalid data from 'buffer':  the readable size is '_Old_3`kernel_width' bytes, but '3' bytes may be read.
#pragma warning (disable: 28182)                            // (stb_rectpack) Dereferencing NULL pointer. 'cur' contains the same NULL value as 'cur->next' did.
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wcast-qual"              // warning: cast from 'const xxxx *' to 'xxx *' drops const qualifier
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"              // warning: comparison is always true due to limited range of data type [-Wtype-limits]
#pragma GCC diagnostic ignored "-Wcast-qual"                // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#endif

#ifndef STB_RECT_PACK_IMPLEMENTATION                        // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION          // in case the user already have an implementation in another compilation unit
#define STBRP_STATIC
#define STBRP_ASSERT(x)     do { IM_ASSERT(x); } while (0)
#define STBRP_SORT          ImQsort
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_RECT_PACK_FILENAME
#include IMGUI_STB_RECT_PACK_FILENAME
#else
#include "imstb_rectpack.h"
#endif
#endif

#ifdef  IMGUI_ENABLE_STB_TRUETYPE
#ifndef STB_TRUETYPE_IMPLEMENTATION                         // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION           // in case the user already have an implementation in another compilation unit
#define STBTT_malloc(x,u)   ((void)(u), IM_ALLOC(x))
#define STBTT_free(x,u)     ((void)(u), IM_FREE(x))
#define STBTT_assert(x)     do { IM_ASSERT(x); } while(0)
#define STBTT_fmod(x,y)     ImFmod(x,y)
#define STBTT_sqrt(x)       ImSqrt(x)
#define STBTT_pow(x,y)      ImPow(x,y)
#define STBTT_fabs(x)       ImFabs(x)
#define STBTT_ifloor(x)     ((int)ImFloor(x))
#define STBTT_iceil(x)      ((int)ImCeil(x))
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#ifdef IMGUI_STB_TRUETYPE_FILENAME
#include IMGUI_STB_TRUETYPE_FILENAME
#else
#include "imstb_truetype.h"
#endif
#endif
#endif // IMGUI_ENABLE_STB_TRUETYPE

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// [SECTION] Style functions
//-----------------------------------------------------------------------------

void ImGui::StyleColorsDark(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGui::StyleColorsClassic(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

// Those light colors are better suited with a thicker font than the default one + FrameBorder
void ImGui::StyleColorsLight(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.90f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawList
//-----------------------------------------------------------------------------

ImDrawListSharedData::ImDrawListSharedData()
{
    IMGUI_MEMSET(this, 0, sizeof(*this));
    for (int i = 0; i < IM_ARRAYSIZE(ArcFastVtx); i++)
    {
        const float a = ((float)i * 2 * IM_PI) / (float)IM_ARRAYSIZE(ArcFastVtx);
        ArcFastVtx[i] = ImVec2(ImCos(a), ImSin(a));
    }
    ArcFastRadiusCutoff = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_R(IM_DRAWLIST_ARCFAST_SAMPLE_MAX, CircleSegmentMaxError);
}

void ImDrawListSharedData::SetCircleTessellationMaxError(float max_error)
{
    if (CircleSegmentMaxError == max_error)
        return;

    IM_ASSERT(max_error > 0.0f);
    CircleSegmentMaxError = max_error;
    for (int i = 0; i < IM_ARRAYSIZE(CircleSegmentCounts); i++)
    {
        const float radius = (float)i;
        CircleSegmentCounts[i] = (ImU8)((i > 0) ? IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, CircleSegmentMaxError) : IM_DRAWLIST_ARCFAST_SAMPLE_MAX);
    }
    ArcFastRadiusCutoff = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_R(IM_DRAWLIST_ARCFAST_SAMPLE_MAX, CircleSegmentMaxError);
}

// Initialize before use in a new frame. We always have a command ready in the buffer.
void ImDrawList::_ResetForNewFrame()
{
    // Verify that the ImDrawCmd fields we want to IMGUI_MEMCMP() are contiguous in memory.
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, ClipRect) == 0);
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, TextureId) == sizeof(ImVec4));
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, VtxOffset) == sizeof(ImVec4) + sizeof(ImTextureID));
    if (_Splitter._Count > 1)
        _Splitter.Merge(this);

    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    Flags = _Data->InitialFlags;
    IMGUI_MEMSET(&_CmdHeader, 0, sizeof(_CmdHeader));
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureIdStack.resize(0);
    _Path.resize(0);
    _Splitter.Clear();
    CmdBuffer.push_back(ImDrawCmd());
    _FringeScale = 1.0f;
}

void ImDrawList::_ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    Flags = ImDrawListFlags_None;
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _Path.clear();
    _Splitter.ClearFreeMemory();
}

ImDrawList* ImDrawList::CloneOutput() const
{
    ImDrawList* dst = IM_NEW(ImDrawList(_Data));
    dst->CmdBuffer = CmdBuffer;
    dst->IdxBuffer = IdxBuffer;
    dst->VtxBuffer = VtxBuffer;
    dst->Flags = Flags;
    return dst;
}

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = _CmdHeader.ClipRect;    // Same as calling ImDrawCmd_HeaderCopy()
    draw_cmd.TextureId = _CmdHeader.TextureId;
    draw_cmd.VtxOffset = _CmdHeader.VtxOffset;
    draw_cmd.IdxOffset = IdxBuffer.Size;

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

// Pop trailing draw command (used before merging or presenting to user)
// Note that this leaves the ImDrawList in a state unfit for further commands, as most code assume that CmdBuffer.Size > 0 && CmdBuffer.back().UserCallback == NULL
void ImDrawList::_PopUnusedDrawCmd()
{
    while (CmdBuffer.Size > 0)
    {
        ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
        if (curr_cmd->ElemCount != 0 || curr_cmd->UserCallback != NULL)
            return;// break;
        CmdBuffer.pop_back();
    }
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* callback_data)
{
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    }
    curr_cmd->UserCallback = callback;
    curr_cmd->UserCallbackData = callback_data;

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Compare ClipRect, TextureId and VtxOffset with a single IMGUI_MEMCMP()
#define ImDrawCmd_HeaderSize                            (offsetof(ImDrawCmd, VtxOffset) + sizeof(unsigned int))
#define ImDrawCmd_HeaderCompare(CMD_LHS, CMD_RHS)       (IMGUI_MEMCMP(CMD_LHS, CMD_RHS, ImDrawCmd_HeaderSize))    // Compare ClipRect, TextureId, VtxOffset
#define ImDrawCmd_HeaderCopy(CMD_DST, CMD_SRC)          (IMGUI_MEMCPY(CMD_DST, CMD_SRC, ImDrawCmd_HeaderSize))    // Copy ClipRect, TextureId, VtxOffset
#define ImDrawCmd_AreSequentialIdxOffset(CMD_0, CMD_1)  (CMD_0->IdxOffset + CMD_0->ElemCount == CMD_1->IdxOffset)

// Try to merge two last draw commands
void ImDrawList::_TryMergeDrawCmds()
{
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (ImDrawCmd_HeaderCompare(curr_cmd, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && curr_cmd->UserCallback == NULL && prev_cmd->UserCallback == NULL)
    {
        prev_cmd->ElemCount += curr_cmd->ElemCount;
        CmdBuffer.pop_back();
    }
}

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::_OnChangedClipRect()
{
    // If current command is used with different settings we need to add a new command
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && IMGUI_MEMCMP(&curr_cmd->ClipRect, &_CmdHeader.ClipRect, sizeof(ImVec4)) != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }

    curr_cmd->ClipRect = _CmdHeader.ClipRect;
}

void ImDrawList::_OnChangedTextureID()
{
    // If current command is used with different settings we need to add a new command
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != _CmdHeader.TextureId)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }

    curr_cmd->TextureId = _CmdHeader.TextureId;
}

void ImDrawList::_OnChangedVtxOffset()
{
    // We don't need to compare curr_cmd->VtxOffset != _CmdHeader.VtxOffset because we know it'll be different at the time we call this.
    _VtxCurrentIdx = 0;
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    //IM_ASSERT(curr_cmd->VtxOffset != _CmdHeader.VtxOffset); // See #3349
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    curr_cmd->VtxOffset = _CmdHeader.VtxOffset;
}

int ImDrawList::_CalcCircleAutoSegmentCount(float radius) const
{
    // Automatic segment count
    const int radius_idx = (int)(radius + 0.999999f); // ceil to never reduce accuracy
    if (radius_idx >= 0 && radius_idx < IM_ARRAYSIZE(_Data->CircleSegmentCounts))
        return _Data->CircleSegmentCounts[radius_idx]; // Use cached value
    else
        return IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, _Data->CircleSegmentMaxError);
}

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(const ImVec2& cr_min, const ImVec2& cr_max, bool intersect_with_current_clip_rect)
{
    ImVec4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
    if (intersect_with_current_clip_rect)
    {
        ImVec4 current = _CmdHeader.ClipRect;
        if (cr.x < current.x) cr.x = current.x;
        if (cr.y < current.y) cr.y = current.y;
        if (cr.z > current.z) cr.z = current.z;
        if (cr.w > current.w) cr.w = current.w;
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    _ClipRectStack.push_back(cr);
    _CmdHeader.ClipRect = cr;
    _OnChangedClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(ImVec2(_Data->ClipRectFullscreen.x, _Data->ClipRectFullscreen.y), ImVec2(_Data->ClipRectFullscreen.z, _Data->ClipRectFullscreen.w));
}

void ImDrawList::PopClipRect()
{
    _ClipRectStack.pop_back();
    _CmdHeader.ClipRect = (_ClipRectStack.Size == 0) ? _Data->ClipRectFullscreen : _ClipRectStack.Data[_ClipRectStack.Size - 1];
    _OnChangedClipRect();
}

void ImDrawList::PushTextureID(ImTextureID texture_id)
{
    _TextureIdStack.push_back(texture_id);
    _CmdHeader.TextureId = texture_id;
    _OnChangedTextureID();
}

void ImDrawList::PopTextureID()
{
    _TextureIdStack.pop_back();
    _CmdHeader.TextureId = (_TextureIdStack.Size == 0) ? (ImTextureID)NULL : _TextureIdStack.Data[_TextureIdStack.Size - 1];
    _OnChangedTextureID();
}

// Reserve space for a number of vertices and indices.
// You must finish filling your reserved data before calling PrimReserve() again, as it may reallocate or
// submit the intermediate results. PrimUnreserve() can be used to release unused allocations.
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    // Large mesh support (when enabled)
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);
    if (sizeof(ImDrawIdx) == 2 && (_VtxCurrentIdx + vtx_count >= (1 << 16)) && (Flags & ImDrawListFlags_AllowVtxOffset))
    {
        // FIXME: In theory we should be testing that vtx_count <64k here.
        // In practice, RenderText() relies on reserving ahead for a worst case scenario so it is currently useful for us
        // to not make that check until we rework the text functions to handle clipping and large horizontal lines better.
        _CmdHeader.VtxOffset = VtxBuffer.Size;
        _OnChangedVtxOffset();
    }

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount += idx_count;

    int vtx_buffer_old_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_old_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_old_size;

    int idx_buffer_old_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_old_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_old_size;
}

// Release the number of reserved vertices/indices from the end of the last reservation made with PrimReserve().
void ImDrawList::PrimUnreserve(int idx_count, int vtx_count)
{
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount -= idx_count;
    VtxBuffer.shrink(VtxBuffer.Size - vtx_count);
    IdxBuffer.shrink(IdxBuffer.Size - idx_count);
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv(_Data->TexUvWhitePixel);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// On AddPolyline() and AddConvexPolyFilled() we intentionally avoid using ImVec2 and superfluous function calls to optimize debug/non-inlined builds.
// - Those macros expects l-values and need to be used as their own statement.
// - Those macros are intentionally not surrounded by the 'do {} while (0)' idiom because even that translates to runtime with debug compilers.
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)     { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = ImRsqrt(d2); VX *= inv_len; VY *= inv_len; } } (void)0
#define IM_FIXNORMAL2F_MAX_INVLEN2          100.0f // 500.0f (see #4053, #3366)
#define IM_FIXNORMAL2F(VX,VY)               { float d2 = VX*VX + VY*VY; if (d2 > 0.000001f) { float inv_len2 = 1.0f / d2; if (inv_len2 > IM_FIXNORMAL2F_MAX_INVLEN2) inv_len2 = IM_FIXNORMAL2F_MAX_INVLEN2; VX *= inv_len2; VY *= inv_len2; } } (void)0

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
// We avoid using the ImVec2 math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, ImDrawFlags flags, float thickness)
{
    if (points_count < 2 || (col & IM_COL32_A_MASK) == 0)
        return;

    const bool closed = (flags & ImDrawFlags_Closed) != 0;
    const ImVec2 opaque_uv = _Data->TexUvWhitePixel;
    const int count = closed ? points_count : points_count - 1; // The number of line segments we need to draw
    const bool thick_line = (thickness > _FringeScale);

    if (Flags & ImDrawListFlags_AntiAliasedLines)
    {
        // Anti-aliased stroke
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;

        // Thicknesses <1.0 should behave like thickness 1.0
        thickness = ImMax(thickness, 1.0f);
        const int integer_thickness = (int)thickness;
        const float fractional_thickness = thickness - integer_thickness;

        // Do we want to draw this line using a texture?
        // - For now, only draw integer-width lines using textures to avoid issues with the way scaling occurs, could be improved.
        // - If AA_SIZE is not 1.0f we cannot use the texture path.
        const bool use_texture = (Flags & ImDrawListFlags_AntiAliasedLinesUseTex) && (integer_thickness < IM_DRAWLIST_TEX_LINES_WIDTH_MAX) && (fractional_thickness <= 0.00001f) && (AA_SIZE == 1.0f);

        // We should never hit this, because NewFrame() doesn't set ImDrawListFlags_AntiAliasedLinesUseTex unless ImFontAtlasFlags_NoBakedLines is off
        IM_ASSERT_PARANOID(!use_texture || !(_Data->Font->ContainerAtlas->Flags & ImFontAtlasFlags_NoBakedLines));

        const int idx_count = use_texture ? (count * 6) : (thick_line ? count * 18 : count * 12);
        const int vtx_count = use_texture ? (points_count * 2) : (thick_line ? points_count * 4 : points_count * 3);
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        // The first <points_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
        _Data->TempBuffer.reserve_discard(points_count * ((use_texture || !thick_line) ? 3 : 5));
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        ImVec2* temp_points = temp_normals + points_count;

        // Calculate normals (tangents) for each line segment
        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            float dx = points[i2].x - points[i1].x;
            float dy = points[i2].y - points[i1].y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i1].x = dy;
            temp_normals[i1].y = -dx;
        }
        if (!closed)
            temp_normals[points_count - 1] = temp_normals[points_count - 2];

        // If we are drawing a one-pixel-wide line without a texture, or a textured line of any width, we only need 2 or 3 vertices per point
        if (use_texture || !thick_line)
        {
            // [PATH 1] Texture-based lines (thick or non-thick)
            // [PATH 2] Non texture-based lines (non-thick)

            // The width of the geometry we need to draw - this is essentially <thickness> pixels for the line itself, plus "one pixel" for AA.
            // - In the texture-based path, we don't use AA_SIZE here because the +1 is tied to the generated texture
            //   (see ImFontAtlasBuildRenderLinesTexData() function), and so alternate values won't work without changes to that code.
            // - In the non texture-based paths, we would allow AA_SIZE to potentially be != 1.0f with a patch (e.g. fringe_scale patch to
            //   allow scaling geometry while preserving one-screen-pixel AA fringe).
            const float half_draw_size = use_texture ? ((thickness * 0.5f) + 1) : AA_SIZE;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * half_draw_size;
                temp_points[1] = points[0] - temp_normals[0] * half_draw_size;
                temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * half_draw_size;
                temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * half_draw_size;
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1; // i2 is the second point of the line segment
                const unsigned int idx2 = ((i1 + 1) == points_count) ? _VtxCurrentIdx : (idx1 + (use_texture ? 2 : 3)); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                dm_x *= half_draw_size; // dm_x, dm_y are offset to the outer edge of the AA area
                dm_y *= half_draw_size;

                // Add temporary vertexes for the outer edges
                ImVec2* out_vtx = &temp_points[i2 * 2];
                out_vtx[0].x = points[i2].x + dm_x;
                out_vtx[0].y = points[i2].y + dm_y;
                out_vtx[1].x = points[i2].x - dm_x;
                out_vtx[1].y = points[i2].y - dm_y;

                if (use_texture)
                {
                    // Add indices for two triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 1); // Right tri
                    _IdxWritePtr[3] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[4] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Left tri
                    _IdxWritePtr += 6;
                }
                else
                {
                    // Add indexes for four triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2); // Right tri 1
                    _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Right tri 2
                    _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0); // Left tri 1
                    _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1); // Left tri 2
                    _IdxWritePtr += 12;
                }

                idx1 = idx2;
            }

            // Add vertexes for each point on the line
            if (use_texture)
            {
                // If we're using textures we only need to emit the left/right edge vertices
                ImVec4 tex_uvs = _Data->TexUvLines[integer_thickness];
                /*if (fractional_thickness != 0.0f) // Currently always zero when use_texture==false!
                {
                    const ImVec4 tex_uvs_1 = _Data->TexUvLines[integer_thickness + 1];
                    tex_uvs.x = tex_uvs.x + (tex_uvs_1.x - tex_uvs.x) * fractional_thickness; // inlined ImLerp()
                    tex_uvs.y = tex_uvs.y + (tex_uvs_1.y - tex_uvs.y) * fractional_thickness;
                    tex_uvs.z = tex_uvs.z + (tex_uvs_1.z - tex_uvs.z) * fractional_thickness;
                    tex_uvs.w = tex_uvs.w + (tex_uvs_1.w - tex_uvs.w) * fractional_thickness;
                }*/
                ImVec2 tex_uv0(tex_uvs.x, tex_uvs.y);
                ImVec2 tex_uv1(tex_uvs.z, tex_uvs.w);
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = temp_points[i * 2 + 0]; _VtxWritePtr[0].uv = tex_uv0; _VtxWritePtr[0].col = col; // Left-side outer edge
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 1]; _VtxWritePtr[1].uv = tex_uv1; _VtxWritePtr[1].col = col; // Right-side outer edge
                    _VtxWritePtr += 2;
                }
            }
            else
            {
                // If we're not using a texture, we need the center vertex as well
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = points[i];              _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;       // Center of line
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 0]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col_trans; // Left-side outer edge
                    _VtxWritePtr[2].pos = temp_points[i * 2 + 1]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col_trans; // Right-side outer edge
                    _VtxWritePtr += 3;
                }
            }
        }
        else
        {
            // [PATH 2] Non texture-based lines (thick): we need to draw the solid line core and thus require four vertices per point
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                const int points_last = points_count - 1;
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 0] = points[points_last] + temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 1] = points[points_last] + temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 2] = points[points_last] - temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 3] = points[points_last] - temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : (i1 + 1); // i2 is the second point of the line segment
                const unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : (idx1 + 4); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                float dm_in_x = dm_x * half_inner_thickness;
                float dm_in_y = dm_y * half_inner_thickness;

                // Add temporary vertices
                ImVec2* out_vtx = &temp_points[i2 * 4];
                out_vtx[0].x = points[i2].x + dm_out_x;
                out_vtx[0].y = points[i2].y + dm_out_y;
                out_vtx[1].x = points[i2].x + dm_in_x;
                out_vtx[1].y = points[i2].y + dm_in_y;
                out_vtx[2].x = points[i2].x - dm_in_x;
                out_vtx[2].y = points[i2].y - dm_in_y;
                out_vtx[3].x = points[i2].x - dm_out_x;
                out_vtx[3].y = points[i2].y - dm_out_y;

                // Add indexes
                _IdxWritePtr[0]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[1]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[2]  = (ImDrawIdx)(idx1 + 2);
                _IdxWritePtr[3]  = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4]  = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5]  = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[6]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8]  = (ImDrawIdx)(idx1 + 0);
                _IdxWritePtr[9]  = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[13] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[14] = (ImDrawIdx)(idx1 + 3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1 + 3); _IdxWritePtr[16] = (ImDrawIdx)(idx2 + 3); _IdxWritePtr[17] = (ImDrawIdx)(idx2 + 2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertices
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = temp_points[i * 4 + 0]; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i * 4 + 1]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i * 4 + 2]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i * 4 + 3]; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // [PATH 4] Non texture-based, Non anti-aliased lines
        const int idx_count = count * 6;
        const int vtx_count = count * 4;    // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            dx *= (thickness * 0.5f);
            dy *= (thickness * 0.5f);

            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + 2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx + 2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx + 3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

// - We intentionally avoid using ImVec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
// - Filled shapes must always use clockwise winding order. The anti-aliasing fringe depends on it. Counter-clockwise shapes will have "inward" anti-aliasing.
void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3 || (col & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2)*3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + ((i - 1) << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (i << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        _Data->TempBuffer.reserve_discard(points_count);
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2)*3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + i - 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::_PathArcToFastEx(const ImVec2& center, float radius, int a_min_sample, int a_max_sample, int a_step)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    // Calculate arc auto segment step size
    if (a_step <= 0)
        a_step = IM_DRAWLIST_ARCFAST_SAMPLE_MAX / _CalcCircleAutoSegmentCount(radius);

    // Make sure we never do steps larger than one quarter of the circle
    a_step = ImClamp(a_step, 1, IM_DRAWLIST_ARCFAST_TABLE_SIZE / 4);

    const int sample_range = ImAbs(a_max_sample - a_min_sample);
    const int a_next_step = a_step;

    int samples = sample_range + 1;
    bool extra_max_sample = false;
    if (a_step > 1)
    {
        samples            = sample_range / a_step + 1;
        const int overstep = sample_range % a_step;

        if (overstep > 0)
        {
            extra_max_sample = true;
            samples++;

            // When we have overstep to avoid awkwardly looking one long line and one tiny one at the end,
            // distribute first step range evenly between them by reducing first step size.
            if (sample_range > 0)
                a_step -= (a_step - overstep) / 2;
        }
    }

    _Path.resize(_Path.Size + samples);
    ImVec2* out_ptr = _Path.Data + (_Path.Size - samples);

    int sample_index = a_min_sample;
    if (sample_index < 0 || sample_index >= IM_DRAWLIST_ARCFAST_SAMPLE_MAX)
    {
        sample_index = sample_index % IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        if (sample_index < 0)
            sample_index += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
    }

    if (a_max_sample >= a_min_sample)
    {
        for (int a = a_min_sample; a <= a_max_sample; a += a_step, sample_index += a_step, a_step = a_next_step)
        {
            // a_step is clamped to IM_DRAWLIST_ARCFAST_SAMPLE_MAX, so we have guaranteed that it will not wrap over range twice or more
            if (sample_index >= IM_DRAWLIST_ARCFAST_SAMPLE_MAX)
                sample_index -= IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

            const ImVec2 s = _Data->ArcFastVtx[sample_index];
            out_ptr->x = center.x + s.x * radius;
            out_ptr->y = center.y + s.y * radius;
            out_ptr++;
        }
    }
    else
    {
        for (int a = a_min_sample; a >= a_max_sample; a -= a_step, sample_index -= a_step, a_step = a_next_step)
        {
            // a_step is clamped to IM_DRAWLIST_ARCFAST_SAMPLE_MAX, so we have guaranteed that it will not wrap over range twice or more
            if (sample_index < 0)
                sample_index += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

            const ImVec2 s = _Data->ArcFastVtx[sample_index];
            out_ptr->x = center.x + s.x * radius;
            out_ptr->y = center.y + s.y * radius;
            out_ptr++;
        }
    }

    if (extra_max_sample)
    {
        int normalized_max_sample = a_max_sample % IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        if (normalized_max_sample < 0)
            normalized_max_sample += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

        const ImVec2 s = _Data->ArcFastVtx[normalized_max_sample];
        out_ptr->x = center.x + s.x * radius;
        out_ptr->y = center.y + s.y * radius;
        out_ptr++;
    }

    IM_ASSERT_PARANOID(_Path.Data + _Path.Size == out_ptr);
}

void ImDrawList::_PathArcToN(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    // Note that we are adding a point at both a_min and a_max.
    // If you are trying to draw a full closed circle you don't want the overlapping points!
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        _Path.push_back(ImVec2(center.x + ImCos(a) * radius, center.y + ImSin(a) * radius));
    }
}

// 0: East, 3: South, 6: West, 9: North, 12: East
void ImDrawList::PathArcToFast(const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }
    _PathArcToFastEx(center, radius, a_min_of_12 * IM_DRAWLIST_ARCFAST_SAMPLE_MAX / 12, a_max_of_12 * IM_DRAWLIST_ARCFAST_SAMPLE_MAX / 12, 0);
}

void ImDrawList::PathArcTo(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    if (num_segments > 0)
    {
        _PathArcToN(center, radius, a_min, a_max, num_segments);
        return;
    }

    // Automatic segment count
    if (radius <= _Data->ArcFastRadiusCutoff)
    {
        const bool a_is_reverse = a_max < a_min;

        // We are going to use precomputed values for mid samples.
        // Determine first and last sample in lookup table that belong to the arc.
        const float a_min_sample_f = IM_DRAWLIST_ARCFAST_SAMPLE_MAX * a_min / (IM_PI * 2.0f);
        const float a_max_sample_f = IM_DRAWLIST_ARCFAST_SAMPLE_MAX * a_max / (IM_PI * 2.0f);

        const int a_min_sample = a_is_reverse ? (int)ImFloor(a_min_sample_f) : (int)ImCeil(a_min_sample_f);
        const int a_max_sample = a_is_reverse ? (int)ImCeil(a_max_sample_f) : (int)ImFloor(a_max_sample_f);
        const int a_mid_samples = a_is_reverse ? ImMax(a_min_sample - a_max_sample, 0) : ImMax(a_max_sample - a_min_sample, 0);

        const float a_min_segment_angle = a_min_sample * IM_PI * 2.0f / IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        const float a_max_segment_angle = a_max_sample * IM_PI * 2.0f / IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        const bool a_emit_start = ImAbs(a_min_segment_angle - a_min) >= 1e-5f;
        const bool a_emit_end = ImAbs(a_max - a_max_segment_angle) >= 1e-5f;

        _Path.reserve(_Path.Size + (a_mid_samples + 1 + (a_emit_start ? 1 : 0) + (a_emit_end ? 1 : 0)));
        if (a_emit_start)
            _Path.push_back(ImVec2(center.x + ImCos(a_min) * radius, center.y + ImSin(a_min) * radius));
        if (a_mid_samples > 0)
            _PathArcToFastEx(center, radius, a_min_sample, a_max_sample, 0);
        if (a_emit_end)
            _Path.push_back(ImVec2(center.x + ImCos(a_max) * radius, center.y + ImSin(a_max) * radius));
    }
    else
    {
        const float arc_length = ImAbs(a_max - a_min);
        const int circle_segment_count = _CalcCircleAutoSegmentCount(radius);
        const int arc_segment_count = ImMax((int)ImCeil(circle_segment_count * arc_length / (IM_PI * 2.0f)), (int)(2.0f * IM_PI / arc_length));
        _PathArcToN(center, radius, a_min, a_max, arc_segment_count);
    }
}

void ImDrawList::PathEllipticalArcTo(const ImVec2& center, const ImVec2& radius, float rot, float a_min, float a_max, int num_segments)
{
    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    _Path.reserve(_Path.Size + (num_segments + 1));

    const float cos_rot = ImCos(rot);
    const float sin_rot = ImSin(rot);
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        ImVec2 point(ImCos(a) * radius.x, ImSin(a) * radius.y);
        const ImVec2 rel((point.x * cos_rot) - (point.y * sin_rot), (point.x * sin_rot) + (point.y * cos_rot));
        point.x = rel.x + center.x;
        point.y = rel.y + center.y;
        _Path.push_back(point);
    }
}

ImVec2 ImBezierCubicCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, float t)
{
    float u = 1.0f - t;
    float w1 = u * u * u;
    float w2 = 3 * u * u * t;
    float w3 = 3 * u * t * t;
    float w4 = t * t * t;
    return ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y);
}

ImVec2 ImBezierQuadraticCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, float t)
{
    float u = 1.0f - t;
    float w1 = u * u;
    float w2 = 2 * u * t;
    float w3 = t * t;
    return ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x, w1 * p1.y + w2 * p2.y + w3 * p3.y);
}

// Closely mimics ImBezierCubicClosestPointCasteljau() in imgui.cpp
static void PathBezierCubicCurveToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = (x2 - x4) * dy - (y2 - y4) * dx;
    float d3 = (x3 - x4) * dy - (y3 - y4) * dx;
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x4, y4));
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x34 = (x3 + x4) * 0.5f, y34 = (y3 + y4) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
        float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;
        PathBezierCubicCurveToCasteljau(path, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
        PathBezierCubicCurveToCasteljau(path, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
    }
}

static void PathBezierQuadraticCurveToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float tess_tol, int level)
{
    float dx = x3 - x1, dy = y3 - y1;
    float det = (x2 - x3) * dy - (y2 - y3) * dx;
    if (det * det * 4.0f < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x3, y3));
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        PathBezierQuadraticCurveToCasteljau(path, x1, y1, x12, y12, x123, y123, tess_tol, level + 1);
        PathBezierQuadraticCurveToCasteljau(path, x123, y123, x23, y23, x3, y3, tess_tol, level + 1);
    }
}

void ImDrawList::PathBezierCubicCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        IM_ASSERT(_Data->CurveTessellationTol > 0.0f);
        PathBezierCubicCurveToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data->CurveTessellationTol, 0); // Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierCubicCalc(p1, p2, p3, p4, t_step * i_step));
    }
}

void ImDrawList::PathBezierQuadraticCurveTo(const ImVec2& p2, const ImVec2& p3, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        IM_ASSERT(_Data->CurveTessellationTol > 0.0f);
        PathBezierQuadraticCurveToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, _Data->CurveTessellationTol, 0);// Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierQuadraticCalc(p1, p2, p3, t_step * i_step));
    }
}

static inline ImDrawFlags FixRectCornerFlags(ImDrawFlags flags)
{
    /*
    IM_STATIC_ASSERT(ImDrawFlags_RoundCornersTopLeft == (1 << 4));
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    // Obsoleted in 1.82 (from February 2021). This code was stripped/simplified and mostly commented in 1.90 (from September 2023)
    // - Legacy Support for hard coded ~0 (used to be a suggested equivalent to ImDrawCornerFlags_All)
    if (flags == ~0)                    { return ImDrawFlags_RoundCornersAll; }
    // - Legacy Support for hard coded 0x01 to 0x0F (matching 15 out of 16 old flags combinations). Read details in older version of this code.
    if (flags >= 0x01 && flags <= 0x0F) { return (flags << 4); }
    // We cannot support hard coded 0x00 with 'float rounding > 0.0f' --> replace with ImDrawFlags_RoundCornersNone or use 'float rounding = 0.0f'
#endif
    */
    // If this assert triggers, please update your code replacing hardcoded values with new ImDrawFlags_RoundCorners* values.
    // Note that ImDrawFlags_Closed (== 0x01) is an invalid flag for AddRect(), AddRectFilled(), PathRect() etc. anyway.
    // See details in 1.82 Changelog as well as 2021/03/12 and 2023/09/08 entries in "API BREAKING CHANGES" section.
    IM_ASSERT((flags & 0x0F) == 0 && "Misuse of legacy hardcoded ImDrawCornerFlags values!");

    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags |= ImDrawFlags_RoundCornersAll;

    return flags;
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, ImDrawFlags flags)
{
    if (rounding >= 0.5f)
    {
        flags = FixRectCornerFlags(flags);
        rounding = ImMin(rounding, ImFabsf(b.x - a.x) * (((flags & ImDrawFlags_RoundCornersTop) == ImDrawFlags_RoundCornersTop) || ((flags & ImDrawFlags_RoundCornersBottom) == ImDrawFlags_RoundCornersBottom) ? 0.5f : 1.0f) - 1.0f);
        rounding = ImMin(rounding, ImFabsf(b.y - a.y) * (((flags & ImDrawFlags_RoundCornersLeft) == ImDrawFlags_RoundCornersLeft) || ((flags & ImDrawFlags_RoundCornersRight) == ImDrawFlags_RoundCornersRight) ? 0.5f : 1.0f) - 1.0f);
    }
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x, a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x, b.y));
    }
    else
    {
        const float rounding_tl = (flags & ImDrawFlags_RoundCornersTopLeft)     ? rounding : 0.0f;
        const float rounding_tr = (flags & ImDrawFlags_RoundCornersTopRight)    ? rounding : 0.0f;
        const float rounding_br = (flags & ImDrawFlags_RoundCornersBottomRight) ? rounding : 0.0f;
        const float rounding_bl = (flags & ImDrawFlags_RoundCornersBottomLeft)  ? rounding : 0.0f;
        PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
        PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
        PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
        PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathLineTo(p1 + ImVec2(0.5f, 0.5f));
    PathLineTo(p2 + ImVec2(0.5f, 0.5f));
    PathStroke(col, 0, thickness);
}

// p_min = upper-left, p_max = lower-right
// Note we don't render 1 pixels sized rectangles properly.
void ImDrawList::AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (Flags & ImDrawListFlags_AntiAliasedLines)
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.50f, 0.50f), rounding, flags);
    else
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.49f, 0.49f), rounding, flags); // Better looking lower-right corner and rounded non-AA shapes.
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        PrimReserve(6, 4);
        PrimRect(p_min, p_max, col);
    }
    else
    {
        PathRect(p_min, p_max, rounding, flags);
        PathFillConvex(col);
    }
}

// p_min = upper-left, p_max = lower-right
void ImDrawList::AddRectFilledMultiColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 3));
    PrimWriteVtx(p_min, uv, col_upr_left);
    PrimWriteVtx(ImVec2(p_max.x, p_min.y), uv, col_upr_right);
    PrimWriteVtx(p_max, uv, col_bot_right);
    PrimWriteVtx(ImVec2(p_min.x, p_max.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathFillConvex(col);
}

void ImDrawList::AddTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathFillConvex(col);
}

void ImDrawList::AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    if (num_segments <= 0)
    {
        // Use arc with automatic segment count
        _PathArcToFastEx(center, radius - 0.5f, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
        _Path.Size--;
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);

        // Because we are filling a closed shape we remove 1 from the count of segments/points
        const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
        PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    }

    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    if (num_segments <= 0)
    {
        // Use arc with automatic segment count
        _PathArcToFastEx(center, radius, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
        _Path.Size--;
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);

        // Because we are filling a closed shape we remove 1 from the count of segments/points
        const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
        PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    }

    PathFillConvex(col);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgon(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Ellipse
void ImDrawList::AddEllipse(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius, rot, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddEllipseFilled(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius, rot, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Cubic Bezier takes 4 controls points
void ImDrawList::AddBezierCubic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierCubicCurveTo(p2, p3, p4, num_segments);
    PathStroke(col, 0, thickness);
}

// Quadratic Bezier takes 3 controls points
void ImDrawList::AddBezierQuadratic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierQuadraticCurveTo(p2, p3, num_segments);
    PathStroke(col, 0, thickness);
}

void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    // Accept null ranges
    if (text_begin == text_end || text_begin[0] == 0)
        return;
    if (text_end == NULL)
        text_end = text_begin + IMGUI_STRLEN(text_begin);

    // Pull default font/size from the shared ImDrawListSharedData instance
    if (font == NULL)
        font = _Data->Font;
    if (font_size == 0.0f)
        font_size = _Data->FontSize;

    IM_ASSERT(font->ContainerAtlas->TexID == _CmdHeader.TextureId);  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    ImVec4 clip_rect = _CmdHeader.ClipRect;
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(this, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != NULL);
}

void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    AddText(NULL, 0.0f, pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimRectUV(p_min, p_max, uv_min, uv_max, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimQuadUV(p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawFlags flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    flags = FixRectCornerFlags(flags);
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        AddImage(user_texture_id, p_min, p_max, uv_min, uv_max, col);
        return;
    }

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    int vert_start_idx = VtxBuffer.Size;
    PathRect(p_min, p_max, rounding, flags);
    PathFillConvex(col);
    int vert_end_idx = VtxBuffer.Size;
    ImGui::ShadeVertsLinearUV(this, vert_start_idx, vert_end_idx, p_min, p_max, uv_min, uv_max, true);

    if (push_texture_id)
        PopTextureID();
}

//-----------------------------------------------------------------------------
// [SECTION] ImTriangulator, ImDrawList concave polygon fill
//-----------------------------------------------------------------------------
// Triangulate concave polygons. Based on "Triangulation by Ear Clipping" paper, O(N^2) complexity.
// Reference: https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
// Provided as a convenience for user but not used by main library.
//-----------------------------------------------------------------------------
// - ImTriangulator [Internal]
// - AddConcavePolyFilled()
//-----------------------------------------------------------------------------

enum ImTriangulatorNodeType
{
    ImTriangulatorNodeType_Convex,
    ImTriangulatorNodeType_Ear,
    ImTriangulatorNodeType_Reflex
};

struct ImTriangulatorNode
{
    ImTriangulatorNodeType  Type;
    int                     Index;
    ImVec2                  Pos;
    ImTriangulatorNode*     Next;
    ImTriangulatorNode*     Prev;

    void    Unlink()        { Next->Prev = Prev; Prev->Next = Next; }
};

struct ImTriangulatorNodeSpan
{
    ImTriangulatorNode**    Data = NULL;
    int                     Size = 0;

    void    push_back(ImTriangulatorNode* node) { Data[Size++] = node; }
    void    find_erase_unsorted(int idx)        { for (int i = Size - 1; i >= 0; i--) if (Data[i]->Index == idx) { Data[i] = Data[Size - 1]; Size--; return; } }
};

struct ImTriangulator
{
    static int EstimateTriangleCount(int points_count)      { return (points_count < 3) ? 0 : points_count - 2; }
    static int EstimateScratchBufferSize(int points_count)  { return sizeof(ImTriangulatorNode) * points_count + sizeof(ImTriangulatorNode*) * points_count * 2; }

    void    Init(const ImVec2* points, int points_count, void* scratch_buffer);
    void    GetNextTriangle(unsigned int out_triangle[3]);     // Return relative indexes for next triangle

    // Internal functions
    void    BuildNodes(const ImVec2* points, int points_count);
    void    BuildReflexes();
    void    BuildEars();
    void    FlipNodeList();
    bool    IsEar(int i0, int i1, int i2, const ImVec2& v0, const ImVec2& v1, const ImVec2& v2) const;
    void    ReclassifyNode(ImTriangulatorNode* node);

    // Internal members
    int                     _TrianglesLeft = 0;
    ImTriangulatorNode*     _Nodes = NULL;
    ImTriangulatorNodeSpan  _Ears;
    ImTriangulatorNodeSpan  _Reflexes;
};

// Distribute storage for nodes, ears and reflexes.
// FIXME-OPT: if everything is convex, we could report it to caller and let it switch to an convex renderer
// (this would require first building reflexes to bail to convex if empty, without even building nodes)
void ImTriangulator::Init(const ImVec2* points, int points_count, void* scratch_buffer)
{
    IM_ASSERT(scratch_buffer != NULL && points_count >= 3);
    _TrianglesLeft = EstimateTriangleCount(points_count);
    _Nodes         = (ImTriangulatorNode*)scratch_buffer;                          // points_count x Node
    _Ears.Data     = (ImTriangulatorNode**)(_Nodes + points_count);                // points_count x Node*
    _Reflexes.Data = (ImTriangulatorNode**)(_Nodes + points_count) + points_count; // points_count x Node*
    BuildNodes(points, points_count);
    BuildReflexes();
    BuildEars();
}

void ImTriangulator::BuildNodes(const ImVec2* points, int points_count)
{
    for (int i = 0; i < points_count; i++)
    {
        _Nodes[i].Type = ImTriangulatorNodeType_Convex;
        _Nodes[i].Index = i;
        _Nodes[i].Pos = points[i];
        _Nodes[i].Next = _Nodes + i + 1;
        _Nodes[i].Prev = _Nodes + i - 1;
    }
    _Nodes[0].Prev = _Nodes + points_count - 1;
    _Nodes[points_count - 1].Next = _Nodes;
}

void ImTriangulator::BuildReflexes()
{
    ImTriangulatorNode* n1 = _Nodes;
    for (int i = _TrianglesLeft; i >= 0; i--, n1 = n1->Next)
    {
        if (ImTriangleIsClockwise(n1->Prev->Pos, n1->Pos, n1->Next->Pos))
            continue;
        n1->Type = ImTriangulatorNodeType_Reflex;
        _Reflexes.push_back(n1);
    }
}

void ImTriangulator::BuildEars()
{
    ImTriangulatorNode* n1 = _Nodes;
    for (int i = _TrianglesLeft; i >= 0; i--, n1 = n1->Next)
    {
        if (n1->Type != ImTriangulatorNodeType_Convex)
            continue;
        if (!IsEar(n1->Prev->Index, n1->Index, n1->Next->Index, n1->Prev->Pos, n1->Pos, n1->Next->Pos))
            continue;
        n1->Type = ImTriangulatorNodeType_Ear;
        _Ears.push_back(n1);
    }
}

void ImTriangulator::GetNextTriangle(unsigned int out_triangle[3])
{
    if (_Ears.Size == 0)
    {
        FlipNodeList();

        ImTriangulatorNode* node = _Nodes;
        for (int i = _TrianglesLeft; i >= 0; i--, node = node->Next)
            node->Type = ImTriangulatorNodeType_Convex;
        _Reflexes.Size = 0;
        BuildReflexes();
        BuildEars();

        // If we still don't have ears, it means geometry is degenerated.
        if (_Ears.Size == 0)
        {
            // Return first triangle available, mimicking the behavior of convex fill.
            IM_ASSERT(_TrianglesLeft > 0); // Geometry is degenerated
            _Ears.Data[0] = _Nodes;
            _Ears.Size    = 1;
        }
    }

    ImTriangulatorNode* ear = _Ears.Data[--_Ears.Size];
    out_triangle[0] = ear->Prev->Index;
    out_triangle[1] = ear->Index;
    out_triangle[2] = ear->Next->Index;

    ear->Unlink();
    if (ear == _Nodes)
        _Nodes = ear->Next;

    ReclassifyNode(ear->Prev);
    ReclassifyNode(ear->Next);
    _TrianglesLeft--;
}

void ImTriangulator::FlipNodeList()
{
    ImTriangulatorNode* prev = _Nodes;
    ImTriangulatorNode* temp = _Nodes;
    ImTriangulatorNode* current = _Nodes->Next;
    prev->Next = prev;
    prev->Prev = prev;
    while (current != _Nodes)
    {
        temp = current->Next;

        current->Next = prev;
        prev->Prev = current;
        _Nodes->Next = current;
        current->Prev = _Nodes;

        prev = current;
        current = temp;
    }
    _Nodes = prev;
}

// A triangle is an ear is no other vertex is inside it. We can test reflexes vertices only (see reference algorithm)
bool ImTriangulator::IsEar(int i0, int i1, int i2, const ImVec2& v0, const ImVec2& v1, const ImVec2& v2) const
{
    ImTriangulatorNode** p_end = _Reflexes.Data + _Reflexes.Size;
    for (ImTriangulatorNode** p = _Reflexes.Data; p < p_end; p++)
    {
        ImTriangulatorNode* reflex = *p;
        if (reflex->Index != i0 && reflex->Index != i1 && reflex->Index != i2)
            if (ImTriangleContainsPoint(v0, v1, v2, reflex->Pos))
                return false;
    }
    return true;
}

void ImTriangulator::ReclassifyNode(ImTriangulatorNode* n1)
{
    // Classify node
    ImTriangulatorNodeType type;
    const ImTriangulatorNode* n0 = n1->Prev;
    const ImTriangulatorNode* n2 = n1->Next;
    if (!ImTriangleIsClockwise(n0->Pos, n1->Pos, n2->Pos))
        type = ImTriangulatorNodeType_Reflex;
    else if (IsEar(n0->Index, n1->Index, n2->Index, n0->Pos, n1->Pos, n2->Pos))
        type = ImTriangulatorNodeType_Ear;
    else
        type = ImTriangulatorNodeType_Convex;

    // Update lists when a type changes
    if (type == n1->Type)
        return;
    if (n1->Type == ImTriangulatorNodeType_Reflex)
        _Reflexes.find_erase_unsorted(n1->Index);
    else if (n1->Type == ImTriangulatorNodeType_Ear)
        _Ears.find_erase_unsorted(n1->Index);
    if (type == ImTriangulatorNodeType_Reflex)
        _Reflexes.push_back(n1);
    else if (type == ImTriangulatorNodeType_Ear)
        _Ears.push_back(n1);
    n1->Type = type;
}

// Use ear-clipping algorithm to triangulate a simple polygon (no self-interaction, no holes).
// (Reminder: we don't perform any coarse clipping/culling in ImDrawList layer!
// It is up to caller to ensure not making costly calls that will be outside of visible area.
// As concave fill is noticeably more expensive than other primitives, be mindful of this...
// Caller can build AABB of points, and avoid filling if 'draw_list->_CmdHeader.ClipRect.Overlays(points_bb) == false')
void ImDrawList::AddConcavePolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3 || (col & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    ImTriangulator triangulator;
    unsigned int triangle[3];
    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2) * 3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;

        _Data->TempBuffer.reserve_discard((ImTriangulator::EstimateScratchBufferSize(points_count) + sizeof(ImVec2)) / sizeof(ImVec2));
        triangulator.Init(points, points_count, _Data->TempBuffer.Data);
        while (triangulator._TrianglesLeft > 0)
        {
            triangulator.GetNextTriangle(triangle);
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (triangle[0] << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (triangle[1] << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (triangle[2] << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        _Data->TempBuffer.reserve_discard(points_count);
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2) * 3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        _Data->TempBuffer.reserve_discard((ImTriangulator::EstimateScratchBufferSize(points_count) + sizeof(ImVec2)) / sizeof(ImVec2));
        triangulator.Init(points, points_count, _Data->TempBuffer.Data);
        while (triangulator._TrianglesLeft > 0)
        {
            triangulator.GetNextTriangle(triangle);
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx + triangle[0]); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + triangle[1]); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + triangle[2]);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawListSplitter
//-----------------------------------------------------------------------------
// FIXME: This may be a little confusing, trying to be a little too low-level/optimal instead of just doing vector swap..
//-----------------------------------------------------------------------------

void ImDrawListSplitter::ClearFreeMemory()
{
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == _Current)
            IMGUI_MEMSET(&_Channels[i], 0, sizeof(_Channels[i]));  // Current channel is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i]._CmdBuffer.clear();
        _Channels[i]._IdxBuffer.clear();
    }
    _Current = 0;
    _Count = 1;
    _Channels.clear();
}

void ImDrawListSplitter::Split(ImDrawList* draw_list, int channels_count)
{
    IM_UNUSED(draw_list);
    IM_ASSERT(_Current == 0 && _Count <= 1 && "Nested channel splitting is not supported. Please use separate instances of ImDrawListSplitter.");
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channels_count)
    {
        _Channels.reserve(channels_count); // Avoid over reserving since this is likely to stay stable
        _Channels.resize(channels_count);
    }
    _Count = channels_count;

    // Channels[] (24/32 bytes each) hold storage that we'll swap with draw_list->_CmdBuffer/_IdxBuffer
    // The content of Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy draw_list->_CmdBuffer/_IdxBuffer into Channels[0] and then Channels[1] into draw_list->CmdBuffer/_IdxBuffer
    IMGUI_MEMSET(&_Channels[0], 0, sizeof(ImDrawChannel));
    for (int i = 1; i < channels_count; i++)
    {
        if (i >= old_channels_count)
        {
            IM_PLACEMENT_NEW(&_Channels[i]) ImDrawChannel();
        }
        else
        {
            _Channels[i]._CmdBuffer.resize(0);
            _Channels[i]._IdxBuffer.resize(0);
        }
    }
}

void ImDrawListSplitter::Merge(ImDrawList* draw_list)
{
    // Note that we never use or rely on _Channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if (_Count <= 1)
        return;

    SetCurrentChannel(draw_list, 0);
    draw_list->_PopUnusedDrawCmd();

    // Calculate our final buffer sizes. Also fix the incorrect IdxOffset values in each command.
    int new_cmd_buffer_count = 0;
    int new_idx_buffer_count = 0;
    ImDrawCmd* last_cmd = (_Count > 0 && draw_list->CmdBuffer.Size > 0) ? &draw_list->CmdBuffer.back() : NULL;
    int idx_offset = last_cmd ? last_cmd->IdxOffset + last_cmd->ElemCount : 0;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (ch._CmdBuffer.Size > 0 && ch._CmdBuffer.back().ElemCount == 0 && ch._CmdBuffer.back().UserCallback == NULL) // Equivalent of PopUnusedDrawCmd()
            ch._CmdBuffer.pop_back();

        if (ch._CmdBuffer.Size > 0 && last_cmd != NULL)
        {
            // Do not include ImDrawCmd_AreSequentialIdxOffset() in the compare as we rebuild IdxOffset values ourselves.
            // Manipulating IdxOffset (e.g. by reordering draw commands like done by RenderDimmedBackgroundBehindWindow()) is not supported within a splitter.
            ImDrawCmd* next_cmd = &ch._CmdBuffer[0];
            if (ImDrawCmd_HeaderCompare(last_cmd, next_cmd) == 0 && last_cmd->UserCallback == NULL && next_cmd->UserCallback == NULL)
            {
                // Merge previous channel last draw command with current channel first draw command if matching.
                last_cmd->ElemCount += next_cmd->ElemCount;
                idx_offset += next_cmd->ElemCount;
                ch._CmdBuffer.erase(ch._CmdBuffer.Data); // FIXME-OPT: Improve for multiple merges.
            }
        }
        if (ch._CmdBuffer.Size > 0)
            last_cmd = &ch._CmdBuffer.back();
        new_cmd_buffer_count += ch._CmdBuffer.Size;
        new_idx_buffer_count += ch._IdxBuffer.Size;
        for (int cmd_n = 0; cmd_n < ch._CmdBuffer.Size; cmd_n++)
        {
            ch._CmdBuffer.Data[cmd_n].IdxOffset = idx_offset;
            idx_offset += ch._CmdBuffer.Data[cmd_n].ElemCount;
        }
    }
    draw_list->CmdBuffer.resize(draw_list->CmdBuffer.Size + new_cmd_buffer_count);
    draw_list->IdxBuffer.resize(draw_list->IdxBuffer.Size + new_idx_buffer_count);

    // Write commands and indices in order (they are fairly small structures, we don't copy vertices only indices)
    ImDrawCmd* cmd_write = draw_list->CmdBuffer.Data + draw_list->CmdBuffer.Size - new_cmd_buffer_count;
    ImDrawIdx* idx_write = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch._CmdBuffer.Size) { IMGUI_MEMCPY(cmd_write, ch._CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch._IdxBuffer.Size) { IMGUI_MEMCPY(idx_write, ch._IdxBuffer.Data, sz * sizeof(ImDrawIdx)); idx_write += sz; }
    }
    draw_list->_IdxWritePtr = idx_write;

    // Ensure there's always a non-callback draw command trailing the command-buffer
    if (draw_list->CmdBuffer.Size == 0 || draw_list->CmdBuffer.back().UserCallback != NULL)
        draw_list->AddDrawCmd();

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();

    _Count = 1;
}

void ImDrawListSplitter::SetCurrentChannel(ImDrawList* draw_list, int idx)
{
    IM_ASSERT(idx >= 0 && idx < _Count);
    if (_Current == idx)
        return;

    // Overwrite ImVector (12/16 bytes), four times. This is merely a silly optimization instead of doing .swap()
    IMGUI_MEMCPY(&_Channels.Data[_Current]._CmdBuffer, &draw_list->CmdBuffer, sizeof(draw_list->CmdBuffer));
    IMGUI_MEMCPY(&_Channels.Data[_Current]._IdxBuffer, &draw_list->IdxBuffer, sizeof(draw_list->IdxBuffer));
    _Current = idx;
    IMGUI_MEMCPY(&draw_list->CmdBuffer, &_Channels.Data[idx]._CmdBuffer, sizeof(draw_list->CmdBuffer));
    IMGUI_MEMCPY(&draw_list->IdxBuffer, &_Channels.Data[idx]._IdxBuffer, sizeof(draw_list->IdxBuffer));
    draw_list->_IdxWritePtr = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size;

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = (draw_list->CmdBuffer.Size == 0) ? NULL : &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd == NULL)
        draw_list->AddDrawCmd();
    else if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawData
//-----------------------------------------------------------------------------

void ImDrawData::Clear()
{
    Valid = false;
    CmdListsCount = TotalIdxCount = TotalVtxCount = 0;
    CmdLists.resize(0); // The ImDrawList are NOT owned by ImDrawData but e.g. by ImGuiContext, so we don't clear them.
    DisplayPos = DisplaySize = FramebufferScale = ImVec2(0.0f, 0.0f);
    OwnerViewport = NULL;
}

// Important: 'out_list' is generally going to be draw_data->CmdLists, but may be another temporary list
// as long at it is expected that the result will be later merged into draw_data->CmdLists[].
void ImGui::AddDrawListToDrawDataEx(ImDrawData* draw_data, ImVector<ImDrawList*>* out_list, ImDrawList* draw_list)
{
    if (draw_list->CmdBuffer.Size == 0)
        return;
    if (draw_list->CmdBuffer.Size == 1 && draw_list->CmdBuffer[0].ElemCount == 0 && draw_list->CmdBuffer[0].UserCallback == NULL)
        return;

    // Draw list sanity check. Detect mismatch between PrimReserve() calls and incrementing _VtxCurrentIdx, _VtxWritePtr etc.
    // May trigger for you if you are using PrimXXX functions incorrectly.
    IM_ASSERT(draw_list->VtxBuffer.Size == 0 || draw_list->_VtxWritePtr == draw_list->VtxBuffer.Data + draw_list->VtxBuffer.Size);
    IM_ASSERT(draw_list->IdxBuffer.Size == 0 || draw_list->_IdxWritePtr == draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size);
    if (!(draw_list->Flags & ImDrawListFlags_AllowVtxOffset))
        IM_ASSERT((int)draw_list->_VtxCurrentIdx == draw_list->VtxBuffer.Size);

    // Check that draw_list doesn't use more vertices than indexable (default ImDrawIdx = unsigned short = 2 bytes = 64K vertices per ImDrawList = per window)
    // If this assert triggers because you are drawing lots of stuff manually:
    // - First, make sure you are coarse clipping yourself and not trying to draw many things outside visible bounds.
    //   Be mindful that the lower-level ImDrawList API doesn't filter vertices. Use the Metrics/Debugger window to inspect draw list contents.
    // - If you want large meshes with more than 64K vertices, you can either:
    //   (A) Handle the ImDrawCmd::VtxOffset value in your renderer backend, and set 'io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset'.
    //       Most example backends already support this from 1.71. Pre-1.71 backends won't.
    //       Some graphics API such as GL ES 1/2 don't have a way to offset the starting vertex so it is not supported for them.
    //   (B) Or handle 32-bit indices in your renderer backend, and uncomment '#define ImDrawIdx unsigned int' line in imconfig.h.
    //       Most example backends already support this. For example, the OpenGL example code detect index size at compile-time:
    //         glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
    //       Your own engine or render API may use different parameters or function calls to specify index sizes.
    //       2 and 4 bytes indices are generally supported by most graphics API.
    // - If for some reason neither of those solutions works for you, a workaround is to call BeginChild()/EndChild() before reaching
    //   the 64K limit to split your draw commands in multiple draw lists.
    if (sizeof(ImDrawIdx) == 2)
        IM_ASSERT(draw_list->_VtxCurrentIdx < (1 << 16) && "Too many vertices in ImDrawList using 16-bit indices. Read comment above");

    // Add to output list + records state in ImDrawData
    out_list->push_back(draw_list);
    draw_data->CmdListsCount++;
    draw_data->TotalVtxCount += draw_list->VtxBuffer.Size;
    draw_data->TotalIdxCount += draw_list->IdxBuffer.Size;
}

void ImDrawData::AddDrawList(ImDrawList* draw_list)
{
    IM_ASSERT(CmdLists.Size == CmdListsCount);
    draw_list->_PopUnusedDrawCmd();
    ImGui::AddDrawListToDrawDataEx(this, &CmdLists, draw_list);
}

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        if (cmd_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(cmd_list->IdxBuffer.Size);
        for (int j = 0; j < cmd_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = cmd_list->VtxBuffer[cmd_list->IdxBuffer[j]];
        cmd_list->VtxBuffer.swap(new_vtx_buffer);
        cmd_list->IdxBuffer.resize(0);
        TotalVtxCount += cmd_list->VtxBuffer.Size;
    }
}

// Helper to scale the ClipRect field of each ImDrawCmd.
// Use if your final output buffer is at a different scale than draw_data->DisplaySize,
// or if there is a difference between your window resolution and framebuffer resolution.
void ImDrawData::ScaleClipRects(const ImVec2& fb_scale)
{
    for (ImDrawList* draw_list : CmdLists)
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            cmd.ClipRect = ImVec4(cmd.ClipRect.x * fb_scale.x, cmd.ClipRect.y * fb_scale.y, cmd.ClipRect.z * fb_scale.x, cmd.ClipRect.w * fb_scale.y);
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers ShadeVertsXXX functions
//-----------------------------------------------------------------------------

// Generic linear color gradient, write to RGB fields, leave A untouched.
void ImGui::ShadeVertsLinearColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    const int col0_r = (int)(col0 >> IM_COL32_R_SHIFT) & 0xFF;
    const int col0_g = (int)(col0 >> IM_COL32_G_SHIFT) & 0xFF;
    const int col0_b = (int)(col0 >> IM_COL32_B_SHIFT) & 0xFF;
    const int col_delta_r = ((int)(col1 >> IM_COL32_R_SHIFT) & 0xFF) - col0_r;
    const int col_delta_g = ((int)(col1 >> IM_COL32_G_SHIFT) & 0xFF) - col0_g;
    const int col_delta_b = ((int)(col1 >> IM_COL32_B_SHIFT) & 0xFF) - col0_b;
    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
        int r = (int)(col0_r + col_delta_r * t);
        int g = (int)(col0_g + col_delta_g * t);
        int b = (int)(col0_b + col_delta_b * t);
        vert->col = (r << IM_COL32_R_SHIFT) | (g << IM_COL32_G_SHIFT) | (b << IM_COL32_B_SHIFT) | (vert->col & IM_COL32_A_MASK);
    }
}

// Distribute UV over (a, b) rectangle
void ImGui::ShadeVertsLinearUV(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, bool clamp)
{
    const ImVec2 size = b - a;
    const ImVec2 uv_size = uv_b - uv_a;
    const ImVec2 scale = ImVec2(
        size.x != 0.0f ? (uv_size.x / size.x) : 0.0f,
        size.y != 0.0f ? (uv_size.y / size.y) : 0.0f);

    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    if (clamp)
    {
        const ImVec2 min = ImMin(uv_a, uv_b);
        const ImVec2 max = ImMax(uv_a, uv_b);
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = ImClamp(uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale), min, max);
    }
    else
    {
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale);
    }
}

void ImGui::ShadeVertsTransformPos(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& pivot_in, float cos_a, float sin_a, const ImVec2& pivot_out)
{
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
        vertex->pos = ImRotate(vertex->pos- pivot_in, cos_a, sin_a) + pivot_out;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontConfig
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    IMGUI_MEMSET(this, 0, sizeof(*this));
    FontDataOwnedByAtlas = true;
    OversampleH = 2;
    OversampleV = 1;
    GlyphMaxAdvanceX = FLT_MAX;
    RasterizerMultiply = 1.0f;
    RasterizerDensity = 1.0f;
    EllipsisChar = (ImWchar)-1;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas
//-----------------------------------------------------------------------------

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The 2x2 white texels on the top left are the ones we'll use everywhere in Dear ImGui to render filled shapes.
// (This is used when io.MouseDrawCursor = true)
const int FONT_ATLAS_DEFAULT_TEX_DATA_W = 122; // Actual texture will be 2 times that + 1 spacing.
const int FONT_ATLAS_DEFAULT_TEX_DATA_H = 27;
static const char FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          - XX       XX "
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         -X..X     X..X"
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         -X...X   X...X"
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         - X...X X...X "
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         -  X...X...X  "
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       -   X.....X   "
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    -    X...X    "
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  -     X.X     "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X -    X...X    "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X-   X.....X   "
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X-  X...X...X  "
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X- X...X X...X "
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X-X...X   X...X"
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X-X..X     X..X"
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X- XX       XX "
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X--------------"
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X -             "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X -             "
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X -             "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  -             "
    "      X..X  -       -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  -             "
    "       XX   -       -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  -             "
    "-------------       -    X    -           X           -X.....................X-           -------------------             "
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                                           "
    "                                                      -  X..X           X..X  -                                           "
    "                                                      -   X.X           X.X   -                                           "
    "                                                      -    XX           XX    -                                           "
};

static const ImVec2 FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[ImGuiMouseCursor_COUNT][3] =
{
    // Pos ........ Size ......... Offset ......
    { ImVec2( 0,3), ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
    { ImVec2(13,0), ImVec2( 7,16), ImVec2( 1, 8) }, // ImGuiMouseCursor_TextInput
    { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_ResizeAll
    { ImVec2(21,0), ImVec2( 9,23), ImVec2( 4,11) }, // ImGuiMouseCursor_ResizeNS
    { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 4) }, // ImGuiMouseCursor_ResizeEW
    { ImVec2(73,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNESW
    { ImVec2(55,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNWSE
    { ImVec2(91,0), ImVec2(17,22), ImVec2( 5, 0) }, // ImGuiMouseCursor_Hand
    { ImVec2(109,0),ImVec2(13,15), ImVec2( 6, 7) }, // ImGuiMouseCursor_NotAllowed
};

ImFontAtlas::ImFontAtlas()
{
    IMGUI_MEMSET(this, 0, sizeof(*this));
    TexGlyphPadding = 1;
    PackIdMouseCursors = PackIdLines = -1;
}

ImFontAtlas::~ImFontAtlas()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (ImFontConfig& font_cfg : ConfigData)
        if (font_cfg.FontData && font_cfg.FontDataOwnedByAtlas)
        {
            IM_FREE(font_cfg.FontData);
            font_cfg.FontData = NULL;
        }

    // When clearing this we lose access to the font name and other information used to build the font.
    for (ImFont* font : Fonts)
        if (font->ConfigData >= ConfigData.Data && font->ConfigData < ConfigData.Data + ConfigData.Size)
        {
            font->ConfigData = NULL;
            font->ConfigDataCount = 0;
        }
    ConfigData.clear();
    CustomRects.clear();
    PackIdMouseCursors = PackIdLines = -1;
    // Important: we leave TexReady untouched
}

void    ImFontAtlas::ClearTexData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    if (TexPixelsAlpha8)
        IM_FREE(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        IM_FREE(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexPixelsUseColors = false;
    // Important: we leave TexReady untouched
}

void    ImFontAtlas::ClearFonts()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    Fonts.clear_delete();
    TexReady = false;
}

void    ImFontAtlas::Clear()
{
    ClearInputData();
    ClearTexData();
    ClearFonts();
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Build atlas on demand
    if (TexPixelsAlpha8 == NULL)
        Build();

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Convert to RGBA32 format on demand
    // Although it is likely to be the most commonly used format, our font rendering is 1 channel / 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels = NULL;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        if (pixels)
        {
            TexPixelsRGBA32 = (unsigned int*)IM_ALLOC((size_t)TexWidth * (size_t)TexHeight * 4);
            const unsigned char* src = pixels;
            unsigned int* dst = TexPixelsRGBA32;
            for (int n = TexWidth * TexHeight; n > 0; n--)
                *dst++ = IM_COL32(255, 255, 255, (unsigned int)(*src++));
        }
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    IM_ASSERT(font_cfg->FontData != NULL && font_cfg->FontDataSize > 0);
    IM_ASSERT(font_cfg->SizePixels > 0.0f);

    // Create new font
    if (!font_cfg->MergeMode)
        Fonts.push_back(IM_NEW(ImFont));
    else
        IM_ASSERT(!Fonts.empty() && "Cannot use MergeMode for the first font"); // When using MergeMode make sure that a font has already been added before. You can use ImGui::GetIO().Fonts->AddFontDefault() to add the default imgui font.

    ConfigData.push_back(*font_cfg);
    ImFontConfig& new_font_cfg = ConfigData.back();
    if (new_font_cfg.DstFont == NULL)
        new_font_cfg.DstFont = Fonts.back();
    if (!new_font_cfg.FontDataOwnedByAtlas)
    {
        new_font_cfg.FontData = IM_ALLOC(new_font_cfg.FontDataSize);
        new_font_cfg.FontDataOwnedByAtlas = true;
        IMGUI_MEMCPY(new_font_cfg.FontData, font_cfg->FontData, (size_t)new_font_cfg.FontDataSize);
    }

    if (new_font_cfg.DstFont->EllipsisChar == (ImWchar)-1)
        new_font_cfg.DstFont->EllipsisChar = font_cfg->EllipsisChar;

    ImFontAtlasUpdateConfigDataPointers(this);

    // Invalidate texture
    TexReady = false;
    ClearTexData();
    return new_font_cfg.DstFont;
}

// Default font TTF is compressed with stb_compress
static unsigned int stb_decompress_length(const unsigned char* input);
static unsigned int stb_decompress(unsigned char* output, const unsigned char* input, unsigned int length);
static const unsigned int* GetDefaultCompressedFontDataTTF();
static const unsigned int GetDefaultCompressedFontDataTTFSize();
static unsigned int Decode85Byte(char c)                                    { return c >= '\\' ? c-36 : c-35; }
static void         Decode85(const unsigned char* src, unsigned char* dst)
{
    while (*src)
    {
        unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
        src += 5;
        dst += 4;
    }
}

// Load embedded ProggyClean.ttf at size 13, disable oversampling
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (!font_cfg_template)
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }
    if (font_cfg.SizePixels <= 0.0f)
        font_cfg.SizePixels = 13.0f * 1.0f;
    if (font_cfg.Name[0] == '\0')
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), xorstr_("ProggyClean.ttf, %dpx"), (int)font_cfg.SizePixels);
    font_cfg.EllipsisChar = (ImWchar)0x0085;
    font_cfg.GlyphOffset.y = 1.0f * IM_TRUNC(font_cfg.SizePixels / 13.0f);  // Add +1 offset per 13 units

    const ImWchar* glyph_ranges = font_cfg.GlyphRanges != NULL ? font_cfg.GlyphRanges : GetGlyphRangesDefault();
    ImFont* font = AddFontFromMemoryCompressedTTF(GetDefaultCompressedFontDataTTF(), GetDefaultCompressedFontDataTTFSize(), font_cfg.SizePixels, &font_cfg, glyph_ranges);
    return font;
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    size_t data_size = 0;
    void* data = ImFileLoadToMemory(filename, xorstr_("rb"), &data_size, 0);
    if (!data)
    {
        IM_ASSERT_USER_ERROR(0, "Could not load font file!");
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + IMGUI_STRLEN(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), xorstr_("%s, %.0fpx"), p, size_pixels);
    }
    return AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* font_data, int font_data_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    IM_ASSERT(font_data_size > 100 && "Incorrect value for font_data_size!"); // Heuristic to prevent accidentally passing a wrong value to font_data_size.
    font_cfg.FontData = font_data;
    font_cfg.FontDataSize = font_data_size;
    font_cfg.SizePixels = size_pixels > 0.0f ? size_pixels : font_cfg.SizePixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((const unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char*)IM_ALLOC(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (const unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, &font_cfg, glyph_ranges);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_ttf_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges)
{
    int compressed_ttf_size = (((int)IMGUI_STRLEN(compressed_ttf_data_base85) + 4) / 5) * 4;
    void* compressed_ttf = IM_ALLOC((size_t)compressed_ttf_size);
    Decode85((const unsigned char*)compressed_ttf_data_base85, (unsigned char*)compressed_ttf);
    ImFont* font = AddFontFromMemoryCompressedTTF(compressed_ttf, compressed_ttf_size, size_pixels, font_cfg, glyph_ranges);
    IM_FREE(compressed_ttf);
    return font;
}

int ImFontAtlas::AddCustomRectRegular(int width, int height)
{
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

int ImFontAtlas::AddCustomRectFontGlyph(ImFont* font, ImWchar id, int width, int height, float advance_x, const ImVec2& offset)
{
#ifdef IMGUI_USE_WCHAR32
    IM_ASSERT(id <= IM_UNICODE_CODEPOINT_MAX);
#endif
    IM_ASSERT(font != NULL);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    r.GlyphID = id;
    r.GlyphAdvanceX = advance_x;
    r.GlyphOffset = offset;
    r.Font = font;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

void ImFontAtlas::CalcCustomRectUV(const ImFontAtlasCustomRect* rect, ImVec2* out_uv_min, ImVec2* out_uv_max) const
{
    IM_ASSERT(TexWidth > 0 && TexHeight > 0);   // Font atlas needs to be built before we can calculate UV coordinates
    IM_ASSERT(rect->IsPacked());                // Make sure the rectangle has been packed
    *out_uv_min = ImVec2((float)rect->X * TexUvScale.x, (float)rect->Y * TexUvScale.y);
    *out_uv_max = ImVec2((float)(rect->X + rect->Width) * TexUvScale.x, (float)(rect->Y + rect->Height) * TexUvScale.y);
}

bool ImFontAtlas::GetMouseCursorTexData(ImGuiMouseCursor cursor_type, ImVec2* out_offset, ImVec2* out_size, ImVec2 out_uv_border[2], ImVec2 out_uv_fill[2])
{
    if (cursor_type <= ImGuiMouseCursor_None || cursor_type >= ImGuiMouseCursor_COUNT)
        return false;
    if (Flags & ImFontAtlasFlags_NoMouseCursors)
        return false;

    IM_ASSERT(PackIdMouseCursors != -1);
    ImFontAtlasCustomRect* r = GetCustomRectByIndex(PackIdMouseCursors);
    ImVec2 pos = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][0] + ImVec2((float)r->X, (float)r->Y);
    ImVec2 size = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][1];
    *out_size = size;
    *out_offset = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][2];
    out_uv_border[0] = (pos) * TexUvScale;
    out_uv_border[1] = (pos + size) * TexUvScale;
    pos.x += FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
    out_uv_fill[0] = (pos) * TexUvScale;
    out_uv_fill[1] = (pos + size) * TexUvScale;
    return true;
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");

    // Default font is none are specified
    if (ConfigData.Size == 0)
        AddFontDefault();

    // Select builder
    // - Note that we do not reassign to atlas->FontBuilderIO, since it is likely to point to static data which
    //   may mess with some hot-reloading schemes. If you need to assign to this (for dynamic selection) AND are
    //   using a hot-reloading scheme that messes up static data, store your own instance of ImFontBuilderIO somewhere
    //   and point to it instead of pointing directly to return value of the GetBuilderXXX functions.
    const ImFontBuilderIO* builder_io = FontBuilderIO;
    if (builder_io == NULL)
    {
#ifdef IMGUI_ENABLE_FREETYPE
        builder_io = ImGuiFreeType::GetBuilderForFreeType();
#elif defined(IMGUI_ENABLE_STB_TRUETYPE)
        builder_io = ImFontAtlasGetBuilderForStbTruetype();
#else
        IM_ASSERT(0); // Invalid Build function
#endif
    }

    // Build
    return builder_io->FontBuilder_Build(this);
}

void    ImFontAtlasBuildMultiplyCalcLookupTable(unsigned char out_table[256], float in_brighten_factor)
{
    for (unsigned int i = 0; i < 256; i++)
    {
        unsigned int value = (unsigned int)(i * in_brighten_factor);
        out_table[i] = value > 255 ? 255 : (value & 0xFF);
    }
}

void    ImFontAtlasBuildMultiplyRectAlpha8(const unsigned char table[256], unsigned char* pixels, int x, int y, int w, int h, int stride)
{
    IM_ASSERT_PARANOID(w <= stride);
    unsigned char* data = pixels + x + y * stride;
    for (int j = h; j > 0; j--, data += stride - w)
        for (int i = w; i > 0; i--, data++)
            *data = table[*data];
}

#ifdef IMGUI_ENABLE_STB_TRUETYPE
// Temporary data for one source font (multiple source fonts can be merged into one destination ImFont)
// (C++03 doesn't allow instancing ImVector<> with function-local types so we declare the type here.)
struct ImFontBuildSrcData
{
    stbtt_fontinfo      FontInfo;
    stbtt_pack_range    PackRange;          // Hold the list of codepoints to pack (essentially points to Codepoints.Data)
    stbrp_rect*         Rects;              // Rectangle to pack. We first fill in their size and the packer will give us their position.
    stbtt_packedchar*   PackedChars;        // Output glyphs
    const ImWchar*      SrcRanges;          // Ranges as requested by user (user is allowed to request too much, e.g. 0x0020..0xFFFF)
    int                 DstIndex;           // Index into atlas->Fonts[] and dst_tmp_array[]
    int                 GlyphsHighest;      // Highest requested codepoint
    int                 GlyphsCount;        // Glyph count (excluding missing glyphs and glyphs already set by an earlier source font)
    ImBitVector         GlyphsSet;          // Glyph bit map (random access, 1-bit per codepoint. This will be a maximum of 8KB)
    ImVector<int>       GlyphsList;         // Glyph codepoints list (flattened version of GlyphsSet)
};

// Temporary data for one destination ImFont* (multiple source fonts can be merged into one destination ImFont)
struct ImFontBuildDstData
{
    int                 SrcCount;           // Number of source fonts targeting this destination font.
    int                 GlyphsHighest;
    int                 GlyphsCount;
    ImBitVector         GlyphsSet;          // This is used to resolve collision when multiple sources are merged into a same destination font.
};

static void UnpackBitVectorToFlatIndexList(const ImBitVector* in, ImVector<int>* out)
{
    IM_ASSERT(sizeof(in->Storage.Data[0]) == sizeof(int));
    const ImU32* it_begin = in->Storage.begin();
    const ImU32* it_end = in->Storage.end();
    for (const ImU32* it = it_begin; it < it_end; it++)
        if (ImU32 entries_32 = *it)
            for (ImU32 bit_n = 0; bit_n < 32; bit_n++)
                if (entries_32 & ((ImU32)1 << bit_n))
                    out->push_back((int)(((it - it_begin) << 5) + bit_n));
}

static bool ImFontAtlasBuildWithStbTruetype(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->ConfigData.Size > 0);

    ImFontAtlasBuildInit(atlas);

    // Clear atlas
    atlas->TexID = (ImTextureID)NULL;
    atlas->TexWidth = atlas->TexHeight = 0;
    atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    atlas->TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    atlas->ClearTexData();

    // Temporary storage for building
    ImVector<ImFontBuildSrcData> src_tmp_array;
    ImVector<ImFontBuildDstData> dst_tmp_array;
    src_tmp_array.resize(atlas->ConfigData.Size);
    dst_tmp_array.resize(atlas->Fonts.Size);
    IMGUI_MEMSET(src_tmp_array.Data, 0, (size_t)src_tmp_array.size_in_bytes());
    IMGUI_MEMSET(dst_tmp_array.Data, 0, (size_t)dst_tmp_array.size_in_bytes());

    // 1. Initialize font loading structure, check font data validity
    for (int src_i = 0; src_i < atlas->ConfigData.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == atlas));

        // Find index from cfg.DstFont (we allow the user to set cfg.DstFont. Also it makes casual debugging nicer than when storing indices)
        src_tmp.DstIndex = -1;
        for (int output_i = 0; output_i < atlas->Fonts.Size && src_tmp.DstIndex == -1; output_i++)
            if (cfg.DstFont == atlas->Fonts[output_i])
                src_tmp.DstIndex = output_i;
        if (src_tmp.DstIndex == -1)
        {
            IM_ASSERT(src_tmp.DstIndex != -1); // cfg.DstFont not pointing within atlas->Fonts[] array?
            return false;
        }
        // Initialize helper structure for font loading and verify that the TTF/OTF data is correct
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)cfg.FontData, cfg.FontNo);
        IM_ASSERT(font_offset >= 0 && "FontData is incorrect, or FontNo cannot be found.");
        if (!stbtt_InitFont(&src_tmp.FontInfo, (unsigned char*)cfg.FontData, font_offset))
        {
            IM_ASSERT(0 && "stbtt_InitFont(): failed to parse FontData. It is correct and complete? Check FontDataSize.");
            return false;
        }

        // Measure highest codepoints
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.SrcRanges = cfg.GlyphRanges ? cfg.GlyphRanges : atlas->GetGlyphRangesDefault();
        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
        {
            // Check for valid range. This may also help detect *some* dangling pointers, because a common
            // user error is to setup ImFontConfig::GlyphRanges with a pointer to data that isn't persistent.
            IM_ASSERT(src_range[0] <= src_range[1]);
            src_tmp.GlyphsHighest = ImMax(src_tmp.GlyphsHighest, (int)src_range[1]);
        }
        dst_tmp.SrcCount++;
        dst_tmp.GlyphsHighest = ImMax(dst_tmp.GlyphsHighest, src_tmp.GlyphsHighest);
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int total_glyphs_count = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.GlyphsSet.Create(src_tmp.GlyphsHighest + 1);
        if (dst_tmp.GlyphsSet.Storage.empty())
            dst_tmp.GlyphsSet.Create(dst_tmp.GlyphsHighest + 1);

        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            for (unsigned int codepoint = src_range[0]; codepoint <= src_range[1]; codepoint++)
            {
                if (dst_tmp.GlyphsSet.TestBit(codepoint))    // Don't overwrite existing glyphs. We could make this an option for MergeMode (e.g. MergeOverwrite==true)
                    continue;
                if (!stbtt_FindGlyphIndex(&src_tmp.FontInfo, codepoint))    // It is actually in the font?
                    continue;

                // Add to avail set/counters
                src_tmp.GlyphsCount++;
                dst_tmp.GlyphsCount++;
                src_tmp.GlyphsSet.SetBit(codepoint);
                dst_tmp.GlyphsSet.SetBit(codepoint);
                total_glyphs_count++;
            }
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        src_tmp.GlyphsList.reserve(src_tmp.GlyphsCount);
        UnpackBitVectorToFlatIndexList(&src_tmp.GlyphsSet, &src_tmp.GlyphsList);
        src_tmp.GlyphsSet.Clear();
        IM_ASSERT(src_tmp.GlyphsList.Size == src_tmp.GlyphsCount);
    }
    for (int dst_i = 0; dst_i < dst_tmp_array.Size; dst_i++)
        dst_tmp_array[dst_i].GlyphsSet.Clear();
    dst_tmp_array.clear();

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear buf_rects, but let's do it for the sake of sanity)
    ImVector<stbrp_rect> buf_rects;
    ImVector<stbtt_packedchar> buf_packedchars;
    buf_rects.resize(total_glyphs_count);
    buf_packedchars.resize(total_glyphs_count);
    IMGUI_MEMSET(buf_rects.Data, 0, (size_t)buf_rects.size_in_bytes());
    IMGUI_MEMSET(buf_packedchars.Data, 0, (size_t)buf_packedchars.size_in_bytes());

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.
    int total_surface = 0;
    int buf_rects_out_n = 0;
    int buf_packedchars_out_n = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        src_tmp.Rects = &buf_rects[buf_rects_out_n];
        src_tmp.PackedChars = &buf_packedchars[buf_packedchars_out_n];
        buf_rects_out_n += src_tmp.GlyphsCount;
        buf_packedchars_out_n += src_tmp.GlyphsCount;

        // Convert our ranges in the format stb_truetype wants
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        src_tmp.PackRange.font_size = cfg.SizePixels * cfg.RasterizerDensity;
        src_tmp.PackRange.first_unicode_codepoint_in_range = 0;
        src_tmp.PackRange.array_of_unicode_codepoints = src_tmp.GlyphsList.Data;
        src_tmp.PackRange.num_chars = src_tmp.GlyphsList.Size;
        src_tmp.PackRange.chardata_for_range = src_tmp.PackedChars;
        src_tmp.PackRange.h_oversample = (unsigned char)cfg.OversampleH;
        src_tmp.PackRange.v_oversample = (unsigned char)cfg.OversampleV;

        // Gather the sizes of all rectangles we will need to pack (this loop is based on stbtt_PackFontRangesGatherRects)
        const float scale = (cfg.SizePixels > 0.0f) ? stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels * cfg.RasterizerDensity) : stbtt_ScaleForMappingEmToPixels(&src_tmp.FontInfo, -cfg.SizePixels * cfg.RasterizerDensity);
        const int padding = atlas->TexGlyphPadding;
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsList.Size; glyph_i++)
        {
            int x0, y0, x1, y1;
            const int glyph_index_in_font = stbtt_FindGlyphIndex(&src_tmp.FontInfo, src_tmp.GlyphsList[glyph_i]);
            IM_ASSERT(glyph_index_in_font != 0);
            stbtt_GetGlyphBitmapBoxSubpixel(&src_tmp.FontInfo, glyph_index_in_font, scale * cfg.OversampleH, scale * cfg.OversampleV, 0, 0, &x0, &y0, &x1, &y1);
            src_tmp.Rects[glyph_i].w = (stbrp_coord)(x1 - x0 + padding + cfg.OversampleH - 1);
            src_tmp.Rects[glyph_i].h = (stbrp_coord)(y1 - y0 + padding + cfg.OversampleV - 1);
            total_surface += src_tmp.Rects[glyph_i].w * src_tmp.Rects[glyph_i].h;
        }
    }

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    // User can override TexDesiredWidth and TexGlyphPadding if they wish, otherwise we use a simple heuristic to select the width based on expected surface.
    const int surface_sqrt = (int)ImSqrt((float)total_surface) + 1;
    atlas->TexHeight = 0;
    if (atlas->TexDesiredWidth > 0)
        atlas->TexWidth = atlas->TexDesiredWidth;
    else
        atlas->TexWidth = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int TEX_HEIGHT_MAX = 1024 * 32;
    stbtt_pack_context spc = {};
    stbtt_PackBegin(&spc, NULL, atlas->TexWidth, TEX_HEIGHT_MAX, 0, atlas->TexGlyphPadding, NULL);
    ImFontAtlasBuildPackCustomRects(atlas, spc.pack_info);

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbrp_pack_rects((stbrp_context*)spc.pack_info, src_tmp.Rects, src_tmp.GlyphsCount);

        // Extend texture height and mark missing glyphs as non-packed so we won't render them.
        // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
            if (src_tmp.Rects[glyph_i].was_packed)
                atlas->TexHeight = ImMax(atlas->TexHeight, src_tmp.Rects[glyph_i].y + src_tmp.Rects[glyph_i].h);
    }

    // 7. Allocate texture
    atlas->TexHeight = (atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) ? (atlas->TexHeight + 1) : ImUpperPowerOfTwo(atlas->TexHeight);
    atlas->TexUvScale = ImVec2(1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
    atlas->TexPixelsAlpha8 = (unsigned char*)IM_ALLOC(atlas->TexWidth * atlas->TexHeight);
    IMGUI_MEMSET(atlas->TexPixelsAlpha8, 0, atlas->TexWidth * atlas->TexHeight);
    spc.pixels = atlas->TexPixelsAlpha8;
    spc.height = atlas->TexHeight;

    // 8. Render/rasterize font characters into the texture
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbtt_PackFontRangesRenderIntoRects(&spc, &src_tmp.FontInfo, &src_tmp.PackRange, 1, src_tmp.Rects);

        // Apply multiply operator
        if (cfg.RasterizerMultiply != 1.0f)
        {
            unsigned char multiply_table[256];
            ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);
            stbrp_rect* r = &src_tmp.Rects[0];
            for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++, r++)
                if (r->was_packed)
                    ImFontAtlasBuildMultiplyRectAlpha8(multiply_table, atlas->TexPixelsAlpha8, r->x, r->y, r->w, r->h, atlas->TexWidth * 1);
        }
        src_tmp.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    buf_rects.clear();

    // 9. Setup ImFont and glyphs for runtime
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        // When merging fonts with MergeMode=true:
        // - We can have multiple input fonts writing into a same destination font.
        // - dst_font->ConfigData is != from cfg which is our source configuration.
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFont* dst_font = cfg.DstFont;

        const float font_scale = stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&src_tmp.FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        const float ascent = ImTrunc(unscaled_ascent * font_scale + ((unscaled_ascent > 0.0f) ? +1 : -1));
        const float descent = ImTrunc(unscaled_descent * font_scale + ((unscaled_descent > 0.0f) ? +1 : -1));
        ImFontAtlasBuildSetupFont(atlas, dst_font, &cfg, ascent, descent);
        const float font_off_x = cfg.GlyphOffset.x;
        const float font_off_y = cfg.GlyphOffset.y + IM_ROUND(dst_font->Ascent);

        const float inv_rasterization_scale = 1.0f / cfg.RasterizerDensity;

        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
        {
            // Register glyph
            const int codepoint = src_tmp.GlyphsList[glyph_i];
            const stbtt_packedchar& pc = src_tmp.PackedChars[glyph_i];
            stbtt_aligned_quad q;
            float unused_x = 0.0f, unused_y = 0.0f;
            stbtt_GetPackedQuad(src_tmp.PackedChars, atlas->TexWidth, atlas->TexHeight, glyph_i, &unused_x, &unused_y, &q, 0);
            float x0 = q.x0 * inv_rasterization_scale + font_off_x;
            float y0 = q.y0 * inv_rasterization_scale + font_off_y;
            float x1 = q.x1 * inv_rasterization_scale + font_off_x;
            float y1 = q.y1 * inv_rasterization_scale + font_off_y;
            dst_font->AddGlyph(&cfg, (ImWchar)codepoint, x0, y0, x1, y1, q.s0, q.t0, q.s1, q.t1, pc.xadvance * inv_rasterization_scale);
        }
    }

    // Cleanup
    src_tmp_array.clear_destruct();

    ImFontAtlasBuildFinish(atlas);
    return true;
}

const ImFontBuilderIO* ImFontAtlasGetBuilderForStbTruetype()
{
    static ImFontBuilderIO io;
    io.FontBuilder_Build = ImFontAtlasBuildWithStbTruetype;
    return &io;
}

#endif // IMGUI_ENABLE_STB_TRUETYPE

void ImFontAtlasUpdateConfigDataPointers(ImFontAtlas* atlas)
{
    for (ImFontConfig& font_cfg : atlas->ConfigData)
    {
        ImFont* font = font_cfg.DstFont;
        if (!font_cfg.MergeMode)
        {
            font->ConfigData = &font_cfg;
            font->ConfigDataCount = 0;
        }
        font->ConfigDataCount++;
    }
}

void ImFontAtlasBuildSetupFont(ImFontAtlas* atlas, ImFont* font, ImFontConfig* font_config, float ascent, float descent)
{
    if (!font_config->MergeMode)
    {
        font->ClearOutputData();
        font->FontSize = font_config->SizePixels;
        IM_ASSERT(font->ConfigData == font_config);
        font->ContainerAtlas = atlas;
        font->Ascent = ascent;
        font->Descent = descent;
    }
}

void ImFontAtlasBuildPackCustomRects(ImFontAtlas* atlas, void* stbrp_context_opaque)
{
    stbrp_context* pack_context = (stbrp_context*)stbrp_context_opaque;
    IM_ASSERT(pack_context != NULL);

    ImVector<ImFontAtlasCustomRect>& user_rects = atlas->CustomRects;
    IM_ASSERT(user_rects.Size >= 1); // We expect at least the default custom rects to be registered, else something went wrong.
#ifdef __GNUC__
    if (user_rects.Size < 1) { __builtin_unreachable(); } // Workaround for GCC bug if IM_ASSERT() is defined to conditionally throw (see #5343)
#endif

    ImVector<stbrp_rect> pack_rects;
    pack_rects.resize(user_rects.Size);
    IMGUI_MEMSET(pack_rects.Data, 0, (size_t)pack_rects.size_in_bytes());
    for (int i = 0; i < user_rects.Size; i++)
    {
        pack_rects[i].w = user_rects[i].Width;
        pack_rects[i].h = user_rects[i].Height;
    }
    stbrp_pack_rects(pack_context, &pack_rects[0], pack_rects.Size);
    for (int i = 0; i < pack_rects.Size; i++)
        if (pack_rects[i].was_packed)
        {
            user_rects[i].X = (unsigned short)pack_rects[i].x;
            user_rects[i].Y = (unsigned short)pack_rects[i].y;
            IM_ASSERT(pack_rects[i].w == user_rects[i].Width && pack_rects[i].h == user_rects[i].Height);
            atlas->TexHeight = ImMax(atlas->TexHeight, pack_rects[i].y + pack_rects[i].h);
        }
}

void ImFontAtlasBuildRender8bppRectFromString(ImFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char, unsigned char in_marker_pixel_value)
{
    IM_ASSERT(x >= 0 && x + w <= atlas->TexWidth);
    IM_ASSERT(y >= 0 && y + h <= atlas->TexHeight);
    unsigned char* out_pixel = atlas->TexPixelsAlpha8 + x + (y * atlas->TexWidth);
    for (int off_y = 0; off_y < h; off_y++, out_pixel += atlas->TexWidth, in_str += w)
        for (int off_x = 0; off_x < w; off_x++)
            out_pixel[off_x] = (in_str[off_x] == in_marker_char) ? in_marker_pixel_value : 0x00;
}

void ImFontAtlasBuildRender32bppRectFromString(ImFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char, unsigned int in_marker_pixel_value)
{
    IM_ASSERT(x >= 0 && x + w <= atlas->TexWidth);
    IM_ASSERT(y >= 0 && y + h <= atlas->TexHeight);
    unsigned int* out_pixel = atlas->TexPixelsRGBA32 + x + (y * atlas->TexWidth);
    for (int off_y = 0; off_y < h; off_y++, out_pixel += atlas->TexWidth, in_str += w)
        for (int off_x = 0; off_x < w; off_x++)
            out_pixel[off_x] = (in_str[off_x] == in_marker_char) ? in_marker_pixel_value : IM_COL32_BLACK_TRANS;
}

static void ImFontAtlasBuildRenderDefaultTexData(ImFontAtlas* atlas)
{
    ImFontAtlasCustomRect* r = atlas->GetCustomRectByIndex(atlas->PackIdMouseCursors);
    IM_ASSERT(r->IsPacked());

    const int w = atlas->TexWidth;
    if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
    {
        // Render/copy pixels
        IM_ASSERT(r->Width == FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1 && r->Height == FONT_ATLAS_DEFAULT_TEX_DATA_H);
        const int x_for_white = r->X;
        const int x_for_black = r->X + FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
        if (atlas->TexPixelsAlpha8 != NULL)
        {
            ImFontAtlasBuildRender8bppRectFromString(atlas, x_for_white, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, '.', 0xFF);
            ImFontAtlasBuildRender8bppRectFromString(atlas, x_for_black, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, 'X', 0xFF);
        }
        else
        {
            ImFontAtlasBuildRender32bppRectFromString(atlas, x_for_white, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, '.', IM_COL32_WHITE);
            ImFontAtlasBuildRender32bppRectFromString(atlas, x_for_black, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, 'X', IM_COL32_WHITE);
        }
    }
    else
    {
        // Render 4 white pixels
        IM_ASSERT(r->Width == 2 && r->Height == 2);
        const int offset = (int)r->X + (int)r->Y * w;
        if (atlas->TexPixelsAlpha8 != NULL)
        {
            atlas->TexPixelsAlpha8[offset] = atlas->TexPixelsAlpha8[offset + 1] = atlas->TexPixelsAlpha8[offset + w] = atlas->TexPixelsAlpha8[offset + w + 1] = 0xFF;
        }
        else
        {
            atlas->TexPixelsRGBA32[offset] = atlas->TexPixelsRGBA32[offset + 1] = atlas->TexPixelsRGBA32[offset + w] = atlas->TexPixelsRGBA32[offset + w + 1] = IM_COL32_WHITE;
        }
    }
    atlas->TexUvWhitePixel = ImVec2((r->X + 0.5f) * atlas->TexUvScale.x, (r->Y + 0.5f) * atlas->TexUvScale.y);
}

static void ImFontAtlasBuildRenderLinesTexData(ImFontAtlas* atlas)
{
    if (atlas->Flags & ImFontAtlasFlags_NoBakedLines)
        return;

    // This generates a triangular shape in the texture, with the various line widths stacked on top of each other to allow interpolation between them
    ImFontAtlasCustomRect* r = atlas->GetCustomRectByIndex(atlas->PackIdLines);
    IM_ASSERT(r->IsPacked());
    for (unsigned int n = 0; n < IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1; n++) // +1 because of the zero-width row
    {
        // Each line consists of at least two empty pixels at the ends, with a line of solid pixels in the middle
        unsigned int y = n;
        unsigned int line_width = n;
        unsigned int pad_left = (r->Width - line_width) / 2;
        unsigned int pad_right = r->Width - (pad_left + line_width);

        // Write each slice
        IM_ASSERT(pad_left + line_width + pad_right == r->Width && y < r->Height); // Make sure we're inside the texture bounds before we start writing pixels
        if (atlas->TexPixelsAlpha8 != NULL)
        {
            unsigned char* write_ptr = &atlas->TexPixelsAlpha8[r->X + ((r->Y + y) * atlas->TexWidth)];
            for (unsigned int i = 0; i < pad_left; i++)
                *(write_ptr + i) = 0x00;

            for (unsigned int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = 0xFF;

            for (unsigned int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = 0x00;
        }
        else
        {
            unsigned int* write_ptr = &atlas->TexPixelsRGBA32[r->X + ((r->Y + y) * atlas->TexWidth)];
            for (unsigned int i = 0; i < pad_left; i++)
                *(write_ptr + i) = IM_COL32(255, 255, 255, 0);

            for (unsigned int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = IM_COL32_WHITE;

            for (unsigned int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = IM_COL32(255, 255, 255, 0);
        }

        // Calculate UVs for this line
        ImVec2 uv0 = ImVec2((float)(r->X + pad_left - 1), (float)(r->Y + y)) * atlas->TexUvScale;
        ImVec2 uv1 = ImVec2((float)(r->X + pad_left + line_width + 1), (float)(r->Y + y + 1)) * atlas->TexUvScale;
        float half_v = (uv0.y + uv1.y) * 0.5f; // Calculate a constant V in the middle of the row to avoid sampling artifacts
        atlas->TexUvLines[n] = ImVec4(uv0.x, half_v, uv1.x, half_v);
    }
}

// Note: this is called / shared by both the stb_truetype and the FreeType builder
void ImFontAtlasBuildInit(ImFontAtlas* atlas)
{
    // Round font size
    // - We started rounding in 1.90 WIP (18991) as our layout system currently doesn't support non-rounded font size well yet.
    // - Note that using io.FontGlobalScale or SetWindowFontScale(), with are legacy-ish, partially supported features, can still lead to unrounded sizes.
    // - We may support it better later and remove this rounding.
    for (ImFontConfig& cfg : atlas->ConfigData)
       cfg.SizePixels = ImTrunc(cfg.SizePixels);

    // Register texture region for mouse cursors or standard white pixels
    if (atlas->PackIdMouseCursors < 0)
    {
        if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
            atlas->PackIdMouseCursors = atlas->AddCustomRectRegular(FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1, FONT_ATLAS_DEFAULT_TEX_DATA_H);
        else
            atlas->PackIdMouseCursors = atlas->AddCustomRectRegular(2, 2);
    }

    // Register texture region for thick lines
    // The +2 here is to give space for the end caps, whilst height +1 is to accommodate the fact we have a zero-width row
    if (atlas->PackIdLines < 0)
    {
        if (!(atlas->Flags & ImFontAtlasFlags_NoBakedLines))
            atlas->PackIdLines = atlas->AddCustomRectRegular(IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 2, IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1);
    }
}

// This is called/shared by both the stb_truetype and the FreeType builder.
void ImFontAtlasBuildFinish(ImFontAtlas* atlas)
{
    // Render into our custom data blocks
    IM_ASSERT(atlas->TexPixelsAlpha8 != NULL || atlas->TexPixelsRGBA32 != NULL);
    ImFontAtlasBuildRenderDefaultTexData(atlas);
    ImFontAtlasBuildRenderLinesTexData(atlas);

    // Register custom rectangle glyphs
    for (int i = 0; i < atlas->CustomRects.Size; i++)
    {
        const ImFontAtlasCustomRect* r = &atlas->CustomRects[i];
        if (r->Font == NULL || r->GlyphID == 0)
            continue;

        // Will ignore ImFontConfig settings: GlyphMinAdvanceX, GlyphMinAdvanceY, GlyphExtraSpacing, PixelSnapH
        IM_ASSERT(r->Font->ContainerAtlas == atlas);
        ImVec2 uv0, uv1;
        atlas->CalcCustomRectUV(r, &uv0, &uv1);
        r->Font->AddGlyph(NULL, (ImWchar)r->GlyphID, r->GlyphOffset.x, r->GlyphOffset.y, r->GlyphOffset.x + r->Width, r->GlyphOffset.y + r->Height, uv0.x, uv0.y, uv1.x, uv1.y, r->GlyphAdvanceX);
    }

    // Build all fonts lookup tables
    for (ImFont* font : atlas->Fonts)
        if (font->DirtyLookupTables)
            font->BuildLookupTable();

    atlas->TexReady = true;
}

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*   ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar*   ImFontAtlas::GetGlyphRangesGreek()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0370, 0x03FF, // Greek and Coptic
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesKorean()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD7A3, // Korean characters
        0xFFFD, 0xFFFD, // Invalid
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseFull()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD, // Invalid
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, ImWchar* out_ranges)
{
    for (int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2)
    {
        out_ranges[0] = out_ranges[1] = (ImWchar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas glyph ranges helpers
//-------------------------------------------------------------------------

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon()
{
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,2,1,2,2,1,1,1,1,1,5,2,1,2,3,3,3,2,2,4,1,1,1,2,1,5,2,3,1,2,1,2,1,1,2,1,1,2,2,1,4,1,1,1,1,5,10,1,2,19,2,1,2,1,2,1,2,1,2,
        1,5,1,6,3,2,1,2,2,1,1,1,4,8,5,1,1,4,1,1,3,1,2,1,5,1,2,1,1,1,10,1,1,5,2,4,6,1,4,2,2,2,12,2,1,1,6,1,1,1,4,1,1,4,6,5,1,4,2,2,4,10,7,1,1,4,2,4,
        2,1,4,3,6,10,12,5,7,2,14,2,9,1,1,6,7,10,4,7,13,1,5,4,8,4,1,1,2,28,5,6,1,1,5,2,5,20,2,2,9,8,11,2,9,17,1,8,6,8,27,4,6,9,20,11,27,6,68,2,2,1,1,
        1,2,1,2,2,7,6,11,3,3,1,1,3,1,2,1,1,1,1,1,3,1,1,8,3,4,1,5,7,2,1,4,4,8,4,2,1,2,1,1,4,5,6,3,6,2,12,3,1,3,9,2,4,3,4,1,5,3,3,1,3,7,1,5,1,1,1,1,2,
        3,4,5,2,3,2,6,1,1,2,1,7,1,7,3,4,5,15,2,2,1,5,3,22,19,2,1,1,1,1,2,5,1,1,1,6,1,1,12,8,2,9,18,22,4,1,1,5,1,16,1,2,7,10,15,1,1,6,2,4,1,2,4,1,6,
        1,1,3,2,4,1,6,4,5,1,2,1,1,2,1,10,3,1,3,2,1,9,3,2,5,7,2,19,4,3,6,1,1,1,1,1,4,3,2,1,1,1,2,5,3,1,1,1,2,2,1,1,2,1,1,2,1,3,1,1,1,3,7,1,4,1,1,2,1,
        1,2,1,2,4,4,3,8,1,1,1,2,1,3,5,1,3,1,3,4,6,2,2,14,4,6,6,11,9,1,15,3,1,28,5,2,5,5,3,1,3,4,5,4,6,14,3,2,3,5,21,2,7,20,10,1,2,19,2,4,28,28,2,3,
        2,1,14,4,1,26,28,42,12,40,3,52,79,5,14,17,3,2,2,11,3,4,6,3,1,8,2,23,4,5,8,10,4,2,7,3,5,1,1,6,3,1,2,2,2,5,28,1,1,7,7,20,5,3,29,3,17,26,1,8,4,
        27,3,6,11,23,5,3,4,6,13,24,16,6,5,10,25,35,7,3,2,3,3,14,3,6,2,6,1,4,2,3,8,2,1,1,3,3,3,4,1,1,13,2,2,4,5,2,1,14,14,1,2,2,1,4,5,2,3,1,14,3,12,
        3,17,2,16,5,1,2,1,8,9,3,19,4,2,2,4,17,25,21,20,28,75,1,10,29,103,4,1,2,1,1,4,2,4,1,2,3,24,2,2,2,1,1,2,1,3,8,1,1,1,2,1,1,3,1,1,1,6,1,5,3,1,1,
        1,3,4,1,1,5,2,1,5,6,13,9,16,1,1,1,1,3,2,3,2,4,5,2,5,2,2,3,7,13,7,2,2,1,1,1,1,2,3,3,2,1,6,4,9,2,1,14,2,14,2,1,18,3,4,14,4,11,41,15,23,15,23,
        176,1,3,4,1,1,1,1,5,3,1,2,3,7,3,1,1,2,1,2,4,4,6,2,4,1,9,7,1,10,5,8,16,29,1,1,2,2,3,1,3,5,2,4,5,4,1,1,2,2,3,3,7,1,6,10,1,17,1,44,4,6,2,1,1,6,
        5,4,2,10,1,6,9,2,8,1,24,1,2,13,7,8,8,2,1,4,1,3,1,3,3,5,2,5,10,9,4,9,12,2,1,6,1,10,1,1,7,7,4,10,8,3,1,13,4,3,1,6,1,3,5,2,1,2,17,16,5,2,16,6,
        1,4,2,1,3,3,6,8,5,11,11,1,3,3,2,4,6,10,9,5,7,4,7,4,7,1,1,4,2,1,3,6,8,7,1,6,11,5,5,3,24,9,4,2,7,13,5,1,8,82,16,61,1,1,1,4,2,2,16,10,3,8,1,1,
        6,4,2,1,3,1,1,1,4,3,8,4,2,2,1,1,1,1,1,6,3,5,1,1,4,6,9,2,1,1,1,2,1,7,2,1,6,1,5,4,4,3,1,8,1,3,3,1,3,2,2,2,2,3,1,6,1,2,1,2,1,3,7,1,8,2,1,2,1,5,
        2,5,3,5,10,1,2,1,1,3,2,5,11,3,9,3,5,1,1,5,9,1,2,1,5,7,9,9,8,1,3,3,3,6,8,2,3,2,1,1,32,6,1,2,15,9,3,7,13,1,3,10,13,2,14,1,13,10,2,1,3,10,4,15,
        2,15,15,10,1,3,9,6,9,32,25,26,47,7,3,2,3,1,6,3,4,3,2,8,5,4,1,9,4,2,2,19,10,6,2,3,8,1,2,2,4,2,1,9,4,4,4,6,4,8,9,2,3,1,1,1,1,3,5,5,1,3,8,4,6,
        2,1,4,12,1,5,3,7,13,2,5,8,1,6,1,2,5,14,6,1,5,2,4,8,15,5,1,23,6,62,2,10,1,1,8,1,2,2,10,4,2,2,9,2,1,1,3,2,3,1,5,3,3,2,1,3,8,1,1,1,11,3,1,1,4,
        3,7,1,14,1,2,3,12,5,2,5,1,6,7,5,7,14,11,1,3,1,8,9,12,2,1,11,8,4,4,2,6,10,9,13,1,1,3,1,5,1,3,2,4,4,1,18,2,3,14,11,4,29,4,2,7,1,3,13,9,2,2,5,
        3,5,20,7,16,8,5,72,34,6,4,22,12,12,28,45,36,9,7,39,9,191,1,1,1,4,11,8,4,9,2,3,22,1,1,1,1,4,17,1,7,7,1,11,31,10,2,4,8,2,3,2,1,4,2,16,4,32,2,
        3,19,13,4,9,1,5,2,14,8,1,1,3,6,19,6,5,1,16,6,2,10,8,5,1,2,3,1,5,5,1,11,6,6,1,3,3,2,6,3,8,1,1,4,10,7,5,7,7,5,8,9,2,1,3,4,1,1,3,1,3,3,2,6,16,
        1,4,6,3,1,10,6,1,3,15,2,9,2,10,25,13,9,16,6,2,2,10,11,4,3,9,1,2,6,6,5,4,30,40,1,10,7,12,14,33,6,3,6,7,3,1,3,1,11,14,4,9,5,12,11,49,18,51,31,
        140,31,2,2,1,5,1,8,1,10,1,4,4,3,24,1,10,1,3,6,6,16,3,4,5,2,1,4,2,57,10,6,22,2,22,3,7,22,6,10,11,36,18,16,33,36,2,5,5,1,1,1,4,10,1,4,13,2,7,
        5,2,9,3,4,1,7,43,3,7,3,9,14,7,9,1,11,1,1,3,7,4,18,13,1,14,1,3,6,10,73,2,2,30,6,1,11,18,19,13,22,3,46,42,37,89,7,3,16,34,2,2,3,9,1,7,1,1,1,2,
        2,4,10,7,3,10,3,9,5,28,9,2,6,13,7,3,1,3,10,2,7,2,11,3,6,21,54,85,2,1,4,2,2,1,39,3,21,2,2,5,1,1,1,4,1,1,3,4,15,1,3,2,4,4,2,3,8,2,20,1,8,7,13,
        4,1,26,6,2,9,34,4,21,52,10,4,4,1,5,12,2,11,1,7,2,30,12,44,2,30,1,1,3,6,16,9,17,39,82,2,2,24,7,1,7,3,16,9,14,44,2,1,2,1,2,3,5,2,4,1,6,7,5,3,
        2,6,1,11,5,11,2,1,18,19,8,1,3,24,29,2,1,3,5,2,2,1,13,6,5,1,46,11,3,5,1,1,5,8,2,10,6,12,6,3,7,11,2,4,16,13,2,5,1,1,2,2,5,2,28,5,2,23,10,8,4,
        4,22,39,95,38,8,14,9,5,1,13,5,4,3,13,12,11,1,9,1,27,37,2,5,4,4,63,211,95,2,2,2,1,3,5,2,1,1,2,2,1,1,1,3,2,4,1,2,1,1,5,2,2,1,1,2,3,1,3,1,1,1,
        3,1,4,2,1,3,6,1,1,3,7,15,5,3,2,5,3,9,11,4,2,22,1,6,3,8,7,1,4,28,4,16,3,3,25,4,4,27,27,1,4,1,2,2,7,1,3,5,2,28,8,2,14,1,8,6,16,25,3,3,3,14,3,
        3,1,1,2,1,4,6,3,8,4,1,1,1,2,3,6,10,6,2,3,18,3,2,5,5,4,3,1,5,2,5,4,23,7,6,12,6,4,17,11,9,5,1,1,10,5,12,1,1,11,26,33,7,3,6,1,17,7,1,5,12,1,11,
        2,4,1,8,14,17,23,1,2,1,7,8,16,11,9,6,5,2,6,4,16,2,8,14,1,11,8,9,1,1,1,9,25,4,11,19,7,2,15,2,12,8,52,7,5,19,2,16,4,36,8,1,16,8,24,26,4,6,2,9,
        5,4,36,3,28,12,25,15,37,27,17,12,59,38,5,32,127,1,2,9,17,14,4,1,2,1,1,8,11,50,4,14,2,19,16,4,17,5,4,5,26,12,45,2,23,45,104,30,12,8,3,10,2,2,
        3,3,1,4,20,7,2,9,6,15,2,20,1,3,16,4,11,15,6,134,2,5,59,1,2,2,2,1,9,17,3,26,137,10,211,59,1,2,4,1,4,1,1,1,2,6,2,3,1,1,2,3,2,3,1,3,4,4,2,3,3,
        1,4,3,1,7,2,2,3,1,2,1,3,3,3,2,2,3,2,1,3,14,6,1,3,2,9,6,15,27,9,34,145,1,1,2,1,1,1,1,2,1,1,1,1,2,2,2,3,1,2,1,1,1,2,3,5,8,3,5,2,4,1,3,2,2,2,12,
        4,1,1,1,10,4,5,1,20,4,16,1,15,9,5,12,2,9,2,5,4,2,26,19,7,1,26,4,30,12,15,42,1,6,8,172,1,1,4,2,1,1,11,2,2,4,2,1,2,1,10,8,1,2,1,4,5,1,2,5,1,8,
        4,1,3,4,2,1,6,2,1,3,4,1,2,1,1,1,1,12,5,7,2,4,3,1,1,1,3,3,6,1,2,2,3,3,3,2,1,2,12,14,11,6,6,4,12,2,8,1,7,10,1,35,7,4,13,15,4,3,23,21,28,52,5,
        26,5,6,1,7,10,2,7,53,3,2,1,1,1,2,163,532,1,10,11,1,3,3,4,8,2,8,6,2,2,23,22,4,2,2,4,2,1,3,1,3,3,5,9,8,2,1,2,8,1,10,2,12,21,20,15,105,2,3,1,1,
        3,2,3,1,1,2,5,1,4,15,11,19,1,1,1,1,5,4,5,1,1,2,5,3,5,12,1,2,5,1,11,1,1,15,9,1,4,5,3,26,8,2,1,3,1,1,15,19,2,12,1,2,5,2,7,2,19,2,20,6,26,7,5,
        2,2,7,34,21,13,70,2,128,1,1,2,1,1,2,1,1,3,2,2,2,15,1,4,1,3,4,42,10,6,1,49,85,8,1,2,1,1,4,4,2,3,6,1,5,7,4,3,211,4,1,2,1,2,5,1,2,4,2,2,6,5,6,
        10,3,4,48,100,6,2,16,296,5,27,387,2,2,3,7,16,8,5,38,15,39,21,9,10,3,7,59,13,27,21,47,5,21,6
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD  // Invalid
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        IMGUI_MEMCPY(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // 2999 ideograms code points for Japanese
    // - 2136 Joyo (meaning "for regular use" or "for common use") Kanji code points
    // - 863 Jinmeiyo (meaning "for personal name") Kanji code points
    // - Sourced from official information provided by the government agencies of Japan:
    //   - List of Joyo Kanji by the Agency for Cultural Affairs
    //     - https://www.bunka.go.jp/kokugo_nihongo/sisaku/joho/joho/kijun/naikaku/kanji/
    //   - List of Jinmeiyo Kanji by the Ministry of Justice
    //     - http://www.moj.go.jp/MINJI/minji86.html
    //   - Available under the terms of the Creative Commons Attribution 4.0 International (CC BY 4.0).
    //     - https://creativecommons.org/licenses/by/4.0/legalcode
    // - You can generate this code by the script at:
    //   - https://github.com/vaiorabbit/everyday_use_kanji
    // - References:
    //   - List of Joyo Kanji
    //     - (Wikipedia) https://en.wikipedia.org/wiki/List_of_j%C5%8Dy%C5%8D_kanji
    //   - List of Jinmeiyo Kanji
    //     - (Wikipedia) https://en.wikipedia.org/wiki/Jinmeiy%C5%8D_kanji
    // - Missing 1 Joyo Kanji: U+20B9F (Kun'yomi: Shikaru, On'yomi: Shitsu,shichi), see https://github.com/ocornut/imgui/pull/3627 for details.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,3,2,2,1,5,3,5,7,5,6,1,2,1,7,2,6,3,1,8,1,1,4,1,1,18,2,11,2,6,2,1,2,1,5,1,2,1,3,1,2,1,2,3,3,1,1,2,3,1,1,1,12,7,9,1,4,5,1,
        1,2,1,10,1,1,9,2,2,4,5,6,9,3,1,1,1,1,9,3,18,5,2,2,2,2,1,6,3,7,1,1,1,1,2,2,4,2,1,23,2,10,4,3,5,2,4,10,2,4,13,1,6,1,9,3,1,1,6,6,7,6,3,1,2,11,3,
        2,2,3,2,15,2,2,5,4,3,6,4,1,2,5,2,12,16,6,13,9,13,2,1,1,7,16,4,7,1,19,1,5,1,2,2,7,7,8,2,6,5,4,9,18,7,4,5,9,13,11,8,15,2,1,1,1,2,1,2,2,1,2,2,8,
        2,9,3,3,1,1,4,4,1,1,1,4,9,1,4,3,5,5,2,7,5,3,4,8,2,1,13,2,3,3,1,14,1,1,4,5,1,3,6,1,5,2,1,1,3,3,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,1,1,1,12,3,3,9,5,
        2,6,1,5,6,1,2,3,18,2,4,14,4,1,3,6,1,1,6,3,5,5,3,2,2,2,2,12,3,1,4,2,3,2,3,11,1,7,4,1,2,1,3,17,1,9,1,24,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,2,4,15,1,
        1,2,1,1,2,1,5,2,5,20,2,5,9,1,10,8,7,6,1,1,1,1,1,1,6,2,1,2,8,1,1,1,1,5,1,1,3,1,1,1,1,3,1,1,12,4,1,3,1,1,1,1,1,10,3,1,7,5,13,1,2,3,4,6,1,1,30,
        2,9,9,1,15,38,11,3,1,8,24,7,1,9,8,10,2,1,9,31,2,13,6,2,9,4,49,5,2,15,2,1,10,2,1,1,1,2,2,6,15,30,35,3,14,18,8,1,16,10,28,12,19,45,38,1,3,2,3,
        13,2,1,7,3,6,5,3,4,3,1,5,7,8,1,5,3,18,5,3,6,1,21,4,24,9,24,40,3,14,3,21,3,2,1,2,4,2,3,1,15,15,6,5,1,1,3,1,5,6,1,9,7,3,3,2,1,4,3,8,21,5,16,4,
        5,2,10,11,11,3,6,3,2,9,3,6,13,1,2,1,1,1,1,11,12,6,6,1,4,2,6,5,2,1,1,3,3,6,13,3,1,1,5,1,2,3,3,14,2,1,2,2,2,5,1,9,5,1,1,6,12,3,12,3,4,13,2,14,
        2,8,1,17,5,1,16,4,2,2,21,8,9,6,23,20,12,25,19,9,38,8,3,21,40,25,33,13,4,3,1,4,1,2,4,1,2,5,26,2,1,1,2,1,3,6,2,1,1,1,1,1,1,2,3,1,1,1,9,2,3,1,1,
        1,3,6,3,2,1,1,6,6,1,8,2,2,2,1,4,1,2,3,2,7,3,2,4,1,2,1,2,2,1,1,1,1,1,3,1,2,5,4,10,9,4,9,1,1,1,1,1,1,5,3,2,1,6,4,9,6,1,10,2,31,17,8,3,7,5,40,1,
        7,7,1,6,5,2,10,7,8,4,15,39,25,6,28,47,18,10,7,1,3,1,1,2,1,1,1,3,3,3,1,1,1,3,4,2,1,4,1,3,6,10,7,8,6,2,2,1,3,3,2,5,8,7,9,12,2,15,1,1,4,1,2,1,1,
        1,3,2,1,3,3,5,6,2,3,2,10,1,4,2,8,1,1,1,11,6,1,21,4,16,3,1,3,1,4,2,3,6,5,1,3,1,1,3,3,4,6,1,1,10,4,2,7,10,4,7,4,2,9,4,3,1,1,1,4,1,8,3,4,1,3,1,
        6,1,4,2,1,4,7,2,1,8,1,4,5,1,1,2,2,4,6,2,7,1,10,1,1,3,4,11,10,8,21,4,6,1,3,5,2,1,2,28,5,5,2,3,13,1,2,3,1,4,2,1,5,20,3,8,11,1,3,3,3,1,8,10,9,2,
        10,9,2,3,1,1,2,4,1,8,3,6,1,7,8,6,11,1,4,29,8,4,3,1,2,7,13,1,4,1,6,2,6,12,12,2,20,3,2,3,6,4,8,9,2,7,34,5,1,18,6,1,1,4,4,5,7,9,1,2,2,4,3,4,1,7,
        2,2,2,6,2,3,25,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,5,3,4,4,3,2,1,1,4,1,2,1,1,3,1,11,1,6,3,1,7,3,6,2,8,8,6,9,3,4,11,3,2,10,12,2,5,11,1,6,4,5,
        3,1,8,5,4,6,6,3,5,1,1,3,2,1,2,2,6,17,12,1,10,1,6,12,1,6,6,19,9,6,16,1,13,4,4,15,7,17,6,11,9,15,12,6,7,2,1,2,2,15,9,3,21,4,6,49,18,7,3,2,3,1,
        6,8,2,2,6,2,9,1,3,6,4,4,1,2,16,2,5,2,1,6,2,3,5,3,1,2,5,1,2,1,9,3,1,8,6,4,8,11,3,1,1,1,1,3,1,13,8,4,1,3,2,2,1,4,1,11,1,5,2,1,5,2,5,8,6,1,1,7,
        4,3,8,3,2,7,2,1,5,1,5,2,4,7,6,2,8,5,1,11,4,5,3,6,18,1,2,13,3,3,1,21,1,1,4,1,4,1,1,1,8,1,2,2,7,1,2,4,2,2,9,2,1,1,1,4,3,6,3,12,5,1,1,1,5,6,3,2,
        4,8,2,2,4,2,7,1,8,9,5,2,3,2,1,3,2,13,7,14,6,5,1,1,2,1,4,2,23,2,1,1,6,3,1,4,1,15,3,1,7,3,9,14,1,3,1,4,1,1,5,8,1,3,8,3,8,15,11,4,14,4,4,2,5,5,
        1,7,1,6,14,7,7,8,5,15,4,8,6,5,6,2,1,13,1,20,15,11,9,2,5,6,2,11,2,6,2,5,1,5,8,4,13,19,25,4,1,1,11,1,34,2,5,9,14,6,2,2,6,1,1,14,1,3,14,13,1,6,
        12,21,14,14,6,32,17,8,32,9,28,1,2,4,11,8,3,1,14,2,5,15,1,1,1,1,3,6,4,1,3,4,11,3,1,1,11,30,1,5,1,4,1,5,8,1,1,3,2,4,3,17,35,2,6,12,17,3,1,6,2,
        1,1,12,2,7,3,3,2,1,16,2,8,3,6,5,4,7,3,3,8,1,9,8,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,4,3,7,5,8,3,3,3,3,3,3,1,23,10,3,1,2,2,6,3,1,16,1,16,
        22,3,10,4,11,6,9,7,7,3,6,2,2,2,4,10,2,1,1,2,8,7,1,6,4,1,3,3,3,5,10,12,12,2,3,12,8,15,1,1,16,6,6,1,5,9,11,4,11,4,2,6,12,1,17,5,13,1,4,9,5,1,11,
        2,1,8,1,5,7,28,8,3,5,10,2,17,3,38,22,1,2,18,12,10,4,38,18,1,4,44,19,4,1,8,4,1,12,1,4,31,12,1,14,7,75,7,5,10,6,6,13,3,2,11,11,3,2,5,28,15,6,18,
        18,5,6,4,3,16,1,7,18,7,36,3,5,3,1,7,1,9,1,10,7,2,4,2,6,2,9,7,4,3,32,12,3,7,10,2,23,16,3,1,12,3,31,4,11,1,3,8,9,5,1,30,15,6,12,3,2,2,11,19,9,
        14,2,6,2,3,19,13,17,5,3,3,25,3,14,1,1,1,36,1,3,2,19,3,13,36,9,13,31,6,4,16,34,2,5,4,2,3,3,5,1,1,1,4,3,1,17,3,2,3,5,3,1,3,2,3,5,6,3,12,11,1,3,
        1,2,26,7,12,7,2,14,3,3,7,7,11,25,25,28,16,4,36,1,2,1,6,2,1,9,3,27,17,4,3,4,13,4,1,3,2,2,1,10,4,2,4,6,3,8,2,1,18,1,1,24,2,2,4,33,2,3,63,7,1,6,
        40,7,3,4,4,2,4,15,18,1,16,1,1,11,2,41,14,1,3,18,13,3,2,4,16,2,17,7,15,24,7,18,13,44,2,2,3,6,1,1,7,5,1,7,1,4,3,3,5,10,8,2,3,1,8,1,1,27,4,2,1,
        12,1,2,1,10,6,1,6,7,5,2,3,7,11,5,11,3,6,6,2,3,15,4,9,1,1,2,1,2,11,2,8,12,8,5,4,2,3,1,5,2,2,1,14,1,12,11,4,1,11,17,17,4,3,2,5,5,7,3,1,5,9,9,8,
        2,5,6,6,13,13,2,1,2,6,1,2,2,49,4,9,1,2,10,16,7,8,4,3,2,23,4,58,3,29,1,14,19,19,11,11,2,7,5,1,3,4,6,2,18,5,12,12,17,17,3,3,2,4,1,6,2,3,4,3,1,
        1,1,1,5,1,1,9,1,3,1,3,6,1,8,1,1,2,6,4,14,3,1,4,11,4,1,3,32,1,2,4,13,4,1,2,4,2,1,3,1,11,1,4,2,1,4,4,6,3,5,1,6,5,7,6,3,23,3,5,3,5,3,3,13,3,9,10,
        1,12,10,2,3,18,13,7,160,52,4,2,2,3,2,14,5,4,12,4,6,4,1,20,4,11,6,2,12,27,1,4,1,2,2,7,4,5,2,28,3,7,25,8,3,19,3,6,10,2,2,1,10,2,5,4,1,3,4,1,5,
        3,2,6,9,3,6,2,16,3,3,16,4,5,5,3,2,1,2,16,15,8,2,6,21,2,4,1,22,5,8,1,1,21,11,2,1,11,11,19,13,12,4,2,3,2,3,6,1,8,11,1,4,2,9,5,2,1,11,2,9,1,1,2,
        14,31,9,3,4,21,14,4,8,1,7,2,2,2,5,1,4,20,3,3,4,10,1,11,9,8,2,1,4,5,14,12,14,2,17,9,6,31,4,14,1,20,13,26,5,2,7,3,6,13,2,4,2,19,6,2,2,18,9,3,5,
        12,12,14,4,6,2,3,6,9,5,22,4,5,25,6,4,8,5,2,6,27,2,35,2,16,3,7,8,8,6,6,5,9,17,2,20,6,19,2,13,3,1,1,1,4,17,12,2,14,7,1,4,18,12,38,33,2,10,1,1,
        2,13,14,17,11,50,6,33,20,26,74,16,23,45,50,13,38,33,6,6,7,4,4,2,1,3,2,5,8,7,8,9,3,11,21,9,13,1,3,10,6,7,1,2,2,18,5,5,1,9,9,2,68,9,19,13,2,5,
        1,4,4,7,4,13,3,9,10,21,17,3,26,2,1,5,2,4,5,4,1,7,4,7,3,4,2,1,6,1,1,20,4,1,9,2,2,1,3,3,2,3,2,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,3,2,10,3,5,3,4,4,
        3,4,16,1,6,1,10,2,4,2,1,1,2,10,11,2,2,3,1,24,31,4,10,10,2,5,12,16,164,15,4,16,7,9,15,19,17,1,2,1,1,5,1,1,1,1,1,3,1,4,3,1,3,1,3,1,2,1,1,3,3,7,
        2,8,1,2,2,2,1,3,4,3,7,8,12,92,2,10,3,1,3,14,5,25,16,42,4,7,7,4,2,21,5,27,26,27,21,25,30,31,2,1,5,13,3,22,5,6,6,11,9,12,1,5,9,7,5,5,22,60,3,5,
        13,1,1,8,1,1,3,3,2,1,9,3,3,18,4,1,2,3,7,6,3,1,2,3,9,1,3,1,3,2,1,3,1,1,1,2,1,11,3,1,6,9,1,3,2,3,1,2,1,5,1,1,4,3,4,1,2,2,4,4,1,7,2,1,2,2,3,5,13,
        18,3,4,14,9,9,4,16,3,7,5,8,2,6,48,28,3,1,1,4,2,14,8,2,9,2,1,15,2,4,3,2,10,16,12,8,7,1,1,3,1,1,1,2,7,4,1,6,4,38,39,16,23,7,15,15,3,2,12,7,21,
        37,27,6,5,4,8,2,10,8,8,6,5,1,2,1,3,24,1,16,17,9,23,10,17,6,1,51,55,44,13,294,9,3,6,2,4,2,2,15,1,1,1,13,21,17,68,14,8,9,4,1,4,9,3,11,7,1,1,1,
        5,6,3,2,1,1,1,2,3,8,1,2,2,4,1,5,5,2,1,4,3,7,13,4,1,4,1,3,1,1,1,5,5,10,1,6,1,5,2,1,5,2,4,1,4,5,7,3,18,2,9,11,32,4,3,3,2,4,7,11,16,9,11,8,13,38,
        32,8,4,2,1,1,2,1,2,4,4,1,1,1,4,1,21,3,11,1,16,1,1,6,1,3,2,4,9,8,57,7,44,1,3,3,13,3,10,1,1,7,5,2,7,21,47,63,3,15,4,7,1,16,1,1,2,8,2,3,42,15,4,
        1,29,7,22,10,3,78,16,12,20,18,4,67,11,5,1,3,15,6,21,31,32,27,18,13,71,35,5,142,4,10,1,2,50,19,33,16,35,37,16,19,27,7,1,133,19,1,4,8,7,20,1,4,
        4,1,10,3,1,6,1,2,51,5,40,15,24,43,22928,11,1,13,154,70,3,1,1,7,4,10,1,2,1,1,2,1,2,1,2,2,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
        3,2,1,1,1,1,2,1,1,
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD  // Invalid
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00)*2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        IMGUI_MEMCPY(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesThai()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x2010, 0x205E, // Punctuations
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesVietnamese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0102, 0x0103,
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,
        0,
    };
    return &ranges[0];
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontGlyphRangesBuilder
//-----------------------------------------------------------------------------

void ImFontGlyphRangesBuilder::AddText(const char* text, const char* text_end)
{
    while (text_end ? (text < text_end) : *text)
    {
        unsigned int c = 0;
        int c_len = ImTextCharFromUtf8(&c, text, text_end);
        text += c_len;
        if (c_len == 0)
            break;
        AddChar((ImWchar)c);
    }
}

void ImFontGlyphRangesBuilder::AddRanges(const ImWchar* ranges)
{
    for (; ranges[0]; ranges += 2)
        for (unsigned int c = ranges[0]; c <= ranges[1] && c <= IM_UNICODE_CODEPOINT_MAX; c++) //-V560
            AddChar((ImWchar)c);
}

void ImFontGlyphRangesBuilder::BuildRanges(ImVector<ImWchar>* out_ranges)
{
    const int max_codepoint = IM_UNICODE_CODEPOINT_MAX;
    for (int n = 0; n <= max_codepoint; n++)
        if (GetBit(n))
        {
            out_ranges->push_back((ImWchar)n);
            while (n < max_codepoint && GetBit(n + 1))
                n++;
            out_ranges->push_back((ImWchar)n);
        }
    out_ranges->push_back(0);
}

//-----------------------------------------------------------------------------
// [SECTION] ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    FallbackChar = (ImWchar)-1;
    EllipsisChar = (ImWchar)-1;
    EllipsisWidth = EllipsisCharStep = 0.0f;
    EllipsisCharCount = 0;
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    ConfigData = NULL;
    ConfigDataCount = 0;
    DirtyLookupTables = false;
    Scale = 1.0f;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
    IMGUI_MEMSET(Used4kPagesMap, 0, sizeof(Used4kPagesMap));
}

ImFont::~ImFont()
{
    ClearOutputData();
}

void    ImFont::ClearOutputData()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    Glyphs.clear();
    IndexAdvanceX.clear();
    IndexLookup.clear();
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    DirtyLookupTables = true;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

static ImWchar FindFirstExistingGlyph(ImFont* font, const ImWchar* candidate_chars, int candidate_chars_count)
{
    for (int n = 0; n < candidate_chars_count; n++)
        if (font->FindGlyphNoFallback(candidate_chars[n]) != NULL)
            return candidate_chars[n];
    return (ImWchar)-1;
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (int i = 0; i != Glyphs.Size; i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    // Build lookup table
    IM_ASSERT(Glyphs.Size > 0 && "Font has not loaded glyph!");
    IM_ASSERT(Glyphs.Size < 0xFFFF); // -1 is reserved
    IndexAdvanceX.clear();
    IndexLookup.clear();
    DirtyLookupTables = false;
    IMGUI_MEMSET(Used4kPagesMap, 0, sizeof(Used4kPagesMap));
    GrowIndex(max_codepoint + 1);
    for (int i = 0; i < Glyphs.Size; i++)
    {
        int codepoint = (int)Glyphs[i].Codepoint;
        IndexAdvanceX[codepoint] = Glyphs[i].AdvanceX;
        IndexLookup[codepoint] = (ImWchar)i;

        // Mark 4K page as used
        const int page_n = codepoint / 4096;
        Used4kPagesMap[page_n >> 3] |= 1 << (page_n & 7);
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (FindGlyph((ImWchar)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times (FIXME: Flaky)
            Glyphs.resize(Glyphs.Size + 1);
        ImFontGlyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((ImWchar)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.AdvanceX *= IM_TABSIZE;
        IndexAdvanceX[(int)tab_glyph.Codepoint] = (float)tab_glyph.AdvanceX;
        IndexLookup[(int)tab_glyph.Codepoint] = (ImWchar)(Glyphs.Size - 1);
    }

    // Mark special glyphs as not visible (note that AddGlyph already mark as non-visible glyphs with zero-size polygons)
    SetGlyphVisible((ImWchar)' ', false);
    SetGlyphVisible((ImWchar)'\t', false);

    // Setup Fallback character
    const ImWchar fallback_chars[] = { (ImWchar)IM_UNICODE_CODEPOINT_INVALID, (ImWchar)'?', (ImWchar)' ' };
    FallbackGlyph = FindGlyphNoFallback(FallbackChar);
    if (FallbackGlyph == NULL)
    {
        FallbackChar = FindFirstExistingGlyph(this, fallback_chars, IM_ARRAYSIZE(fallback_chars));
        FallbackGlyph = FindGlyphNoFallback(FallbackChar);
        if (FallbackGlyph == NULL)
        {
            FallbackGlyph = &Glyphs.back();
            FallbackChar = (ImWchar)FallbackGlyph->Codepoint;
        }
    }
    FallbackAdvanceX = FallbackGlyph->AdvanceX;
    for (int i = 0; i < max_codepoint + 1; i++)
        if (IndexAdvanceX[i] < 0.0f)
            IndexAdvanceX[i] = FallbackAdvanceX;

    // Setup Ellipsis character. It is required for rendering elided text. We prefer using U+2026 (horizontal ellipsis).
    // However some old fonts may contain ellipsis at U+0085. Here we auto-detect most suitable ellipsis character.
    // FIXME: Note that 0x2026 is rarely included in our font ranges. Because of this we are more likely to use three individual dots.
    const ImWchar ellipsis_chars[] = { (ImWchar)0x2026, (ImWchar)0x0085 };
    const ImWchar dots_chars[] = { (ImWchar)'.', (ImWchar)0xFF0E };
    if (EllipsisChar == (ImWchar)-1)
        EllipsisChar = FindFirstExistingGlyph(this, ellipsis_chars, IM_ARRAYSIZE(ellipsis_chars));
    const ImWchar dot_char = FindFirstExistingGlyph(this, dots_chars, IM_ARRAYSIZE(dots_chars));
    if (EllipsisChar != (ImWchar)-1)
    {
        EllipsisCharCount = 1;
        EllipsisWidth = EllipsisCharStep = FindGlyph(EllipsisChar)->X1;
    }
    else if (dot_char != (ImWchar)-1)
    {
        const ImFontGlyph* glyph = FindGlyph(dot_char);
        EllipsisChar = dot_char;
        EllipsisCharCount = 3;
        EllipsisCharStep = (glyph->X1 - glyph->X0) + 1.0f;
        EllipsisWidth = EllipsisCharStep * 3.0f - 1.0f;
    }
}

// API is designed this way to avoid exposing the 4K page size
// e.g. use with IsGlyphRangeUnused(0, 255)
bool ImFont::IsGlyphRangeUnused(unsigned int c_begin, unsigned int c_last)
{
    unsigned int page_begin = (c_begin / 4096);
    unsigned int page_last = (c_last / 4096);
    for (unsigned int page_n = page_begin; page_n <= page_last; page_n++)
        if ((page_n >> 3) < sizeof(Used4kPagesMap))
            if (Used4kPagesMap[page_n >> 3] & (1 << (page_n & 7)))
                return false;
    return true;
}

void ImFont::SetGlyphVisible(ImWchar c, bool visible)
{
    if (ImFontGlyph* glyph = (ImFontGlyph*)(void*)FindGlyph((ImWchar)c))
        glyph->Visible = visible ? 1 : 0;
}

void ImFont::GrowIndex(int new_size)
{
    IM_ASSERT(IndexAdvanceX.Size == IndexLookup.Size);
    if (new_size <= IndexLookup.Size)
        return;
    IndexAdvanceX.resize(new_size, -1.0f);
    IndexLookup.resize(new_size, (ImWchar)-1);
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
// 'cfg' is not necessarily == 'this->ConfigData' because multiple source fonts+configs can be used to build one target font.
void ImFont::AddGlyph(const ImFontConfig* cfg, ImWchar codepoint, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x)
{
    if (cfg != NULL)
    {
        // Clamp & recenter if needed
        const float advance_x_original = advance_x;
        advance_x = ImClamp(advance_x, cfg->GlyphMinAdvanceX, cfg->GlyphMaxAdvanceX);
        if (advance_x != advance_x_original)
        {
            float char_off_x = cfg->PixelSnapH ? ImTrunc((advance_x - advance_x_original) * 0.5f) : (advance_x - advance_x_original) * 0.5f;
            x0 += char_off_x;
            x1 += char_off_x;
        }

        // Snap to pixel
        if (cfg->PixelSnapH)
            advance_x = IM_ROUND(advance_x);

        // Bake spacing
        advance_x += cfg->GlyphExtraSpacing.x;
    }

    Glyphs.resize(Glyphs.Size + 1);
    ImFontGlyph& glyph = Glyphs.back();
    glyph.Codepoint = (unsigned int)codepoint;
    glyph.Visible = (x0 != x1) && (y0 != y1);
    glyph.Colored = false;
    glyph.X0 = x0;
    glyph.Y0 = y0;
    glyph.X1 = x1;
    glyph.Y1 = y1;
    glyph.U0 = u0;
    glyph.V0 = v0;
    glyph.U1 = u1;
    glyph.V1 = v1;
    glyph.AdvanceX = advance_x;

    // Compute rough surface usage metrics (+1 to account for average padding, +0.99 to round)
    // We use (U1-U0)*TexWidth instead of X1-X0 to account for oversampling.
    float pad = ContainerAtlas->TexGlyphPadding + 0.99f;
    DirtyLookupTables = true;
    MetricsTotalSurface += (int)((glyph.U1 - glyph.U0) * ContainerAtlas->TexWidth + pad) * (int)((glyph.V1 - glyph.V0) * ContainerAtlas->TexHeight + pad);
}

void ImFont::AddRemapChar(ImWchar dst, ImWchar src, bool overwrite_dst)
{
    IM_ASSERT(IndexLookup.Size > 0);    // Currently this can only be called AFTER the font has been built, aka after calling ImFontAtlas::GetTexDataAs*() function.
    unsigned int index_size = (unsigned int)IndexLookup.Size;

    if (dst < index_size && IndexLookup.Data[dst] == (ImWchar)-1 && !overwrite_dst) // 'dst' already exists
        return;
    if (src >= index_size && dst >= index_size) // both 'dst' and 'src' don't exist -> no-op
        return;

    GrowIndex(dst + 1);
    IndexLookup[dst] = (src < index_size) ? IndexLookup.Data[src] : (ImWchar)-1;
    IndexAdvanceX[dst] = (src < index_size) ? IndexAdvanceX.Data[src] : 1.0f;
}

const ImFontGlyph* ImFont::FindGlyph(ImWchar c) const
{
    if (c >= (size_t)IndexLookup.Size)
        return FallbackGlyph;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return FallbackGlyph;
    return &Glyphs.Data[i];
}

const ImFontGlyph* ImFont::FindGlyphNoFallback(ImWchar c) const
{
    if (c >= (size_t)IndexLookup.Size)
        return NULL;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return NULL;
    return &Glyphs.Data[i];
}

// Wrapping skips upcoming blanks
static inline const char* CalcWordWrapNextLineStartA(const char* text, const char* text_end)
{
    while (text < text_end && ImCharIsBlankA(*text))
        text++;
    if (*text == '\n')
        text++;
    return text;
}

// Simple word-wrapping for English, not full-featured. Please submit failing cases!
// This will return the next location to wrap from. If no wrapping if necessary, this will fast-forward to e.g. text_end.
// FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)
const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const
{
    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"
    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    IM_ASSERT(text_end != NULL);
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX);
        if (ImCharIsBlankW(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = (c != '.' && c != ',' && c != ';' && c != '!' && c != '?' && c != '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width > wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
    // +1 may not be a character start point in UTF-8 but it's ok because caller loops use (text >= word_wrap_eol).
    if (s == text && text < text_end)
        return s + 1;
    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + IMGUI_STRLEN(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    const float scale = size / FontSize;

    ImVec2 text_size = ImVec2(0, 0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;
                s = CalcWordWrapNextLineStartA(s, text_end); // Wrapping skips upcoming blanks
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.x = ImMax(text_size.x, line_width);
                text_size.y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX) * scale;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
void ImFont::RenderChar(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, ImWchar c) const
{
    const ImFontGlyph* glyph = FindGlyph(c);
    if (!glyph || !glyph->Visible)
        return;
    if (glyph->Colored)
        col |= ~IM_COL32_A_MASK;
    float scale = (size >= 0.0f) ? (size / FontSize) : 1.0f;
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);
    draw_list->PrimReserve(6, 4);
    draw_list->PrimRectUV(ImVec2(x + glyph->X0 * scale, y + glyph->Y0 * scale), ImVec2(x + glyph->X1 * scale, y + glyph->Y1 * scale), ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), col);
}

// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
void ImFont::RenderText(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip) const
{
    if (!text_end)
        text_end = text_begin + IMGUI_STRLEN(text_begin); // ImGui:: functions generally already provides a valid text_end, so this is merely to handle direct calls.

    // Align to be pixel perfect
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);
    if (y > clip_rect.w)
        return;

    const float start_x = x;
    const float scale = size / FontSize;
    const float line_height = FontSize * scale;
    const bool word_wrap_enabled = (wrap_width > 0.0f);

    // Fast-forward to first visible line
    const char* s = text_begin;
    if (y + line_height < clip_rect.y)
        while (y + line_height < clip_rect.y && s < text_end)
        {
            const char* line_end = (const char*)IMGUI_MEMCHR(s, '\n', text_end - s);
            if (word_wrap_enabled)
            {
                // FIXME-OPT: This is not optimal as do first do a search for \n before calling CalcWordWrapPositionA().
                // If the specs for CalcWordWrapPositionA() were reworked to optionally return on \n we could combine both.
                // However it is still better than nothing performing the fast-forward!
                s = CalcWordWrapPositionA(scale, s, line_end ? line_end : text_end, wrap_width);
                s = CalcWordWrapNextLineStartA(s, text_end);
            }
            else
            {
                s = line_end ? line_end + 1 : text_end;
            }
            y += line_height;
        }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if (text_end - s > 10000 && !word_wrap_enabled)
    {
        const char* s_end = s;
        float y_end = y;
        while (y_end < clip_rect.w && s_end < text_end)
        {
            s_end = (const char*)IMGUI_MEMCHR(s_end, '\n', text_end - s_end);
            s_end = s_end ? s_end + 1 : text_end;
            y_end += line_height;
        }
        text_end = s_end;
    }
    if (s == text_end)
        return;

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int vtx_count_max = (int)(text_end - s) * 4;
    const int idx_count_max = (int)(text_end - s) * 6;
    const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
    draw_list->PrimReserve(idx_count_max, vtx_count_max);
    ImDrawVert*  vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx*   idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_index = draw_list->_VtxCurrentIdx;

    const ImU32 col_untinted = col | ~IM_COL32_A_MASK;
    const char* word_wrap_eol = NULL;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - start_x));

            if (s >= word_wrap_eol)
            {
                x = start_x;
                y += line_height;
                word_wrap_eol = NULL;
                s = CalcWordWrapNextLineStartA(s, text_end); // Wrapping skips upcoming blanks
                continue;
            }
        }

        // Decode and advance source
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                x = start_x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                continue;
            }
            if (c == '\r')
                continue;
        }

        const ImFontGlyph* glyph = FindGlyph((ImWchar)c);
        if (glyph == NULL)
            continue;

        float char_width = glyph->AdvanceX * scale;
        if (glyph->Visible)
        {
            // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
            float x1 = x + glyph->X0 * scale;
            float x2 = x + glyph->X1 * scale;
            float y1 = y + glyph->Y0 * scale;
            float y2 = y + glyph->Y1 * scale;
            if (x1 <= clip_rect.z && x2 >= clip_rect.x)
            {
                // Render a character
                float u1 = glyph->U0;
                float v1 = glyph->V0;
                float u2 = glyph->U1;
                float v2 = glyph->V1;

                // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                if (cpu_fine_clip)
                {
                    if (x1 < clip_rect.x)
                    {
                        u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                        x1 = clip_rect.x;
                    }
                    if (y1 < clip_rect.y)
                    {
                        v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                        y1 = clip_rect.y;
                    }
                    if (x2 > clip_rect.z)
                    {
                        u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                        x2 = clip_rect.z;
                    }
                    if (y2 > clip_rect.w)
                    {
                        v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                        y2 = clip_rect.w;
                    }
                    if (y1 >= y2)
                    {
                        x += char_width;
                        continue;
                    }
                }

                // Support for untinted glyphs
                ImU32 glyph_col = glyph->Colored ? col_untinted : col;

                // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
                {
                    vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = glyph_col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                    vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = glyph_col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                    vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = glyph_col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                    vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = glyph_col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                    idx_write[0] = (ImDrawIdx)(vtx_index); idx_write[1] = (ImDrawIdx)(vtx_index + 1); idx_write[2] = (ImDrawIdx)(vtx_index + 2);
                    idx_write[3] = (ImDrawIdx)(vtx_index); idx_write[4] = (ImDrawIdx)(vtx_index + 2); idx_write[5] = (ImDrawIdx)(vtx_index + 3);
                    vtx_write += 4;
                    vtx_index += 4;
                    idx_write += 6;
                }
            }
        }
        x += char_width;
    }

    // Give back unused vertices (clipped ones, blanks) ~ this is essentially a PrimUnreserve() action.
    draw_list->VtxBuffer.Size = (int)(vtx_write - draw_list->VtxBuffer.Data); // Same as calling shrink()
    draw_list->IdxBuffer.Size = (int)(idx_write - draw_list->IdxBuffer.Data);
    draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_IdxWritePtr = idx_write;
    draw_list->_VtxCurrentIdx = vtx_index;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGui Internal Render Helpers
//-----------------------------------------------------------------------------
// Vaguely redesigned to stop accessing ImGui global state:
// - RenderArrow()
// - RenderBullet()
// - RenderCheckMark()
// - RenderArrowPointingAt()
// - RenderRectFilledRangeH()
// - RenderRectFilledWithHole()
//-----------------------------------------------------------------------------
// Function in need of a redesign (legacy mess)
// - RenderColorRectWithAlphaCheckerboard()
//-----------------------------------------------------------------------------

// Render an arrow aimed to be aligned with text (p_min is a position in the same space text would be positioned). To e.g. denote expanded/collapsed state
void ImGui::RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
{
    const float h = draw_list->_Data->FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        a = ImVec2(+0.000f, +0.750f) * r;
        b = ImVec2(-0.866f, -0.750f) * r;
        c = ImVec2(+0.866f, -0.750f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        a = ImVec2(+0.750f, +0.000f) * r;
        b = ImVec2(-0.750f, +0.866f) * r;
        c = ImVec2(-0.750f, -0.866f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }
    draw_list->AddTriangleFilled(center + a, center + b, center + c, col);
}

void ImGui::RenderBullet(ImDrawList* draw_list, ImVec2 pos, ImU32 col)
{
    // FIXME-OPT: This should be baked in font.
    draw_list->AddCircleFilled(pos, draw_list->_Data->FontSize * 0.20f, col, 8);
}

void ImGui::RenderCheckMark(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz)
{
    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;
    draw_list->PathLineTo(ImVec2(bx - third, by - third));
    draw_list->PathLineTo(ImVec2(bx, by));
    draw_list->PathLineTo(ImVec2(bx + third * 2.0f, by - third * 2.0f));
    draw_list->PathStroke(col, 0, thickness);
}

// Render an arrow. 'pos' is position of the arrow tip. half_sz.x is length from base to tip. half_sz.y is length on each side.
void ImGui::RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 col)
{
    switch (direction)
    {
    case ImGuiDir_Left:  draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Right: draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_Up:    draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Down:  draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_None: case ImGuiDir_COUNT: break; // Fix warnings
    }
}

static inline float ImAcos01(float x)
{
    if (x <= 0.0f) return IM_PI * 0.5f;
    if (x >= 1.0f) return 0.0f;
    return ImAcos(x);
    //return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f; // Cheap approximation, may be enough for what we do.
}

// FIXME: Cleanup and move code to ImDrawList.
void ImGui::RenderRectFilledRangeH(ImDrawList* draw_list, const ImRect& rect, ImU32 col, float x_start_norm, float x_end_norm, float rounding)
{
    if (x_end_norm == x_start_norm)
        return;
    if (x_start_norm > x_end_norm)
        ImSwap(x_start_norm, x_end_norm);

    ImVec2 p0 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_start_norm), rect.Min.y);
    ImVec2 p1 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_end_norm), rect.Max.y);
    if (rounding == 0.0f)
    {
        draw_list->AddRectFilled(p0, p1, col, 0.0f);
        return;
    }

    rounding = ImClamp(ImMin((rect.Max.x - rect.Min.x) * 0.5f, (rect.Max.y - rect.Min.y) * 0.5f) - 1.0f, 0.0f, rounding);
    const float inv_rounding = 1.0f / rounding;
    const float arc0_b = ImAcos01(1.0f - (p0.x - rect.Min.x) * inv_rounding);
    const float arc0_e = ImAcos01(1.0f - (p1.x - rect.Min.x) * inv_rounding);
    const float half_pi = IM_PI * 0.5f; // We will == compare to this because we know this is the exact value ImAcos01 can return.
    const float x0 = ImMax(p0.x, rect.Min.x + rounding);
    if (arc0_b == arc0_e)
    {
        draw_list->PathLineTo(ImVec2(x0, p1.y));
        draw_list->PathLineTo(ImVec2(x0, p0.y));
    }
    else if (arc0_b == 0.0f && arc0_e == half_pi)
    {
        draw_list->PathArcToFast(ImVec2(x0, p1.y - rounding), rounding, 3, 6); // BL
        draw_list->PathArcToFast(ImVec2(x0, p0.y + rounding), rounding, 6, 9); // TR
    }
    else
    {
        draw_list->PathArcTo(ImVec2(x0, p1.y - rounding), rounding, IM_PI - arc0_e, IM_PI - arc0_b); // BL
        draw_list->PathArcTo(ImVec2(x0, p0.y + rounding), rounding, IM_PI + arc0_b, IM_PI + arc0_e); // TR
    }
    if (p1.x > rect.Min.x + rounding)
    {
        const float arc1_b = ImAcos01(1.0f - (rect.Max.x - p1.x) * inv_rounding);
        const float arc1_e = ImAcos01(1.0f - (rect.Max.x - p0.x) * inv_rounding);
        const float x1 = ImMin(p1.x, rect.Max.x - rounding);
        if (arc1_b == arc1_e)
        {
            draw_list->PathLineTo(ImVec2(x1, p0.y));
            draw_list->PathLineTo(ImVec2(x1, p1.y));
        }
        else if (arc1_b == 0.0f && arc1_e == half_pi)
        {
            draw_list->PathArcToFast(ImVec2(x1, p0.y + rounding), rounding, 9, 12); // TR
            draw_list->PathArcToFast(ImVec2(x1, p1.y - rounding), rounding, 0, 3);  // BR
        }
        else
        {
            draw_list->PathArcTo(ImVec2(x1, p0.y + rounding), rounding, -arc1_e, -arc1_b); // TR
            draw_list->PathArcTo(ImVec2(x1, p1.y - rounding), rounding, +arc1_b, +arc1_e); // BR
        }
    }
    draw_list->PathFillConvex(col);
}

void ImGui::RenderRectFilledWithHole(ImDrawList* draw_list, const ImRect& outer, const ImRect& inner, ImU32 col, float rounding)
{
    const bool fill_L = (inner.Min.x > outer.Min.x);
    const bool fill_R = (inner.Max.x < outer.Max.x);
    const bool fill_U = (inner.Min.y > outer.Min.y);
    const bool fill_D = (inner.Max.y < outer.Max.y);
    if (fill_L) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Min.y), ImVec2(inner.Min.x, inner.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_U ? 0 : ImDrawFlags_RoundCornersTopLeft)    | (fill_D ? 0 : ImDrawFlags_RoundCornersBottomLeft));
    if (fill_R) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Min.y), ImVec2(outer.Max.x, inner.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_U ? 0 : ImDrawFlags_RoundCornersTopRight)   | (fill_D ? 0 : ImDrawFlags_RoundCornersBottomRight));
    if (fill_U) draw_list->AddRectFilled(ImVec2(inner.Min.x, outer.Min.y), ImVec2(inner.Max.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_L ? 0 : ImDrawFlags_RoundCornersTopLeft)    | (fill_R ? 0 : ImDrawFlags_RoundCornersTopRight));
    if (fill_D) draw_list->AddRectFilled(ImVec2(inner.Min.x, inner.Max.y), ImVec2(inner.Max.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_L ? 0 : ImDrawFlags_RoundCornersBottomLeft) | (fill_R ? 0 : ImDrawFlags_RoundCornersBottomRight));
    if (fill_L && fill_U) draw_list->AddRectFilled(ImVec2(outer.Min.x, outer.Min.y), ImVec2(inner.Min.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersTopLeft);
    if (fill_R && fill_U) draw_list->AddRectFilled(ImVec2(inner.Max.x, outer.Min.y), ImVec2(outer.Max.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersTopRight);
    if (fill_L && fill_D) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Max.y), ImVec2(inner.Min.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersBottomLeft);
    if (fill_R && fill_D) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Max.y), ImVec2(outer.Max.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersBottomRight);
}

// Helper for ColorPicker4()
// NB: This is rather brittle and will show artifact when rounding this enabled if rounded corners overlap multiple cells. Caller currently responsible for avoiding that.
// Spent a non reasonable amount of time trying to getting this right for ColorButton with rounding+anti-aliasing+ImGuiColorEditFlags_HalfAlphaPreview flag + various grid sizes and offsets, and eventually gave up... probably more reasonable to disable rounding altogether.
// FIXME: uses ImGui::GetColorU32
void ImGui::RenderColorRectWithAlphaCheckerboard(ImDrawList* draw_list, ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, ImDrawFlags flags)
{
    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags = ImDrawFlags_RoundCornersDefault_;
    if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
    {
        ImU32 col_bg1 = GetColorU32(ImAlphaBlendColors(IM_COL32(204, 204, 204, 255), col));
        ImU32 col_bg2 = GetColorU32(ImAlphaBlendColors(IM_COL32(128, 128, 128, 255), col));
        draw_list->AddRectFilled(p_min, p_max, col_bg1, rounding, flags);

        int yi = 0;
        for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
        {
            float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
            if (y2 <= y1)
                continue;
            for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
            {
                float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
                if (x2 <= x1)
                    continue;
                ImDrawFlags cell_flags = ImDrawFlags_RoundCornersNone;
                if (y1 <= p_min.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersTopLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersTopRight; }
                if (y2 >= p_max.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersBottomLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersBottomRight; }

                // Combine flags
                cell_flags = (flags == ImDrawFlags_RoundCornersNone || cell_flags == ImDrawFlags_RoundCornersNone) ? ImDrawFlags_RoundCornersNone : (cell_flags & flags);
                draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col_bg2, rounding, cell_flags);
            }
        }
    }
    else
    {
        draw_list->AddRectFilled(p_min, p_max, col, rounding, flags);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Decompression code
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array and encoded as base85.
// Use the program in misc/fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(const unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier_out_e, *stb__barrier_out_b;
static const unsigned char *stb__barrier_in_b;
static unsigned char *stb__dout;
static void stb__match(const unsigned char *data, unsigned int length)
{
    // INVERSE of IMGUI_MEMMOVE(... write each byte before copying the next...
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_out_b) { stb__dout = stb__barrier_out_e+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(const unsigned char *data, unsigned int length)
{
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_in_b) { stb__dout = stb__barrier_out_e+1; return; }
    IMGUI_MEMCPY(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static const unsigned char *stb_decompress_token(const unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen = buflen % 5552;

    unsigned long i;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    const unsigned int olen = stb_decompress_length(i);
    stb__barrier_in_b = i;
    stb__barrier_out_e = output + olen;
    stb__barrier_out_b = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        const unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen);
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Default font data (ProggyClean.ttf)
//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.proggyfonts.net/index.php?menu=download)
// Download and more information at http://www.proggyfonts.net or http://upperboundsinteractive.com/fonts.php
//-----------------------------------------------------------------------------
// File: 'ProggyClean.ttf' (41208 bytes)
// Exported using misc/fonts/binary_to_compressed_c.cpp (with compression + base85 string encoding).
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
//-----------------------------------------------------------------------------
static const unsigned int proggy_clean_ttf_compressesed_compressed_size = 9583;
static const unsigned int proggy_clean_ttf_compressesed_compressed_data[9584/4] =
{
    0x0000bc57, 0x00000000, 0xf8a00000, 0x00000400, 0x00010037, 0x000c0000, 0x00030080, 0x2f534f40, 0x74eb8832, 0x01000090, 0x2c158248, 0x616d634e, 
    0x23120270, 0x03000075, 0x241382a0, 0x74766352, 0x82178220, 0xfc042102, 0x02380482, 0x66796c67, 0x5689af12, 0x04070000, 0x80920000, 0x64616568, 
    0xd36691d7, 0xcc201b82, 0x36210382, 0x27108268, 0xc3014208, 0x04010000, 0x243b0f82, 0x78746d68, 0x807e008a, 0x98010000, 0x06020000, 0x61636f6c, 
    0xd8b0738c, 0x82050000, 0x0402291e, 0x7078616d, 0xda00ae01, 0x28201f82, 0x202c1082, 0x656d616e, 0x96bb5925, 0x84990000, 0x9e2c1382, 0x74736f70, 
    0xef83aca6, 0x249b0000, 0xd22c3382, 0x70657270, 0x12010269, 0xf4040000, 0x08202f82, 0x012ecb84, 0x553c0000, 0x0f5fd5e9, 0x0300f53c, 0x00830008, 
    0x7767b722, 0x002b3f82, 0xa692bd00, 0xfe0000d7, 0x83800380, 0x21f1826f, 0x00850002, 0x41820120, 0x40fec026, 0x80030000, 0x05821083, 0x07830120, 
    0x0221038a, 0x24118200, 0x90000101, 0x82798200, 0x00022617, 0x00400008, 0x2009820a, 0x82098276, 0x82002006, 0x9001213b, 0x0223c883, 0x828a02bc, 
    0x858f2010, 0xc5012507, 0x00023200, 0x04210083, 0x91058309, 0x6c412b03, 0x40007374, 0xac200000, 0x00830008, 0x01000523, 0x834d8380, 0x80032103, 
    0x012101bf, 0x23b88280, 0x00800000, 0x0b830382, 0x07820120, 0x83800021, 0x88012001, 0x84002009, 0x2005870f, 0x870d8301, 0x2023901b, 0x83199501, 
    0x82002015, 0x84802000, 0x84238267, 0x88002027, 0x8561882d, 0x21058211, 0x13880000, 0x01800022, 0x05850d85, 0x0f828020, 0x03208384, 0x03200582, 
    0x47901b84, 0x1b850020, 0x1f821d82, 0x3f831d88, 0x3f410383, 0x84058405, 0x210982cd, 0x09830000, 0x03207789, 0xf38a1384, 0x01203782, 0x13872384, 
    0x0b88c983, 0x0d898f84, 0x00202982, 0x23900383, 0x87008021, 0x83df8301, 0x86118d03, 0x863f880d, 0x8f35880f, 0x2160820f, 0x04830300, 0x1c220382, 
    0x05820100, 0x4c000022, 0x09831182, 0x04001c24, 0x11823000, 0x0800082e, 0x00000200, 0xff007f00, 0xffffac20, 0x00220982, 0x09848100, 0xdf216682, 
    0x843586d5, 0x06012116, 0x04400684, 0xa58120d7, 0x00b127d8, 0x01b88d01, 0x2d8685ff, 0xc100c621, 0xf4be0801, 0x9e011c01, 0x88021402, 0x1403fc02, 
    0x9c035803, 0x1404de03, 0x50043204, 0xa2046204, 0x66051605, 0x1206bc05, 0xd6067406, 0x7e073807, 0x4e08ec07, 0x96086c08, 0x1009d008, 0x88094a09, 
    0x800a160a, 0x560b040b, 0x2e0cc80b, 0xea0c820c, 0xa40d5e0d, 0x500eea0d, 0x280f960e, 0x1210b00f, 0xe0107410, 0xb6115211, 0x6e120412, 0x4c13c412, 
    0xf613ac13, 0xae145814, 0x4015ea14, 0xa6158015, 0x1216b815, 0xc6167e16, 0x8e173417, 0x5618e017, 0xee18ba18, 0x96193619, 0x481ad419, 0xf01a9c1a, 
    0xc81b5c1b, 0x4c1c041c, 0xea1c961c, 0x921d2a1d, 0x401ed21d, 0xe01e8e1e, 0x761f241f, 0xa61fa61f, 0x01821020, 0x8a202e34, 0xc820b220, 0x74211421, 
    0xee219821, 0x86226222, 0x01820c23, 0x83238021, 0x23983c01, 0x24d823b0, 0x244a2400, 0x24902468, 0x250625ae, 0x25822560, 0x26f825f8, 0x82aa2658, 
    0xd8be0801, 0x9a274027, 0x68280a28, 0x0e29a828, 0xb8292029, 0x362af829, 0x602a602a, 0x2a2b022b, 0xac2b5e2b, 0x202ce62b, 0x9a2c342c, 0x5c2d282d, 
    0xaa2d782d, 0x262ee82d, 0x262fa62e, 0xf42fb62f, 0xc8305e30, 0xb4313e31, 0x9e321e32, 0x82331e33, 0x5c34ee33, 0x3a35ce34, 0xd4358635, 0x72362636, 
    0x7637e636, 0x3a38d837, 0x1239a638, 0xae397439, 0x9a3a2e3a, 0x7c3b063b, 0x3a3ce83b, 0x223d963c, 0xec3d863d, 0xc63e563e, 0x9a3f2a3f, 0x6a401240, 
    0x3641d040, 0x0842a241, 0x7a424042, 0xf042b842, 0xcc436243, 0x8a442a44, 0x5845ee44, 0xe245b645, 0xb4465446, 0x7a471447, 0x5448da47, 0x4049c648, 
    0x15462400, 0x034d0808, 0x0b000700, 0x13000f00, 0x1b001700, 0x23001f00, 0x2b002700, 0x33002f00, 0x3b003700, 0x43003f00, 0x4b004700, 0x53004f00, 
    0x5b005700, 0x63005f00, 0x6b006700, 0x73006f00, 0x7b007700, 0x83007f00, 0x8b008700, 0x00008f00, 0x15333511, 0x20039631, 0x20178205, 0xd3038221, 
    0x20739707, 0x25008580, 0x028080fc, 0x05be8080, 0x04204a85, 0x05ce0685, 0x0107002a, 0x02000080, 0x00000400, 0x250d8b41, 0x33350100, 0x03920715, 
    0x13820320, 0x858d0120, 0x0e8d0320, 0xff260d83, 0x00808000, 0x54820106, 0x04800223, 0x845b8c80, 0x41332059, 0x078b068f, 0x82000121, 0x82fe2039, 
    0x84802003, 0x83042004, 0x23598a0e, 0x00180000, 0x03210082, 0x42ab9080, 0x73942137, 0x2013bb41, 0x8f978205, 0x2027a39b, 0x20b68801, 0x84b286fd, 
    0x91c88407, 0x41032011, 0x11a51130, 0x15000027, 0x80ff8000, 0x11af4103, 0x841b0341, 0x8bd983fd, 0x9be99bc9, 0x8343831b, 0x21f1821f, 0xb58300ff, 
    0x0f84e889, 0xf78a0484, 0x8000ff22, 0x0020eeb3, 0x14200082, 0x2130ef41, 0xeb431300, 0x4133200a, 0xd7410ecb, 0x9a07200b, 0x2027871b, 0x21238221, 
    0xe7828080, 0xe784fd20, 0xe8848020, 0xfe808022, 0x08880d85, 0xba41fd20, 0x82248205, 0x85eab02a, 0x008022e7, 0x2cd74200, 0x44010021, 0xd34406eb, 
    0x44312013, 0xcf8b0eef, 0x0d422f8b, 0x82332007, 0x0001212f, 0x8023cf82, 0x83000180, 0x820583de, 0x830682d4, 0x820020d4, 0x82dc850a, 0x20e282e9, 
    0xb2ff85fe, 0x010327e9, 0x02000380, 0x0f440400, 0x0c634407, 0x68825982, 0x85048021, 0x260a825d, 0x010b0000, 0x4400ff00, 0x2746103f, 0x08d74209, 
    0x4d440720, 0x0eaf4406, 0xc3441d20, 0x23078406, 0xff800002, 0x04845b83, 0x8d05b241, 0x1781436f, 0x6b8c87a5, 0x1521878e, 0x06474505, 0x01210783, 
    0x84688c00, 0x8904828e, 0x441e8cf7, 0x0b270cff, 0x80008000, 0x45030003, 0xfb430fab, 0x080f4107, 0x410bf942, 0xd34307e5, 0x070d4207, 0x80800123, 
    0x205d85fe, 0x849183fe, 0x20128404, 0x82809702, 0x00002217, 0x41839a09, 0x6b4408cf, 0x0733440f, 0x3b460720, 0x82798707, 0x97802052, 0x0000296f, 
    0xff800004, 0x01800100, 0x0021ef89, 0x0a914625, 0x410a4d41, 0x00250ed4, 0x00050000, 0x056d4280, 0x210a7b46, 0x21481300, 0x46ed8512, 0x00210bd1, 
    0x89718202, 0x21738877, 0x2b850001, 0x00220582, 0x87450a00, 0x0ddb4606, 0x41079b42, 0x9d420c09, 0x0b09420b, 0x8d820720, 0x9742fc84, 0x42098909, 
    0x00241e0f, 0x00800014, 0x0b47da82, 0x0833442a, 0x49078d41, 0x2f450f13, 0x42278f17, 0x01200751, 0x22063742, 0x44808001, 0x20450519, 0x88068906, 
    0x83fe2019, 0x4203202a, 0x1a941a58, 0x00820020, 0xe7a40e20, 0x420ce146, 0x854307e9, 0x0fcb4713, 0xff20a182, 0xfe209b82, 0x0c867f8b, 0x0021aea4, 
    0x219fa40f, 0x7d41003b, 0x07194214, 0xbf440520, 0x071d4206, 0x6941a590, 0x80802309, 0x028900ff, 0xa9a4b685, 0xc5808021, 0x449b82ab, 0x152007eb, 
    0x42134d46, 0x61440a15, 0x051e4208, 0x222b0442, 0x47001100, 0xfd412913, 0x17194714, 0x410f5b41, 0x02220773, 0x09428080, 0x21a98208, 0xd4420001, 
    0x481c840d, 0x00232bc9, 0x42120000, 0xe74c261b, 0x149d4405, 0x07209d87, 0x410db944, 0x14421c81, 0x42fd2005, 0x80410bd2, 0x203d8531, 0x06874100, 
    0x48256f4a, 0xcb420c95, 0x13934113, 0x44075d44, 0x044c0855, 0x00ff2105, 0xfe228185, 0x45448000, 0x22c5b508, 0x410c0000, 0x7b412087, 0x1bb74514, 
    0x32429c85, 0x0a574805, 0x21208943, 0x8ba01300, 0x440dfb4e, 0x77431437, 0x245b4113, 0x200fb145, 0x41108ffe, 0x80203562, 0x00200082, 0x46362b42, 
    0x1742178d, 0x4527830f, 0x0f830b2f, 0x4a138146, 0x802409a1, 0xfe8000ff, 0x94419982, 0x09294320, 0x04000022, 0x49050f4f, 0xcb470a63, 0x48032008, 
    0x2b48067b, 0x85022008, 0x82638338, 0x00002209, 0x05af4806, 0x900e9f49, 0x84c5873f, 0x214285bd, 0x064900ff, 0x0c894607, 0x00000023, 0x4903820a, 
    0x714319f3, 0x0749410c, 0x8a07a145, 0x02152507, 0xfe808000, 0x74490386, 0x8080211b, 0x0c276f82, 0x00018000, 0x48028003, 0x2b2315db, 0x43002f00, 
    0x6f82142f, 0x44011521, 0x93510da7, 0x20e68508, 0x06494d80, 0x8e838020, 0x06821286, 0x124bff20, 0x25f3830c, 0x03800080, 0xe74a0380, 0x207b8715, 
    0x876b861d, 0x4a152007, 0x07870775, 0xf6876086, 0x8417674a, 0x0a0021f2, 0x431c9743, 0x8d421485, 0x200b830b, 0x06474d03, 0x71828020, 0x04510120, 
    0x42da8606, 0x1f831882, 0x001a0022, 0xff4d0082, 0x0b0f532c, 0x0d449b94, 0x4e312007, 0x074f12e7, 0x0bf3490b, 0xbb412120, 0x413f820a, 0xef490857, 
    0x80002313, 0xe2830001, 0x6441fc20, 0x8b802006, 0x00012108, 0xfd201582, 0x492c9b48, 0x802014ff, 0x51084347, 0x0f4327f3, 0x17bf4a14, 0x201b7944, 
    0x06964201, 0x134ffe20, 0x20d6830b, 0x25d78280, 0xfd800002, 0x05888000, 0x9318dc41, 0x21d282d4, 0xdb481800, 0x0dff542a, 0x45107743, 0xe14813f5, 
    0x0f034113, 0x83135d45, 0x47b28437, 0xe4510e73, 0x21f58e06, 0x2b8400fd, 0x1041fcac, 0x08db4b0b, 0x421fdb41, 0xdf4b18df, 0x011d210a, 0x420af350, 
    0x6e8308af, 0xac85cb86, 0x1e461082, 0x82b7a407, 0x411420a3, 0xa34130ab, 0x178f4124, 0x41139741, 0x86410d93, 0x82118511, 0x057243d8, 0x8941d9a4, 
    0x3093480c, 0x4a13474f, 0xfb5016a9, 0x07ad4108, 0x4a0f9d42, 0xfe200fad, 0x4708aa41, 0x83482dba, 0x288f4d06, 0xb398c3bb, 0x44267b41, 0xb34439d7, 
    0x0755410f, 0x200ebb45, 0x0f5f4215, 0x20191343, 0x06df5301, 0xf04c0220, 0x2ba64d07, 0x82050841, 0x430020ce, 0xa78f3627, 0x5213ff42, 0x2f970bc1, 
    0x4305ab55, 0xa084111b, 0x450bac45, 0x5f4238b8, 0x010c2106, 0x0220ed82, 0x441bb344, 0x875010af, 0x0737480f, 0x490c5747, 0x0c840c03, 0x4c204b42, 
    0x8ba905d7, 0x8b948793, 0x510c0c51, 0xfb4b24b9, 0x1b174107, 0x5709d74c, 0xd1410ca5, 0x079d480f, 0x201ff541, 0x06804780, 0x7d520120, 0x80002205, 
    0x20a983fe, 0x47bb83fe, 0x1b8409b4, 0x81580220, 0x4e00202c, 0x4f41282f, 0x0eab4f17, 0x57471520, 0x0e0f4808, 0x8221e041, 0x3e1b4a8b, 0x4407175d, 
    0x1b4b071f, 0x4a0f8b07, 0x174a0703, 0x0ba5411b, 0x430fb141, 0x0120057b, 0xfc20dd82, 0x4a056047, 0xf4850c0c, 0x01221982, 0x02828000, 0x1a5d088b, 
    0x20094108, 0x8c0e3941, 0x4900200e, 0x7744434f, 0x200b870b, 0x0e4b5a33, 0x2b41f78b, 0x8b138307, 0x0b9f450b, 0x2406f741, 0xfd808001, 0x09475a00, 
    0x84000121, 0x5980200e, 0x85450e5d, 0x832c8206, 0x4106831e, 0x00213814, 0x28b34810, 0x410c2f4b, 0x5f4a13d7, 0x0b2b4113, 0x6e43a883, 0x11174b05, 
    0x4b066a45, 0xcc470541, 0x5000202b, 0xcb472f4b, 0x44b59f0f, 0xc5430b5b, 0x0d654907, 0x21065544, 0xd6828080, 0xfe201982, 0x8230ec4a, 0x120025c2, 
    0x80ff8000, 0x4128d74d, 0x3320408b, 0x410a9f50, 0xdb822793, 0x822bd454, 0x61134b2e, 0x410b214a, 0xad4117c9, 0x0001211f, 0x4206854f, 0x4b430596, 
    0x06bb5530, 0x2025cf46, 0x0ddd5747, 0x500ea349, 0x0f840fa7, 0x5213c153, 0x634e08d1, 0x0bbe4809, 0x59316e4d, 0x5b50053f, 0x203f6323, 0x5117eb46, 
    0x94450a63, 0x246e410a, 0x63410020, 0x0bdb5f2f, 0x4233ab44, 0x39480757, 0x112d4a07, 0x7241118f, 0x000e2132, 0x9f286f41, 0x0f8762c3, 0x33350723, 
    0x094e6415, 0x2010925f, 0x067252fe, 0xd0438020, 0x63a68225, 0x11203a4f, 0x480e6360, 0x5748131f, 0x079b521f, 0x200e2f43, 0x864b8315, 0x113348e7, 
    0x85084e48, 0x06855008, 0x5880fd21, 0x7c420925, 0x0c414824, 0x37470c86, 0x1b8b422b, 0x5b0a8755, 0x23410c21, 0x0b83420b, 0x5a082047, 0xf482067f, 
    0xa80b4c47, 0x0c0021cf, 0x20207b42, 0x0fb74100, 0x420b8744, 0xeb43076f, 0x0f6f420b, 0x4261fe20, 0x439aa00c, 0x215034e3, 0x0ff9570f, 0x4b1f2d5d, 
    0x2d5d0c6f, 0x09634d0b, 0x1f51b8a0, 0x620f200c, 0xaf681e87, 0x24f94d07, 0x4e0f4945, 0xfe200c05, 0x22139742, 0x57048080, 0x23950c20, 0x97601585, 
    0x4813201f, 0xad620523, 0x200f8f0f, 0x9e638f15, 0x00002181, 0x41342341, 0x0f930f0b, 0x210b4b62, 0x978f0001, 0xfe200f84, 0x8425c863, 0x2704822b, 
    0x80000a00, 0x00038001, 0x610e9768, 0x834514bb, 0x0bc3430f, 0x2107e357, 0x80848080, 0x4400fe21, 0x2e410983, 0x00002a1a, 0x00000700, 0x800380ff, 
    0x0fdf5800, 0x59150021, 0xd142163d, 0x0c02410c, 0x01020025, 0x65800300, 0x00240853, 0x1d333501, 0x15220382, 0x35420001, 0x44002008, 0x376406d7, 
    0x096f6b19, 0x480bc142, 0x8f4908a7, 0x211f8b1f, 0x9e830001, 0x0584fe20, 0x4180fd21, 0x11850910, 0x8d198259, 0x000021d4, 0x5a08275d, 0x275d1983, 
    0x06d9420e, 0x9f08b36a, 0x0f7d47b5, 0x8d8a2f8b, 0x4c0e0b57, 0xe7410e17, 0x42d18c1a, 0xb351087a, 0x1ac36505, 0x4b4a2f20, 0x0b9f450d, 0x430beb53, 
    0xa7881015, 0xa5826a83, 0x80200f82, 0x86185a65, 0x4100208e, 0x176c3367, 0x0fe7650b, 0x4a17ad4b, 0x0f4217ed, 0x112e4206, 0x41113a42, 0xf7423169, 
    0x0cb34737, 0x560f8b46, 0xa75407e5, 0x5f01200f, 0x31590c48, 0x80802106, 0x42268841, 0x0020091e, 0x4207ef64, 0x69461df7, 0x138d4114, 0x820f5145, 
    0x53802090, 0xff200529, 0xb944b183, 0x417e8505, 0x00202561, 0x15210082, 0x42378200, 0x9b431cc3, 0x004f220d, 0x0dd54253, 0x4213f149, 0x7d41133b, 
    0x42c9870b, 0x802010f9, 0x420b2c42, 0x8f441138, 0x267c4408, 0x600cb743, 0x8f4109d3, 0x05ab701d, 0x83440020, 0x3521223f, 0x0b794733, 0xfb62fe20, 
    0x4afd2010, 0xaf410ae7, 0x25ce8525, 0x01080000, 0x7b6b0000, 0x0973710b, 0x82010021, 0x49038375, 0x33420767, 0x052c4212, 0x58464b85, 0x41fe2005, 
    0x50440c27, 0x000c2209, 0x1cb36b80, 0x9b06df44, 0x0f93566f, 0x52830220, 0xfe216e8d, 0x200f8200, 0x0fb86704, 0xb057238d, 0x050b5305, 0x7217eb47, 
    0xbd410b6b, 0x0f214610, 0x871f9956, 0x1e91567e, 0x2029b741, 0x20008200, 0x18b7410a, 0x27002322, 0x41095543, 0x0f8f0fb3, 0x41000121, 0x889d111c, 
    0x14207b82, 0x00200382, 0x73188761, 0x475013a7, 0x6e33200c, 0x234e0ea3, 0x9b138313, 0x08e54d17, 0x9711094e, 0x2ee74311, 0x4908875e, 0xd75d1f1f, 
    0x19ab5238, 0xa2084d48, 0x63a7a9b3, 0x55450b83, 0x0fd74213, 0x440d814c, 0x4f481673, 0x05714323, 0x13000022, 0x412e1f46, 0xdf493459, 0x21c7550f, 
    0x8408215f, 0x201d49cb, 0xb1103043, 0x0f0d65d7, 0x452b8d41, 0x594b0f8d, 0x0b004605, 0xb215eb46, 0x000a24d7, 0x47000080, 0x002118cf, 0x06436413, 
    0x420bd750, 0x2b500743, 0x076a470c, 0x4105c050, 0xd942053f, 0x0d00211a, 0x5f44779c, 0x0ce94805, 0x51558186, 0x14a54c0b, 0x49082b41, 0x0a4b0888, 
    0x8080261f, 0x0d000000, 0x20048201, 0x1deb6a03, 0x420cb372, 0x07201783, 0x4306854d, 0x8b830c59, 0x59093c74, 0x0020250f, 0x67070f4a, 0x2341160b, 
    0x00372105, 0x431c515d, 0x554e17ef, 0x0e5d6b05, 0x41115442, 0xb74a1ac1, 0x2243420a, 0x5b4f878f, 0x7507200f, 0x384b086f, 0x09d45409, 0x0020869a, 
    0x12200082, 0xab460382, 0x10075329, 0x54138346, 0xaf540fbf, 0x1ea75413, 0x9a0c9e54, 0x0f6b44c1, 0x41000021, 0x47412a4f, 0x07374907, 0x5310bf76, 
    0xff2009b4, 0x9a09a64c, 0x8200208d, 0x34c34500, 0x970fe141, 0x1fd74b0f, 0x440a3850, 0x206411f0, 0x27934609, 0x470c5d41, 0x555c2947, 0x1787540f, 
    0x6e0f234e, 0x7d540a1b, 0x1d736b08, 0x0026a088, 0x80000e00, 0x9b5200ff, 0x08ef4318, 0x450bff77, 0x1d4d0b83, 0x081f7006, 0xcb691b86, 0x4b022008, 
    0xc34b0b33, 0x1d0d4a0c, 0x8025a188, 0x0b000000, 0x52a38201, 0xbf7d0873, 0x0c234511, 0x8f0f894a, 0x4101200f, 0x0c880c9d, 0x2b418ea1, 0x06c74128, 
    0x66181341, 0x7b4c0bb9, 0x0c06630b, 0xfe200c87, 0x9ba10882, 0x27091765, 0x01000008, 0x02800380, 0x48113f4e, 0x29430cf5, 0x09a75a0b, 0x31618020, 
    0x6d802009, 0x61840e33, 0x8208bf51, 0x0c637d61, 0x7f092379, 0x4f470f4b, 0x1797510c, 0x46076157, 0xf5500fdf, 0x0f616910, 0x1171fe20, 0x82802006, 
    0x08696908, 0x41127a4c, 0x3f4a15f3, 0x01042607, 0x0200ff00, 0x1cf77700, 0xff204185, 0x00235b8d, 0x43100000, 0x3b22243f, 0x3b4d3f00, 0x0b937709, 
    0xad42f18f, 0x0b1f420f, 0x51084b43, 0x8020104a, 0xb557ff83, 0x052b7f2a, 0x0280ff22, 0x250beb78, 0x00170013, 0xbf6d2500, 0x07db760e, 0x410e2b7f, 
    0x00230e4f, 0x49030000, 0x0582055b, 0x07000326, 0x00000b00, 0x580bcd46, 0x00200cdd, 0x57078749, 0x8749160f, 0x0f994f0a, 0x41134761, 0x01200b31, 
    0xeb796883, 0x0b41500b, 0x0e90b38e, 0x202e7b51, 0x05d95801, 0x41080570, 0x1d530fc9, 0x0b937a0f, 0xaf8eb387, 0xf743b98f, 0x07c74227, 0x80000523, 
    0x0fcb4503, 0x430ca37b, 0x7782077f, 0x8d0a9947, 0x08af4666, 0xeb798020, 0x6459881e, 0xc3740bbf, 0x0feb6f0b, 0x20072748, 0x052b6102, 0x435e0584, 
    0x7d088308, 0x03200afd, 0x92109e41, 0x28aa8210, 0x80001500, 0x80030000, 0x0fdb5805, 0x209f4018, 0xa7418d87, 0x0aa3440f, 0x20314961, 0x073a52ff, 
    0x6108505d, 0x43181051, 0x00223457, 0xe7820500, 0x50028021, 0x81410d33, 0x063d7108, 0xdb41af84, 0x4d888205, 0x00201198, 0x463d835f, 0x152106d7, 
    0x0a355a33, 0x6917614e, 0x75411f4d, 0x184b8b07, 0x1809c344, 0x21091640, 0x0b828000, 0x42808021, 0x26790519, 0x86058605, 0x2428422d, 0x22123b42, 
    0x42000080, 0xf587513b, 0x7813677b, 0xaf4d139f, 0x00ff210c, 0x5e0a1d57, 0x3b421546, 0x01032736, 0x02000380, 0x41180480, 0x2f420f07, 0x0c624807, 
    0x00000025, 0x18000103, 0x83153741, 0x430120c3, 0x042106b2, 0x088d4d00, 0x2f830620, 0x1810434a, 0x18140345, 0x8507fb41, 0x5ee582ea, 0x0023116c, 
    0x8d000600, 0x053b56af, 0xa6554fa2, 0x0d704608, 0x40180d20, 0x47181a43, 0xd37b07ff, 0x0b79500c, 0x420fd745, 0x47450bd9, 0x8471830a, 0x095a777e, 
    0x84137542, 0x82002013, 0x2f401800, 0x0007213b, 0x4405e349, 0x0d550ff3, 0x16254c0c, 0x820ffe4a, 0x0400218a, 0x89066f41, 0x106b414f, 0xc84d0120, 
    0x80802206, 0x0c9a4b03, 0x00100025, 0x68000200, 0x9d8c2473, 0x44134344, 0xf36a0f33, 0x4678860f, 0x1b440a25, 0x41988c0a, 0x80201879, 0x43079b5e, 
    0x4a18080b, 0x0341190b, 0x1259530c, 0x43251552, 0x908205c8, 0x0cac4018, 0x86000421, 0x0e504aa2, 0x0020b891, 0xfb450082, 0x51132014, 0x8f5205f3, 
    0x35052108, 0x8505cb59, 0x0f6d4f70, 0x82150021, 0x29af5047, 0x4f004b24, 0x75795300, 0x1b595709, 0x460b6742, 0xbf4b0f0d, 0x5743870b, 0xcb6d1461, 
    0x08f64505, 0x4e05ab6c, 0x334126c3, 0x0bcb6b0d, 0x1811034d, 0x4111ef4b, 0x814f1ce5, 0x20af8227, 0x07fd7b80, 0x41188e84, 0xef410f33, 0x80802429, 
    0x410d0000, 0xa34205ab, 0x76b7881c, 0xff500b89, 0x0741430f, 0x20086f4a, 0x209d8200, 0x234c18fd, 0x05d4670a, 0x4509af51, 0x9642078d, 0x189e831d, 
    0x7c1cc74b, 0xcd4c07b9, 0x0e7c440f, 0x8b7b0320, 0x21108210, 0xc76c8080, 0x03002106, 0x6b23bf41, 0xc549060b, 0x7946180b, 0x0ff7530f, 0x17ad4618, 
    0x200ecd45, 0x208c83fd, 0x5e0488fe, 0x032009c6, 0x420d044e, 0x0d8f0d7f, 0x00820020, 0x18001021, 0x6d273b45, 0xfd4c0c93, 0xcf451813, 0x0fe5450f, 
    0x5a47c382, 0x820a8b0a, 0x282b4998, 0x410a8b5b, 0x4b232583, 0x54004f00, 0x978f0ce3, 0x500f1944, 0xa95f1709, 0x0280220b, 0x05ba7080, 0xa1530682, 
    0x06324c13, 0x91412582, 0x05536e2c, 0x63431020, 0x0f434706, 0x8c11374c, 0x176143d7, 0x4d0f454c, 0xd3680bed, 0x0bee4d17, 0x212b9a41, 0x0f530a00, 
    0x140d531c, 0x43139143, 0x95610e8d, 0x0f094415, 0x4205fb56, 0x1b4205cf, 0x17015225, 0x5e0c477f, 0xaf6e0aeb, 0x0ff36218, 0x04849a84, 0x0a454218, 
    0x9c430420, 0x23c6822b, 0x04000102, 0x45091b4b, 0xf05f0955, 0x82802007, 0x421c2023, 0x5218282b, 0x7b53173f, 0x0fe7480c, 0x74173b7f, 0x47751317, 
    0x634d1807, 0x0f6f430f, 0x24086547, 0xfc808002, 0x0b3c7f80, 0x10840120, 0x188d1282, 0x20096b43, 0x0fc24403, 0x00260faf, 0x0180000b, 0x3f500280, 
    0x18002019, 0x450b4941, 0xf3530fb9, 0x18002010, 0x8208a551, 0x06234d56, 0xcb58a39b, 0xc3421805, 0x1313461e, 0x0f855018, 0xd34b0120, 0x6cfe2008, 
    0x574f0885, 0x09204114, 0x07000029, 0x00008000, 0x44028002, 0x01420f57, 0x10c95c10, 0x11184c18, 0x80221185, 0x7f421e00, 0x00732240, 0x09cd4977, 
    0x6d0b2b42, 0x4f180f8f, 0x8f5a0bcb, 0x9b0f830f, 0x0fb9411f, 0x230b5756, 0x00fd8080, 0x82060745, 0x000121d5, 0x8e0fb277, 0x4a8d4211, 0x24061c53, 
    0x04000007, 0x12275280, 0x430c954c, 0x80201545, 0x200f764f, 0x20008200, 0x20ce8308, 0x09534f02, 0x660edf64, 0x73731771, 0xe7411807, 0x20a2820c, 
    0x13b64404, 0x8f5d6682, 0x1d6b4508, 0x0cff4d18, 0x3348c58f, 0x0fc34c07, 0x31558b84, 0x8398820f, 0x17514712, 0x240b0e46, 0x80000a00, 0x093b4502, 
    0x420f9759, 0xa54c0bf1, 0x0f2b470c, 0x410d314b, 0x2584170c, 0x73b30020, 0xb55fe782, 0x204d8410, 0x08e043fe, 0x4f147e41, 0x022008ab, 0x4b055159, 
    0x2950068f, 0x00022208, 0x48511880, 0x82002009, 0x00112300, 0x634dff00, 0x24415f27, 0x180f6d43, 0x4d0b5d45, 0x4d5f05ef, 0x01802317, 0x56188000, 
    0xa7840807, 0xc6450220, 0x21ca8229, 0x4b781a00, 0x3359182c, 0x0cf3470f, 0x180bef46, 0x420b0354, 0xff470b07, 0x4515200a, 0x9758239b, 0x4a80200c, 
    0xd2410a26, 0x05fb4a08, 0x4b05e241, 0x03200dc9, 0x92290941, 0x00002829, 0x00010900, 0x5b020001, 0x23201363, 0x460d776a, 0xef530fdb, 0x209a890c, 
    0x13fc4302, 0x00008024, 0xc4820104, 0x08820220, 0x20086b5b, 0x18518700, 0x8408d349, 0x0da449a1, 0x00080024, 0x7b690280, 0x4c438b1a, 0x01220f63, 
    0x4c878000, 0x5c149c53, 0xfb430868, 0x2f56181e, 0x0ccf7b1b, 0x0f075618, 0x2008e347, 0x14144104, 0x00207f83, 0x00207b82, 0x201adf47, 0x16c35a13, 
    0x540fdf47, 0x802006c8, 0x5418f185, 0x29430995, 0x00002419, 0x58001600, 0x5720316f, 0x4d051542, 0x4b7b1b03, 0x138f4707, 0xb747b787, 0x4aab8213, 
    0x058305fc, 0x20115759, 0x82128401, 0x0a0b44e8, 0x46800121, 0xe64210d0, 0x82129312, 0x4bffdffe, 0x3b41171b, 0x9b27870f, 0x808022ff, 0x085c68fe, 
    0x41800021, 0x01410b20, 0x001a213a, 0x47480082, 0x11374e12, 0x56130b4c, 0xdf4b0c65, 0x0b0f590b, 0x0f574c18, 0x830feb4b, 0x075f480f, 0x480b4755, 
    0x40490b73, 0x80012206, 0x09d74280, 0x80fe8022, 0x80210e86, 0x056643ff, 0x10820020, 0x420b2646, 0x0b58391a, 0xd74c1808, 0x078b4e22, 0x2007f55f, 
    0x4b491807, 0x83802017, 0x65aa82a7, 0x3152099e, 0x068b7616, 0x9b431220, 0x09bb742c, 0x500e376c, 0x8342179b, 0x0a4d5d0f, 0x8020a883, 0x180cd349, 
    0x2016bb4b, 0x14476004, 0x84136c43, 0x08cf7813, 0x4f4c0520, 0x156f420f, 0x20085f42, 0x6fd3be03, 0xd4d30803, 0xa7411420, 0x004b222c, 0x0d3b614f, 
    0x3f702120, 0x1393410a, 0x8f132745, 0x47421827, 0x41e08209, 0xb05e2bb9, 0x18b7410c, 0x18082647, 0x4107a748, 0xeb8826bf, 0x0ca76018, 0x733ecb41, 
    0xd0410d83, 0x43ebaf2a, 0x0420067f, 0x721dab4c, 0x472005bb, 0x4105d341, 0x334844cb, 0x20dba408, 0x47d6ac00, 0x034e3aef, 0x0f8f421b, 0x930f134d, 
    0x3521231f, 0xb7421533, 0x42f5ad0a, 0x1e961eaa, 0x17000022, 0x4c367b50, 0x7d491001, 0x0bf5520f, 0x4c18fda7, 0xb8460c55, 0x83fe2005, 0x00fe25b9, 
    0x80000180, 0x9e751085, 0x261b5c12, 0x82110341, 0x001123fb, 0x4518fe80, 0xf38c2753, 0x6d134979, 0x295107a7, 0xaf5f180f, 0x0fe3660c, 0x180b6079, 
    0x2007bd5f, 0x9aab9103, 0x2f4d1811, 0x05002109, 0x44254746, 0x1d200787, 0x450bab75, 0x4f180f57, 0x4f181361, 0x3b831795, 0xeb4b0120, 0x0b734805, 
    0x84078f48, 0x2e1b47bc, 0x00203383, 0xaf065f45, 0x831520d7, 0x130f51a7, 0x1797bf97, 0x2b47d783, 0x18fe2005, 0x4a18a44f, 0xa64d086d, 0x1ab0410d, 
    0x6205a258, 0xdbab069f, 0x4f06f778, 0xa963081d, 0x133b670a, 0x8323d141, 0x13195b23, 0x530f5e70, 0xe5ad0824, 0x58001421, 0x1f472b4b, 0x47bf410c, 
    0x82000121, 0x83fe20cb, 0x07424404, 0x68068243, 0xd7ad0d3d, 0x00010d26, 0x80020000, 0x4a1c6f43, 0x23681081, 0x10a14f13, 0x8a070e57, 0x430a848f, 
    0x7372243e, 0x4397a205, 0xb56c1021, 0x43978f0f, 0x64180505, 0x99aa0ff2, 0x0e000022, 0x20223341, 0x094b4f37, 0x074a3320, 0x2639410a, 0xfe208e84, 
    0x8b0e0048, 0x508020a3, 0x9e4308fe, 0x073f4115, 0xe3480420, 0x0c9b5f1b, 0x7c137743, 0x9a95185b, 0x6122b148, 0x979b08df, 0x0fe36c18, 0x48109358, 
    0x23441375, 0x0ffd5c0b, 0x180fc746, 0x2011d157, 0x07e95702, 0x58180120, 0x18770ac3, 0x51032008, 0x7d4118e3, 0x80802315, 0x3b4c1900, 0xbb5a1830, 
    0x0ceb6109, 0x5b0b3d42, 0x4f181369, 0x4f180b8d, 0x4f180f75, 0x355a1b81, 0x200d820d, 0x18e483fd, 0x4528854f, 0x89420846, 0x1321411f, 0x44086b60, 
    0x07421d77, 0x107d4405, 0x4113fd41, 0x5a181bf1, 0x4f180db3, 0x8021128f, 0x20f68280, 0x44a882fe, 0x334d249a, 0x052f6109, 0x1520c3a7, 0xef4eb783, 
    0x4ec39b1b, 0xc4c90ee7, 0x20060b4d, 0x256f4905, 0x4d0cf761, 0xcf9b1f13, 0xa213d74e, 0x0e1145d4, 0x50135b42, 0xcb4e398f, 0x20d79f27, 0x08865d80, 
    0x186d5018, 0xa90f7142, 0x067342d7, 0x3f450420, 0x65002021, 0xe3560771, 0x24d38f23, 0x15333531, 0x0eb94d01, 0x451c9f41, 0x384322fb, 0x00092108, 
    0x19af6b18, 0x6e0c6f5a, 0xbd770bfb, 0x22bb7718, 0x20090f57, 0x25e74204, 0x4207275a, 0xdb5408ef, 0x1769450f, 0x1b1b5518, 0x210b1f57, 0x5e4c8001, 
    0x55012006, 0x802107f1, 0x0a306a80, 0x45808021, 0x0d850b88, 0x31744f18, 0x1808ec54, 0x2009575b, 0x45ffa505, 0x1b420c73, 0x180f9f0f, 0x4a0cf748, 
    0x501805b2, 0x00210f40, 0x4d118f80, 0xd6823359, 0x072b5118, 0x314ad7aa, 0x8fc79f08, 0x45d78b1f, 0xfe20058f, 0x23325118, 0x7b54d9b5, 0x9fc38f46, 
    0x10bb410f, 0x41077b42, 0xc1410faf, 0x27cf441d, 0x46051b4f, 0x04200683, 0x2121d344, 0x8f530043, 0x8fcf9f0e, 0x21df8c1f, 0x50188000, 0x5d180e52, 
    0xfd201710, 0x4405c341, 0xd68528e3, 0x20071f6b, 0x1b734305, 0x6b080957, 0x7d422b1f, 0x67002006, 0x7f8317b1, 0x2024cb48, 0x08676e00, 0x8749a39b, 
    0x18132006, 0x410a6370, 0x8f490b47, 0x7e1f8f13, 0x551805c3, 0x4c180915, 0xfe200e2f, 0x244d5d18, 0x270bcf44, 0xff000019, 0x04800380, 0x5f253342, 
    0xff520df7, 0x13274c18, 0x5542dd93, 0x0776181b, 0xf94a1808, 0x084a4c0c, 0x4308ea5b, 0xde831150, 0x7900fd21, 0x00492c1e, 0x060f4510, 0x17410020, 
    0x0ce74526, 0x6206b341, 0x1f561083, 0x9d6c181b, 0x08a0500e, 0x112e4118, 0x60000421, 0xbf901202, 0x4408e241, 0xc7ab0513, 0xb40f0950, 0x055943c7, 
    0x4f18ff20, 0xc9ae1cad, 0x32b34f18, 0x7a180120, 0x3d520a05, 0x53d1b40a, 0x80200813, 0x1b815018, 0x832bf86f, 0x67731847, 0x297f4308, 0x6418d54e, 
    0x734213f7, 0x056b4b27, 0xdba5fe20, 0x1828aa4e, 0x2031a370, 0x06cb6101, 0x2040ad41, 0x07365300, 0x2558d985, 0x83fe200c, 0x0380211c, 0x542c4743, 
    0x052006b7, 0x6021df45, 0x897b0707, 0x18d3c010, 0x20090e70, 0x1d5843ff, 0x540a0e44, 0x002126c5, 0x322f7416, 0x636a5720, 0x0f317409, 0x610fe159, 
    0x294617e7, 0x08555213, 0x2006a75d, 0x6cec84fd, 0xfb5907be, 0x3a317405, 0x83808021, 0x180f20ea, 0x4626434a, 0x531818e3, 0xdb59172d, 0x0cbb460c, 
    0x2013d859, 0x18b94502, 0x8f46188d, 0x77521842, 0x0a184e38, 0x9585fd20, 0x6a180684, 0xc64507e9, 0x51cbb230, 0xd3440cf3, 0x17ff6a0f, 0x450f5b42, 
    0x276407c1, 0x4853180a, 0x21ccb010, 0xcf580013, 0x0c15442d, 0x410a1144, 0x1144359d, 0x5cfe2006, 0xa1410a43, 0x2bb64519, 0x2f5b7618, 0xb512b745, 
    0x0cfd6fd1, 0x42089f59, 0xb8450c70, 0x0000232d, 0x50180900, 0xb9491ae3, 0x0fc37610, 0x01210f83, 0x0f3b4100, 0xa01b2742, 0x0ccd426f, 0x6e8f6f94, 
    0x9c808021, 0xc7511870, 0x17c74b08, 0x9b147542, 0x44fe2079, 0xd5480c7e, 0x95ef861d, 0x101b597b, 0xf5417594, 0x9f471808, 0x86868d0e, 0x3733491c, 
    0x690f4d6d, 0x43440b83, 0x1ba94c0b, 0x660cd16b, 0x802008ae, 0x74126448, 0xcb4f38a3, 0x2cb74b0b, 0x47137755, 0xe3971777, 0x1b5d0120, 0x057a4108, 
    0x6e08664d, 0x17421478, 0x11af4208, 0x850c3f42, 0x08234f0c, 0x4321eb4a, 0xf3451095, 0x0f394e0f, 0x4310eb45, 0xc09707b1, 0x54431782, 0xaec08d1d, 
    0x0f434dbb, 0x9f0c0b45, 0x0a3b4dbb, 0x4618bdc7, 0x536032eb, 0x17354213, 0x4d134169, 0xc7a30c2f, 0x4e254342, 0x174332cf, 0x43cdae17, 0x6b4706e4, 
    0x0e16430d, 0x530b5542, 0x2f7c26bb, 0x13075f31, 0x43175342, 0x60181317, 0x6550114e, 0x28624710, 0x58070021, 0x59181683, 0x2d540cf5, 0x05d5660c, 
    0x20090c7b, 0x0e157e02, 0x8000ff2b, 0x14000080, 0x80ff8000, 0x27137e03, 0x336a4b20, 0x0f817107, 0x13876e18, 0x730f2f7e, 0x2f450b75, 0x6d02200b, 
    0x6d66094c, 0x4b802009, 0x15820a02, 0x2f45fe20, 0x5e032006, 0x00202fd9, 0x450af741, 0xeb412e0f, 0x0ff3411f, 0x420a8b65, 0xf7410eae, 0x1c664810, 
    0x540e1145, 0xbfa509f3, 0x42302f58, 0x80200c35, 0xcb066c47, 0x4b1120c1, 0x41492abb, 0x34854110, 0xa7097b72, 0x251545c7, 0x4b2c7f56, 0xc5b40bab, 
    0x940cd54e, 0x2e6151c8, 0x09f35f18, 0x4b420420, 0x09677121, 0x8f24f357, 0x1b5418e1, 0x08915a1f, 0x3143d894, 0x22541805, 0x1b9b4b0e, 0x8c0d3443, 
    0x1400240d, 0x18ff8000, 0x582e6387, 0xf99b2b3b, 0x8807a550, 0x17a14790, 0x2184fd20, 0x5758fe20, 0x2354882c, 0x15000080, 0x5e056751, 0x334c2c2f, 
    0x97c58f0c, 0x1fd7410f, 0x0d4d4018, 0x4114dc41, 0x04470ed6, 0x0dd54128, 0x00820020, 0x02011523, 0x22008700, 0x86480024, 0x0001240a, 0x8682001a, 
    0x0002240b, 0x866c000e, 0x8a03200b, 0x8a042017, 0x0005220b, 0x22218614, 0x84060000, 0x86012017, 0x8212200f, 0x250b8519, 0x000d0001, 0x0b850031, 
    0x07000224, 0x0b862600, 0x11000324, 0x0b862d00, 0x238a0420, 0x0a000524, 0x17863e00, 0x17840620, 0x01000324, 0x57820904, 0x0b85a783, 0x0b85a785, 
    0x0b85a785, 0x22000325, 0x85007a00, 0x85a7850b, 0x85a7850b, 0x22a7850b, 0x82300032, 0x00342201, 0x0805862f, 0x35003131, 0x54207962, 0x74736972, 
    0x47206e61, 0x6d6d6972, 0x65527265, 0x616c7567, 0x58545472, 0x6f725020, 0x43796767, 0x6e61656c, 0x30325454, 0x822f3430, 0x35313502, 0x79006200, 
    0x54002000, 0x69007200, 0x74007300, 0x6e006100, 0x47200f82, 0x6d240f84, 0x65006d00, 0x52200982, 0x67240582, 0x6c007500, 0x72201d82, 0x54222b82, 
    0x23825800, 0x19825020, 0x67006f22, 0x79220182, 0x1b824300, 0x3b846520, 0x1f825420, 0x41000021, 0x1422099b, 0x0b410000, 0x87088206, 0x01012102, 
    0x78080982, 0x01020101, 0x01040103, 0x01060105, 0x01080107, 0x010a0109, 0x010c010b, 0x010e010d, 0x0110010f, 0x01120111, 0x01140113, 0x01160115, 
    0x01180117, 0x011a0119, 0x011c011b, 0x011e011d, 0x0020011f, 0x00040003, 0x00060005, 0x00080007, 0x000a0009, 0x000c000b, 0x000e000d, 0x0010000f, 
    0x00120011, 0x00140013, 0x00160015, 0x00180017, 0x001a0019, 0x001c001b, 0x001e001d, 0x08bb821f, 0x22002142, 0x24002300, 0x26002500, 0x28002700, 
    0x2a002900, 0x2c002b00, 0x2e002d00, 0x30002f00, 0x32003100, 0x34003300, 0x36003500, 0x38003700, 0x3a003900, 0x3c003b00, 0x3e003d00, 0x40003f00, 
    0x42004100, 0x4b09f382, 0x00450044, 0x00470046, 0x00490048, 0x004b004a, 0x004d004c, 0x004f004e, 0x00510050, 0x00530052, 0x00550054, 0x00570056, 
    0x00590058, 0x005b005a, 0x005d005c, 0x005f005e, 0x01610060, 0x01220121, 0x01240123, 0x01260125, 0x01280127, 0x012a0129, 0x012c012b, 0x012e012d, 
    0x0130012f, 0x01320131, 0x01340133, 0x01360135, 0x01380137, 0x013a0139, 0x013c013b, 0x013e013d, 0x0140013f, 0x00ac0041, 0x008400a3, 0x00bd0085, 
    0x00e80096, 0x008e0086, 0x009d008b, 0x00a400a9, 0x008a00ef, 0x008300da, 0x00f20093, 0x008d00f3, 0x00880097, 0x00de00c3, 0x009e00f1, 0x00f500aa, 
    0x00f600f4, 0x00ad00a2, 0x00c700c9, 0x006200ae, 0x00900063, 0x00cb0064, 0x00c80065, 0x00cf00ca, 0x00cd00cc, 0x00e900ce, 0x00d30066, 0x00d100d0, 
    0x006700af, 0x009100f0, 0x00d400d6, 0x006800d5, 0x00ed00eb, 0x006a0089, 0x006b0069, 0x006c006d, 0x00a0006e, 0x0071006f, 0x00720070, 0x00750073, 
    0x00760074, 0x00ea0077, 0x007a0078, 0x007b0079, 0x007c007d, 0x00a100b8, 0x007e007f, 0x00810080, 0x00ee00ec, 0x6e750eba, 0x646f6369, 0x78302365, 
    0x31303030, 0x32200e8d, 0x33200e8d, 0x34200e8d, 0x35200e8d, 0x36200e8d, 0x37200e8d, 0x38200e8d, 0x39200e8d, 0x61200e8d, 0x62200e8d, 0x63200e8d, 
    0x64200e8d, 0x65200e8d, 0x66200e8d, 0x31210e8c, 0x8d0e8d30, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 
    0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x66312def, 0x6c656406, 0x04657465, 0x6f727545, 0x3820ec8c, 0x3820ec8d, 
    0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 
    0x3820ec8d, 0x200ddc41, 0x0ddc4139, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 
    0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0x00663923, 0x48fa0500, 0x00f762f9, 
};

static const unsigned int* GetDefaultCompressedFontDataTTF()
{
    return proggy_clean_ttf_compressesed_compressed_data;
}

static const unsigned int GetDefaultCompressedFontDataTTFSize()
{
    return proggy_clean_ttf_compressesed_compressed_size;
}

#endif // #ifndef IMGUI_DISABLE
