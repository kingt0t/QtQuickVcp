import QtQuick 2.0
import QtQuick.Controls 1.2
import Machinekit.Application 1.0

Action {
    property var status: {"synced": false}
    property var command: {"connected": false}
    property int axis: 0

    property bool _ready: status.synced && command.connected

    id: root
    text: qsTr("Home")
    shortcut: "Ctrl+Home"
    tooltip: qsTr("Home Axis ") + axis + " [" + shortcut + "]"
    onTriggered: {
        if (status.task.taskMode !== ApplicationStatus.TaskModeManual)
            command.setTaskMode(ApplicationCommand.TaskModeManual)
        command.homeAxis(axis)
    }

    enabled: _ready
             && (status.task.taskState === ApplicationStatus.TaskStateOn)
             && !status.running
}
