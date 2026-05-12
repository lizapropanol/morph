import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

ApplicationWindow {
    id: window
    visible: true
    width: 1075
    height: 700
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
    property bool saveLastImport: true
    property int likesVersion: 0
    property int playlistsVersion: 0
    property int settingsVersion: 0
    property int cacheVersion: 0
    property bool repeatOne: false
    property var fullPlaylistTracks: []
    property int loadedTracksCount: 0
    property bool isRestoringSession: false
    property bool isRecovering: false
    property var streamUrlCache: ({})
    property real lastKnownPosition: 0

    function preResolveNext() {
        var model = currentView === "search" ? (searchModel.count > 0 ? searchModel : historyModel) : libraryModel
        if (currentTrackIndex + 1 < model.count) {
            var nextTrack = model.get(currentTrackIndex + 1)
            var service = nextTrack.service || (nextTrack.coverUrl && nextTrack.coverUrl.indexOf("yandex") !== -1 ? "Yandex" : "SoundCloud")
            if (!streamUrlCache[nextTrack.id]) {
                MorphServices.resolve(service, nextTrack.id)
            }
        }
    }

    Component.onCompleted: {
        var yToken = MorphSettings.getYandexToken()
        var sToken = MorphSettings.getSoundCloudToken()
        if (yToken) MorphServices.setYandexToken(yToken)
        if (sToken) MorphServices.setSoundCloudClientId(sToken)
        
        MorphServices.getCharts()
        MorphServices.getDailyMixes()
        var hist = MorphSettings.getSearchHistory()
        for (var i = 0; i < hist.length; i++) historyModel.append(hist[i])
        
        var session = MorphSettings.loadSession()
        if (session.track) {
            isRestoringSession = true
            currentTrack = session.track
            MorphAudio.volume = session.volume || 50
            lastKnownPosition = session.position || 0
            MorphServices.resolve(currentTrack.service, currentTrack.id)
            MorphMpris.updateMetadata(currentTrack)
            repeatOne = session.repeatOne || false
            
            if (session.queue) {
                fullPlaylistTracks = session.queue
                currentPlaylist = session.playlist || ""
                currentTrackIndex = session.index !== undefined ? session.index : -1
                saveLastImport = session.saveLastImport !== undefined ? session.saveLastImport : true
                libraryModel.clear()
                for (var i = 0; i < fullPlaylistTracks.length; i++) {
                    libraryModel.append(fullPlaylistTracks[i])
                }
                loadedTracksCount = fullPlaylistTracks.length
            }
        }
    }

    onClosing: {
        MorphSettings.saveSession({
            "track": currentTrack,
            "volume": MorphAudio.volume,
            "position": MorphAudio.position,
            "repeatOne": repeatOne,
            "queue": fullPlaylistTracks,
            "playlist": currentPlaylist,
            "index": currentTrackIndex,
            "saveLastImport": saveLastImport
        })
    }

    function getServiceIcon(serviceName) {
        if (serviceName === "Yandex") return "assets/yandex_music_icon.svg"
        if (serviceName === "SoundCloud") return "assets/soundcloud_icon.svg"
        return ""
    }

    function openPlaylist(name) {
        currentPlaylist = name
        saveLastImport = true
        libraryModel.clear()
        fullPlaylistTracks = []
        loadedTracksCount = 0
        if (name === "LIKED") {
            currentPlaylist = ""
            var likes = MorphSettings.getLikedTracks()
            for(var i = likes.length - 1; i >= 0; i--) {
                var item = likes[i]
                if (!item.service) item.service = "Yandex"
                fullPlaylistTracks.push(item)
            }
        } else {
            var pls = MorphSettings.getPlaylists()
            var pData = pls[name]
            if (pData) {
                var ts = pData.tracks || (Array.isArray(pData) ? pData : [])
                for (var i = 0; i < ts.length; i++) {
                    var t = ts[i]
                    if (!t.service) {
                        if (t.coverUrl && t.coverUrl.indexOf("yandex") !== -1) t.service = "Yandex"
                        else if (t.coverUrl && t.coverUrl.indexOf("sndcdn") !== -1) t.service = "SoundCloud"
                        else t.service = "Yandex"
                    }
                    fullPlaylistTracks.push(t)
                }
            }
        }
        loadNextChunk()
        librarySubView = "tracks"
        libraryFlickable.contentY = 0
    }

    function loadNextChunk() {
        if (loadedTracksCount >= fullPlaylistTracks.length) return
        var limit = Math.min(loadedTracksCount + 100, fullPlaylistTracks.length)
        for (var i = loadedTracksCount; i < limit; i++) {
            libraryModel.append(fullPlaylistTracks[i])
        }
        loadedTracksCount = limit
    }

    function playTrack(track, index) {
        var service = track.service || (track.coverUrl && track.coverUrl.indexOf("yandex") !== -1 ? "Yandex" : "SoundCloud")
        if (currentTrack && currentTrack.id === track.id && currentTrack.service === service) {
            if (MorphAudio.isPlaying) MorphAudio.pause()
            else MorphAudio.resume()
            return
        }

        var cleanTrack = {
            id: track.id,
            title: track.title,
            artist: track.artist,
            album: track.album || "",
            coverUrl: track.coverUrl,
            service: service,
            webUrl: track.webUrl || ""
        }
        currentTrack = cleanTrack
        currentTrackIndex = index
        
        if (streamUrlCache[cleanTrack.id]) {
            MorphAudio.play(streamUrlCache[cleanTrack.id])
        } else if (MorphCache.isTrackCached(cleanTrack.id)) {
            var cachedUrl = MorphCache.getTrackUrl(cleanTrack.id)
            streamUrlCache[cleanTrack.id] = cachedUrl
            MorphAudio.play(cachedUrl)
        } else {
            MorphServices.resolve(cleanTrack.service, cleanTrack.id)
            MorphAudio.play("")
        }

        MorphServices.reportPlay(cleanTrack.service, cleanTrack.id, cleanTrack.album)
        MorphMpris.updateMetadata(cleanTrack)
        preResolveNext()
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
    ListModel { id: dailyMixesModel }

    property bool isStartup: true
    property real revealRadius: 0
    property real logoScale: 1.0

    Timer {
        interval: 2200
        running: true
        onTriggered: {
            isStartup = false
            revealAnimation.start()
        }
    }

    ParallelAnimation {
        id: revealAnimation
        NumberAnimation {
            target: window
            property: "revealRadius"
            from: 0
            to: Math.sqrt(Math.pow(window.width, 2) + Math.pow(window.height, 2)) * 1.2
            duration: 800
            easing.type: Easing.InExpo
        }
        NumberAnimation {
            target: window
            property: "logoScale"
            from: 1.0
            to: 15.0
            duration: 800
            easing.type: Easing.InExpo
        }
    }

    Item {
        id: splashScreen
        anchors.fill: parent
        z: 9999
        visible: isStartup || revealAnimation.running

        Rectangle { id: splashBlack; anchors.fill: parent; color: "black"; visible: false }
        
        Item {
            id: maskSource
            anchors.fill: parent
            visible: false
            Rectangle {
                width: revealRadius; height: width
                radius: width / 2
                color: "white"
                anchors.centerIn: parent
            }
        }

        OpacityMask {
            anchors.fill: parent
            source: splashBlack
            maskSource: maskSource
            invert: true
        }

        Image {
            id: splashLogo
            anchors.centerIn: parent
            source: "assets/logo.svg"
            width: 80; height: 80
            scale: logoScale
            smooth: true
        }
        
        MouseArea { anchors.fill: parent; enabled: isStartup }
    }

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
                                saveLastImport = true
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
                            Flickable {
                                id: searchFlickable
                                anchors.fill: parent; contentHeight: searchContent.height + 70; clip: true
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                ColumnLayout {
                                    id: searchContent
                                    width: searchFlickable.width - 70
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top; anchors.topMargin: 35
                                    spacing: 20
                                    
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
                                        Layout.fillWidth: true; Layout.preferredHeight: {
                                            if (searchModel.count > 0) return searchResultsList.contentHeight + 24
                                            return historyList.contentHeight + 64
                                        }
                                        currentIndex: searchModel.count > 0 ? 1 : 0
                                        
                                        ColumnLayout {
                                            spacing: 20
                                            Text { text: "RECENT SEARCHES"; color: "#444"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Black }
                                            Rectangle {
                                                Layout.fillWidth: true; Layout.preferredHeight: historyList.contentHeight + 20; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true
                                                ListView { 
                                                    id: historyList
                                                    anchors.fill: parent; anchors.margins: 10; interactive: false; clip: true
                                                    model: historyModel; delegate: trackDelegate
                                                }
                                            }
                                        }
                                        
                                        Rectangle {
                                            Layout.fillWidth: true; Layout.preferredHeight: searchResultsList.contentHeight + 20; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true
                                            ListView { 
                                                id: searchResultsList
                                                anchors.fill: parent; anchors.margins: 10; interactive: false; clip: true
                                                model: searchModel; delegate: trackDelegate
                                            }
                                        }
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
                                    
                                    function getGreeting() {
                                        var hour = new Date().getHours()
                                        if (hour >= 4 && hour < 12) return "GOOD MORNING"
                                        if (hour >= 12 && hour < 18) return "GOOD AFTERNOON"
                                        if (hour >= 18 && hour < 24) return "GOOD EVENING"
                                        return "GOOD NIGHT"
                                    }

                                    RowLayout {
                                        spacing: 12
                                        Rectangle {
                                            id: greetingDot
                                            width: 8; height: 8; radius: 4
                                            property color dotColor: (new Date().getHours() >= 4 && new Date().getHours() < 18) ? "#44ff44" : "#bb66ff"
                                            color: dotColor
                                            layer.enabled: true; layer.effect: DropShadow { transparentBorder: true; radius: 8; samples: 17; color: greetingDot.dotColor }
                                        }
                                        Text { 
                                            text: homeContent.getGreeting()
                                            color: "white"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Black
                                            opacity: 1.0
                                        }
                                    }

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
                                        visible: dailyMixesModel.count > 0
                                        Text { text: "DAILY MIXES"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Black }
                                        Rectangle {
                                            Layout.fillWidth: true; Layout.preferredHeight: 160; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true
                                            Item {
                                                anchors.fill: parent; anchors.margins: 10
                                                ListView {
                                                    id: dailyMixesListView; anchors.fill: parent; orientation: ListView.Horizontal; spacing: 15
                                                    model: dailyMixesModel; clip: true; boundsBehavior: Flickable.StopAtBounds
                                                    delegate: Item {
                                                        width: 140; height: 140
                                                        Rectangle {
                                                            anchors.fill: parent; radius: 15; color: "#1a1a1a"; clip: true
                                                            Image {
                                                                anchors.fill: parent; source: model.coverUrl || ""; fillMode: Image.PreserveAspectCrop; asynchronous: true
                                                                layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 140; height: 140; radius: 15 } }
                                                                onStatusChanged: if (status === Image.Ready && source.toString().startsWith("http")) MorphCache.cacheCover(source)
                                                            }
                                                            Rectangle {
                                                                anchors.fill: parent; color: "#aa000000"; visible: mixMouseArea.containsMouse; radius: 15
                                                                Image {
                                                                    anchors.centerIn: parent; source: "assets/play.svg"; Layout.preferredWidth: 40; Layout.preferredHeight: 40
                                                                    layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                                }
                                                            }
                                                            MouseArea {
                                                                id: mixMouseArea; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                                                                onClicked: if (model.permalink_url) {
                                                                    saveLastImport = false
                                                                    MorphServices.importPlaylist(model.permalink_url)
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                
                                                Rectangle {
                                                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                                    width: 32; height: 32; radius: 16; color: "#cc000000"
                                                    visible: dailyMixesListView.contentWidth > dailyMixesListView.width && dailyMixesListView.contentX > 10
                                                    Image {
                                                        anchors.centerIn: parent; source: "assets/chevron-left.svg"; width: 16; height: 16
                                                        layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                    }
                                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true; onClicked: dailyMixesListView.flick(2000, 0) }
                                                }

                                                Rectangle {
                                                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                                    width: 32; height: 32; radius: 16; color: "#cc000000"
                                                    visible: dailyMixesListView.contentWidth > dailyMixesListView.width && dailyMixesListView.contentX < dailyMixesListView.contentWidth - dailyMixesListView.width - 10
                                                    Image {
                                                        anchors.centerIn: parent; source: "assets/chevron-right.svg"; width: 16; height: 16
                                                        layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                    }
                                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true; onClicked: dailyMixesListView.flick(-2000, 0) }
                                                }
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 20
                                        Text { text: "CHARTS"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Black }
                                        Rectangle {
                                            Layout.fillWidth: true; Layout.preferredHeight: chartsListView.contentHeight + 20; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true
                                            ListView {
                                                id: chartsListView
                                                anchors.fill: parent; anchors.margins: 10; interactive: false; clip: true
                                                model: chartsModel.count > 0 ? Math.min(10, Math.ceil(chartsModel.count / 2)) : 0
                                                delegate: RowLayout {
                                                    width: chartsListView.width; height: 54; spacing: 20
                                                    property var leftTrack: chartsModel.get(index)
                                                    property var rightTrack: (index + 10 < chartsModel.count) ? chartsModel.get(index + 10) : null
                                                
                                                Rectangle {
                                                    Layout.fillWidth: true; Layout.preferredWidth: 1; height: 54; color: (currentTrack && leftTrack && currentTrack.id === leftTrack.id && currentTrack.service === "Yandex") ? "#252525" : (leftChartsMouseArea.containsMouse ? "#222" : "transparent"); radius: 6
                                                    RowLayout {
                                                        anchors.fill: parent; anchors.margins: 10; spacing: 15
                                                        Text { text: (index + 1).toString(); color: (currentTrack && leftTrack && currentTrack.id === leftTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "#888"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; Layout.preferredWidth: 25; horizontalAlignment: Text.AlignRight }
                                                        Image { 
                                                            source: leftTrack ? (leftTrack.coverUrl || "") : ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop; asynchronous: true
                                                            layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 36; height: 36; radius: 6 } }
                                                            onStatusChanged: if (status === Image.Ready && source.toString().startsWith("http")) MorphCache.cacheCover(source)
                                                        }
                                                        ColumnLayout {
                                                            Layout.fillWidth: true; spacing: 2; Layout.alignment: Qt.AlignVCenter
                                                            Text { Layout.fillWidth: true; text: leftTrack ? leftTrack.title : ""; color: (currentTrack && leftTrack && currentTrack.id === leftTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                                                            Text { Layout.fillWidth: true; text: leftTrack ? leftTrack.artist : ""; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; elide: Text.ElideRight }
                                                        }
                                                        Rectangle {
                                                            width: 6; height: 6; radius: 3; color: "#44ff44"; visible: (window.cacheVersion, leftTrack ? MorphCache.isTrackCached(leftTrack.id) : false)
                                                            Layout.alignment: Qt.AlignVCenter
                                                        }
                                                    }
                                                    MouseArea { 
                                                        id: leftChartsMouseArea; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton
                                                        onClicked: if (leftTrack) {
                                                            if (mouse.button === Qt.RightButton) { 
                                                                trackContextMenu.openAt(mouse.x, mouse.y, leftChartsMouseArea, leftTrack)
                                                            }
                                                            else {
                                                                libraryModel.clear()
                                                                for(var i=0; i<chartsModel.count; i++) libraryModel.append(chartsModel.get(i))
                                                                playTrack(libraryModel.get(index), index)
                                                            }
                                                        }
                                                    }
                                                }

                                                Rectangle {
                                                    Layout.fillWidth: true; Layout.preferredWidth: 1; height: 54; visible: rightTrack !== null; color: (currentTrack && rightTrack && currentTrack.id === rightTrack.id && currentTrack.service === "Yandex") ? "#252525" : (rightChartsMouseArea.containsMouse ? "#222" : "transparent"); radius: 6
                                                    RowLayout {
                                                        anchors.fill: parent; anchors.margins: 10; spacing: 15
                                                        Text { text: (index + 11).toString(); color: (currentTrack && rightTrack && currentTrack.id === rightTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "#888"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; Layout.preferredWidth: 25; horizontalAlignment: Text.AlignRight }
                                                        Image { 
                                                            source: rightTrack ? (rightTrack.coverUrl || "") : ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop; asynchronous: true
                                                            layer.enabled: true; layer.effect: OpacityMask { maskSource: Rectangle { width: 36; height: 36; radius: 6 } }
                                                            onStatusChanged: if (status === Image.Ready && source.toString().startsWith("http")) MorphCache.cacheCover(source)
                                                        }
                                                        ColumnLayout {
                                                            Layout.fillWidth: true; spacing: 2; Layout.alignment: Qt.AlignVCenter
                                                            Text { Layout.fillWidth: true; text: rightTrack ? rightTrack.title : ""; color: (currentTrack && rightTrack && currentTrack.id === rightTrack.id && currentTrack.service === "Yandex") ? "#44ff44" : "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                                                            Text { Layout.fillWidth: true; text: rightTrack ? rightTrack.artist : ""; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; elide: Text.ElideRight }
                                                        }
                                                        Rectangle {
                                                            id: rightCacheDot
                                                            width: 6; height: 6; radius: 3; color: "#44ff44"; visible: (window.cacheVersion, rightTrack ? MorphCache.isTrackCached(rightTrack.id) : false)
                                                            Layout.alignment: Qt.AlignVCenter
                                                        }
                                                    }
                                                    MouseArea { 
                                                        id: rightChartsMouseArea; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton
                                                        onClicked: if (rightTrack) {
                                                            if (mouse.button === Qt.RightButton) { 
                                                                trackContextMenu.openAt(mouse.x, mouse.y, rightChartsMouseArea, rightTrack)
                                                            }
                                                            else {
                                                                libraryModel.clear()
                                                                for(var i=0; i<chartsModel.count; i++) libraryModel.append(chartsModel.get(i))
                                                                playTrack(libraryModel.get(index + 10), index + 10)
                                                            }
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
                        }
                        
                        Item {
                            Flickable {
                                id: libraryFlickable
                                anchors.fill: parent; contentHeight: libraryContent.height + 70; clip: true
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                                onAtYEndChanged: if (atYEnd && librarySubView === "tracks") loadNextChunk()

                                ColumnLayout {
                                    id: libraryContent
                                    width: libraryFlickable.width - 70
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top; anchors.topMargin: 35
                                    spacing: 20
                                    
                                    StackLayout {
                                        Layout.fillWidth: true; Layout.preferredHeight: {
                                            if (librarySubView === "grid") return libraryGridView.contentHeight + 60
                                            return libraryTracksColumn.height
                                        }
                                        currentIndex: librarySubView === "grid" ? 0 : 1

                                        ColumnLayout {
                                            spacing: 20
                                            RowLayout {
                                                Layout.fillWidth: true
                                                Text { text: "LIBRARY"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Black }
                                                Item { Layout.fillWidth: true }
                                                RowLayout {
                                                    spacing: 10
                                                    Button {
                                                        text: "IMPORT"
                                                        onClicked: importPlaylistPopup.open()
                                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                        contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                        background: Rectangle { color: "#444"; radius: 6 }
                                                    }
                                                    Button {
                                                        text: "NEW PLAYLIST"
                                                        onClicked: createPlaylistPopup.open()
                                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                        contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                        background: Rectangle { color: "#333"; radius: 6 }
                                                    }
                                                }
                                            }                                            
                                            Rectangle {
                                                Layout.fillWidth: true; Layout.preferredHeight: libraryGridView.contentHeight + 20; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true
                                                GridView {
                                                    id: libraryGridView
                                                    anchors.fill: parent; anchors.margins: 10; interactive: false; clip: true
                                                    cellWidth: 160; cellHeight: 200
                                                    model: playlistsModel
                                                    header: Item {
                                                        width: libraryGridView.width; height: 200
                                                        Rectangle {
                                                            anchors.fill: parent; anchors.margins: 10; color: "#1a1a1a"; radius: 12; border.color: likedMouseArea.containsMouse ? "white" : "#333"; border.width: 1
                                                            Rectangle {
                                                                anchors.fill: parent; anchors.margins: 10; color: "#333"; radius: 8
                                                                Image {
                                                                    anchors.centerIn: parent
                                                                    source: "assets/heart.svg"; Layout.preferredWidth: 32; Layout.preferredHeight: 32; sourceSize: Qt.size(64, 64)
                                                                    layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                                }
                                                                Text { 
                                                                    text: "LIKED TRACKS"; color: "white"; font.family: "Rubik"; font.pixelSize: 13; font.weight: Font.Black
                                                                    anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.margins: 12
                                                                }
                                                            }
                                                            MouseArea { id: likedMouseArea; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true; onClicked: openPlaylist("LIKED") }
                                                        }
                                                    }
                                                    delegate: Item {
                                                        width: 160; height: 200
                                                        Rectangle {
                                                            anchors.fill: parent; anchors.margins: 10; color: "#1a1a1a"; radius: 12; border.color: playlistMouseArea.containsMouse ? "white" : "#333"; border.width: 1
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
                                                            MouseArea { id: playlistMouseArea; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true; onClicked: openPlaylist(model.name) }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        ColumnLayout {
                                            id: libraryTracksColumn
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
                                                    visible: currentPlaylist !== "" && saveLastImport
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
                                            Rectangle {
                                                Layout.fillWidth: true; Layout.preferredHeight: libraryTracksList.contentHeight + 20; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true
                                                ListView { 
                                                    id: libraryTracksList
                                                    anchors.fill: parent; anchors.margins: 10; interactive: false; clip: true
                                                    model: libraryModel; delegate: trackDelegate
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Item {
                            Flickable {
                                id: settingsFlickable
                                anchors.fill: parent; contentHeight: settingsContent.height + 70; clip: true
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                ColumnLayout {
                                    id: settingsContent
                                    width: settingsFlickable.width - 70
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top; anchors.topMargin: 35
                                    spacing: 25
                                    Text { text: "SETTINGS"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Bold }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 10
                                        Text { text: "Yandex Music Token"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                        TextField {
                                            id: yandexTokenField; text: MorphSettings.getYandexToken(); Layout.fillWidth: true
                                            color: "white"; font.family: "Rubik"; font.pixelSize: 13; padding: 12; echoMode: TextInput.Password
                                            background: Rectangle { color: "#151515"; radius: 6; border.color: "#333" }
                                            onEditingFinished: {
                                                MorphSettings.setYandexToken(text)
                                                MorphServices.setYandexToken(text)
                                            }
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 10
                                        Text { text: "SoundCloud Client ID"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                        TextField {
                                            id: soundcloudTokenField; text: MorphSettings.getSoundCloudToken(); Layout.fillWidth: true
                                            color: "white"; font.family: "Rubik"; font.pixelSize: 13; padding: 12; echoMode: TextInput.Password
                                            background: Rectangle { color: "#151515"; radius: 6; border.color: "#333" }
                                            onEditingFinished: {
                                                MorphSettings.setSoundCloudToken(text)
                                                MorphServices.setSoundCloudClientId(text)
                                            }
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 10
                                        Text { text: "Audio Quality"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                        RowLayout {
                                            spacing: 10
                                            Button {
                                                id: lowQualBtn; text: "192k"; Layout.preferredWidth: 100
                                                property bool active: (window.settingsVersion, MorphSettings.getAudioQuality() === "low")
                                                onClicked: {
                                                    MorphSettings.setAudioQuality("low")
                                                    MorphServices.setAudioQuality("low")
                                                    streamUrlCache = ({})
                                                    if (currentTrack && currentTrack.service === "Yandex") {
                                                        lastKnownPosition = MorphAudio.position
                                                        MorphServices.resolve(currentTrack.service, currentTrack.id)
                                                        currentTrackIndex = -1
                                                    }
                                                }
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: lowQualBtn.active ? "black" : "white"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter }
                                                background: Rectangle { color: lowQualBtn.active ? "white" : "#1a1a1a"; radius: 6; border.color: "#333" }
                                            }
                                            Button {
                                                id: medQualBtn; text: "320k"; Layout.preferredWidth: 100
                                                property bool active: (window.settingsVersion, MorphSettings.getAudioQuality() === "medium")
                                                onClicked: {
                                                    MorphSettings.setAudioQuality("medium")
                                                    MorphServices.setAudioQuality("medium")
                                                    streamUrlCache = ({})
                                                    if (currentTrack && currentTrack.service === "Yandex") {
                                                        lastKnownPosition = MorphAudio.position
                                                        MorphServices.resolve(currentTrack.service, currentTrack.id)
                                                        currentTrackIndex = -1
                                                    }
                                                }
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: medQualBtn.active ? "black" : "white"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter }
                                                background: Rectangle { color: medQualBtn.active ? "white" : "#1a1a1a"; radius: 6; border.color: "#333" }
                                            }
                                            Button {
                                                id: highQualBtn; text: "LOSSLESS"; Layout.preferredWidth: 100
                                                property bool active: (window.settingsVersion, MorphSettings.getAudioQuality() === "high")
                                                onClicked: {
                                                    MorphSettings.setAudioQuality("high")
                                                    MorphServices.setAudioQuality("high")
                                                    streamUrlCache = ({})
                                                    if (currentTrack && currentTrack.service === "Yandex") {
                                                        lastKnownPosition = MorphAudio.position
                                                        MorphServices.resolve(currentTrack.service, currentTrack.id)
                                                        currentTrackIndex = -1
                                                    }
                                                }
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: highQualBtn.active ? "black" : "white"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter }
                                                background: Rectangle { color: highQualBtn.active ? "white" : "#1a1a1a"; radius: 6; border.color: "#333" }
                                            }
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 10
                                        Text { text: "Cache Management"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                        RowLayout {
                                            spacing: 10
                                            Button {
                                                text: "CLEAR TRACKS"; Layout.preferredWidth: 120
                                                onClicked: MorphCache.clearTrackCache()
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter }
                                                background: Rectangle { color: "#1a1a1a"; radius: 6; border.color: "#333" }
                                            }
                                            Button {
                                                text: "CLEAR COVERS"; Layout.preferredWidth: 120
                                                onClicked: MorphCache.clearCoverCache()
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter }
                                                background: Rectangle { color: "#1a1a1a"; radius: 6; border.color: "#333" }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    color: "black"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 5
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            Slider {
                                id: progressSlider; anchors.fill: parent
                                from: 0; to: MorphAudio.duration > 0 ? MorphAudio.duration : 1
                                value: MorphAudio.position; onMoved: MorphAudio.position = value; padding: 0
                                
                                MouseArea { 
                                    id: progressHoverArea
                                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton 
                                }

                                Text {
                                    visible: progressHoverArea.containsMouse
                                    text: formatTime(MorphAudio.position)
                                    color: "white"; font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Bold
                                    y: -12; x: (progressSlider.visualPosition * parent.width) - (width / 2)
                                }

                                Text {
                                    visible: progressHoverArea.containsMouse
                                    text: formatTime(MorphAudio.duration)
                                    color: "#666"; font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Bold
                                    anchors.right: parent.right; y: -12
                                }

                                background: Rectangle { anchors.bottom: parent.bottom; anchors.bottomMargin: 1; width: parent.width; height: 4; radius: 16; color: "#1a1a1a"; Rectangle { width: progressSlider.visualPosition * parent.width; height: parent.height; color: "white"; radius: 16 } }
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
            color: (currentTrack && currentTrack.id === model.id && currentTrack.service === model.service) ? "#252525" : (trackMouseArea.containsMouse ? "#222" : "transparent")
            radius: 6

            MouseArea {
                id: trackMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onPressed: {
                    if (mouse.button === Qt.RightButton) {
                        var track;
                        if (currentView === "search") {
                            track = (searchModel.count > 0) ? searchModel.get(index) : historyModel.get(index)
                        } else {
                            track = libraryModel.get(index)
                        }
                        trackContextMenu.openAt(mouse.x, mouse.y, trackMouseArea, track)
                    }
                }
                onClicked: {
                    if (mouse.button === Qt.LeftButton) {
                        var track; var m;
                        if (currentView === "search") {
                            if (searchModel.count > 0) { track = searchModel.get(index); m = searchModel }
                            else { track = historyModel.get(index); m = historyModel }

                            var tObj = { "id": track.id, "title": track.title, "artist": track.artist, "coverUrl": track.coverUrl, "service": track.service, "webUrl": track.webUrl || "" }
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
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 15
                Image { 
                    source: model.coverUrl || ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop 
                    layer.enabled: true
                    layer.effect: OpacityMask { maskSource: Rectangle { width: 36; height: 36; radius: 6 } }
                    onStatusChanged: if (status === Image.Ready && source.toString().startsWith("http")) MorphCache.cacheCover(source)
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 2; Layout.alignment: Qt.AlignVCenter
                    Text { Layout.fillWidth: true; text: model.title || ""; color: (currentTrack && currentTrack.id === model.id && currentTrack.service === model.service) ? "#44ff44" : "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 6
                        Image { source: getServiceIcon(model.service); Layout.preferredWidth: 12; Layout.preferredHeight: 12 }
                        Text { Layout.fillWidth: true; text: model.artist || ""; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; elide: Text.ElideRight }
                    }
                }
                Rectangle {
                    width: 6; height: 6; radius: 3; color: "#44ff44"; visible: (window.cacheVersion, MorphCache.isTrackCached(model.id))
                    Layout.alignment: Qt.AlignVCenter
                }
                Image {
                    source: (window.likesVersion, MorphSettings.isLiked(model.id)) ? "assets/heart.svg" : "assets/heart-outline.svg"; Layout.preferredWidth: 18; Layout.preferredHeight: 18; Layout.alignment: Qt.AlignVCenter; layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                    MouseArea { 
                        anchors.fill: parent; onClicked: MorphSettings.toggleLike({ "id": model.id, "title": model.title, "artist": model.artist, "coverUrl": model.coverUrl, "service": model.service, "album": model.album || "", "webUrl": model.webUrl || "" }); cursorShape: Qt.PointingHandCursor 
                    }
                }
            }
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

    property var targetContextTrack: null
    Popup {
        id: trackContextMenu
        parent: Overlay.overlay
        width: 140; height: 40
        padding: 0
        background: Rectangle { color: "#1a1a1a"; radius: 6; border.color: "#333"; border.width: 1 }
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
        
        contentItem: Rectangle {
            anchors.fill: parent; color: "transparent"; radius: 6
            Rectangle {
                anchors.fill: parent; anchors.margins: 2; color: copyItemMouse.containsMouse ? "#333" : "transparent"; radius: 4
                Text { anchors.centerIn: parent; text: "COPY LINK"; color: "white"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Bold }
                MouseArea {
                    id: copyItemMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (targetContextTrack && targetContextTrack.webUrl && targetContextTrack.webUrl !== "") {
                            MorphSettings.copyToClipboard(targetContextTrack.webUrl)
                        } else if (targetContextTrack && targetContextTrack.service === "Yandex") {
                            MorphSettings.copyToClipboard("https://music.yandex.ru/track/" + targetContextTrack.id)
                        } else if (targetContextTrack && targetContextTrack.service === "SoundCloud") {
                            MorphSettings.copyToClipboard("https://soundcloud.com/tracks/" + targetContextTrack.id)
                        }
                        trackContextMenu.close()
                    }
                }
            }
        }
        
        function openAt(mx, my, targetItem, track) {
            targetContextTrack = track
            var coords = targetItem.mapToItem(Overlay.overlay, mx, my)
            x = coords.x; y = coords.y
            open()
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
        function onDailyMixesReady(serviceName, results) {
            if (results.length > 0) dailyMixesModel.clear()
            for (var i = 0; i < results.length; i++) {
                dailyMixesModel.append(results[i])
            }
        }
    function onStreamUrlReady(trackId, streamUrl) {
        streamUrlCache[trackId] = streamUrl
        if (currentTrack && currentTrack.id === trackId) {
            if (isRestoringSession || isRecovering) {
                if (isRestoringSession) MorphAudio.load(streamUrl)
                else MorphAudio.play(streamUrl)
                
                MorphAudio.position = lastKnownPosition
                isRestoringSession = false
                isRecovering = false
            } else {
                MorphAudio.play(streamUrl)
            }
            
            if (currentTrackIndex === -1) { 
                currentTrackIndex = loadedTracksCount
            }
        }
    }
        function onPlaylistImported(name, coverUrl, tracks) {
            if (saveLastImport) {
                MorphSettings.createPlaylistWithTracks(name, coverUrl, tracks)
            }
            
            currentView = "library"
            librarySubView = "list"
            currentPlaylist = name
            
            libraryModel.clear()
            for (var i = 0; i < tracks.length; i++) {
                libraryModel.append(tracks[i])
            }
            
            if (libraryModel.count > 0) {
                playTrack(libraryModel.get(0), 0)
            }

            importPlaylistPopup.isBusy = false
            importPlaylistPopup.close()
        }
        function onErrorOccurred(message) {
            importPlaylistPopup.isBusy = false
            importPlaylistPopup.errorMsg = message
        }
    }
    
    Connections {
        target: MorphSettings
        function onSettingsChanged() { window.settingsVersion++ }
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
        target: MorphCache
        function onTrackCached(trackId, localPath) { 
            streamUrlCache[trackId] = localPath
            window.cacheVersion++ 
        }
        function onCoverCached() { window.cacheVersion++ }
    }

    Connections {
        target: MorphAudio
        function onFinished() { if (repeatOne) { MorphAudio.position = 0; MorphAudio.resume() } else playNext() }
        function onError(errorString) {
            if (currentTrack && currentTrack.service === "SoundCloud") {
                if (errorString.indexOf("Forbidden") !== -1) {
                    isRecovering = true
                    lastKnownPosition = MorphAudio.position
                    MorphServices.resolve(currentTrack.service, currentTrack.id)
                }
            }
        }
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

    Popup {
        id: importPlaylistPopup
        parent: Overlay.overlay
        x: (parent.width - width) / 2; y: (parent.height - height) / 2
        width: 450; height: 280; modal: true; focus: true
        background: Rectangle { color: "#1a1a1a"; radius: 12; border.color: "#333" }
        
        property bool isBusy: false
        property string errorMsg: ""

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 15
            Text { text: "IMPORT PLAYLIST"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Bold }
            
            TextField {
                id: importUrlField; placeholderText: "YANDEX OR SOUNDCLOUD URL"; Layout.fillWidth: true
                color: "white"; font.family: "Rubik"; font.pixelSize: 12; padding: 10; enabled: !importPlaylistPopup.isBusy
                background: Rectangle { color: "#111"; radius: 6; border.color: "#333" }
            }

            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true
                visible: importPlaylistPopup.errorMsg !== ""
                clip: true
                TextArea {
                    text: importPlaylistPopup.errorMsg; color: "#ff4444"; font.family: "Rubik"; font.pixelSize: 10
                    readOnly: true; selectByMouse: true; wrapMode: Text.Wrap
                    background: Rectangle { color: "#100000"; radius: 4; border.color: "#300" }
                }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 15
                BusyIndicator { 
                    running: importPlaylistPopup.isBusy; visible: running
                    Layout.preferredWidth: 30; Layout.preferredHeight: 30
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "IMPORT"; Layout.preferredWidth: 100; Layout.preferredHeight: 40
                    visible: !importPlaylistPopup.isBusy
                    onClicked: {
                        if (importUrlField.text !== "") {
                            importPlaylistPopup.isBusy = true
                            importPlaylistPopup.errorMsg = ""
                            saveLastImport = true
                            MorphServices.importPlaylist(importUrlField.text)
                        }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                    contentItem: Text { text: parent.text; color: "black"; font.family: "Rubik"; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: "white"; radius: 6 }
                }
                Button {
                    text: "CANCEL"; Layout.preferredWidth: 100; Layout.preferredHeight: 40
                    visible: !importPlaylistPopup.isBusy
                    onClicked: importPlaylistPopup.close()
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                    contentItem: Text { text: parent.text; color: "white"; font.family: "Rubik"; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: "#333"; radius: 6 }
                }
            }
        }
        onClosed: { importPlaylistPopup.isBusy = false; importPlaylistPopup.errorMsg = ""; importUrlField.text = "" }
    }
}
