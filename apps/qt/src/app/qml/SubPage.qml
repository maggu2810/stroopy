// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// Secondary page with a title bar and a back button.
Page {
    id: page

    signal back()

    background: Rectangle { color: AppTheme.background }

    header: ToolBar {
        Material.background: AppTheme.surface

        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "‹"
                font.pixelSize: 28
                onClicked: page.back()
            }
            Label {
                Layout.fillWidth: true
                text: page.title
                font.pixelSize: 20
                font.bold: true
                color: AppTheme.text
            }
        }
    }
}
