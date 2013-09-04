/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    id: root
    color: "black"
    property alias effect: effectLoader.item
    property alias gripSize: divider.gripSize
    property string effectSource
    property real volume: 0.5

    signal videoFramePainted

    Divider {
        id: divider
        visible: false
        z: 1.0
        onValueChanged: updateDivider()
    }

    ShaderEffectSource {
        id: theSource
        smooth: true
        hideSource: true
    }

    Loader {
        id: contentLoader
    }

    Loader {
        id: effectLoader
        source: effectSource
    }

    Connections {
        id: videoFramePaintedConnection
        onFramePainted: {
            if (performanceLoader.item)
               root.videoFramePainted()
        }
        ignoreUnknownSignals: true
    }

    onWidthChanged: {
        if (effectLoader.item)
            effectLoader.item.targetWidth = root.width
    }

    onHeightChanged: {
        if (effectLoader.item)
            effectLoader.item.targetHeight = root.height
    }

    onEffectSourceChanged: {
        console.log("[qmlvideofx] Content.onEffectSourceChanged " + effectSource)
        effectLoader.source = effectSource
        effectLoader.item.parent = root
        effectLoader.item.targetWidth = root.width
        effectLoader.item.targetHeight = root.height
        updateSource()
        effectLoader.item.source = theSource
        divider.visible = effectLoader.item.divider
        updateDivider()
    }

    function init() {
        console.log("[qmlvideofx] Content.init")
        openImage("qrc:/images/qt-logo.png")
        root.effectSource = "EffectPassThrough.qml"
    }

    function updateDivider() {
        if (effectLoader.item && effectLoader.item.divider)
            effectLoader.item.dividerValue = divider.value
    }

    function updateSource() {
        console.log("[qmlvideofx] Content.updateSource")
        if (contentLoader.item) {
            contentLoader.item.parent = root
            contentLoader.item.anchors.fill = root
            theSource.sourceItem = contentLoader.item
            if (effectLoader.item)
                effectLoader.item.anchors.fill = contentLoader.item
        }
    }

    function openImage(path) {
        console.log("[qmlvideofx] Content.openImage \"" + path + "\"")
        stop()
        contentLoader.source = "ContentImage.qml"
        videoFramePaintedConnection.target = null
        contentLoader.item.source = path
        updateSource()
    }

    function openVideo(path) {
        console.log("[qmlvideofx] Content.openVideo \"" + path + "\"")
        stop()
        contentLoader.source = "ContentVideo.qml"
        videoFramePaintedConnection.target = contentLoader.item
        contentLoader.item.mediaSource = path
        contentLoader.item.volume = volume
        contentLoader.item.play()
        updateSource()
    }

    function openCamera() {
        console.log("[qmlvideofx] Content.openCamera")
        stop()
        contentLoader.source = "ContentCamera.qml"
        videoFramePaintedConnection.target = contentLoader.item
        updateSource()
    }

    function stop() {
        console.log("[qmlvideofx] Content.stop")
        if (contentLoader.source == "ContentVideo.qml")
            contentLoader.item.stop()
        theSource.sourceItem = null
    }
}
