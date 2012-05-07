/*
This file is part of Helium.

Helium is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Helium is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Helium.  If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick 1.1
import com.nokia.meego 1.0
import org.jensge.UPnP 1.0

import "components"

Page {
    property alias page: pageHeader.text

    QueryDialog {
        id: dlgError
        property string errorMessage
        property int    errorId

        titleText: qsTr("Error")
        message: qsTr("Error accessing server \"%1\":\n%2\n(Error code %3)").arg(page).arg(errorMessage).arg(errorId)
        acceptButtonText: qsTr("Ok")
        onAccepted: {
            browseModelStack.pop();
            if (browseModelStack.empty()) {
                main.pageStackBrowse.pop();
            }
        }
    }

    Connections {
        target: server
        onUnavailable: {
            dlgServerLost.open()
        }

        onError: {
            dlgError.errorMessage=message;
            dlgError.errorId=code;
            dlgError.open();
        }
    }

    QueryDialog {
        id: dlgServerLost
        titleText: qsTr("Server gone")
        message: qsTr("Connection to server \"%1\" was lost").arg(page);
        acceptButtonText: qsTr("Ok")
        onAccepted: {
            browseModelStack.clear();
            main.pageStackBrowse.pop();
        }
    }

    Rectangle {
        color: "black"
        anchors.top: pageHeader.bottom
        anchors.topMargin: 10
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        BusyIndicator {
            visible: browseModel.busy
            running: browseModel.busy
            platformStyle: BusyIndicatorStyle {
                size: "large"
            }
            anchors.centerIn: parent
        }

        ListView {
            visible: !browseModel.busy
            id: browseListView
            width: parent.width
            height: parent.height
            spacing: 15
            cacheBuffer: parent.height + 100
            snapMode: ListView.SnapToItem
            model: browseModel
            currentIndex: browseModel.lastIndex
            highlightMoveSpeed: -1

            delegate: BrowseDelegate {
                mainText: model.title
                subText: model.detail
                image: model.icon
                iconAnnotated: model.uri === ""
                iconVisible: settings.displayMediaArt
                drillDown: model.type === "container"

                ActiveSelection {
                    id: selectedHighlight
                    visible: model.type !== "container" && browseModel.lastIndex === index
                }

                onClicked: {
                    browseModel.lastIndex = index
                    if (type === "container") {
                        server.browse(upnpId, upnpClass, renderer.protocolInfo);
                    }
                }

                onPressAndHold: {
                    browseModel.lastIndex = index
                    if (type !== "container" && uri !== "") {
                        feedback.start();
                        renderer.setAVTransportUri(uri, metadata);
                        if (renderer.state === "STOPPED") {
                            renderer.play();
                        }
                    }
                }
            }
        }
        ScrollDecorator {
            flickableItem: browseListView
        }
    }

    PageHeader {
        id: pageHeader
        busy: !browseModel.busy && !browseModel.done
        onClicked: browseModel.refresh()
    }
}
