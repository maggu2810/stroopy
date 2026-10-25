// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

SubPage {
    id: page

    // Prefilled from a stroopy://play link.
    property string initialText
    readonly property var decoded: Sharing.decode(input.text)

    signal play(mode: int, seedHex: string, score: int, reached: int)

    title: qsTr("Enter code")
    StackView.onActivated: playButton.enabled ? playButton.forceActiveFocus() : input.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Paste a code or a whole message containing one.")
            color: AppTheme.textMuted
            wrapMode: Text.Wrap
        }

        TextArea {
            id: input

            Layout.fillWidth: true
            Layout.preferredHeight: 120
            text: page.initialText
            placeholderText: "SYG1-…"
            wrapMode: TextEdit.WrapAnywhere
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            // Leave the text area with Tab / arrow down instead of inserting a tab.
            KeyNavigation.tab: pasteButton
            Keys.onReturnPressed: event => {
                if (playButton.enabled)
                    playButton.click()
                else
                    event.accepted = false
            }
        }

        Button {
            id: pasteButton

            text: qsTr("Paste")
            onClicked: input.text = Sharing.clipboardText()
        }

        Label {
            Layout.fillWidth: true
            visible: input.text.length > 0
            wrapMode: Text.Wrap
            font.pixelSize: 18
            color: page.decoded.ok && page.decoded.playable ? AppTheme.text : AppTheme.bad
            text: {
                const d = page.decoded
                if (!d.ok)
                    return d.error === "unsupported" ? qsTr("This code needs a newer version of Stroopy.")
                                                     : qsTr("No valid code found.")
                if (!d.playable)
                    return qsTr("This code needs a newer version of Stroopy (rules %1).").arg(d.rules)
                let text = AppTheme.modeName(d.mode)
                if (d.hasResult)
                    text += "\n" + qsTr("Score to beat: %1 (out at #%2)").arg(d.score).arg(d.reached)
                return text
            }
        }

        Item {
            Layout.fillHeight: true
        }

        Button {
            id: playButton

            Layout.fillWidth: true
            Layout.preferredHeight: 64
            text: qsTr("Play")
            highlighted: true
            enabled: page.decoded.ok && page.decoded.playable
            onClicked: page.play(page.decoded.mode, page.decoded.seedHex, page.decoded.score, page.decoded.reached)
        }
    }
}
