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

Effect {
    id: root
    divider: false
    parameters: ListModel {
        ListElement {
            name: "radius"
            value: 0.5
        }
        ListElement {
            name: "diffraction"
            value: 0.5
        }
    }

    property real posX: -1
    property real posY: -1

    QtObject {
        id: d
        property real oldTargetWidth: root.targetWidth
        property real oldTargetHeight: root.targetHeight
    }

    // Transform slider values, and bind result to shader uniforms
    property real radius: parameters.get(0).value * 100
    property real diffractionIndex: parameters.get(1).value

    onTargetWidthChanged: {
        if (posX == -1)
            posX = targetWidth / 2
        else if (d.oldTargetWidth != 0)
            posX *= (targetWidth / d.oldTargetWidth)
        d.oldTargetWidth = targetWidth
    }

    onTargetHeightChanged: {
        if (posY == -1)
            posY = targetHeight / 2
        else if (d.oldTargetHeight != 0)
            posY *= (targetHeight / d.oldTargetHeight)
        d.oldTargetHeight = targetHeight
    }

    fragmentShaderFilename: "magnify.fsh"

    MouseArea {
        anchors.fill: parent
        onPositionChanged: { root.posX = mouse.x; root.posY = root.targetHeight - mouse.y }
    }
}
