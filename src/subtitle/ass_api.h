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

#ifndef ASS_API_H
#define ASS_API_H

// no need to include the C header if only functions declared there
extern "C" {
#include "ass/ass.h"
}

namespace ass {
class api_dll;
class api
{
public:
    api();
    virtual ~api();
    virtual bool loaded() const; // user may inherits multiple api classes: final::loaded() { return base1::loaded() && base2::loaded();}
#ifndef CAPI_LINK_ASS
// mkapi code generation BEGIN
    int ass_library_version();
    ASS_Library * ass_library_init();
    void ass_library_done(ASS_Library * priv);
    void ass_set_fonts_dir(ASS_Library * priv, const char * fonts_dir);
    void ass_set_extract_fonts(ASS_Library * priv, int extract);
    void ass_set_style_overrides(ASS_Library * priv, char ** list);
    void ass_process_force_style(ASS_Track * track);
    void ass_set_message_cb(ASS_Library * priv, void (*msg_cb)
                        (int level, const char *fmt, va_list args, void *data), void * data);
    ASS_Renderer * ass_renderer_init(ASS_Library *);
    void ass_renderer_done(ASS_Renderer * priv);
    void ass_set_frame_size(ASS_Renderer * priv, int w, int h);
    void ass_set_storage_size(ASS_Renderer * priv, int w, int h);
    void ass_set_shaper(ASS_Renderer * priv, ASS_ShapingLevel level);
    void ass_set_margins(ASS_Renderer * priv, int t, int b, int l, int r);
    void ass_set_use_margins(ASS_Renderer * priv, int use);
    void ass_set_pixel_aspect(ASS_Renderer * priv, double par);
    void ass_set_aspect_ratio(ASS_Renderer * priv, double dar, double sar);
    void ass_set_font_scale(ASS_Renderer * priv, double font_scale);
    void ass_set_hinting(ASS_Renderer * priv, ASS_Hinting ht);
    void ass_set_line_spacing(ASS_Renderer * priv, double line_spacing);
    void ass_set_line_position(ASS_Renderer * priv, double line_position);
    void ass_set_fonts(ASS_Renderer * priv, const char * default_font, const char * default_family, int fc, const char * config, int update);
    void ass_set_selective_style_override_enabled(ASS_Renderer * priv, int bits);
    void ass_set_selective_style_override(ASS_Renderer * priv, ASS_Style * style);
    int ass_fonts_update(ASS_Renderer * priv);
    void ass_set_cache_limits(ASS_Renderer * priv, int glyph_max, int bitmap_max_size);
    ASS_Image * ass_render_frame(ASS_Renderer * priv, ASS_Track * track, long long now, int * detect_change);
    ASS_Track * ass_new_track(ASS_Library *);
    void ass_free_track(ASS_Track * track);
    int ass_alloc_style(ASS_Track * track);
    int ass_alloc_event(ASS_Track * track);
    void ass_free_style(ASS_Track * track, int sid);
    void ass_free_event(ASS_Track * track, int eid);
    void ass_process_data(ASS_Track * track, char * data, int size);
    void ass_process_codec_private(ASS_Track * track, char * data, int size);
    void ass_process_chunk(ASS_Track * track, char * data, int size, long long timecode, long long duration);
    void ass_flush_events(ASS_Track * track);
    ASS_Track * ass_read_file(ASS_Library * library, char * fname, char * codepage);
    ASS_Track * ass_read_memory(ASS_Library * library, char * buf, size_t bufsize, char * codepage);
    int ass_read_styles(ASS_Track * track, char * fname, char * codepage);
    void ass_add_font(ASS_Library * library, char * name, char * data, int data_size);
    void ass_clear_fonts(ASS_Library * library);
    long long ass_step_sub(ASS_Track * track, long long now, int movement);
// mkapi code generation END
#endif //CAPI_LINK_ASS
private:
    api_dll *dll;
};
} //namespace ass


#endif // ASS_API_H
