import QtQuick 2.11
import QtQuick.Window 2.3
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2

ApplicationWindow {
    visible: true
    width: 400
    height: 500
    title: qsTr("SSH Drive")
    //Material.theme: Material.Dark
    Material.accent: Material.Teal
    Material.elevation: 1

    ColumnLayout {
        anchors.fill : parent
        anchors.margins: 9
        spacing: 20

        Label {
            text: qsTr("First number")
            Layout.alignment: Qt.AlignCenter
        }

        ComboBox {
            id: cboDrive
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: 200
            model: ListModel {
                id: model
                ListElement { text: "S:   Simulation" }
                ListElement { text: "I:   IT" }
                ListElement { text: "X:   Exploration" }
            }
            onAccepted: {
                if (find(editText) === -1)
                    model.append({text: editText})
            }
        }
        RoundButton {
            id: btnConnect
            Layout.preferredWidth: 200
            Layout.preferredHeight: 80

            Layout.alignment: Qt.AlignCenter
            text: qsTr("Connect")
            Material.background: Material.Teal
            Material.foreground: "white"


            onClicked: {
                // Invoke the calculator slot to sum the numbers
                calculator.sum(firstNumber.text, secondNumber.text)
            }
        }

        ProgressBar {
            id: progressBar
            indeterminate: true
            Layout.preferredWidth: btnConnect.width - 40
            Layout.alignment: Qt.AlignCenter
        }

        Label {
            id: lblStatus
            text: qsTr("Status")
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignCenter
        }

    }

    // Here we take the result of sum or subtracting numbers
    Connections {
        target: calculator

        // Sum signal handler
        onSumResult: {
            // sum was set through arguments=['sum']
            sumResult.text = sum
        }

        // Subtraction signal handler
        onSubResult: {
            // sub was set through arguments=['sub']
            subResult.text = sub
        }
    }
}
