#### MISC
- LOGO!
- qtav.org website
- Enable all features using my another project(libswresample and cuda tested): https://github.com/wang-bin/dllapi
- SDK document(doxygen seems greate)
- 3 level API like OpenMAX
  * AL: use AVPlayer, VideoRenderer is enought
  * IL: use codec, demuxer, renderer etc
  * DL: private headers required. e.g. implement decoder
- ring buffer instead of queue
- tests and benchmark
- meta data
- component model, plugin
- filter factory and manager
- sws filter and other apis. cpu flags. see vlc/modules/video_chroma/swscale.c
- blu-ray

#### Platform Support
- Maemo: audio not work now
- Android: no audio now. Need OpenSL.
- Raspberry Pi
- iOS: improve OpenAL
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
- Rendering YUV with GLSL for QML. 
- Redesign VideoFrame class to support buffers in both device and host. Thus no copy required. e.g. DXVA, VAAPI has direct rendering api.
ref: qtmmwidgets
- OpenVG or GL text renderering, dwrite text renderering
- D3D and DDraw: not that important.

#### Filters
- Integrate libavfilter
- Write some hardware accelerated filters using OpenCL/GLSL/CUDA. For example, stero 3d, yuv<->rgb, hue/contrast/brightness/gamma/sharp
- OpenCL, GLSL shader(use FBO) based filter chain. User can add custom cl/shaders
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
- Better CUDA support. No CPU copy, all done in gpu from decoding to filtering to renderering.
- DXVA HD
- SSE4 optimized copy. Ref: VLC
- OMX
- Cedarv enhancement if I have a device to test
- ATI solutions. ATI UVD, XvBA
- XVMC?

#### Player
- QMLPlayer. Morden UI
