// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Material

// A button styled like the web app's: a pill on the surface colour without shadow, semi-bold text.
// `primary` fills it with the accent, `flat` leaves it transparent, `checked` inverts it.
Button {
    id: control

    property bool primary: false
    property color fill: control.flat ? "transparent"
                       : control.checked ? AppTheme.text
                       : control.primary ? AppTheme.accent : AppTheme.surface
    property real radius: height / 2

    implicitHeight: Math.max(48, implicitContentHeight + topPadding + bottomPadding)
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    leftPadding: 20
    rightPadding: 20
    font.pixelSize: 16
    font.weight: Font.DemiBold
    opacity: enabled ? 1 : 0.4
    Material.foreground: control.checked || control.primary ? AppTheme.background : AppTheme.text

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.Material.foreground
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: control.radius
        color: control.down ? Qt.darker(control.fill, 1.08)
             : control.hovered ? Qt.lighter(control.fill, 1.12) : control.fill
    }
}
