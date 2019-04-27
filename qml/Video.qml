
import QtQuick 2.0
import QtAV 1.7

/*!
    \qmltype Video
    \inherits Item
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml
    \inqmlmodule QtAV
    \brief A convenience type for showing a specified video.

    \c Video is a convenience type combining the functionality
    of a \l MediaPlayer and a \l VideoOutput into one. It provides
    simple video playback functionality without having to declare multiple
    types.

    \qml
    import QtQuick 2.0
    import QtAV 1.7

    Video {
        id: video
        width : 800
        height : 600
        source: "video.avi"

        MouseArea {
            anchors.fill: parent
            onClicked: {
                video.play()
            }
        }

        focus: true
        Keys.onSpacePressed: video.playbackState == MediaPlayer.PlayingState ? video.pause() : video.play()
        Keys.onLeftPressed: video.seek(video.position - 5000)
        Keys.onRightPressed: video.seek(video.position + 5000)
    }
    \endqml

    \c Video supports untransformed, stretched, and uniformly scaled
    video presentation. For a description of stretched uniformly scaled
    presentation, see the \l fillMode property description.

    \sa MediaPlayer, VideoOutput

    \section1 Screen Saver

    If it is likely that an application will be playing video for an extended
    period of time without user interaction, it may be necessary to disable
    the platform's screen saver. The \l ScreenSaver (from \l QtSystemInfo)
    may be used to disable the screensaver in this fashion:

    \qml
    import QtSystemInfo 5.0

    ScreenSaver { screenSaverEnabled: false }
    \endqml
*/

