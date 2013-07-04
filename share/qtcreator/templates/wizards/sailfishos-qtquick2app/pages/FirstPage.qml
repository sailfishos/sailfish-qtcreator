import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0


Page {
    id: page

    // To enable PullDownMenu, place our content in a SilicaFlickable
    SilicaFlickable {
        anchors.fill: parent
        
        // PullDownMenu and PushUpMenu must be declared in SilicaFlickable, SilicaListView or SilicaGridView
        PullDownMenu {
            MenuItem {
                text: "Show Page 2"
                onClicked: pageStack.push(Qt.resolvedUrl("SecondPage.qml"))	            
            }
        }

        // Tell SilicaFlickable the height of its content.
        contentHeight: childrenRect.height

        // Place our content in a Column.  The PageHeader is always placed at the top
        // of the page, followed by our content.
        Column {
            width: page.width
            spacing: Theme.paddingLarge
            PageHeader {
                title: "UI Template"
            }
            Label { 
                x: Theme.paddingLarge
                text: "Hello Sailors" 
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeLarge
            }
        }
    }
}

