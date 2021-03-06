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

import "pages"
import "components"

PageStackWindow {
    id: appWindow

    property Style platformLabelStyle: LabelStyle {
        inverted: theme.inverted
    }
    property Style platformDialogStyle: SelectionDialogStyle {
        inverted: theme.inverted
    }

    initialPage: MainPage {
        id: main
    }

    About {
        id: about
    }

    Settings {
        id: settingsPage
    }

    ServerList {
        id: serverList
        model: serverModel
        role: "server"
        title: qsTr("Servers")
        delegate: BrowseDelegate {
            id: serverDelegate
            mainText: model.friendlyName
            subText: model.udn
            image: model.icon
            iconVisible: settings.displayDeviceIcons
            iconAnnotated: false
            drillDown: true

            onClicked: {
                serverDelegate.ListView.view.currentIndex = index;
                server.wrapDevice(serverModel.get(index));
                server.browse("0", "object.container", renderer.protocolInfo);
                browse.page = friendlyName
                pageStack.push(browse)
            }
        }
    }

    ServerList {
        id: playerList
        model: rendererModel
        role: "renderer"
        title: qsTr("Renderer")
        delegate: RendererDelegate {}
    }

    Browse {
        id: browse
    }

    UPnPRenderer {
        id: renderer
        onProtocolInfoChanged: {
            browseModel.protocolInfo = renderer.protocolInfo;
        }
    }

    UPnPMediaServer {
        id: server
    }

    Component.onCompleted: {
        theme.inverted = true
    }
}
