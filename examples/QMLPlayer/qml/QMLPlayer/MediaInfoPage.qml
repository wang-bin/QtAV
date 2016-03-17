import QtQuick 2.0
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Media Information")
    height: Math.min(maxHeight, scroll.contentHeight)
    property var information: "unknow" //let it be defined
    Flickable {
        id: scroll
        anchors.fill: content
        Text {
            id: info
            wrapMode: Text.Wrap
            color: "white"
            font.pixelSize: Utils.kFontSize
            anchors.fill: parent
            onContentSizeChanged: {
                scroll.contentHeight = contentHeight + Utils.scaled(32)
            }
        }
    }
    onInformationChanged: {
        console.log(information)
        var metaData = information.metaData
        var text = "<p>" + information.source + "</p>"
        if (typeof metaData.duration != "undefined") {
            text += "<p>" + qsTr("Duration: ")
            if (metaData.startTime > 0)
                text += metaData.startTime + "+"
            text += metaData.duration + " ms</p>"
        }
        if (typeof metaData.size != "undefined")
            text += "<p>" + qsTr("Size: ") + Math.round(metaData.size/1024/1024*100)/100 + " M</p>"
        if (typeof metaData.title != "undefined")
            text += "<p>" + qsTr("Title") + ": " +metaData.title + "</p>"
        if (typeof metaData.albumTitle != "undefined")
            text += "<p>" + qsTr("Album") + ": " + Utils.htmlEscaped(metaData.albumTitle) + "</p>"
        if (typeof metaData.title != "undefined")
            text += "<p>" + qsTr("Comment") + ": " + Utils.htmlEscaped(metaData.comment) + "</p>"
        if (typeof metaData.year != "undefined")
            text += "<p>" + qsTr("Year") + ": " + metaData.year + "</p>"
        if (typeof metaData.date != "undefined")
            text += "<p>" + qsTr("Date") + ": " + metaData.date + "</p>"
        if (typeof metaData.author != "undefined")
            text += "<p>" + qsTr("Author") + ": " + Utils.htmlEscaped(metaData.author) + "</p>"
        if (typeof metaData.publisher != "undefined")
            text += "<p>" + qsTr("Publisher") + ": " + Utils.htmlEscaped(metaData.publisher) + "</p>"
        if (typeof metaData.genre != "undefined")
            text += "<p>" + qsTr("Genre") + ": " + metaData.genre + "</p>"
        if (typeof metaData.trackNumber != "undefined")
            text += "<p>" + qsTr("Track") + ": " + metaData.trackNumber + "</p>"
        if (typeof metaData.trackCount != "undefined")
            text += "<p>" + qsTr("Track count") + ": " + metaData.trackCount + "</p>"

        if (information.hasVideo) {
            text += "<h4>" + qsTr("Video") + "</h4>"
                    + "<p>" + qsTr("Resolution") + ": " + metaData.resolution.width + "x" +  + metaData.resolution.height
                    + "</p><p>" + qsTr("Bit rate") + ": " + metaData.videoBitRate
                    + "</p><p>" + qsTr("Codec") + ": " + metaData.videoCodec
                    + "</p><p>" + qsTr("Pixel format") + ": " + metaData.pixelFormat
                    + "</p><p>" + qsTr("Frames") + ": " + metaData.videoFrames + ", " + qsTr("Frame rate") + ": " + Math.round(100*metaData.videoFrameRate)/100 + "</p>"
        }
        if (information.hasAudio) {
            text += "<h4>" + qsTr("Audio") + "</h4>"
                    + "<p>" + qsTr("Codec") + ": " + metaData.audioCodec
                    + "</p><p>" + qsTr("Sample format") + ": " + metaData.sampleFormat
                    + "</p><p>" + qsTr("Sample rate") + ": " + metaData.sampleRate
                    + "</p><p>" + qsTr("Channels") + ": " + metaData.channelCount + " (" + metaData.channelLayout + ")"
                    + "</p><p>" + qsTr("Bit rate") + ": " + metaData.audioBitRate
            + "</p>"
        }
        info.text = text
    }
}