Item {
    id: video

    property alias startPosition: player.startPosition
    property alias stopPosition: player.stopPosition
    property alias videoFiltersGPU: videoOut.filters
    property alias audioFilters: player.audioFilters
    property alias videoFilters: player.videoFilters
    property alias audioBackends: player.audioBackends
    property alias supportedAudioBackends: player.supportedAudioBackends
    property alias backgroundColor: videoOut.backgroundColor
    property alias brightness: videoOut.brightness
    property alias contrast: videoOut.contrast
    property alias hue: videoOut.hue
    property alias saturation: videoOut.saturation
    property alias frameSize: videoOut.frameSize
    property alias sourceAspectRatio: videoOut.sourceAspectRatio
    property alias opengl: videoOut.opengl
    property alias fastSeek: player.fastSeek
    property alias timeout: player.timeout
    property alias abortOnTimeout: player.abortOnTimeout
    property alias subtitle: subtitle
    property alias subtitleText: text_sub // not for ass.
    property alias videoCapture: player.videoCapture
    property alias audioTrack: player.audioTrack
    property alias videoTrack: player.videoTrack
    property alias externalAudio: player.externalAudio
    property alias internalAudioTracks: player.internalAudioTracks
    property alias externalAudioTracks: player.externalAudioTracks
    property alias internalVideoTracks: player.internalVideoTracks
    property alias internalSubtitleTracks: player.internalSubtitleTracks
    property alias internalSubtitleTrack: player.internalSubtitleTrack
    /*** Properties of VideoOutput ***/
    /*!
        \qmlproperty enumeration Video::fillMode

        Set this property to define how the video is scaled to fit the target
        area.

        \list
        \li VideoOutput.Stretch - the video is scaled to fit
        \li VideoOutput.PreserveAspectFit - the video is scaled uniformly to fit without
            cropping
        \li VideoOutput.PreserveAspectCrop - the video is scaled uniformly to fill, cropping
            if necessary
        \endlist

        Because this type is for convenience in QML, it does not
        support enumerations directly, so enumerations from \c VideoOutput are
        used to access the available fill modes.

        The default fill mode is preserveAspectFit.
    */
    property alias fillMode:            videoOut.fillMode

    /*!
        \qmlproperty int Video::orientation

        The orientation of the \c Video in degrees. Only multiples of 90
        degrees is supported, that is 0, 90, 180, 270, 360, etc.
    */
    property alias orientation:         videoOut.orientation


    /*** Properties of MediaPlayer ***/

    /*!
      A list of video codec names in priority order.
      Example: videoCodecPriority: ["VAAPI", "FFmpeg"]
      Default is ["FFmpeg"]
    s*/
    property alias videoCodecPriority:   player.videoCodecPriority
    property alias channelLayout:        player.channelLayout
    /*!
        \qmlproperty enumeration Video::playbackState

        This read only property indicates the playback state of the media.

        \list
        \li MediaPlayer.PlayingState - the media is playing
        \li MediaPlayer.PausedState - the media is paused
        \li MediaPlayer.StoppedState - the media is stopped
        \endlist

        The default state is MediaPlayer.StoppedState.
    */
    property alias playbackState:        player.playbackState

    /*!
        \qmlproperty bool Video::autoLoad

        This property indicates if loading of media should begin immediately.

        Defaults to true, if false media will not be loaded until playback is
        started.
    */
    property alias autoLoad:        player.autoLoad

    /*!
        \qmlproperty real Video::bufferProgress

        This property holds how much of the data buffer is currently filled,
        from 0.0 (empty) to 1.0
        (full).
    */
    property alias bufferProgress:  player.bufferProgress

    /*!
        \qmlproperty int Video::bufferSize

        This property holds the buffer value.
    */
    property alias bufferSize:  player.bufferSize

    /*!
        \qmlproperty int Video::duration

        This property holds the duration of the media in milliseconds.

        If the media doesn't have a fixed duration (a live stream for example)
        this will be 0.
    */
    property alias duration:        player.duration

    /*!
        \qmlproperty enumeration Video::error

        This property holds the error state of the video.  It can be one of:

        \list
        \li MediaPlayer.NoError - there is no current error.
        \li MediaPlayer.ResourceError - the video cannot be played due to a problem
            allocating resources.
        \li MediaPlayer.FormatError - the video format is not supported.
        \li MediaPlayer.NetworkError - the video cannot be played due to network issues.
        \li MediaPlayer.AccessDenied - the video cannot be played due to insufficient
            permissions.
        \li MediaPlayer.ServiceMissing -  the video cannot be played because the media
            service could not be
        instantiated.
        \endlist
    */
    property alias error:           player.error

    /*!
        \qmlproperty string Video::errorString

        This property holds a string describing the current error condition in more detail.
    */
    property alias errorString:     player.errorString

    /*!
        \qmlproperty enumeration Video::availability

        Returns the availability state of the video instance.

        This is one of:
        \table
        \header \li Value \li Description
        \row \li MediaPlayer.Available
            \li The video player is available to use.
        \row \li MediaPlayer.Busy
            \li The video player is usually available, but some other
               process is utilizing the hardware necessary to play media.
        \row \li MediaPlayer.Unavailable
            \li There are no supported video playback facilities.
        \row \li MediaPlayer.ResourceMissing
            \li There is one or more resources missing, so the video player cannot
               be used.  It may be possible to try again at a later time.
        \endtable
     */
    //property alias availability:    player.availability

    /*!
        \qmlproperty bool Video::hasAudio

        This property holds whether the current media has audio content.
    */
    property alias hasAudio:        player.hasAudio

    /*!
        \qmlproperty bool Video::hasVideo

        This property holds whether the current media has video content.
    */
    property alias hasVideo:        player.hasVideo

    /* documented below due to length of metaData documentation */
    property alias metaData:        player.metaData

    /*!
        \qmlproperty bool Video::muted

        This property holds whether the audio output is muted.
    */
    property alias muted:           player.muted

    /*!
        \qmlproperty real Video::playbackRate

        This property holds the rate at which video is played at as a multiple
        of the normal rate.
    */
    property alias playbackRate:    player.playbackRate

    /*!
        \qmlproperty int Video::position

        This property holds the current playback position in milliseconds.

        To change this position, use the \l seek() method.

        \sa seek()
    */
    property alias position:        player.position

    /*!
        \qmlproperty bool Video::seekable

        This property holds whether the playback position of the video can be
        changed.

        If true, calling the \l seek() method will cause playback to seek to the new position.
    */
    property alias seekable:        player.seekable

    /*!
        \qmlproperty url Video::source

        This property holds the source URL of the media.
    */
    property alias source:          player.source

    /*!
        \qmlproperty enumeration Video::status

        This property holds the status of media loading. It can be one of:

        \list
        \li MediaPlayer.NoMedia - no media has been set.
        \li MediaPlayer.Loading - the media is currently being loaded.
        \li MediaPlayer.Loaded - the media has been loaded.
        \li MediaPlayer.Buffering - the media is buffering data.
        \li MediaPlayer.Stalled - playback has been interrupted while the media is buffering data.
        \li MediaPlayer.Buffered - the media has buffered data.
        \li MediaPlayer.EndOfMedia - the media has played to the end.
        \li MediaPlayer.InvalidMedia - the media cannot be played.
        \li MediaPlayer.UnknownStatus - the status of the media cannot be determined.
        \endlist
    */
    property alias status:          player.status

    /*!
        \qmlproperty real Video::volume

        This property holds the volume of the audio output, from 0.0 (silent) to 1.0 (maximum volume).
    */
    property alias volume:          player.volume

    /*!
        \qmlproperty bool Video::autoPlay

        This property determines whether the media should begin playback automatically.

        Setting to \c true also sets \l autoLoad to \c true. The default is \c false.
    */
    property alias autoPlay:        player.autoPlay

    /*!
        \qmlsignal Video::paused()

        This signal is emitted when playback is paused.
    */
    signal paused

    /*!
        \qmlsignal Video::stopped()

        This signal is emitted when playback is stopped.
    */
    signal stopped

    /*!
        \qmlsignal Video::playing()

        This signal is emitted when playback is started or continued.
    */
    signal playing

    signal seekFinished

    VideoOutput2 {
        id: videoOut
        anchors.fill: video
        source: player
    }

    SubtitleItem {
        id: ass_sub
        rotation: -videoOut.orientation
        fillMode: videoOut.fillMode
        source: subtitle
        anchors.fill: videoOut
    }
    Text {
        id: text_sub
        rotation: -videoOut.orientation
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignBottom
        font {
            pointSize: 20
            bold: true
        }
        style: Text.Outline
        styleColor: "blue"
        color: "white"
        anchors.fill: videoOut
    }

    MediaPlayer {
        id: player
        onPaused:  video.paused()
        onStopped: video.stopped()
        onPlaying: video.playing()
        onSeekFinished: video.seekFinished()
    }

    function stepForward() {
        player.stepForward()
    }

    function stepBackward() {
        player.stepBackward()
    }

    /*!
        \qmlmethod Video::play()

        Starts playback of the media.
    */
    function play() {
        player.play();
    }

    /*!
        \qmlmethod Video::pause()

        Pauses playback of the media.
    */
    function pause() {
        player.pause();
    }

    /*!
        \qmlmethod Video::stop()

        Stops playback of the media.
    */
    function stop() {
        player.stop();
    }

    /*!
        \qmlmethod Video::seek(offset)

        If the \l seekable property is true, seeks the current
        playback position to \a offset.

        Seeking may be asynchronous, so the \l position property
        may not be updated immediately.

        \sa seekable, position
    */
    function seek(offset) {
        player.seek(offset);
    }

    Subtitle {
        id: subtitle
        player: player
        onContentChanged: {
            if (!canRender || !ass_sub.visible)
                text_sub.text = text
        }
        onEngineChanged: { // assume a engine canRender is only used as a renderer
            ass_sub.visible = canRender
            text_sub.visible = !canRender
        }
        onEnabledChanged: {
            ass_sub.visible = enabled
            text_sub.visible = enabled
        }
    }
}

