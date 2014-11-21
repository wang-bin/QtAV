/******************************************************************************
    mkapi dynamic load code generation for capi template
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>
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
//#define DEBUG_RESOLVE
#define DEBUG_LOAD

#include "ass_api.h"
#include "capi.h"

namespace ass {
static const char* names[] = {
    "ass",
    NULL
};

#define CAPI_ASS_VERSION
#ifdef CAPI_ASS_VERSION
static const int versions[] = {
    capi::NoVersion,
    5,
    4,
    capi::EndVersion
};
CAPI_BEGIN_DLL_VER(names, versions)
#else
CAPI_BEGIN_DLL(names)
#endif
#ifndef CAPI_LINK_ASS
// CAPI_DEFINE_RESOLVER(argc, return_type, name, argv_no_name)
// mkapi code generation BEGIN
CAPI_DEFINE_RESOLVER(0, int, ass_library_version)
CAPI_DEFINE_RESOLVER(0, ASS_Library *, ass_library_init)
CAPI_DEFINE_RESOLVER(1, void, ass_library_done, ASS_Library *)
CAPI_DEFINE_RESOLVER(2, void, ass_set_fonts_dir, ASS_Library *, const char *)
CAPI_DEFINE_RESOLVER(2, void, ass_set_extract_fonts, ASS_Library *, int)
CAPI_DEFINE_RESOLVER(2, void, ass_set_style_overrides, ASS_Library *, char **)
CAPI_DEFINE_RESOLVER(1, void, ass_process_force_style, ASS_Track *)
CAPI_DEFINE_RESOLVER(3, void, ass_set_message_cb, ASS_Library *, void (*msg_cb)
                        (int level, const char *fmt, va_list args, void *data), void *)
CAPI_DEFINE_RESOLVER(1, ASS_Renderer *, ass_renderer_init, ASS_Library *)
CAPI_DEFINE_RESOLVER(1, void, ass_renderer_done, ASS_Renderer *)
CAPI_DEFINE_RESOLVER(3, void, ass_set_frame_size, ASS_Renderer *, int, int)
CAPI_DEFINE_RESOLVER(3, void, ass_set_storage_size, ASS_Renderer *, int, int)
CAPI_DEFINE_RESOLVER(2, void, ass_set_shaper, ASS_Renderer *, ASS_ShapingLevel)
CAPI_DEFINE_RESOLVER(5, void, ass_set_margins, ASS_Renderer *, int, int, int, int)
CAPI_DEFINE_RESOLVER(2, void, ass_set_use_margins, ASS_Renderer *, int)
CAPI_DEFINE_RESOLVER(2, void, ass_set_pixel_aspect, ASS_Renderer *, double)
CAPI_DEFINE_RESOLVER(3, void, ass_set_aspect_ratio, ASS_Renderer *, double, double)
CAPI_DEFINE_RESOLVER(2, void, ass_set_font_scale, ASS_Renderer *, double)
CAPI_DEFINE_RESOLVER(2, void, ass_set_hinting, ASS_Renderer *, ASS_Hinting)
CAPI_DEFINE_RESOLVER(2, void, ass_set_line_spacing, ASS_Renderer *, double)
CAPI_DEFINE_RESOLVER(2, void, ass_set_line_position, ASS_Renderer *, double)
CAPI_DEFINE_RESOLVER(6, void, ass_set_fonts, ASS_Renderer *, const char *, const char *, int, const char *, int)
CAPI_DEFINE_RESOLVER(2, void, ass_set_selective_style_override_enabled, ASS_Renderer *, int)
CAPI_DEFINE_RESOLVER(2, void, ass_set_selective_style_override, ASS_Renderer *, ASS_Style *)
CAPI_DEFINE_RESOLVER(1, int, ass_fonts_update, ASS_Renderer *)
CAPI_DEFINE_RESOLVER(3, void, ass_set_cache_limits, ASS_Renderer *, int, int)
CAPI_DEFINE_RESOLVER(4, ASS_Image *, ass_render_frame, ASS_Renderer *, ASS_Track *, long long, int *)
CAPI_DEFINE_RESOLVER(1, ASS_Track *, ass_new_track, ASS_Library *)
CAPI_DEFINE_RESOLVER(1, void, ass_free_track, ASS_Track *)
CAPI_DEFINE_RESOLVER(1, int, ass_alloc_style, ASS_Track *)
CAPI_DEFINE_RESOLVER(1, int, ass_alloc_event, ASS_Track *)
CAPI_DEFINE_RESOLVER(2, void, ass_free_style, ASS_Track *, int)
CAPI_DEFINE_RESOLVER(2, void, ass_free_event, ASS_Track *, int)
CAPI_DEFINE_RESOLVER(3, void, ass_process_data, ASS_Track *, char *, int)
CAPI_DEFINE_RESOLVER(3, void, ass_process_codec_private, ASS_Track *, char *, int)
CAPI_DEFINE_RESOLVER(5, void, ass_process_chunk, ASS_Track *, char *, int, long long, long long)
CAPI_DEFINE_RESOLVER(1, void, ass_flush_events, ASS_Track *)
CAPI_DEFINE_RESOLVER(3, ASS_Track *, ass_read_file, ASS_Library *, char *, char *)
CAPI_DEFINE_RESOLVER(4, ASS_Track *, ass_read_memory, ASS_Library *, char *, size_t, char *)
CAPI_DEFINE_RESOLVER(3, int, ass_read_styles, ASS_Track *, char *, char *)
CAPI_DEFINE_RESOLVER(4, void, ass_add_font, ASS_Library *, char *, char *, int)
CAPI_DEFINE_RESOLVER(1, void, ass_clear_fonts, ASS_Library *)
CAPI_DEFINE_RESOLVER(3, long long, ass_step_sub, ASS_Track *, long long, int)
// mkapi code generation END
#endif //CAPI_LINK_ASS
CAPI_END_DLL()

api::api() : dll(new api_dll()) {
    qDebug("capi::version: %s build %s", capi::version::name, capi::version::build());
}
api::~api() { delete dll;}
bool api::loaded() const {
#ifdef CAPI_LINK_ASS
    return true;
#else
    return dll->isLoaded();
#endif //CAPI_LINK_ASS
}

#ifndef CAPI_LINK_ASS
// CAPI_DEFINE(argc, return_type, name, argv_no_name)
typedef void (*msg_cb)
                        (int level, const char *fmt, va_list args, void *data);
// mkapi code generation BEGIN
CAPI_DEFINE(0, int, ass_library_version)
CAPI_DEFINE(0, ASS_Library *, ass_library_init)
CAPI_DEFINE(1, void, ass_library_done, ASS_Library *)
CAPI_DEFINE(2, void, ass_set_fonts_dir, ASS_Library *, const char *)
CAPI_DEFINE(2, void, ass_set_extract_fonts, ASS_Library *, int)
CAPI_DEFINE(2, void, ass_set_style_overrides, ASS_Library *, char **)
CAPI_DEFINE(1, void, ass_process_force_style, ASS_Track *)
CAPI_DEFINE(3, void, ass_set_message_cb, ASS_Library *, msg_cb, void *)
CAPI_DEFINE(1, ASS_Renderer *, ass_renderer_init, ASS_Library *)
CAPI_DEFINE(1, void, ass_renderer_done, ASS_Renderer *)
CAPI_DEFINE(3, void, ass_set_frame_size, ASS_Renderer *, int, int)
CAPI_DEFINE(3, void, ass_set_storage_size, ASS_Renderer *, int, int)
CAPI_DEFINE(2, void, ass_set_shaper, ASS_Renderer *, ASS_ShapingLevel)
CAPI_DEFINE(5, void, ass_set_margins, ASS_Renderer *, int, int, int, int)
CAPI_DEFINE(2, void, ass_set_use_margins, ASS_Renderer *, int)
CAPI_DEFINE(2, void, ass_set_pixel_aspect, ASS_Renderer *, double)
CAPI_DEFINE(3, void, ass_set_aspect_ratio, ASS_Renderer *, double, double)
CAPI_DEFINE(2, void, ass_set_font_scale, ASS_Renderer *, double)
CAPI_DEFINE(2, void, ass_set_hinting, ASS_Renderer *, ASS_Hinting)
CAPI_DEFINE(2, void, ass_set_line_spacing, ASS_Renderer *, double)
CAPI_DEFINE(2, void, ass_set_line_position, ASS_Renderer *, double)
CAPI_DEFINE(6, void, ass_set_fonts, ASS_Renderer *, const char *, const char *, int, const char *, int)
CAPI_DEFINE(2, void, ass_set_selective_style_override_enabled, ASS_Renderer *, int)
CAPI_DEFINE(2, void, ass_set_selective_style_override, ASS_Renderer *, ASS_Style *)
CAPI_DEFINE(1, int, ass_fonts_update, ASS_Renderer *)
CAPI_DEFINE(3, void, ass_set_cache_limits, ASS_Renderer *, int, int)
CAPI_DEFINE(4, ASS_Image *, ass_render_frame, ASS_Renderer *, ASS_Track *, long long, int *)
CAPI_DEFINE(1, ASS_Track *, ass_new_track, ASS_Library *)
CAPI_DEFINE(1, void, ass_free_track, ASS_Track *)
CAPI_DEFINE(1, int, ass_alloc_style, ASS_Track *)
CAPI_DEFINE(1, int, ass_alloc_event, ASS_Track *)
CAPI_DEFINE(2, void, ass_free_style, ASS_Track *, int)
CAPI_DEFINE(2, void, ass_free_event, ASS_Track *, int)
CAPI_DEFINE(3, void, ass_process_data, ASS_Track *, char *, int)
CAPI_DEFINE(3, void, ass_process_codec_private, ASS_Track *, char *, int)
CAPI_DEFINE(5, void, ass_process_chunk, ASS_Track *, char *, int, long long, long long)
CAPI_DEFINE(1, void, ass_flush_events, ASS_Track *)
CAPI_DEFINE(3, ASS_Track *, ass_read_file, ASS_Library *, char *, char *)
CAPI_DEFINE(4, ASS_Track *, ass_read_memory, ASS_Library *, char *, size_t, char *)
CAPI_DEFINE(3, int, ass_read_styles, ASS_Track *, char *, char *)
CAPI_DEFINE(4, void, ass_add_font, ASS_Library *, char *, char *, int)
CAPI_DEFINE(1, void, ass_clear_fonts, ASS_Library *)
CAPI_DEFINE(3, long long, ass_step_sub, ASS_Track *, long long, int)
// mkapi code generation END
#endif //CAPI_LINK_ASS
} //namespace ass
