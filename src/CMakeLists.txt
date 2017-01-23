set(MODULE QtAV)
# TODO: translations, rpath, rc template,egl, dygl, uchardet, d3d11, cuda

INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckIncludeFile)
include(CheckLibraryExists)
INCLUDE(CheckTypeSize)

# set qt dirs first so we can find depends in qt install dir
include_directories(${QTDIR}/include) #TODO: remove. use external/include

get_filename_component(QTDIR "${QTDIR}" ABSOLUTE)

list(APPEND EXTRA_INCLUDE ${CMAKE_SOURCE_DIR}/external/include)
if(APPLE)
    if(IOS)
      #set_xcode_property(myioslib IPHONEOS_DEPLOYMENT_TARGET "8.0")
    else()
      list(APPEND EXTRA_INCLUDE /usr/local/include) #macOS only
      list(APPEND EXTRA_LIBS -L/usr/local/lib)
    endif()
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/contrib/capi/capi.h)
    set(HAVE_CAPI 1)
    list(APPEND EXTRA_INCLUDE ${CMAKE_SOURCE_DIR}/contrib/capi) # TODO: only files use capi.h
    list(APPEND EXTRA_DEFS -DQTAV_HAVE_CAPI=1)
endif()

# check ffmpeg headers. assume libs exist. lib order matters if they are static
set(AVMODULES avformat avcodec swscale avutil) # avdevice avfilter avresample swscale)
# CMAKE_FIND_ROOT_PATH_BOTH: ensure find_file can search from given paths for cross compiling, may be set to ONLY in toolchain file
foreach(av ${AVMODULES})
    string(TOUPPER ${av} AV)
    find_file(HAVE_${AV} lib${av}/${av}.h HINTS ${EXTRA_INCLUDE} PATHS ${EXTRA_INCLUDE} CMAKE_FIND_ROOT_PATH_BOTH)
    message("HAVE_${AV}: ${HAVE_${AV}}")
    if(NOT ${HAVE_${AV}} MATCHES HAVE_${AV}-NOTFOUND)
      set(HAVE_${AV} 1)
      list(APPEND EXTRA_LIBS ${av})
      list(APPEND EXTRA_DEFS "-DQTAV_HAVE_${AV}=1")
    else()
      message(FATAL_ERROR "${av} is required")
    endif()
    #check_include_file(lib${av}/${av}.h HAVE_${AV}) # -D__STDC_CONSTANT_MACROS is required!
endforeach()

foreach(av avfilter avdevice avresample swresample)
  string(TOUPPER ${av} AV)
  find_file(HAVE_${AV} lib${av}/${av}.h HINTS ${EXTRA_INCLUDE} PATHS ${EXTRA_INCLUDE} CMAKE_FIND_ROOT_PATH_BOTH)
  message("HAVE_${AV}: ${HAVE_${AV}}")
  if(NOT ${HAVE_${AV}} MATCHES HAVE_${AV}-NOTFOUND)
    set(HAVE_${AV} 1)
    list(APPEND EXTRA_LIBS ${av})
    list(APPEND EXTRA_DEFS "-DQTAV_HAVE_${AV}=1")
    list(APPEND AVMODULES ${av})
  endif()
endforeach()

if (NOT HAVE_SWRESAMPLE)
  if (NOT HAVE_AVRESAMPLE)
    message(FATAL_ERROR "swresample or avresample is required")
  endif()
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

