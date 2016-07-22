/******************************************************************************
    mkapi dynamic load code generation for capi template
    Copyright (C) 2014-2016 Wang Bin <wbsecg1@gmail.com>
    https://github.com/wang-bin/mkapi
    https://github.com/wang-bin/capi

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/
//%DEFS%

#define ASS_CAPI_BUILD
//#define CAPI_IS_LAZY_RESOLVE 0
#ifndef CAPI_LINK_ASS
#include "capi.h"
#endif //CAPI_LINK_ASS
#include "ass_api.h" //include last to avoid covering types later

namespace ass {
#ifdef CAPI_LINK_ASS
api::api(){dll=0;}
api::~api(){}
bool api::loaded() const{return true;}
#else
static const char* names[] = {
    "ass", //%LIBS%
#ifdef CAPI_TARGET_OS_WIN
    "libass",
#endif
    NULL
};

typedef ::capi::dso user_dso; //%DSO%

#if 1
static const int versions[] = {
    ::capi::NoVersion,
// the following line will be replaced by the content of config/ass/version if exists
5, 4,
    ::capi::EndVersion
};
CAPI_BEGIN_DLL_VER(names, versions, user_dso)
# else
CAPI_BEGIN_DLL(names, user_dso)
# endif //1
// CAPI_DEFINE_RESOLVER(argc, return_type, name, argv_no_name)
// mkapi code generation BEGIN
//CAPI_DEFINE_ENTRY(int, ass_library_version, CAPI_ARG0())
CAPI_DEFINE_ENTRY(ASS_Library *, ass_library_init, CAPI_ARG0())
CAPI_DEFINE_ENTRY(void, ass_library_done, CAPI_ARG1(ASS_Library *))
CAPI_DEFINE_ENTRY(void, ass_set_fonts_dir, CAPI_ARG2(ASS_Library *, const char *))
CAPI_DEFINE_ENTRY(void, ass_set_extract_fonts, CAPI_ARG2(ASS_Library *, int))
CAPI_DEFINE_ENTRY(void, ass_set_style_overrides, CAPI_ARG2(ASS_Library *, char **))
//CAPI_DEFINE_ENTRY(void, ass_process_force_style, CAPI_ARG1(ASS_Track *))
CAPI_DEFINE_ENTRY(void, ass_set_message_cb, CAPI_ARG3(ASS_Library *, void (*msg_cb)
                        (int level, const char *fmt, va_list args, void *data), void *))
CAPI_DEFINE_ENTRY(ASS_Renderer *, ass_renderer_init, CAPI_ARG1(ASS_Library *))
CAPI_DEFINE_ENTRY(void, ass_renderer_done, CAPI_ARG1(ASS_Renderer *))
CAPI_DEFINE_ENTRY(void, ass_set_frame_size, CAPI_ARG3(ASS_Renderer *, int, int))
//CAPI_DEFINE_ENTRY(void, ass_set_storage_size, CAPI_ARG3(ASS_Renderer *, int, int))
CAPI_DEFINE_ENTRY(void, ass_set_shaper, CAPI_ARG2(ASS_Renderer *, ASS_ShapingLevel))
CAPI_DEFINE_ENTRY(void, ass_set_margins, CAPI_ARG5(ASS_Renderer *, int, int, int, int))
CAPI_DEFINE_ENTRY(void, ass_set_use_margins, CAPI_ARG2(ASS_Renderer *, int))
//CAPI_DEFINE_ENTRY(void, ass_set_pixel_aspect, CAPI_ARG2(ASS_Renderer *, double))
//CAPI_DEFINE_ENTRY(void, ass_set_aspect_ratio, CAPI_ARG3(ASS_Renderer *, double, double))
CAPI_DEFINE_ENTRY(void, ass_set_font_scale, CAPI_ARG2(ASS_Renderer *, double))
//CAPI_DEFINE_ENTRY(void, ass_set_hinting, CAPI_ARG2(ASS_Renderer *, ASS_Hinting))
//CAPI_DEFINE_ENTRY(void, ass_set_line_spacing, CAPI_ARG2(ASS_Renderer *, double))
//CAPI_DEFINE_ENTRY(void, ass_set_line_position, CAPI_ARG2(ASS_Renderer *, double))
CAPI_DEFINE_ENTRY(void, ass_set_fonts, CAPI_ARG6(ASS_Renderer *, const char *, const char *, int, const char *, int))
//CAPI_DEFINE_ENTRY(void, ass_set_selective_style_override_enabled, CAPI_ARG2(ASS_Renderer *, int))
//CAPI_DEFINE_ENTRY(void, ass_set_selective_style_override, CAPI_ARG2(ASS_Renderer *, ASS_Style *))
CAPI_DEFINE_ENTRY(int, ass_fonts_update, CAPI_ARG1(ASS_Renderer *))
//CAPI_DEFINE_ENTRY(void, ass_set_cache_limits, CAPI_ARG3(ASS_Renderer *, int, int))
CAPI_DEFINE_ENTRY(ASS_Image *, ass_render_frame, CAPI_ARG4(ASS_Renderer *, ASS_Track *, long long, int *))
CAPI_DEFINE_ENTRY(ASS_Track *, ass_new_track, CAPI_ARG1(ASS_Library *))
CAPI_DEFINE_ENTRY(void, ass_free_track, CAPI_ARG1(ASS_Track *))
//CAPI_DEFINE_ENTRY(int, ass_alloc_style, CAPI_ARG1(ASS_Track *))
//CAPI_DEFINE_ENTRY(int, ass_alloc_event, CAPI_ARG1(ASS_Track *))
//CAPI_DEFINE_ENTRY(void, ass_free_style, CAPI_ARG2(ASS_Track *, int))
//CAPI_DEFINE_ENTRY(void, ass_free_event, CAPI_ARG2(ASS_Track *, int))
CAPI_DEFINE_ENTRY(void, ass_process_data, CAPI_ARG3(ASS_Track *, char *, int))
CAPI_DEFINE_ENTRY(void, ass_process_codec_private, CAPI_ARG3(ASS_Track *, char *, int))
CAPI_DEFINE_ENTRY(void, ass_process_chunk, CAPI_ARG5(ASS_Track *, char *, int, long long, long long))
CAPI_DEFINE_ENTRY(void, ass_set_check_readorder, CAPI_ARG2(ASS_Track *, int))
CAPI_DEFINE_ENTRY(void, ass_flush_events, CAPI_ARG1(ASS_Track *))
CAPI_DEFINE_ENTRY(ASS_Track *, ass_read_file, CAPI_ARG3(ASS_Library *, char *, char *))
CAPI_DEFINE_ENTRY(ASS_Track *, ass_read_memory, CAPI_ARG4(ASS_Library *, char *, size_t, char *))
//CAPI_DEFINE_ENTRY(int, ass_read_styles, CAPI_ARG3(ASS_Track *, char *, char *))
//CAPI_DEFINE_ENTRY(void, ass_add_font, CAPI_ARG4(ASS_Library *, char *, char *, int))
//CAPI_DEFINE_ENTRY(void, ass_clear_fonts, CAPI_ARG1(ASS_Library *))
//CAPI_DEFINE_ENTRY(long long, ass_step_sub, CAPI_ARG3(ASS_Track *, long long, int))
// mkapi code generation END
CAPI_END_DLL()
CAPI_DEFINE_DLL
// CAPI_DEFINE(argc, return_type, name, argv_no_name)
typedef void (*ass_set_message_cb_msg_cb1)
                        (int level, const char *fmt, va_list args, void *data);
// mkapi code generation BEGIN
//CAPI_DEFINE(int, ass_library_version, CAPI_ARG0())
CAPI_DEFINE(ASS_Library *, ass_library_init, CAPI_ARG0())
CAPI_DEFINE(void, ass_library_done, CAPI_ARG1(ASS_Library *))
CAPI_DEFINE(void, ass_set_fonts_dir, CAPI_ARG2(ASS_Library *, const char *))
CAPI_DEFINE(void, ass_set_extract_fonts, CAPI_ARG2(ASS_Library *, int))
CAPI_DEFINE(void, ass_set_style_overrides, CAPI_ARG2(ASS_Library *, char **))
//CAPI_DEFINE(void, ass_process_force_style, CAPI_ARG1(ASS_Track *))
CAPI_DEFINE(void, ass_set_message_cb, CAPI_ARG3(ASS_Library *, ass_set_message_cb_msg_cb1, void *))
CAPI_DEFINE(ASS_Renderer *, ass_renderer_init, CAPI_ARG1(ASS_Library *))
CAPI_DEFINE(void, ass_renderer_done, CAPI_ARG1(ASS_Renderer *))
CAPI_DEFINE(void, ass_set_frame_size, CAPI_ARG3(ASS_Renderer *, int, int))
//CAPI_DEFINE(void, ass_set_storage_size, CAPI_ARG3(ASS_Renderer *, int, int))
CAPI_DEFINE(void, ass_set_shaper, CAPI_ARG2(ASS_Renderer *, ASS_ShapingLevel))
CAPI_DEFINE(void, ass_set_margins, CAPI_ARG5(ASS_Renderer *, int, int, int, int))
CAPI_DEFINE(void, ass_set_use_margins, CAPI_ARG2(ASS_Renderer *, int))
//CAPI_DEFINE(void, ass_set_pixel_aspect, CAPI_ARG2(ASS_Renderer *, double))
//CAPI_DEFINE(void, ass_set_aspect_ratio, CAPI_ARG3(ASS_Renderer *, double, double))
CAPI_DEFINE(void, ass_set_font_scale, CAPI_ARG2(ASS_Renderer *, double))
//CAPI_DEFINE(void, ass_set_hinting, CAPI_ARG2(ASS_Renderer *, ASS_Hinting))
//CAPI_DEFINE(void, ass_set_line_spacing, CAPI_ARG2(ASS_Renderer *, double))
//CAPI_DEFINE(void, ass_set_line_position, CAPI_ARG2(ASS_Renderer *, double))
CAPI_DEFINE(void, ass_set_fonts, CAPI_ARG6(ASS_Renderer *, const char *, const char *, int, const char *, int))
//CAPI_DEFINE(void, ass_set_selective_style_override_enabled, CAPI_ARG2(ASS_Renderer *, int))
//CAPI_DEFINE(void, ass_set_selective_style_override, CAPI_ARG2(ASS_Renderer *, ASS_Style *))
CAPI_DEFINE(int, ass_fonts_update, CAPI_ARG1(ASS_Renderer *))
//CAPI_DEFINE(void, ass_set_cache_limits, CAPI_ARG3(ASS_Renderer *, int, int))
CAPI_DEFINE(ASS_Image *, ass_render_frame, CAPI_ARG4(ASS_Renderer *, ASS_Track *, long long, int *))
CAPI_DEFINE(ASS_Track *, ass_new_track, CAPI_ARG1(ASS_Library *))
CAPI_DEFINE(void, ass_free_track, CAPI_ARG1(ASS_Track *))
//CAPI_DEFINE(int, ass_alloc_style, CAPI_ARG1(ASS_Track *))
//CAPI_DEFINE(int, ass_alloc_event, CAPI_ARG1(ASS_Track *))
//CAPI_DEFINE(void, ass_free_style, CAPI_ARG2(ASS_Track *, int))
//CAPI_DEFINE(void, ass_free_event, CAPI_ARG2(ASS_Track *, int))
CAPI_DEFINE(void, ass_process_data, CAPI_ARG3(ASS_Track *, char *, int))
CAPI_DEFINE(void, ass_process_codec_private, CAPI_ARG3(ASS_Track *, char *, int))
CAPI_DEFINE(void, ass_process_chunk, CAPI_ARG5(ASS_Track *, char *, int, long long, long long))
CAPI_DEFINE(void, ass_set_check_readorder, CAPI_ARG2(ASS_Track *, int))
CAPI_DEFINE(void, ass_flush_events, CAPI_ARG1(ASS_Track *))
CAPI_DEFINE(ASS_Track *, ass_read_file, CAPI_ARG3(ASS_Library *, char *, char *))
CAPI_DEFINE(ASS_Track *, ass_read_memory, CAPI_ARG4(ASS_Library *, char *, size_t, char *))
//CAPI_DEFINE(int, ass_read_styles, CAPI_ARG3(ASS_Track *, char *, char *))
//CAPI_DEFINE(void, ass_add_font, CAPI_ARG4(ASS_Library *, char *, char *, int))
//CAPI_DEFINE(void, ass_clear_fonts, CAPI_ARG1(ASS_Library *))
//CAPI_DEFINE(long long, ass_step_sub, CAPI_ARG3(ASS_Track *, long long, int))
// mkapi code generation END
#endif //CAPI_LINK_ASS
} //namespace ass
//this file is generated by "mkapi.sh -name ass /usr/local/include/ass/ass.h"
