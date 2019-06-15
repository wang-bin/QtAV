set(MODULE QtAV)
# TODO: translations, rpath, uchardet, x11extras, android

include(CheckCCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
INCLUDE(CheckTypeSize)

# set qt dirs first so we can find depends in qt install dir
include_directories(${QTDIR}/include) #TODO: remove. use external/include
get_filename_component(QTDIR "${QTDIR}" ABSOLUTE)

find_package(PkgConfig)

list(APPEND EXTRA_INCLUDE ${QTAV_SOURCE_DIR}/external/include)
list(APPEND EXTRA_LIBS ${CMAKE_LIBRARY_PATH_FLAG}${QTAV_SOURCE_DIR}/external/lib)
if(APPLE)
    if(IOS)
      #set_xcode_property(myioslib IPHONEOS_DEPLOYMENT_TARGET "8.0")
    else()
      list(APPEND EXTRA_INCLUDE /usr/local/include) #macOS only
      list(APPEND EXTRA_LIBS -L/usr/local/lib)
    endif()
endif()
if(EXISTS ${QTAV_SOURCE_DIR}/contrib/capi/capi.h)
    set(HAVE_CAPI 1)
    list(APPEND EXTRA_INCLUDE ${QTAV_SOURCE_DIR}/contrib/capi) # TODO: only files use capi.h
    list(APPEND EXTRA_DEFS -DQTAV_HAVE_CAPI=1)
endif()

pkg_check_modules (AVFORMAT libavformat REQUIRED)
if(${AVFORMAT_FOUND})
    list(APPEND PKG_CONFIG_MODULES AVFORMAT)
    message("Found AVFORMAT: ${AVFORMAT_LINK_LIBRARIES}")
endif()

pkg_check_modules (AVCODEC libavcodec REQUIRED)
if(${AVCODEC_FOUND})
    list(APPEND PKG_CONFIG_MODULES AVCODEC)
    message("Found AVCODEC: ${AVCODEC_LINK_LIBRARIES}")
endif()

pkg_check_modules (SWSCALE libswscale REQUIRED)
if(${SWSCALE_FOUND})
    list(APPEND PKG_CONFIG_MODULES SWSCALE)
    message("Found SWSCALE: ${SWSCALE_LINK_LIBRARIES}")
endif()

pkg_check_modules (AVUTIL libavutil REQUIRED)
if(${AVUTIL_FOUND})
    list(APPEND PKG_CONFIG_MODULES AVUTIL)
    message("Found AVUTIL: ${AVUTIL_LINK_LIBRARIES}")
endif()

pkg_check_modules (AVFILTER libavfilter)
if(${AVFILTER_FOUND})
    list(APPEND PKG_CONFIG_MODULES AVFILTER)
    message("Found AVFILTER: ${AVFILTER_LINK_LIBRARIES}")
endif()

pkg_check_modules (AVDEVICE libavdevice)
if(${AVDEVICE_FOUND})
    list(APPEND PKG_CONFIG_MODULES AVDEVICE)
    message("Found AVDEVICE: ${AVDEVICE_LINK_LIBRARIES}")
endif()

pkg_check_modules (AVRESAMPLE libavresample)
if(${AVRESAMPLE_FOUND})
    list(APPEND PKG_CONFIG_MODULES AVRESAMPLE)
    message("Found AVRESAMPLE: ${AVRESAMPLE_LINK_LIBRARIES}")
endif()

pkg_check_modules (SWRESAMPLE libswresample)
if(${SWRESAMPLE_FOUND})
    list(APPEND PKG_CONFIG_MODULES SWRESAMPLE)
    message("Found SWRESAMPLE: ${SWRESAMPLE_LINK_LIBRARIES}")
endif()

if (NOT AVRESAMPLE_FOUND AND NOT SWRESAMPLE_FOUND)
    message(FATAL_ERROR "swresample or avresample is required")
endif()

list(APPEND EXTRA_DEFS -DBUILD_QTAV_LIB -D__STDC_CONSTANT_MACROS)

check_include_file(ass/ass.h HAVE_ASS_H)
if(HAVE_ASS_H)
  if(HAVE_CAPI) # dynamic load
    if(IOS OR ANDROID OR WindowsPhone OR WindowsStore)
    else()
      set(HAVE_ASS 1)
    endif()
  endif()
  if(NOT HAVE_ASS) # link to ass
    check_library_exists(ass ass_library_init "" HAVE_ASS)
    if(HAVE_ASS)
      set(DYNAMIC_ASS 0)
      list(APPEND EXTRA_DEFS -DCAPI_LINK_ASS)
      list(APPEND EXTRA_LIBS ass)
    endif()
  endif()
endif()
if(HAVE_ASS)
  list(APPEND EXTRA_DEFS -DQTAV_HAVE_LIBASS=1)
  list(APPEND SOURCES
    capi/ass_api.cpp
    subtitle/SubtitleProcessorLibASS.cpp
  )
endif()

set(DYNAMIC_OPENAL ${HAVE_CAPI})
if(APPLE)
  set(HAVE_OPENAL 1)
  if(NOT DYNAMIC_OPENAL)
    list(APPEND EXTRA_LIBS "-framework OpenAL")
  endif()
  list(APPEND EXTRA_DEFS -DHEADER_OPENAL_PREFIX)
else()
  set(OLD_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_INCLUDES ${EXTRA_INCLUDE})
  check_include_files(AL/al.h HAVE_AL_H)
  set(CMAKE_REQUIRED_INCLUDES ${OLD_REQUIRED_INCLUDES})
  if(HAVE_AL_H)
    if(HAVE_CAPI) #TODO: option(LINK_OPENAL...)
      set(HAVE_OPENAL 1)
      # DYNAMIC_OPENAL is set by user
    else()
      if(WIN32)
        check_library_exists(OpenAL alGetError "" HAVE_OPENAL)
      else()
        check_library_exists(openal alGetError "" HAVE_OPENAL)
        if(NOT HAVE_OPENAL)
            check_library_exists(OpenAL alGetError "" HAVE_OPENAL) # blackberry
            if(HAVE_OPENAL)
              list(APPEND EXTRA_LIBS OpenAL)
            endif()
        else()
            list(APPEND EXTRA_LIBS openal)
        endif()
      endif()
      if(HAVE_OPENAL)
        set(DYNAMIC_OPENAL 0)
      endif()
    endif()
  endif()
endif()
if(HAVE_OPENAL AND NOT DYNAMIC_OPENAL)
  list(APPEND EXTRA_DEFS -DCAPI_LINK_OPENAL)
endif()
if(HAVE_OPENAL)
  list(APPEND SOURCES
    capi/openal_api.cpp
    output/audio/AudioOutputOpenAL.cpp
    )
endif()

if (APPLE)
  set(CMAKE_CXX_SYSTEM_FRAMEWORK_SEARCH_FLAG "-F ") # original is "-iframework", lots of build errors
  set(CMAKE_C_SYSTEM_FRAMEWORK_SEARCH_FLAG "-F ")
  set(OLD_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} avcodec)
  check_c_source_compiles(
    "#include <libavcodec/videotoolbox.h>
    int main() {
        av_videotoolbox_alloc_context();
        return 0;
    }" HAVE_VIDEOTOOLBOX)
  set(CMAKE_REQUIRED_LIBRARIES ${OLD_REQUIRED_LIBRARIES})

  if (HAVE_VIDEOTOOLBOX)
    list(APPEND EXTRA_DEFS -DQTAV_HAVE_VIDEOTOOLBOX=1)
    list(APPEND EXTRA_LIBS "-framework CoreMedia" "-framework VideoToolbox" "-framework CoreVideo")
    list(APPEND SOURCES 
      codec/video/VideoDecoderVideoToolbox.cpp
      codec/video/SurfaceInteropCV.cpp
      )
    if (IOS)
      list(APPEND SOURCES 
        codec/video/SurfaceInteropCVOpenGLES.mm
        )
    else()
      list(APPEND SOURCES 
        codec/video/SurfaceInteropIOSurface.cpp
        )
    endif()
  endif()

  list(APPEND SOURCES output/audio/AudioOutputAudioToolbox.cpp)
  list(APPEND EXTRA_LIBS "-framework CoreFoundation" "-framework AudioToolbox" "-framework OpenGL")
endif()

set(DEP_H  OPENAL VIDEOTOOLBOX)
foreach (D ${DEP_H})
  if (${D})
    ADD_DEFINITIONS(-DQTAV_HAVE_${D}=1)
  endif()
endforeach()

if (AVRESAMPLE_FOUND)
  list(APPEND SOURCES AudioResamplerLibav.cpp)
endif()
if (SWRESAMPLE_FOUND)
  list(APPEND SOURCES AudioResamplerFF.cpp)
endif()

set(HAVE_EGL 0)
set(DYNAMIC_GL 0)
set(OPENGL 0)     #desktop opengl only
set(OPENGLES 0)   #glesv2 only
set(HAVE_OPENGL 1) # qt with opengl enabled. 1 of DYNAMIC_GL, OPENGL and OPENGLES is true
#get_target_property(QtGui_location Qt5::Gui LOCATION) #dll path

if(Qt5Gui_OPENGL_IMPLEMENTATION STREQUAL GL) # windows always set to GL
  if(EXISTS ${QT_INSTALL_HEADERS}/QtANGLE)
    set(DYNAMIC_GL 1) # assume Qt5Gui_OPENGL_IMPLEMENTATION is not GL if build with ANGLE only
    if(NOT DEFINED Qt5Gui_EGL_INCLUDE_DIRS)
      set(Qt5Gui_EGL_INCLUDE_DIRS ${QT_INSTALL_HEADERS}/QtANGLE)
    endif()
  else()
    set(OPENGL 1)
  endif()
  if(DEFINED Qt5Gui_EGL_LIBRARIES) # x11?
    set(HAVE_EGL 1)
  endif()
else()
  set(OPENGLES 1)
endif()
if(OPENGLES OR DYNAMIC_GL)
  set(HAVE_EGL 1)
endif()
if(NOT OPENGL AND NOT OPENGLES AND NOT DYNAMIC_GL)
  set(HAVE_OPENGL 0)
endif()
if(HAVE_OPENGL)
  if(WIN32 AND NOT DYNAMIC_GL AND NOT OPENGLES) # msys2
    find_package(OpenGL REQUIRED)
    list(APPEND EXTRA_LIBS ${OPENGL_gl_LIBRARY})
  endif()
endif()

file(GLOB SDK_HEADERS QtAV/*.h)
list(APPEND SDK_HEADERS QtAV/QtAV)
file(GLOB SDK_PRIVATE_HEADERS QtAV/private/*.h)

list(APPEND SOURCES
    AVCompat.cpp
    QtAV_Global.cpp
    subtitle/CharsetDetector.cpp
    subtitle/PlainText.cpp
    subtitle/PlayerSubtitle.cpp
    subtitle/Subtitle.cpp
    subtitle/SubtitleProcessor.cpp
    subtitle/SubtitleProcessorFFmpeg.cpp
    subtitle/SubImage.cpp
    utils/GPUMemCopy.cpp
    utils/Logger.cpp
    AudioThread.cpp
    utils/internal.cpp
    AVThread.cpp
    AudioFormat.cpp
    AudioFrame.cpp
    AudioResampler.cpp
    AudioResamplerTemplate.cpp
    codec/audio/AudioDecoder.cpp
    codec/audio/AudioDecoderFFmpeg.cpp
    codec/audio/AudioEncoder.cpp
    codec/audio/AudioEncoderFFmpeg.cpp
    codec/AVDecoder.cpp
    codec/AVEncoder.cpp
    AVMuxer.cpp
    AVDemuxer.cpp
    AVDemuxThread.cpp
    ColorTransform.cpp
    Frame.cpp
    FrameReader.cpp
    filter/Filter.cpp
    filter/FilterContext.cpp
    filter/FilterManager.cpp
    filter/LibAVFilter.cpp
    filter/SubtitleFilter.cpp
    filter/EncodeFilter.cpp
    ImageConverter.cpp
    ImageConverterFF.cpp
    Packet.cpp
    PacketBuffer.cpp
    AVError.cpp
    AVPlayer.cpp
    AVPlayerPrivate.cpp
    AVTranscoder.cpp
    AVClock.cpp
    VideoCapture.cpp
    VideoFormat.cpp
    VideoFrame.cpp
    io/MediaIO.cpp
    io/QIODeviceIO.cpp
    output/audio/AudioOutput.cpp
    output/audio/AudioOutputBackend.cpp
    output/audio/AudioOutputNull.cpp
    output/video/VideoRenderer.cpp
    output/video/VideoOutput.cpp
    output/video/QPainterRenderer.cpp
    output/AVOutput.cpp
    output/OutputSet.cpp
    Statistics.cpp
    codec/video/VideoDecoder.cpp
    codec/video/VideoDecoderFFmpegBase.cpp
    codec/video/VideoDecoderFFmpeg.cpp
    codec/video/VideoDecoderFFmpegHW.cpp
    codec/video/VideoEncoder.cpp
    codec/video/VideoEncoderFFmpeg.cpp
    VideoThread.cpp
    VideoFrameExtractor.cpp
    )

if(HAVE_OPENGL)
  aux_source_directory(opengl SRC_OPENGL) 
  list(APPEND SOURCES ${SRC_OPENGL})
  list(APPEND SOURCES
    filter/GLSLFilter.cpp
    output/video/OpenGLRendererBase.cpp
  )
  if(NOT Qt5Gui_VERSION VERSION_LESS 5.4.0)
    list(APPEND SDK_HEADERS QtAV/OpenGLWindowRenderer.h)
    list(APPEND SOURCES output/video/OpenGLWindowRenderer.cpp)
  endif()
endif()

if(HAVE_EGL AND HAVE_CAPI)
  list(APPEND SOURCES capi/egl_api.cpp)
  # TODO: X11Extras
  if(DEFINED Qt5Gui_EGL_INCLUDE_DIRS) # ANGLE
    list(APPEND EXTRA_INCLUDE ${Qt5Gui_EGL_INCLUDE_DIRS})
    #list(APPEND EXTRA_DEFS -DQTAV_HAVE_QT_EGL=1)
    if(NOT IOS)
      list(APPEND EXTRA_DEFS -DQTAV_HAVE_EGL_CAPI=1)
      if(WindowsPhone OR WindowsStore)
        list(APPEND EXTRA_DEFS -DCAPI_LINK_EGL)
      endif()
    endif()
  endif()
endif()
if(WIN32 OR WindowsStore OR WindowsPhone)
  check_include_files(XAudio2.h HAVE_XAUDIO2_H)
  if(NOT HAVE_XAUDIO2_H)
    list(APPEND EXTRA_INCLUDE ${QTAV_SOURCE_DIR}/contrib/dxsdk)
  endif()
  message("Qt5Gui_EGL_INCLUDE_DIRS: ${Qt5Gui_EGL_INCLUDE_DIRS}")
  list(APPEND HEADERS
    output/audio/xaudio2_compat.h
    codec/video/VideoDecoderD3D.h
  )
  list(APPEND SOURCES
    utils/DirectXHelper.cpp
    output/audio/AudioOutputXAudio2.cpp
    codec/video/VideoDecoderD3D.cpp
  )
  list(APPEND EXTRA_DEFS -DQTAV_HAVE_XAUDIO2=1)
  set(OLD_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} avcodec) # TODO: lib dir
  set(OLD_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_INCLUDES ${EXTRA_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR})
  check_cxx_source_compiles("
  #include \"directx/dxcompat.h\"
  #include <d3d11.h>
  extern \"C\" {
  #include <libavcodec/d3d11va.h>
  }
  #include <wrl/client.h> //ComPtr is used in QtAV
  int main() {
    av_d3d11va_alloc_context();
    D3D11_VIDEO_PROCESSOR_STREAM s; //used by vp
    return 0;
  }
  " HAVE_D3D11)
  set(CMAKE_REQUIRED_INCLUDES ${OLD_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_LIBRARIES ${OLD_REQUIRED_LIBRARIES})
  if(HAVE_D3D11)
    # https://crascit.com/2015/03/28/enabling-cxx11-in-cmake/ (global and specified target)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    list(APPEND EXTRA_DEFS -DQTAV_HAVE_D3D11VA=1)
    list(APPEND SOURCES
      codec/video/VideoDecoderD3D11.cpp
      directx/SurfaceInteropD3D11.cpp
      directx/D3D11VP.cpp
    )
    if(HAVE_EGL)
      list(APPEND SOURCES directx/SurfaceInteropD3D11EGL.cpp)
    endif()
    if(HAVE_OPENGL AND NOT OPENGLES) # dynamic gl or desktop
      list(APPEND SOURCES directx/SurfaceInteropD3D11GL.cpp)
    endif()
  endif()
  if(WindowsStore OR WindowsPhone)
    list(APPEND SOURCES io/WinRTIO.cpp)
    list(APPEND EXTRA_LIBS
      xaudio2 #only for xbox or >=win8
      shcore
    )
    if(HAVE_D3D11)
      list(APPEND EXTRA_LIBS d3d11)
    endif()
  else()
    list(APPEND EXTRA_LIBS ole32 user32) #ole32: CoInitializeEx for vs2008, but can not find the symbol at runtime
    list(APPEND SOURCES
      output/audio/AudioOutputDSound.cpp
      directx/SurfaceInteropD3D9.cpp
      codec/video/VideoDecoderDXVA.cpp
    )
    if(HAVE_EGL)
      list(APPEND SOURCES directx/SurfaceInteropD3D9EGL.cpp)
    endif()
    if(OPENGL OR DYNAMIC_GL)
      list(APPEND SOURCES directx/SurfaceInteropD3D9GL.cpp)
    endif()
    list(APPEND EXTRA_DEFS
      -DQTAV_HAVE_DSOUND=1
      -DQTAV_HAVE_DXVA=1
    )
  endif()
endif()
check_library_exists(portaudio Pa_Initialize "" HAVE_PORTAUDIO)
if(HAVE_PORTAUDIO)
  list(APPEND SOURCES output/audio/AudioOutputPortAudio.cpp)
  list(APPEND EXTRA_DEFS -DQTAV_HAVE_PORTAUDIO=1)
  list(APPEND EXTRA_LIBS portaudio)
endif()
check_library_exists(pulse pa_threaded_mainloop_new "" HAVE_PULSE)
if(HAVE_PULSE)
  list(APPEND SOURCES output/audio/AudioOutputPulse.cpp)
  list(APPEND EXTRA_DEFS -DQTAV_HAVE_PULSEAUDIO=1)
  list(APPEND EXTRA_LIBS pulse)
endif()
check_library_exists(OpenSLES slCreateEngine "" HAVE_OPENSL)
if(HAVE_OPENSL)
  list(APPEND SOURCES output/audio/AudioOutputOpenSL.cpp)
  list(APPEND EXTRA_DEFS -DQTAV_HAVE_OPENSL=1)
  list(APPEND EXTRA_LIBS OpenSLES)
endif()
check_library_exists(va vaInitialize "" HAVE_VAAPI)
if(HAVE_VAAPI)
  list(APPEND SOURCES
    vaapi/vaapi_helper.cpp
    vaapi/SurfaceInteropVAAPI.cpp
    codec/video/VideoDecoderVAAPI.cpp
  )
  list(APPEND EXTRA_DEFS -DQTAV_HAVE_VAAPI=1)
  list(APPEND EXTRA_LIBS va X11)
endif()

if(NOT APPLE)
  list(APPEND EXTRA_DEFS -DQTAV_HAVE_CUDA=1)
  list(APPEND EXTRA_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/cuda)
  list(APPEND SOURCES
    cuda/cuda_api.cpp
    codec/video/SurfaceInteropCUDA.cpp
    codec/video/VideoDecoderCUDA.cpp
  )
endif()

list(APPEND HEADERS ${SDK_HEADERS} ${SDK_PRIVATE_HEADERS}
    AVPlayerPrivate.h
    AVDemuxThread.h
    AVThread.h
    AVThread_p.h
    AudioThread.h
    PacketBuffer.h
    VideoThread.h
    ImageConverter.h
    ImageConverter_p.h
    codec/video/VideoDecoderFFmpegBase.h
    codec/video/VideoDecoderFFmpegHW.h
    codec/video/VideoDecoderFFmpegHW_p.h
    filter/FilterManager.h
    subtitle/CharsetDetector.h
    subtitle/PlainText.h
    utils/BlockingQueue.h
    utils/GPUMemCopy.h
    utils/Logger.h
    utils/SharedPtr.h
    utils/ring.h
    utils/internal.h
    output/OutputSet.h
    ColorTransform.h
    )

# TODO: rc template
SET(RESOURCES ${MODULE}.qrc shaders/shaders.qrc)
SOURCE_GROUP("Resources" FILES ${RESOURCES})
QT5_ADD_RESOURCES(RESOURCES_SOURCES ${RESOURCES})
set_source_files_properties(${RESOURCES_SOURCES} PROPERTIES GENERATED ON)
if(WIN32)
  set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MODULE}.rc)
  configure_file(${QTAV_SOURCE_DIR}/cmake/QtAV.rc.in ${RC_FILE})
endif()
# add HEADERS for moc
add_library(${MODULE} SHARED ${SOURCES} ${RESOURCES_SOURCES} ${HEADERS} ${RC_FILE})

list(REMOVE_DUPLICATES EXTRA_LIBS)
# qt5_use_modules(since 2.8.9) is deprecated since cmake 2.8.11
target_link_libraries(${MODULE} 
  LINK_PRIVATE ${EXTRA_LIBS}
  LINK_PUBLIC Qt5::Core Qt5::Gui # will add include dirs, macros etc.
)

foreach(PKG_CONFIG_MODULE ${PKG_CONFIG_MODULES})
    target_link_libraries(${MODULE} PRIVATE ${${PKG_CONFIG_MODULE}_LINK_LIBRARIES})
    target_include_directories(${MODULE} PRIVATE ${${PKG_CONFIG_MODULE}_INCLUDE_DIRS})
    target_compile_options(${MODULE} PRIVATE ${${PKG_CONFIG_MODULE}_CFLAGS_OTHER})
    target_compile_definitions(${MODULE} PRIVATE -DQTAV_HAVE_${PKG_CONFIG_MODULE}=1)
endforeach()

set_target_properties(${MODULE} PROPERTIES
  MACOSX_RPATH ON
  FRAMEWORK ON
    #  MACOSX_FRAMEWORK_INFO_PLIST qtav-Info.plist
    #PUBLIC_HEADER ${SDK_HEADERS}
    #PRIVATE_HEADER ${SDK_PRIVATE_HEADERS}
    XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"

  VERSION ${PROJECT_VERSION}
  SOVERSION ${SO_VERSION}
  OUTPUT_NAME ${MODULE}
  CLEAN_DIRECT_OUTPUT 1
  #LINK_SEARCH_START_STATIC 1
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

#http://stackoverflow.com/questions/19866424/cmake-and-qt5-automoc-error
#http://doc.qt.io/qt-5/cmake-manual.html
set(CMAKE_INCLUDE_CURRENT_DIR ON) # for .moc
target_include_directories(${MODULE}
  PUBLIC $<INSTALL_INTERFACE:include>
  PUBLIC $<INSTALL_INTERFACE:include/${MODULE}>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}>
  PRIVATE ${EXTRA_INCLUDE}
  )
target_compile_definitions(${MODULE} PRIVATE ${EXTRA_DEFS})

install(FILES
  ${SDK_HEADERS}
  DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE}
)
install(FILES
  ${SDK_PRIVATE_HEADERS}
  DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE}/private
)
install(TARGETS ${MODULE}
  EXPORT ${MODULE}-targets
  #PUBLIC_HEADER DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE} # list is too long
  #PRIVATE_HEADER DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE}/private
  RUNTIME DESTINATION ${QTAV_INSTALL_BINS}
  LIBRARY DESTINATION ${QTAV_INSTALL_LIBS}
  ARCHIVE DESTINATION ${QTAV_INSTALL_LIBS}
  FRAMEWORK DESTINATION ${QTAV_INSTALL_LIBS}
)

# FIXME: why QTAV_INCLUDE_DIRS is empty?
install(EXPORT ${MODULE}-targets
  DESTINATION ${QTAV_INSTALL_LIBS}/cmake/${MODULE}
  FILE ${MODULE}-config.cmake
)

