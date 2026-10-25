// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// Create or join a room (docs/design/product.md, "Multiplayer").
SubPage {
    id: page

    readonly property var parsed: Multiplayer.parseInvitation(codeField.text)

    title: qsTr("Multiplayer")
    StackView.onActivated: createButton.forceActiveFocus()

    ScrollView {
        id: scroll

        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            x: (scroll.availableWidth - width) / 2
            width: Math.min(scroll.availableWidth - 48, 560)
            spacing: 12

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 24
                text: qsTr("Play the same sequence at the same time, over the internet. Everyone sees how far the others get.")
                color: AppTheme.textMuted
                wrapMode: Text.Wrap
            }
            Label {
                Layout.topMargin: 8
                text: qsTr("Player name")
                color: AppTheme.textMuted
            }
            TextField {
                Layout.fillWidth: true
                text: AppSettings.playerName
                maximumLength: 24
                onEditingFinished: AppSettings.playerName = text
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Shown to other players. Names and scores travel unencrypted through public Nostr relays.")
                color: AppTheme.textMuted
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
            Button {
                id: createButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.preferredHeight: 64
                text: qsTr("Create room")
                highlighted: true
                onClicked: Multiplayer.createRoom()
            }
            Label {
                Layout.topMargin: 16
                text: qsTr("Join room")
                font.pixelSize: 20
                font.bold: true
                color: AppTheme.text
            }
            TextField {
                id: codeField

                Layout.fillWidth: true
                placeholderText: qsTr("Paste the invitation (SYM1-…) or the message with it")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                onAccepted: joinButton.click()
            }
            RowLayout {
                spacing: 12

                Button {
                    text: qsTr("Paste")
                    // One line: a wrapped invitation still parses, white space inside it is skipped.
                    onClicked: codeField.text = Sharing.clipboardText().replace(/\s+/g, " ").trim()
                }
                Button {
                    id: joinButton

                    text: qsTr("Join")
                    highlighted: true
                    enabled: page.parsed.ok
                    onClicked: Multiplayer.join(codeField.text)
                }
            }
            Label {
                Layout.fillWidth: true
                Layout.bottomMargin: 24
                visible: codeField.text.length > 0 && !page.parsed.ok
                text: page.parsed.error === "invalid" ? qsTr("No valid invitation found.")
                                                      : qsTr("This room needs a newer version of Stroopy.")
                color: AppTheme.bad
                wrapMode: Text.Wrap
            }
        }
    }
}
