/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

import QtQuick 2.15
import MuseScore.Ui 1.0
import MuseScore.UiComponents 1.0
import MuseScore.Audio 1.0

MixerPanelSection {
    id: root

    headerTitle: qsTrc("playback", "Audio Fx")

    Column {
        id: content

        y: 0

        height: childrenRect.height
        width: root.delegateDefaultWidth

        property string accessibleName: (Boolean(root.needReadChannelName) ? item.title + " " : "") + root.headerTitle

        spacing: 6

        Repeater {
            id: repeater
            anchors.horizontalCenter: parent.horizontalCenter

            model: item.outputResourceItemList
            delegate: AudioResourceControl {
                id: inputResourceControl

                anchors.horizontalCenter: parent.horizontalCenter

                menuAnchorItem: root.rootPanel
                resourceItemModel: modelData
                active: modelData.isActive

                navigationPanel: item.panel
                navigationRowStart: root.navigationRowStart + (model.index * 3) // NOTE: 3 - because AudioResourceControl have 3 controls
                navigationName: modelData.id
                accessibleName: content.accessibleName

                onTurnedOn: {
                    modelData.isActive = true
                }

                onTurnedOff: {
                    modelData.isActive = false
                }

                onTitleClicked: {
                    modelData.requestToLaunchNativeEditorView()
                }

                onNavigateControlIndexChanged: {
                    root.navigateControlIndexChanged(index)
                }
            }
        }
    }
}