if(APPLE)
  set(HAVE_OPENAL 1)
  set(DYNAMIC_OPENAL ${HAVE_CAPI})
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
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} -lavcodec)
  check_c_source_compiles(
    "#include <libavcodec/videotoolbox.h>
    int main() {
        av_videotoolbox_alloc_context();
        return 0;
    }" HAVE_VIDEOTOOLBOX)

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

if (HAVE_AVRESAMPLE)
  list(APPEND SOURCES AudioResamplerLibav.cpp)
endif()
if (HAVE_SWRESAMPLE)
  list(APPEND SOURCES AudioResamplerFF.cpp)
endif()

set(CMAKE_AUTOMOC ON)

set(SDK_HEADERS 
      QtAV/QtAV
    QtAV/QtAV.h
    QtAV/dptr.h
    QtAV/QtAV_Global.h
    QtAV/AudioResampler.h
    QtAV/AudioDecoder.h
    QtAV/AudioEncoder.h
    QtAV/AudioFormat.h
    QtAV/AudioFrame.h
    QtAV/AudioOutput.h
    QtAV/AVDecoder.h
    QtAV/AVEncoder.h
    QtAV/AVDemuxer.h
    QtAV/AVMuxer.h
    QtAV/Filter.h
    QtAV/FilterContext.h
    QtAV/GLSLFilter.h
    QtAV/LibAVFilter.h
    QtAV/EncodeFilter.h
    QtAV/Frame.h
    QtAV/FrameReader.h
    QtAV/QPainterRenderer.h
    QtAV/Packet.h
    QtAV/AVError.h
    QtAV/AVPlayer.h
    QtAV/AVTranscoder.h
    QtAV/VideoCapture.h
    QtAV/VideoRenderer.h
    QtAV/VideoOutput.h
    QtAV/MediaIO.h
    QtAV/AVOutput.h
    QtAV/AVClock.h
    QtAV/VideoDecoder.h
    QtAV/VideoEncoder.h
    QtAV/VideoFormat.h
    QtAV/VideoFrame.h
    QtAV/VideoFrameExtractor.h
    QtAV/FactoryDefine.h
    QtAV/Statistics.h
    QtAV/SubImage.h
    QtAV/Subtitle.h
    QtAV/SubtitleFilter.h
    QtAV/SurfaceInterop.h
    QtAV/OpenGLVideo.h
    QtAV/OpenGLRendererBase.h
    QtAV/VideoShader.h
    QtAV/VideoShaderObject.h
    QtAV/version.h
    )
set(SDK_PRIVATE_HEADERS
    QtAV/private/factory.h
    QtAV/private/mkid.h
    QtAV/private/prepost.h
    QtAV/private/singleton.h
    QtAV/private/PlayerSubtitle.h
    QtAV/private/SubtitleProcessor.h
    QtAV/private/AVCompat.h
    QtAV/private/AudioOutputBackend.h
    QtAV/private/AudioResampler_p.h
    QtAV/private/AVDecoder_p.h
    QtAV/private/AVEncoder_p.h
    QtAV/private/MediaIO_p.h
    QtAV/private/AVOutput_p.h
    QtAV/private/Filter_p.h
    QtAV/private/Frame_p.h
    QtAV/private/VideoShader_p.h
    QtAV/private/VideoRenderer_p.h
    QtAV/private/QPainterRenderer_p.h
    QtAV/private/OpenGLRendererBase_p.h
    )

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
    filter/GLSLFilter.cpp
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
    output/video/OpenGLRendererBase.cpp #TODO: check opengl
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

#TODO: check qt opengl support, egl
if(NOT Qt5Gui_VERSION VERSION_LESS 5.4.0)
  list(APPEND SDK_HEADERS QtAV/OpenGLWindowRenderer.h)
  list(APPEND SOURCES output/video/OpenGLWindowRenderer.cpp)
endif()
if(WIN32 OR WindowsStore OR WindowsPhone)
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
  if(WindowsStore OR WindowsPhone)
    list(APPEND SOURCES io/WinRTIO.cpp)
    list(APPEND EXTRA_LIBS
      xaudio2 #only for xbox or >=win8
      shcore
    )
  else()
    list(APPEND EXTRA_LIBS ole32 user32) #ole32: CoInitializeEx for vs2008, but can not find the symbol at runtime
    list(APPEND SOURCES
      output/audio/AudioOutputDSound.cpp
      directx/SurfaceInteropD3D9.cpp
      directx/SurfaceInteropD3D9EGL.cpp # TODO: check EGL/dynamic gl
      directx/SurfaceInteropD3D9GL.cpp # TODO: check desktop/dynamic gl
      codec/video/VideoDecoderDXVA.cpp
    )
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

aux_source_directory(opengl SRC_OPENGL) 
list(APPEND SOURCES ${SRC_OPENGL})
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

# add HEADERS for moc
add_library(${MODULE} SHARED ${SOURCES} ${RESOURCES_SOURCES} ${HEADERS})

list(REMOVE_DUPLICATES EXTRA_LIBS)
# qt5_use_modules(since 2.8.9) is deprecated since cmake 2.8.11
target_link_libraries(${MODULE} 
  LINK_PRIVATE ${EXTRA_LIBS}
  LINK_PUBLIC Qt5::Core Qt5::Gui # will add include dirs, macros etc.
)
set_target_properties(${MODULE} PROPERTIES
  MACOSX_RPATH ON
  FRAMEWORK ON
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
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/QtAV
  PRIVATE ${EXTRA_INCLUDE}
  )
target_compile_definitions(${MODULE} PRIVATE ${EXTRA_DEFS})
