#### MISC
- LOGO!
- qtav.org website
- Enable all features using my another project(libswresample tested): https://github.com/wang-bin/dllapi
- SDK document(doxygen seems greate)
- ring buffer instead of queue
- tests and benchmark
- meta data
- component model, plugin
- filter factory and manager
- sws filter and other apis. cpu flags. see vlc/modules/video_chroma/swscale.c

#### Platform Support
- Maemo: can build now. No QtQuick support
- Android: Can build now. Need OpenSL.
- Raspberry Pi
- iOS
- Black Berry
- WinRT. Win8 app
- debian PPA

#### Subtitle
- In which thread? 
- VideoThread: store in Statistics.subtitle_only.
- Effect

#### Audio
- OpenAL enhancement.
- OpenSL support.
- AudioFrame 
- ALSA, PulseAudio


####Rendering
- Use OpenGL shaders if possible. Currently is only available for ES2. But most desktop OpenGL support shaders. 
- Redesign VideoFrame class to support buffers in both device and host. Thus no copy required. e.g. DXVA, VAAPI has direct rendering api.
ref: qtmmwidgets
- OpenVG or GL text renderering, dwrite text renderering
- D3D and DDraw: not that important.

#### Filters
- Integrate libavfilter
- Write some hardware accelerated filters using OpenCL/GLSL/CUDA. For example, stero 3d, yuv<->rgb, hue/contrast/brightness/gamma/sharp
- Audio filters
- IPP
- DShow filters support(mplayer dsnative?)

#### Error control
- Math


#### Additional component
- option parser
- slave mode use socket and then use option parser (https://github.com/dantas/QtAV/commit/a0d93f93a6c8b3b0faea379edb42187cb87a476c)
- log module and log viewer
- config module and a gui one

#### Hardware decoding
- Cuda support. Continue the work on `cuda` branch
- DXVA HD
- SSE4 optimized copy. Ref: VLC
- OMX
- Cedarv enhancement if I have a device to test
- ATI solutions. ATI UVD, XvBA
- XVMC?

#### Player
- QMLPlayer. Morden UI
