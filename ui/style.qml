import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 720
    minimumWidth: 600
    minimumHeight: 320
    title: "morph"
    color: "black"

    property var currentTrack: null
    property int currentTrackIndex: -1
    property string currentView: "home"
    property string currentPlaylist: ""
    property string searchSource: "all"
    property string oldPlaylistName: ""
    property string librarySubView: "grid"
    property bool isEditingPlaylist: false
    property int likesVersion: 0
    property int playlistsVersion: 0
    property bool repeatOne: false

    Component.onCompleted: {
        var yToken = MorphSettings.getYandexToken()
        var sToken = MorphSettings.getSoundCloudToken()
        if (yToken) MorphServices.setYandexToken(yToken)
        if (sToken) MorphServices.setSoundCloudClientId(sToken)
        
        MorphServices.getCharts()
        var hist = MorphSettings.getSearchHistory()
        for (var i = 0; i < hist.length; i++) historyModel.append(hist[i])
        
        var session = MorphSettings.loadSession()
        if (session.track) {
            currentTrack = session.track
            MorphAudio.volume = session.volume || 50
            MorphServices.resolve(currentTrack.service, currentTrack.id)
            MorphAudio.position = session.position || 0
            MorphMpris.updateMetadata(currentTrack)
            repeatOne = session.repeatOne || false
        }
    }

    onClosing: {
        MorphSettings.saveSession({
            "track": currentTrack,
            "volume": MorphAudio.volume,
            "position": MorphAudio.position,
            "repeatOne": repeatOne
        })
    }

    function getServiceIcon(serviceName) {
        if (serviceName === "Yandex") return "assets/yandex_music_icon.svg"
        if (serviceName === "SoundCloud") return "assets/soundcloud_icon.svg"
        return ""
    }

    function openPlaylist(name) {
        currentPlaylist = name
        libraryModel.clear()
        if (name === "LIKED") {
            currentPlaylist = ""
            var likes = MorphSettings.getLikedTracks()
            for(var i = likes.length - 1; i >= 0; i--) libraryModel.append(likes[i])
        } else {
            var pls = MorphSettings.getPlaylists()
            var pData = pls[name]
            if (pData) {
                var tracks = pData.tracks || (Array.isArray(pData) ? pData : [])
                for (var i = 0; i < tracks.length; i++) libraryModel.append(tracks[i])
            }
        }
        librarySubView = "tracks"
    }

    function playTrack(track, index) {
        var cleanTrack = {
            id: track.id,
            title: track.title,
            artist: track.artist,
            album: track.album || "",
            coverUrl: track.coverUrl,
            service: track.service
        }
        currentTrack = cleanTrack
        currentTrackIndex = index
        MorphServices.resolve(cleanTrack.service, cleanTrack.id)
        MorphServices.reportPlay(cleanTrack.service, cleanTrack.id, cleanTrack.album)
        MorphMpris.updateMetadata(cleanTrack)
        MorphAudio.play("")
    }

    function playNext() {
        var model = currentView === "search" ? (searchModel.count > 0 ? searchModel : historyModel) : libraryModel
        if (currentTrackIndex + 1 < model.count) playTrack(model.get(currentTrackIndex + 1), currentTrackIndex + 1)
    }

    function playPrevious() {
        if (MorphAudio.position > 3000) MorphAudio.position = 0
        else if (currentTrackIndex - 1 >= 0) {
            var model = currentView === "search" ? (searchModel.count > 0 ? searchModel : historyModel) : libraryModel
            playTrack(model.get(currentTrackIndex - 1), currentTrackIndex - 1)
        }
    }

    ListModel { id: searchModel }
    ListModel { id: libraryModel }
    ListModel { id: playlistsModel }
    ListModel { id: historyModel }
    ListModel { id: chartsModel }

    Rectangle {
        anchors.fill: parent
        color: "black"
        radius: 20
        clip: true

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 200
                color: "black"
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 30
                    spacing: 15
                    
                    Text { 
                        text: "MORPH"
                        color: "white"
                        font.family: "Rubik"
                        font.pixelSize: 20
                        font.weight: Font.Black
                        Layout.bottomMargin: 25
                    }
                    
                    Repeater {
                        model: ["Search", "Home", "Library", "Settings"]
                        delegate: ItemDelegate {
                            Layout.fillWidth: true
                            height: 40
                            onClicked: {
                                currentView = modelData.toLowerCase()
                                if (currentView === "library") {
                                    librarySubView = "grid"
                                    playlistsModel.clear()
                                    var pls = MorphSettings.getPlaylists()
                                    for (var p in pls) playlistsModel.append({ "name": p, "coverUrl": pls[p].coverUrl || "" })
                                }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                            contentItem: Text {
                                text: modelData
                                color: currentView === modelData.toLowerCase() ? "white" : "#444"
                                font.family: "Rubik"
                                font.pixelSize: 15
                                font.weight: currentView === modelData.toLowerCase() ? Font.Medium : Font.Normal
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Item {}
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 12
                    color: "#222222"
                    radius: 18
                    clip: true

                    StackLayout {
                        anchors.fill: parent
                        currentIndex: {
                            if (currentView === "search") return 0
                            if (currentView === "home") return 1
                            if (currentView === "library") return 2
                            return 3
                        }
                        
                        Item {
                            ColumnLayout {
                                anchors.fill: parent; anchors.margins: 35; spacing: 20
                                
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 10
                                    
                                    TextField {
                                        id: searchField; placeholderText: "SEARCH"; Layout.fillWidth: true; color: "white"; font.family: "Rubik"; font.pixelSize: 14; padding: 12
                                        background: Rectangle { color: "#151515"; radius: 8; border.color: "#333" }
                                        onAccepted: { searchModel.clear(); MorphServices.search(text, searchSource) }
                                    }
                                    
                                    RowLayout {
                                        spacing: 5
                                        Repeater {
                                            model: ["all", "yandex", "soundcloud"]
                                            Button {
                                                Layout.preferredHeight: 36; Layout.preferredWidth: 80
                                                text: modelData.toUpperCase()
                                                onClicked: { searchSource = modelData; if(searchField.text !== "") { searchModel.clear(); MorphServices.search(searchField.text, searchSource) } }
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: searchSource === modelData ? "black" : "#888"; font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                background: Rectangle { color: searchSource === modelData ? "white" : "#1a1a1a"; radius: 6; border.color: "#333" }
                                            }
                                        }
                                    }
                                }
                                
                                StackLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    currentIndex: searchModel.count > 0 ? 1 : 0
                                    
                                    ColumnLayout {
                                        spacing: 20
                                        Text { text: "RECENT SEARCHES"; color: "#444"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Black }
                                        ListView { 
                                            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                                            model: historyModel; delegate: trackDelegate
                                        }
                                    }
                                    
                                    ListView { 
                                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                                        model: searchModel; delegate: trackDelegate
                                    }
                                }
                            }
                        }

                        Item {
                            Flickable {
                                id: homeFlickable
                                anchors.fill: parent; contentHeight: homeContent.height + 70; clip: true
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                                
                                ColumnLayout {
                                    id: homeContent
                                    width: homeFlickable.width - 70
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top; anchors.topMargin: 35
                                    spacing: 35
                                    
                                    Rectangle {
                                        id: vibeCard
                                        Layout.fillWidth: true; Layout.preferredHeight: 180; radius: 25
                                        clip: true; border.color: "#333"; border.width: 1
                                        color: "#1a1a1a"
                                        
                                        ColumnLayout {
                                            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; anchors.margins: 30; spacing: 5
                                            Text { text: "MY VIBE"; color: "white"; font.family: "Rubik"; font.pixelSize: 28; font.weight: Font.Black }
                                            Text { 
                                                text: (currentPlaylist === "MY_VIBE" && MorphAudio.isPlaying) ? "PLAYING NOW" : "PERSONALIZED WAVE"
                                                color: (currentPlaylist === "MY_VIBE" && MorphAudio.isPlaying) ? "#44ff44" : "white"
                                                font.family: "Rubik"; font.pixelSize: 14; opacity: (currentPlaylist === "MY_VIBE" && MorphAudio.isPlaying) ? 1.0 : 0.8 
                                                font.weight: (currentPlaylist === "MY_VIBE" && MorphAudio.isPlaying) ? Font.Bold : Font.Normal
                                            }
                                        }
                                        
                                        Rectangle {
                                            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.margins: 30
                                            width: 64; height: 64; radius: 32; color: "#333"
                                            Image {
                                                anchors.centerIn: parent; 
                                                source: (currentPlaylist === "MY_VIBE" && MorphAudio.isPlaying) ? "assets/pause.svg" : "assets/play.svg"
                                                Layout.preferredWidth: 32; Layout.preferredHeight: 32
                                                layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                            }
                                            MouseArea { 
                                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor; 
                                                onClicked: {
                                                    if (currentPlaylist === "MY_VIBE") {
                                                        MorphAudio.isPlaying ? MorphAudio.pause() : MorphAudio.resume()
                                                    } else {
                                                        MorphServices.getWave()
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 20
                                        Text { text: "CHARTS"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Black }
                                        ListView {
                                            id: chartsListView
                                            Layout.fillWidth: true; Layout.preferredHeight: contentHeight; interactive: false; clip: true
                                            model: chartsModel.count > 0 ? Math.min(10, Math.ceil(chartsModel.count / 2)) : 0
                                            delegate: RowLayout {
                                                width: chartsListView.width; height: 54; spacing: 20
                                                property var leftTrack: chartsModel.get(index)
                                                property var rightTrack: (index + 10 < chartsModel.count) ? chartsModel.get(index + 10) : null
                                                
                                                ItemDelegate {
                                                    Layout.fillWidth: true; Layout.preferredWidth: 1; height: 54; hoverEnabled: true
                                                    background: Rectangle { color: (currentTrack && leftTrack && currentTrack.id === leftTrack.id && currentTrack.service === "Yandex") ? "#1a1a1a" : (parent.hovered ? "#222" : "transparent"); radius: 6 }
                                                    contentItem: RowLayout {
                                                        spacing: 15
                                                        Text { text: (index + 1).toString(); color: (currentTrack && leftTrack && currentTrack.id === leftTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "#888"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; Layout.preferredWidth: 25; horizontalAlignment: Text.AlignRight }
                                                        Image { 
                                                            source: leftTrack ? (leftTrack.coverUrl || "") : ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop 
                                                            layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 36; height: 36; radius: 6 } }
                                                        }
                                                        ColumnLayout {
                                                            Layout.fillWidth: true; spacing: 2
                                                            Text { Layout.fillWidth: true; text: leftTrack ? leftTrack.title : ""; color: (currentTrack && leftTrack && currentTrack.id === leftTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                                                            Text { Layout.fillWidth: true; text: leftTrack ? leftTrack.artist : ""; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; elide: Text.ElideRight }
                                                        }
                                                    }                                                    MouseArea { 
                                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                        onClicked: if (leftTrack) {
                                                            libraryModel.clear()
                                                            for(var i=0; i<chartsModel.count; i++) libraryModel.append(chartsModel.get(i))
                                                            playTrack(libraryModel.get(index), index)
                                                        }
                                                    }
                                                }

                                                ItemDelegate {
                                                    Layout.fillWidth: true; Layout.preferredWidth: 1; height: 54; hoverEnabled: true
                                                    visible: rightTrack !== null
                                                    background: Rectangle { color: (currentTrack && rightTrack && currentTrack.id === rightTrack.id && currentTrack.service === "Yandex") ? "#1a1a1a" : (parent.hovered ? "#222" : "transparent"); radius: 6 }
                                                    contentItem: RowLayout {
                                                        spacing: 15
                                                        Text { text: (index + 11).toString(); color: (currentTrack && rightTrack && currentTrack.id === rightTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "#888"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; Layout.preferredWidth: 25; horizontalAlignment: Text.AlignRight }
                                                        Image { 
                                                            source: rightTrack ? (rightTrack.coverUrl || "") : ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop 
                                                            layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 36; height: 36; radius: 6 } }
                                                        }
                                                        ColumnLayout {
                                                            Layout.fillWidth: true; spacing: 2
                                                            Text { Layout.fillWidth: true; text: rightTrack ? rightTrack.title : ""; color: (currentTrack && rightTrack && currentTrack.id === rightTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                                                            Text { Layout.fillWidth: true; text: rightTrack ? rightTrack.artist : ""; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; elide: Text.ElideRight }
                                                        }
                                                    }                                                    MouseArea { 
                                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                        onClicked: if (rightTrack) {
                                                            libraryModel.clear()
                                                            for(var i=0; i<chartsModel.count; i++) libraryModel.append(chartsModel.get(i))
                                                            playTrack(libraryModel.get(index + 10), index + 10)
                                                        }
                                                    }
                                                }
                                                
                                                Item { Layout.fillWidth: true; Layout.preferredWidth: 1; visible: rightTrack === null }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        Item {
                            ColumnLayout {
                                anchors.fill: parent; anchors.margins: 35; spacing: 20
                                
                                StackLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    currentIndex: librarySubView === "grid" ? 0 : 1

                                    ColumnLayout {
                                        spacing: 20
                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text { text: "LIBRARY"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Black }
                                            Item { Layout.fillWidth: true }
                                            Button {
                                                text: "NEW PLAYLIST"
                                                onClicked: createPlaylistPopup.open()
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                background: Rectangle { color: "#333"; radius: 6 }
                                            }
                                        }
                                        
                                        GridView {
                                            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                                            cellWidth: 160; cellHeight: 200
                                            model: playlistsModel
                                            header: Item {
                                                width: 160; height: 200
                                                Rectangle {
                                                    anchors.fill: parent; anchors.margins: 10; color: "#1a1a1a"; radius: 12
                                                    ColumnLayout {
                                                        anchors.fill: parent; anchors.margins: 10; spacing: 8
                                                        Rectangle {
                                                            Layout.fillWidth: true; Layout.preferredHeight: width; color: "#333"; radius: 8
                                                            Image {
                                                                anchors.centerIn: parent
                                                                source: "assets/heart.svg"; Layout.preferredWidth: 32; Layout.preferredHeight: 32; sourceSize: Qt.size(64, 64)
                                                                layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                            }
                                                        }
                                                        Text { text: "LIKED TRACKS"; color: "white"; font.family: "Rubik"; font.pixelSize: 13; font.weight: Font.Bold; elide: Text.ElideRight; Layout.fillWidth: true }
                                                    }
                                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: openPlaylist("LIKED") }
                                                }
                                            }
                                            delegate: Item {
                                                width: 160; height: 200
                                                Rectangle {
                                                    anchors.fill: parent; anchors.margins: 10; color: "#1a1a1a"; radius: 12
                                                    ColumnLayout {
                                                        anchors.fill: parent; anchors.margins: 10; spacing: 8
                                                        Rectangle {
                                                            Layout.fillWidth: true; Layout.preferredHeight: width; color: "#333"; radius: 8
                                                            Image {
                                                                anchors.fill: parent; source: model.coverUrl || ""; fillMode: Image.PreserveAspectCrop
                                                                visible: model.coverUrl !== ""; layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 120; height: 120; radius: 8 } }
                                                            }
                                                            Text { anchors.centerIn: parent; text: "♪"; color: "#444"; font.pixelSize: 40; visible: model.coverUrl === "" }
                                                        }
                                                        Text { text: model.name; color: "white"; font.family: "Rubik"; font.pixelSize: 13; font.weight: Font.Bold; elide: Text.ElideRight; Layout.fillWidth: true }
                                                    }
                                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: openPlaylist(model.name) }
                                                }
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: 20
                                        RowLayout {
                                            Layout.fillWidth: true; spacing: 15
                                            Button {
                                                text: "← BACK"
                                                onClicked: librarySubView = "grid"
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: "#888"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold }
                                                background: Item {}
                                            }
                                            Image {
                                                id: playlistHeaderImage
                                                source: {
                                                    if (currentPlaylist === "") return ""
                                                    var pls = MorphSettings.getPlaylists()
                                                    return (pls[currentPlaylist] && pls[currentPlaylist].coverUrl) ? pls[currentPlaylist].coverUrl : ""
                                                }
                                                visible: currentPlaylist !== "" && source.toString() !== ""
                                                Layout.preferredWidth: visible ? 40 : 0; Layout.preferredHeight: 40; fillMode: Image.PreserveAspectCrop
                                                layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 40; height: 40; radius: 8 } }
                                            }
                                            Text { 
                                                text: currentPlaylist === "" ? "LIKED TRACKS" : currentPlaylist.toUpperCase()
                                                color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Bold 
                                            }
                                            Item { Layout.fillWidth: true }
                                            
                                            RowLayout {
                                                visible: currentPlaylist !== ""
                                                spacing: 10
                                                Button {
                                                    text: "EDIT"
                                                    onClicked: {
                                                        var pls = MorphSettings.getPlaylists()
                                                        plNameField.text = currentPlaylist
                                                        oldPlaylistName = currentPlaylist
                                                        plCoverField.text = pls[currentPlaylist].coverUrl || ""
                                                        isEditingPlaylist = true
                                                        createPlaylistPopup.open()
                                                    }
                                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                    contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                    background: Rectangle { color: "#333"; radius: 4; border.color: "#444" }
                                                }
                                                Button {
                                                    text: "DELETE"
                                                    onClicked: deleteConfirmationPopup.open()
                                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                    contentItem: Text { text: parent.text; color: "#ff4444"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                    background: Rectangle { color: "#220000"; radius: 4; border.color: "#441111" }
                                                }
                                            }
                                        }
                                        ListView { 
                                            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                                            model: libraryModel; delegate: trackDelegate
                                        }
                                    }
                                }
                            }
                        }

                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 35
                                spacing: 25
                                Text { text: "SETTINGS"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Bold }
                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: 10
                                    Text { text: "Yandex Music Token"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                    TextField {
                                        id: yandexTokenField; text: MorphSettings.getYandexToken(); Layout.fillWidth: true
                                        color: "white"; font.family: "Rubik"; font.pixelSize: 13; padding: 12; echoMode: TextInput.Password
                                        background: Rectangle { color: "#151515"; radius: 6; border.color: "#333" }
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: 10
                                    Text { text: "SoundCloud Client ID"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                    TextField {
                                        id: soundcloudTokenField; text: MorphSettings.getSoundCloudToken(); Layout.fillWidth: true
                                        color: "white"; font.family: "Rubik"; font.pixelSize: 13; padding: 12; echoMode: TextInput.Password
                                        background: Rectangle { color: "#151515"; radius: 6; border.color: "#333" }
                                    }
                                }
                                Button {
                                    text: "SAVE SETTINGS"; Layout.preferredWidth: 150
                                    onClicked: {
                                        MorphSettings.setYandexToken(yandexTokenField.text); MorphSettings.setSoundCloudToken(soundcloudTokenField.text)
                                        MorphServices.setYandexToken(yandexTokenField.text); MorphServices.setSoundCloudClientId(soundcloudTokenField.text)
                                    }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                    contentItem: Text { text: parent.text; color: "black"; font.family: "Rubik"; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter }
                                    background: Rectangle { color: "white"; radius: 6 }
                                }
                                Item { Layout.fillHeight: true }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    color: "black"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 4
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            Slider {
                                id: progressSlider; anchors.fill: parent
                                from: 0; to: MorphAudio.duration > 0 ? MorphAudio.duration : 1
                                value: MorphAudio.position; onMoved: MorphAudio.position = value; padding: 0
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                background: Rectangle { radius: 16; color: "#1a1a1a"; Rectangle { width: progressSlider.visualPosition * parent.width; height: parent.height; color: "white"; radius: 16 } }
                                handle: Item {}
                            }
                        }

                        Item {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 30; anchors.rightMargin: 30; spacing: 0
                                
                                Item {
                                    Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                                    RowLayout {
                                        id: trackInfoRow
                                        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                        width: Math.min(parent.width, 350); spacing: 15; visible: currentTrack !== null
                                        Image { 
                                            source: currentTrack ? currentTrack.coverUrl : ""; Layout.preferredWidth: 48; Layout.preferredHeight: 48; fillMode: Image.PreserveAspectCrop
                                            layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 48; height: 48; radius: 10 } }
                                        }
                                        Column {
                                            Layout.fillWidth: true; clip: true
                                            Text { width: parent.width; text: currentTrack ? currentTrack.title : ""; color: "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                                            RowLayout {
                                                width: parent.width; spacing: 6
                                                Image { source: currentTrack ? getServiceIcon(currentTrack.service) : ""; Layout.preferredWidth: 10; Layout.preferredHeight: 10 }
                                                Text { Layout.fillWidth: true; text: currentTrack ? currentTrack.artist : ""; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; elide: Text.ElideRight }
                                            }
                                        }
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: playbackControls.width; Layout.fillHeight: true
                                    RowLayout {
                                        id: playbackControls
                                        anchors.centerIn: parent; spacing: 25
                                        Image {
                                            source: repeatOne ? "assets/repeat-once.svg" : "assets/repeat.svg"; Layout.preferredWidth: 22; Layout.preferredHeight: 22; sourceSize: Qt.size(64, 64); layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                            MouseArea { anchors.fill: parent; onClicked: repeatOne = !repeatOne; cursorShape: Qt.PointingHandCursor }
                                        }
                                        Image {
                                            source: "assets/skip-previous.svg"; Layout.preferredWidth: 26; Layout.preferredHeight: 26; sourceSize: Qt.size(64, 64); smooth: true; layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                            MouseArea { anchors.fill: parent; onClicked: playPrevious(); cursorShape: Qt.PointingHandCursor }
                                        }
                                        Image {
                                            source: MorphAudio.isPlaying ? "assets/pause-circle.svg" : "assets/play-circle.svg"; Layout.preferredWidth: 50; Layout.preferredHeight: 50; sourceSize: Qt.size(128, 128); smooth: true; layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                            MouseArea { anchors.fill: parent; onClicked: MorphAudio.isPlaying ? MorphAudio.pause() : MorphAudio.resume(); cursorShape: Qt.PointingHandCursor }
                                        }
                                        Image {
                                            source: "assets/skip-next.svg"; Layout.preferredWidth: 26; Layout.preferredHeight: 26; sourceSize: Qt.size(64, 64); smooth: true; layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                            MouseArea { anchors.fill: parent; onClicked: playNext(); cursorShape: Qt.PointingHandCursor }
                                        }
                                        Image {
                                            source: (window.likesVersion, currentTrack && MorphSettings.isLiked(currentTrack.id)) ? "assets/heart.svg" : "assets/heart-outline.svg"; Layout.preferredWidth: 22; Layout.preferredHeight: 22; sourceSize: Qt.size(64, 64); layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                            MouseArea { anchors.fill: parent; onClicked: if(currentTrack) MorphSettings.toggleLike(currentTrack); cursorShape: Qt.PointingHandCursor }
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                                    RowLayout {
                                        id: volumeRow
                                        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                        spacing: window.width > 700 ? 12 : 8
                                        
                                        Rectangle {
                                            visible: MorphAudio.bitrate > 0 && window.width > 900
                                            width: bitrateText.width + 10; height: 16; radius: 4; color: "#1a1a1a"
                                            Text {
                                                id: bitrateText; anchors.centerIn: parent
                                                text: MorphAudio.bitrate + " kbps"
                                                color: MorphAudio.bitrate <= 128 ? "#ff4444" : (MorphAudio.bitrate <= 256 ? "#ffcc00" : "#44ff44")
                                                font.family: "Rubik"; font.pixelSize: 9; font.weight: Font.Bold
                                            }
                                        }

                                        Text { text: "VOL"; color: "#444"; font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Black; visible: window.width > 800 }
                                        Slider {
                                            id: volumeSlider; Layout.preferredWidth: window.width > 700 ? 80 : 50; Layout.preferredHeight: 20; from: 0; to: 100; value: MorphAudio.volume; onMoved: MorphAudio.volume = value
                                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                            background: Rectangle { x: volumeSlider.leftPadding; y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2; width: volumeSlider.availableWidth; height: 3; radius: 1.5; color: "#333"
                                                Rectangle { width: volumeSlider.visualPosition * parent.width; height: parent.height; color: "white"; radius: 1.5 } }
                                            handle: Rectangle { x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width); y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2; width: 10; height: 10; radius: 5; color: "white" }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: trackDelegate
        Rectangle {
            id: trackDelegateRoot
            width: ListView.view.width
            height: 54
            color: (currentTrack && currentTrack.id === model.id && currentTrack.service === model.service) ? "#1a1a1a" : (trackMouseArea.containsMouse ? "#222" : "transparent")
            radius: 6

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 15
                Image { 
                    source: model.coverUrl || ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop 
                    layer.enabled: true
                    layer.effect: OpacityMask { maskSource: Rectangle { width: 36; height: 36; radius: 6 } }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 2
                    Text { Layout.fillWidth: true; text: model.title || ""; color: (currentTrack && currentTrack.id === model.id && currentTrack.service === model.service) ? "#44ff44" : "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 6
                        Image { source: getServiceIcon(model.service); Layout.preferredWidth: 12; Layout.preferredHeight: 12 }
                        Text { Layout.fillWidth: true; text: model.artist || ""; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; elide: Text.ElideRight }
                    }
                }
                Image {
                    source: (window.likesVersion, MorphSettings.isLiked(model.id)) ? "assets/heart.svg" : "assets/heart-outline.svg"; Layout.preferredWidth: 18; Layout.preferredHeight: 18; layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                    MouseArea { 
                        anchors.fill: parent; onClicked: MorphSettings.toggleLike({ "id": model.id, "title": model.title, "artist": model.artist, "coverUrl": model.coverUrl, "service": model.service }); cursorShape: Qt.PointingHandCursor 
                        preventStealing: true
                    }
                }
            }

            MouseArea {
                id: trackMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                z: -1
                onClicked: {
                    var track; var m;
                    if (currentView === "search") {
                        if (searchModel.count > 0) { track = searchModel.get(index); m = searchModel }
                        else { track = historyModel.get(index); m = historyModel }

                        var tObj = { "id": track.id, "title": track.title, "artist": track.artist, "coverUrl": track.coverUrl, "service": track.service }
                        MorphSettings.addSearchHistory(tObj)

                        historyModel.clear()
                        var hist = MorphSettings.getSearchHistory()
                        for (var i = 0; i < hist.length; i++) historyModel.append(hist[i])

                        var newIdx = index
                        if (m === historyModel) {
                            for(var j=0; j<historyModel.count; j++) {
                                if(historyModel.get(j).id === tObj.id) { newIdx = j; break }
                            }
                        }
                        playTrack(tObj, newIdx)
                    } else {
                        track = libraryModel.get(index)
                        playTrack(track, index)
                    }
                }            }
        }
    }

    property var trackToPlaylist: null
    Menu {
        id: playlistMenu
        Repeater {
            model: playlistsModel
            MenuItem {
                text: model.name
                onClicked: if(trackToPlaylist) MorphSettings.addToPlaylist(model.name, trackToPlaylist)
            }
        }
    }

    function formatTime(ms) {
        var m = Math.floor(ms / 60000); var s = Math.floor((ms % 60000) / 1000)
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    Connections {
        target: MorphServices
        function onSearchResultsReady(serviceName, results) {
            for (var i = 0; i < results.length; i++) {
                var item = results[i]; item.service = serviceName; searchModel.append(item)
            }
        }
        function onChartsReady(serviceName, results) {
            if (results.length > 0) chartsModel.clear()
            for (var i = 0; i < results.length; i++) {
                var item = results[i]; item.service = serviceName; chartsModel.append(item)
            }
        }
        function onWaveReady(serviceName, results) {
            currentPlaylist = "MY_VIBE"
            libraryModel.clear()
            for (var i = 0; i < results.length; i++) {
                var item = results[i]; item.service = serviceName; libraryModel.append(item)
            }
            if (libraryModel.count > 0) playTrack(libraryModel.get(0), 0)
        }
        function onStreamUrlReady(trackId, streamUrl) {
            MorphAudio.play(streamUrl)
            if (currentTrackIndex === -1) { var oldPos = MorphAudio.position; MorphAudio.pause(); MorphAudio.position = oldPos }
        }
    }
    
    Connections {
        target: MorphSettings
        function onLikesChanged() { 
            window.likesVersion++
            if (currentView === "library" && currentPlaylist === "") {
                libraryModel.clear()
                var likes = MorphSettings.getLikedTracks()
                for(var i = likes.length - 1; i >= 0; i--) libraryModel.append(likes[i])
            }
        }
        function onPlaylistsChanged() {
            window.playlistsVersion++
            if (currentView === "library") {
                playlistsModel.clear()
                var pls = MorphSettings.getPlaylists()
                for (var p in pls) playlistsModel.append({ "name": p, "coverUrl": pls[p].coverUrl || "" })
                
                if (currentPlaylist !== "") {
                    libraryModel.clear()
                    var pData = pls[currentPlaylist]
                    if (pData) {
                        var tracks = pData.tracks || (Array.isArray(pData) ? pData : [])
                        for (var i = 0; i < tracks.length; i++) libraryModel.append(tracks[i])
                    }
                }
            }
        }
    }

    Connections {
        target: MorphMpris
        function onNextRequested() { playNext() }
        function onPreviousRequested() { playPrevious() }
    }

    Connections {
        target: MorphAudio
        function onFinished() { if (repeatOne) { MorphAudio.setPosition(0); MorphAudio.resume() } else playNext() }
    }

    Popup {
        id: createPlaylistPopup
        parent: Overlay.overlay
        x: (parent.width - width) / 2; y: (parent.height - height) / 2
        width: 300; height: 250; modal: true; focus: true
        background: Rectangle { color: "#1a1a1a"; radius: 12; border.color: "#333" }
        
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 15
            Text { text: isEditingPlaylist ? "EDIT PLAYLIST" : "CREATE PLAYLIST"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold }
            TextField {
                id: plNameField; placeholderText: "PLAYLIST NAME"; Layout.fillWidth: true
                color: "white"; font.family: "Rubik"; font.pixelSize: 12; padding: 10
                background: Rectangle { color: "#111"; radius: 6; border.color: "#333" }
            }
            TextField {
                id: plCoverField; placeholderText: "COVER URL (IMGUR/PINTEREST)"; Layout.fillWidth: true
                color: "white"; font.family: "Rubik"; font.pixelSize: 12; padding: 10
                background: Rectangle { color: "#111"; radius: 6; border.color: "#333" }
            }
            Button {
                text: isEditingPlaylist ? "SAVE" : "CREATE"; Layout.fillWidth: true; Layout.preferredHeight: 40
                onClicked: {
                    if (plNameField.text !== "") {
                        if (isEditingPlaylist) {
                            MorphSettings.renamePlaylist(oldPlaylistName, plNameField.text, plCoverField.text)
                            currentPlaylist = plNameField.text
                        } else {
                            MorphSettings.createPlaylist(plNameField.text, plCoverField.text)
                        }
                        plNameField.text = ""; plCoverField.text = ""; createPlaylistPopup.close()
                    }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                contentItem: Text { text: parent.text; color: "black"; font.family: "Rubik"; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: "white"; radius: 6 }
            }
        }
        onClosed: { isEditingPlaylist = false; oldPlaylistName = "" }
    }

    Popup {
        id: deleteConfirmationPopup
        parent: Overlay.overlay
        x: (parent.width - width) / 2; y: (parent.height - height) / 2
        width: 280; height: 150; modal: true; focus: true
        background: Rectangle { color: "#1a1a1a"; radius: 12; border.color: "#333" }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 20
            Text { text: "DELETE PLAYLIST?"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
            RowLayout {
                spacing: 15; Layout.fillWidth: true
                Button {
                    text: "CANCEL"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: deleteConfirmationPopup.close()
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                    contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: "#333"; radius: 6 }
                }
                Button {
                    text: "DELETE"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: {
                        MorphSettings.deletePlaylist(currentPlaylist)
                        librarySubView = "grid"
                        deleteConfirmationPopup.close()
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                    contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: "#ff4444"; radius: 6 }
                }
            }
        }
    }
}
