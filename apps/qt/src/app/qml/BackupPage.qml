// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// Export and import of the backup code (docs/design/product.md, "Backup").
SubPage {
    id: page

    readonly property var inspected: Backup.inspect(input.text)
    property string exported: Highscores.revision >= 0 ? Backup.exportCode() : ""
    property string status

    title: qsTr("Backup")
    StackView.onActivated: copyButton.forceActiveFocus()

    // Settings changes alter the code too.
    Connections {
        target: AppSettings

        function onLanguageChanged() { page.exported = Backup.exportCode() }
        function onColourBlindChanged() { page.exported = Backup.exportCode() }
        function onVibrationChanged() { page.exported = Backup.exportCode() }
        function onPlayerNameChanged() { page.exported = Backup.exportCode() }
        function onRelaysChanged() { page.exported = Backup.exportCode() }
    }

    ScrollView {
        id: scroll

        anchors.fill: parent
        contentWidth: availableWidth

        GridLayout {
            x: (scroll.availableWidth - width) / 2
            width: Math.min(scroll.availableWidth - 32, page.width > page.height ? 1100 : 420)
            columns: page.width > page.height ? 2 : 1
            columnSpacing: 48
            rowSpacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1 // equal columns in landscape
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 24
                spacing: 14

                Label {
                    text: qsTr("Export")
                    font.pixelSize: 20
                    font.bold: true
                    color: AppTheme.text
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("All highscores and settings in one code. Keep it somewhere safe, e.g. in a note "
                               + "or a message to yourself, and import it on another device or in another "
                               + "Stroopy app.")
                    color: AppTheme.textMuted
                    wrapMode: Text.Wrap
                }
                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    readOnly: true
                    text: page.exported
                    wrapMode: TextEdit.WrapAnywhere
                    font.family: "monospace"
                    font.pixelSize: 13
                    focusPolicy: Qt.NoFocus
                }
                AppButton {
                    id: copyButton

                    text: qsTr("Copy")
                    onClicked: {
                        Sharing.copyToClipboard(page.exported)
                        page.status = qsTr("Backup copied.")
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1 // equal columns in landscape
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                spacing: 14

                Label {
                    text: qsTr("Import")
                    font.pixelSize: 20
                    font.bold: true
                    color: AppTheme.text
                }
                TextArea {
                    id: input

                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    placeholderText: "SYB1-…"
                    wrapMode: TextEdit.WrapAnywhere
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                    KeyNavigation.tab: pasteButton
                }
                AppButton {
                    id: pasteButton

                    text: qsTr("Paste")
                    onClicked: input.text = Sharing.clipboardText()
                }
                Label {
                    Layout.fillWidth: true
                    visible: input.text.length > 0
                    wrapMode: Text.Wrap
                    color: page.inspected.ok ? AppTheme.text : AppTheme.bad
                    text: {
                        const d = page.inspected
                        if (!d.ok)
                            return d.error === "unsupported" ? qsTr("This backup needs a newer version of Stroopy.")
                                                             : qsTr("No valid backup found.")
                        let lines = [qsTr("Highscores: %1").arg(d.entries)]
                        for (const m of d.perMode)
                            lines.push("  " + AppTheme.modeName(m.mode) + ": " + m.count)
                        lines.push(d.hasSettings ? qsTr("Settings included") : qsTr("No settings included"))
                        return lines.join("\n")
                    }
                }
                Switch {
                    id: restoreSettings

                    leftPadding: 0
                    text: qsTr("Also restore settings")
                    checked: true
                    enabled: page.inspected.ok && page.inspected.hasSettings
                }
                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Import")
                    primary: true
                    enabled: page.inspected.ok
                    onClicked: {
                        const added = Backup.importCode(input.text, restoreSettings.checked)
                        page.status = qsTr("Import done. New highscores: %1").arg(added)
                        input.clear()
                    }
                }
            }
        }
    }

    footer: Label {
        visible: page.status.length > 0
        padding: 12
        horizontalAlignment: Text.AlignHCenter
        text: page.status
        color: AppTheme.good
        background: Rectangle { color: AppTheme.surface }
    }
}
