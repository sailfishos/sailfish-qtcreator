import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0


Page {
    id: page
    SilicaListView {
        id: listView
        model: 20
        anchors.fill: parent
        header: PageHeader {
            title: "Nested Page"
        }
        delegate: BackgroundItem {
            Label {
                x: Theme.paddingLarge
                text: "Item " + index
            }
            onClicked: console.log("Clicked " + index)
        }
    }
}