// ***************************************
// Documentation for meta-data properties.
// ***************************************

/*!
    \qmlproperty variant Video::metaData

    This property holds a collection of all the meta-data for the media.

    You can access individual properties like \l {Video::metaData.title}{metaData.title}
    or \l {Video::metaData.trackNumber} {metaData.trackNumber}.
*/

/*!
    \qmlproperty variant Video::metaData.title

    This property holds the title of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.subTitle

    This property holds the sub-title of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.author

    This property holds the author of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.comment

    This property holds a user comment about the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.description

    This property holds a description of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.category

    This property holds the category of the media

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.genre

    This property holds the genre of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.year

    This property holds the year of release of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.date

    This property holds the date of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.userRating

    This property holds a user rating of the media in the range of 0 to 100.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.keywords

    This property holds a list of keywords describing the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.language

    This property holds the language of the media, as an ISO 639-2 code.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.publisher

    This property holds the publisher of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.copyright

    This property holds the media's copyright notice.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.parentalRating

    This property holds the parental rating of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.ratingOrganization

    This property holds the name of the rating organization responsible for the
    parental rating of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.size

    This property property holds the size of the media in bytes.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.mediaType

    This property holds the type of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.audioBitRate

    This property holds the bit rate of the media's audio stream in bits per
    second.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.audioCodec

    This property holds the encoding of the media audio stream.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.averageLevel

    This property holds the average volume level of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.channelCount

    This property holds the number of channels in the media's audio stream.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.peakValue

    This property holds the peak volume of the media's audio stream.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.sampleRate

    This property holds the sample rate of the media's audio stream in Hertz.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.albumTitle

    This property holds the title of the album the media belongs to.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.albumArtist

    This property holds the name of the principal artist of the album the media
    belongs to.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.contributingArtist

    This property holds the names of artists contributing to the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.composer

    This property holds the composer of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.conductor

    This property holds the conductor of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.lyrics

    This property holds the lyrics to the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.mood

    This property holds the mood of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.trackNumber

    This property holds the track number of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.trackCount

    This property holds the number of track on the album containing the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.coverArtUrlSmall

    This property holds the URL of a small cover art image.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.coverArtUrlLarge

    This property holds the URL of a large cover art image.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.resolution

    This property holds the dimension of an image or video.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.pixelAspectRatio

    This property holds the pixel aspect ratio of an image or video.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.videoFrameRate

    This property holds the frame rate of the media's video stream.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.videoBitRate

    This property holds the bit rate of the media's video stream in bits per
    second.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.videoCodec

    This property holds the encoding of the media's video stream.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.posterUrl

    This property holds the URL of a poster image.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.chapterNumber

    This property holds the chapter number of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.director

    This property holds the director of the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.leadPerformer

    This property holds the lead performer in the media.

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.writer

    This property holds the writer of the media.

    \sa {QMediaMetaData}
*/

// The remaining properties are related to photos, and are technically
// available but will certainly never have values.

/*!
    \qmlproperty variant Video::metaData.cameraManufacturer

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.cameraModel

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.event

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.subject

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.orientation

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.exposureTime

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.fNumber

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.exposureProgram

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.isoSpeedRatings

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.exposureBiasValue

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.dateTimeDigitized

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.subjectDistance

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.meteringMode

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.lightSource

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.flash

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.focalLength

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.exposureMode

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.whiteBalance

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.DigitalZoomRatio

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.focalLengthIn35mmFilm

    \sa {QMediaMetaData::FocalLengthIn35mmFile}
*/

/*!
    \qmlproperty variant Video::metaData.sceneCaptureType

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.gainControl

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.contrast

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.saturation

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.sharpness

    \sa {QMediaMetaData}
*/

/*!
    \qmlproperty variant Video::metaData.deviceSettingDescription

    \sa {QMediaMetaData}
*/

