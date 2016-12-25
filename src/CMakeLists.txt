

INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckTypeSize)

# set qt dirs first so we can find depends in qt install dir
include_directories(${QTDIR}/include)
set(MODULES Core Gui)

foreach(M ${MODULES})
    set(mod Qt5${M})
    # look for the config files in the QtConfigRoot defined above
    set(${mod}_DIR ${QTCONFIGROOT}${M})

    # look for the actual package
    find_package(${mod} REQUIRED)

    include_directories(${${mod}_INCLUDE_DIRS})
    include_directories(${${mod}_PRIVATE_INCLUDE_DIRS})

    list(APPEND QT5_LIBRARIES ${${mod}_LIBRARIES})
    list(APPEND QT5_CFLAGS ${${mod}_EXECUTABLE_COMPILE_FLAGS})
endforeach(M ${MODULES})

get_target_property(QtCore_location Qt5::Core LOCATION)
get_filename_component(QTDIR "${QTDIR}" ABSOLUTE)
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L${QTDIR}/lib") # unix: for extra libs
CHECK_INCLUDE_FILE(ass/ass.h HAVE_ASS_H)
#if (HAVE_ASS_H)
  # check link
#endif()
CHECK_INCLUDE_FILE(libavfilter/avfilter.h HAVE_AVFILTER_H)
CHECK_INCLUDE_FILE(libavdevice/avdevice.h HAVE_AVDEVICE_H)
CHECK_INCLUDE_FILE(libavresample/avresample.h HAVE_AVRESAMPLE_H)
CHECK_INCLUDE_FILE(libswresample/swresample.h HAVE_SWRESAMPLE_H)
if (NOT HAVE_SWRESAMPLE_H)
  if (NOT HAVE_AVRESAMPLE_H)
    #message(FATAL_ERROR "swresample or avresample is required")
  endif()
endif()

set(QTAV_EXTRA_DEFS -DBUILD_QTAV_LIB -DQ_AV_EXPORT -D__STDC_CONSTANT_MACROS)
set(QTAV_EXTRA_LIBS avcodec avformat avutil swscale)
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
    list(APPEND QTAV_EXTRA_DEFS -DQTAV_HAVE_VIDEOTOOLBOX=1)
    list(APPEND QTAV_EXTRA_LIBS "-framework CoreMedia -framework VideoToolbox -framework CoreVideo")
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
  set(HAVE_OPENAL_H 1)
  add_definitions(-DHEADER_OPENAL_PREFIX)

  list(APPEND SOURCES output/audio/AudioOutputAudioToolbox.cpp)
  list(APPEND QTAV_EXTRA_LIBS "-framework CoreFoundation -framework AudioToolbox -framework OpenGL")
endif()

set(DEP_H AVRESAMPLE SWRESAMPLE AVDEVICE AVFILTER OPENAL VIDEOTOOLBOX)
foreach (D ${DEP_H})
  if (${D})
    ADD_DEFINITIONS(-DQTAV_HAVE_${D}=1)
  endif()
endforeach()

if (HAVE_AVFILTER_H)
  list(APPEND QTAV_EXTRA_DEFS -DQTAV_HAVE_AVFILTER=1)
  list(APPEND QTAV_EXTRA_LIBS avfilter)
endif()
if (HAVE_AVDEVICE_H)
  list(APPEND QTAV_EXTRA_DEFS -DQTAV_HAVE_AVDEVICE=1)
  list(APPEND QTAV_EXTRA_LIBS avdevice)
endif()
if (HAVE_AVRESAMPLE_H)
  list(APPEND QTAV_EXTRA_DEFS -DQTAV_HAVE_AVRESAMPLE=1)
  list(APPEND QTAV_EXTRA_LIBS avresample)
  list(APPEND SOURCES AudioResamplerLibav.cpp)
endif()
if (HAVE_SWRESAMPLE_H)
  list(APPEND QTAV_EXTRA_DEFS -DQTAV_HAVE_SWRESAMPLE=1)
  list(APPEND QTAV_EXTRA_LIBS swresample)
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
    QtAV/LibAVFilter.h
    QtAV/EncodeFilter.h
    QtAV/Frame.h
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
    QtAV/Subtitle.h
    QtAV/SubtitleFilter.h
    QtAV/SurfaceInterop.h
    QtAV/OpenGLVideo.h
    QtAV/OpenGLRendererBase.h
    QtAV/OpenGLWindowRenderer.h
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
    output/video/OpenGLRendererBase.cpp
    output/video/OpenGLWindowRenderer.cpp
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

SET(RESOURCES QtAV.qrc shaders/shaders.qrc)
SOURCE_GROUP("Resources" FILES ${RESOURCES})
QT5_ADD_RESOURCES(RESOURCES_SOURCES ${RESOURCES})
set_source_files_properties(${RESOURCES_SOURCES} PROPERTIES GENERATED ON)

# add HEADERS for moc
add_library(QtAV SHARED ${SOURCES} ${RESOURCES_SOURCES} ${HEADERS})
qt5_use_modules(QtAV Core Gui)
target_link_libraries(QtAV ${QTAV_EXTRA_LIBS})
set_target_properties(QtAV PROPERTIES
  MACOSX_RPATH ON
  FRAMEWORK ON
  VERSION ${PROJECT_VERSION}
  SOVERSION ${SO_VERSION}
  OUTPUT_NAME "QtAV"
  CLEAN_DIRECT_OUTPUT 1
  #LINK_SEARCH_START_STATIC 1
)


#http://stackoverflow.com/questions/19866424/cmake-and-qt5-automoc-error
#http://doc.qt.io/qt-5/cmake-manual.html
set(CMAKE_INCLUDE_CURRENT_DIR ON) # for .moc
target_include_directories(QtAV
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_compile_definitions(QtAV
  PRIVATE ${QTAV_EXTRA_DEFS}
)
