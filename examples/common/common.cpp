#include "common.h"

void _link_hack()
{

}

QOptions get_common_options()
{
    static QOptions ops = QOptions().addDescription("Options for QtAV players")
            .add("common options")
            ("help,h", "print this")
            ("x", 0, "")
            ("y", 0, "y")
            ("--width", 800, "width of player")
            ("height", 450, "height of player")
            ("fullscreen", "fullscreen")
            ("deocder", "FFmpeg", "use a given decoder")
            ("decoders,-vd", "cuda;vaapi;vda;dxva;cedarv;ffmpeg", "decoder name list in priority order seperated by ';'")
            ("file,f", "", "file or url to play")
            ;
    return ops;
}
