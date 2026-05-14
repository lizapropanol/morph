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
    property string settingsSubView: "main"
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
    property bool isSearching: false
    property var streamUrlCache: ({})
    property real lastKnownPosition: 0

    function formatSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB"
        return (bytes / (1024 * 1024)).toFixed(1) + " MB"
    }

    function loadDetailedTracks() {
        if (detailedTracksModel.count > 0) return
        var tracks = MorphCache.getTrackCacheItems()
        for (var i = 0; i < tracks.length; i++) {
            var item = tracks[i]; item.selected = false
            detailedTracksModel.append(item)
        }
    }

    function loadDetailedCovers() {
        if (detailedCoversModel.count > 0) return
        var covers = MorphCache.getCoverCacheItems()
        for (var i = 0; i < covers.length; i++) {
            var item = covers[i]; item.selected = false
            detailedCoversModel.append(item)
        }
    }

    function refreshDetailedCache() {
        detailedTracksModel.clear()
        detailedCoversModel.clear()
        if (tracksExpanded) loadDetailedTracks()
        if (coversExpanded) loadDetailedCovers()
    }

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
            MorphDiscord.updateMetadata(currentTrack)
            repeatOne = session.repeatOne || false
            
            if (session.queue) {
                fullPlaylistTracks = session.queue
                currentPlaylist = session.playlist || ""
                currentTrackIndex = session.index !== undefined ? session.index : -1
                saveLastImport = session.saveLastImport !== undefined ? session.saveLastImport : true
                libraryModel.clear()
                for (var i = 0; i < fullPlaylistTracks.length; i++) {
                    var item = fullPlaylistTracks[i]
                    if (item.durationMs === undefined) item.durationMs = 0
                    libraryModel.append(item)
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
                if (item.durationMs === undefined) item.durationMs = 0
                fullPlaylistTracks.push(item)
            }
        }
 else {
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
                    if (t.durationMs === undefined) t.durationMs = 0
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
            webUrl: track.webUrl || "",
            durationMs: track.durationMs || 0
        }
        currentTrack = cleanTrack
        currentTrackIndex = index
        
        var streamUrl = streamUrlCache[cleanTrack.id]
        if (streamUrl) {
            if (streamUrl.startsWith("file://") && !MorphCache.isTrackCached(cleanTrack.id)) {
                delete streamUrlCache[cleanTrack.id]
                MorphServices.resolve(cleanTrack.service, cleanTrack.id)
                MorphAudio.play("")
            } else {
                MorphAudio.play(streamUrl)
            }
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
        MorphDiscord.updateMetadata(cleanTrack)
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

    function forceRole(model) {
        if (model.count === 0) {
            model.append({ "id": "", "title": "", "artist": "", "coverUrl": "", "service": "", "webUrl": "", "durationMs": 0 })
            model.clear()
        }
    }

    ListModel { id: searchModel; Component.onCompleted: forceRole(this) }
    ListModel { id: libraryModel; Component.onCompleted: forceRole(this) }
    ListModel { id: playlistsModel }
    ListModel { id: historyModel; Component.onCompleted: forceRole(this) }
    ListModel { id: chartsModel; Component.onCompleted: forceRole(this) }
    ListModel { id: dailyMixesModel }
    ListModel { id: detailedTracksModel }
    ListModel { id: detailedCoversModel }

    property bool isStartup: true
    property bool tracksExpanded: false
    property bool coversExpanded: false
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

        MouseArea {
            anchors.fill: parent
            z: -1
            onPressed: {
                searchField.focus = false
                window.contentItem.forceActiveFocus()
            }
        }

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
                                if (currentView === "settings") {
                                    settingsSubView = "main"
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
                            Timer {
                                id: searchTimer; interval: 600; repeat: false
                                onTriggered: { if (searchField.text.trim().length > 0) { searchModel.clear(); isSearching = true; MorphServices.search(searchField.text, searchSource) } }
                            }
                            Flickable {
                                id: searchFlickable
                                anchors.fill: parent; contentHeight: searchContent.height + 100; clip: true
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                MouseArea {
                                    anchors.fill: parent
                                    onPressed: {
                                        searchField.focus = false
                                        window.contentItem.forceActiveFocus()
                                        mouse.accepted = false
                                    }
                                }

                                ColumnLayout {
                                    id: searchContent
                                    width: searchFlickable.width - 70; anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top; anchors.topMargin: 35; spacing: 30
                                    
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 15
                                        Rectangle {
                                            Layout.fillWidth: true; Layout.preferredHeight: 52
                                            color: "#111"; radius: 14; border.color: searchField.activeFocus ? "#44ff44" : "#222"; border.width: 1
                                            RowLayout {
                                                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 12
                                                Image {
                                                    source: "assets/magnify.svg"; Layout.preferredWidth: 20; Layout.preferredHeight: 20
                                                    layer.enabled: true; layer.effect: ColorOverlay { color: searchField.activeFocus ? "#44ff44" : "#666" }
                                                }
                                                TextField {
                                                    id: searchField; placeholderText: "What do you want to listen to?"; Layout.fillWidth: true; color: "white"
                                                    font.family: "Rubik"; font.pixelSize: 16; background: null; verticalAlignment: TextInput.AlignVCenter
                                                    onTextChanged: { if (text.trim() === "") { searchModel.clear(); isSearching = false; searchTimer.stop() } else searchTimer.restart() }
                                                    onAccepted: { searchTimer.stop(); searchModel.clear(); isSearching = true; MorphServices.search(text, searchSource) }
                                                }
                                                Button {
                                                    id: clearSearchBtn; visible: searchField.text !== ""; Layout.preferredWidth: 28; Layout.preferredHeight: 28; background: null
                                                    contentItem: Image {
                                                        source: "assets/close.svg"; sourceSize.width: 18; sourceSize.height: 18; fillMode: Image.PreserveAspectFit
                                                        opacity: clearSearchBtn.hovered ? 1.0 : 0.5; layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                    }
                                                    onClicked: { searchField.text = ""; searchModel.clear(); isSearching = false }
                                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                }
                                            }
                                        }
                                        RowLayout {
                                            Layout.alignment: Qt.AlignLeft; spacing: 8
                                            Repeater {
                                                model: ["all", "yandex", "soundcloud"]
                                                Rectangle {
                                                    Layout.preferredHeight: 32; Layout.preferredWidth: filterText.width + 24
                                                    color: searchSource === modelData ? "#44ff44" : "#111"; radius: 16; border.color: searchSource === modelData ? "#44ff44" : "#222"; border.width: 1
                                                    Text {
                                                        id: filterText; anchors.centerIn: parent; text: modelData === "all" ? "All" : (modelData === "yandex" ? "Yandex Music" : "SoundCloud")
                                                        color: searchSource === modelData ? "black" : "#aaa"; font.family: "Rubik"; font.pixelSize: 12; font.weight: Font.Medium
                                                    }
                                                    MouseArea {
                                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                        onClicked: { searchSource = modelData; if (searchField.text.trim() !== "") { searchModel.clear(); isSearching = true; MorphServices.search(searchField.text, searchSource) } }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        visible: !isSearching && searchField.text.trim() === ""
                                        spacing: 20
                                        RowLayout {
                                            id: historyLabelRow; Layout.fillWidth: true
                                            Text { text: "Recent searches"; color: "white"; font.family: "Rubik"; font.pixelSize: 18; font.weight: Font.Bold; Layout.fillWidth: true }
                                            Text { 
                                                text: "Clear all"; color: "#888"; font.family: "Rubik"; font.pixelSize: 12; visible: historyModel.count > 0
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { MorphSettings.clearSearchHistory(); historyModel.clear() } }
                                            }
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true; Layout.preferredHeight: historyList.contentHeight + 20; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true; visible: historyModel.count > 0
                                            ListView { id: historyList; anchors.fill: parent; anchors.margins: 10; interactive: false; clip: true; model: historyModel; delegate: trackDelegate }
                                        }
                                        Text { text: "Your recent searches will appear here"; color: "#444"; font.family: "Rubik"; font.pixelSize: 14; visible: historyModel.count === 0; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 40 }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        visible: searchModel.count > 0
                                        spacing: 20
                                        Rectangle {
                                            Layout.fillWidth: true; Layout.preferredHeight: searchResultsList.contentHeight + 20; color: "#1a1a1a"; radius: 12; border.color: "#333"; border.width: 1; clip: true
                                            ListView { id: searchResultsList; anchors.fill: parent; anchors.margins: 10; interactive: false; clip: true; model: searchModel; delegate: trackDelegate }
                                        }
                                    }

                                }
                            }

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 20
                                visible: isSearching && searchModel.count === 0
                                BusyIndicator { 
                                    Layout.alignment: Qt.AlignHCenter; running: parent.visible 
                                }
                                Text { 
                                    text: "Searching for tracks..."; color: "#666"; font.family: "Rubik"; font.pixelSize: 14; Layout.alignment: Qt.AlignHCenter 
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
                                                                anchors.fill: parent; source: MorphCache.getCachedCover(model.coverUrl || ""); fillMode: Image.PreserveAspectCrop; asynchronous: true
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
                                                            source: leftTrack ? MorphCache.getCachedCover(leftTrack.coverUrl || "") : ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop; asynchronous: true
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
                                                            source: rightTrack ? MorphCache.getCachedCover(rightTrack.coverUrl || "") : ""; Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop; asynchronous: true
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
                                                                        anchors.fill: parent; source: MorphCache.getCachedCover(model.coverUrl || ""); fillMode: Image.PreserveAspectCrop
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
                                                        var url = (pls[currentPlaylist] && pls[currentPlaylist].coverUrl) ? pls[currentPlaylist].coverUrl : ""
                                                        return MorphCache.getCachedCover(url)
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
                                StackLayout {
                                    id: settingsStack
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: settingsSubView === "main" ? settingsMainContent.height : cacheContent.height
                                    currentIndex: settingsSubView === "main" ? 0 : 1

                                    ColumnLayout {
                                        id: settingsMainContent
                                        spacing: 25
                                        Text { text: "SETTINGS"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Bold }
                                        ColumnLayout {
                                            Layout.fillWidth: true; spacing: 10
                                            RowLayout {
                                                spacing: 8
                                                Image {
                                                    source: "assets/yandex_music_icon.svg"; Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                                    sourceSize.width: 32; sourceSize.height: 32
                                                }
                                                Text { text: "Yandex Music Token"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                            }
                                            TextField {
                                                id: yandexTokenField; text: MorphSettings.getYandexToken(); Layout.fillWidth: true
                                                color: "white"; font.family: "Rubik"; font.pixelSize: 13; padding: 12; echoMode: TextInput.Password
                                                background: Rectangle { color: "#1a1a1a"; radius: 6; border.color: "#333" }
                                                onEditingFinished: {
                                                    MorphSettings.setYandexToken(text)
                                                    MorphServices.setYandexToken(text)
                                                }
                                            }
                                        }
                                        ColumnLayout {
                                            Layout.fillWidth: true; spacing: 10
                                            RowLayout {
                                                spacing: 8
                                                Image {
                                                    source: "assets/soundcloud_icon.svg"; Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                                    sourceSize.width: 32; sourceSize.height: 32
                                                }
                                                Text { text: "SoundCloud Client ID"; color: "#888"; font.family: "Rubik"; font.pixelSize: 11 }
                                            }
                                            TextField {
                                                id: soundcloudTokenField; text: MorphSettings.getSoundCloudToken(); Layout.fillWidth: true
                                                color: "white"; font.family: "Rubik"; font.pixelSize: 13; padding: 12; echoMode: TextInput.Password
                                                background: Rectangle { color: "#1a1a1a"; radius: 6; border.color: "#333" }
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
                                        Button {
                                            Layout.fillWidth: true; Layout.preferredHeight: 50
                                            onClicked: {
                                                detailedTracksModel.clear()
                                                detailedCoversModel.clear()
                                                settingsSubView = "cache"
                                                tracksExpanded = false
                                                coversExpanded = false
                                            }
                                            background: Rectangle { color: "#1a1a1a"; radius: 10; border.color: "#333" }
                                            contentItem: RowLayout {
                                                anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                                anchors.leftMargin: 15; anchors.rightMargin: 15; spacing: 10
                                                Image {
                                                    source: "assets/harddisk.svg"; Layout.preferredWidth: 20; Layout.preferredHeight: 20; Layout.alignment: Qt.AlignVCenter
                                                    sourceSize.width: 40; sourceSize.height: 40
                                                    layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                }
                                                Text { text: "Manage Storage"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Medium; Layout.fillWidth: true; verticalAlignment: Text.AlignVCenter }
                                                Text { 
                                                    text: formatSize((window.cacheVersion, MorphCache.getTrackCacheSize() + MorphCache.getCoverCacheSize()))
                                                    color: "#888"; font.family: "Rubik"; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter
                                                }
                                                Image {
                                                    source: "assets/chevron-right.svg"; Layout.preferredWidth: 16; Layout.preferredHeight: 16; Layout.alignment: Qt.AlignVCenter
                                                    layer.enabled: true; layer.effect: ColorOverlay { color: "#444" }
                                                }
                                            }
                                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                        }

                                        Button {
                                            Layout.fillWidth: true; Layout.preferredHeight: 50
                                            onClicked: MorphSettings.setDiscordRpcEnabled(!MorphSettings.getDiscordRpcEnabled())
                                            background: Rectangle { color: "#1a1a1a"; radius: 10; border.color: "#333" }
                                            contentItem: RowLayout {
                                                anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                                anchors.leftMargin: 15; anchors.rightMargin: 15; spacing: 10
                                                Image {
                                                    source: "assets/discord.svg"; Layout.preferredWidth: 20; Layout.preferredHeight: 20; Layout.alignment: Qt.AlignVCenter
                                                    sourceSize.width: 40; sourceSize.height: 40
                                                    layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                                                }
                                                Text { text: "Discord RPC"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Medium; Layout.fillWidth: true; verticalAlignment: Text.AlignVCenter }
                                                Switch {
                                                    id: discordRpcSwitch; Layout.alignment: Qt.AlignVCenter; Layout.preferredHeight: 20; padding: 0
                                                    checked: (window.settingsVersion, MorphSettings.getDiscordRpcEnabled())
                                                    onToggled: MorphSettings.setDiscordRpcEnabled(checked)
                                                    indicator: Rectangle {
                                                        implicitWidth: 36; implicitHeight: 20; radius: 10
                                                        color: discordRpcSwitch.checked ? "#5865f2" : "#222"
                                                        Behavior on color { ColorAnimation { duration: 150 } }
                                                        Rectangle {
                                                            x: discordRpcSwitch.checked ? parent.width - width - 2 : 2; y: 2
                                                            width: 16; height: 16; radius: 8; color: "white"
                                                            Behavior on x { NumberAnimation { duration: 150 } }
                                                        }
                                                    }
                                                }
                                            }
                                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                        }
                                    }

                                    ColumnLayout {
                                        id: cacheContent
                                        spacing: 25
                                        property bool clearTracks: true
                                        property bool clearCovers: true
                                        property bool showSuccess: false
                                        
                                        Timer { id: successTimer; interval: 2000; onTriggered: cacheContent.showSuccess = false }
                                        Timer { id: clearTracksTimer; interval: 350; onTriggered: detailedTracksModel.clear() }
                                        Timer { id: clearCoversTimer; interval: 350; onTriggered: detailedCoversModel.clear() }

                                        RowLayout {
                                            Layout.fillWidth: true; spacing: 15
                                            Button {
                                                text: "← BACK"
                                                onClicked: settingsSubView = "main"
                                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                contentItem: Text { text: parent.text; color: "#888"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Bold }
                                                background: Item {}
                                            }
                                            Text { text: "STORAGE USAGE"; color: "white"; font.family: "Rubik"; font.pixelSize: 16; font.weight: Font.Bold; Layout.alignment: Qt.AlignVCenter }
                                            Item { Layout.fillWidth: true }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true; Layout.preferredHeight: 120; color: "#1a1a1a"; radius: 15; border.color: "#333"
                                            ColumnLayout {
                                                anchors.fill: parent; anchors.margins: 20; spacing: 15
                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    Text { text: "Total Cache"; color: "#888"; font.family: "Rubik"; font.pixelSize: 13; Layout.fillWidth: true }
                                                    Text { 
                                                        text: formatSize((window.cacheVersion, MorphCache.getTrackCacheSize() + MorphCache.getCoverCacheSize()))
                                                        color: "white"; font.family: "Rubik"; font.pixelSize: 15; font.weight: Font.Bold 
                                                    }
                                                }
                                                Item {
                                                    Layout.fillWidth: true; Layout.preferredHeight: 8
                                                    Rectangle { anchors.fill: parent; color: "#222"; radius: 4 }
                                                    Rectangle { 
                                                        id: trackBar
                                                        height: parent.height; color: "#44ff44"; radius: 4
                                                        width: parent.width * (window.cacheVersion, (MorphCache.getTrackCacheSize() / Math.max(1, MorphCache.getTrackCacheSize() + MorphCache.getCoverCacheSize())))
                                                        Rectangle {
                                                            anchors.right: parent.right; width: 4; height: parent.height; color: "#44ff44"
                                                            visible: coverBar.visible && parent.width > 4
                                                        }
                                                    }
                                                    Rectangle { 
                                                        id: coverBar
                                                        height: parent.height; color: "#bb66ff"; radius: 4 
                                                        anchors.left: trackBar.right
                                                        anchors.right: parent.right
                                                        visible: (window.cacheVersion, MorphCache.getCoverCacheSize() > 0)
                                                        Rectangle {
                                                            anchors.left: parent.left; width: 4; height: parent.height; color: "#bb66ff"
                                                            visible: trackBar.width > 4
                                                        }
                                                    }
                                                }
                                                RowLayout {
                                                    spacing: 15
                                                    RowLayout {
                                                        Rectangle { width: 8; height: 8; radius: 4; color: "#44ff44" }
                                                        Text { text: "Tracks (" + Math.round((window.cacheVersion, MorphCache.getTrackCacheSize() / Math.max(1, MorphCache.getTrackCacheSize() + MorphCache.getCoverCacheSize())) * 100) + "%)"; color: "#666"; font.family: "Rubik"; font.pixelSize: 11 }
                                                    }
                                                    RowLayout {
                                                        Rectangle { width: 8; height: 8; radius: 4; color: "#bb66ff" }
                                                        Text { text: "Covers (" + Math.round((window.cacheVersion, MorphCache.getCoverCacheSize() / Math.max(1, MorphCache.getTrackCacheSize() + MorphCache.getCoverCacheSize())) * 100) + "%)"; color: "#666"; font.family: "Rubik"; font.pixelSize: 11 }
                                                    }
                                                }
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true; spacing: 5
                                            Text { text: "SELECT DATA TO CLEAR"; color: "#444"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Black; Layout.bottomMargin: 5 }
                                            
                                            Rectangle {
                                                Layout.fillWidth: true; Layout.preferredHeight: contentColumn.implicitHeight; color: "#1a1a1a"; radius: 10; border.color: "#333"
                                                ColumnLayout {
                                                    id: contentColumn
                                                    anchors.left: parent.left; anchors.right: parent.right; spacing: 0
                                                    
                                                    Item {
                                                        Layout.fillWidth: true; Layout.preferredHeight: 50
                                                        RowLayout {
                                                            anchors.fill: parent; anchors.margins: 15; spacing: 15
                                                            Rectangle { 
                                                                width: 20; height: 20; radius: 4; color: cacheContent.clearTracks ? "#44ff44" : "#333"; Layout.alignment: Qt.AlignVCenter
                                                                Image { anchors.centerIn: parent; source: "assets/check.svg"; width: 12; height: 12; visible: cacheContent.clearTracks; layer.enabled: true; layer.effect: ColorOverlay { color: "black" } }
                                                            }
                                                            Text { text: "Track Cache"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter }
                                                            Text { 
                                                                text: (window.cacheVersion, MorphCache.getTrackCacheCount()) + " tracks, " + formatSize((window.cacheVersion, MorphCache.getTrackCacheSize()))
                                                                color: "#888"; font.family: "Rubik"; font.pixelSize: 13; Layout.alignment: Qt.AlignVCenter
                                                            }
                                                            Image {
                                                                source: "assets/chevron-down.svg"; Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                                                rotation: tracksExpanded ? 180 : 0
                                                                layer.enabled: true; layer.effect: ColorOverlay { color: "#444" }
                                                                Behavior on rotation { NumberAnimation { duration: 200 } }
                                                            }
                                                        }
                                                        MouseArea { 
                                                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                            onClicked: {
                                                                if (mouse.x > 60) {
                                                                    if (tracksExpanded) {
                                                                        tracksExpanded = false
                                                                        clearTracksTimer.start()
                                                                    } else {
                                                                        clearTracksTimer.stop()
                                                                        loadDetailedTracks()
                                                                        tracksExpanded = true
                                                                    }
                                                                } else {
                                                                    cacheContent.clearTracks = !cacheContent.clearTracks
                                                                    if (!cacheContent.clearTracks) {
                                                                        for (var i = 0; i < detailedTracksModel.count; i++) {
                                                                            detailedTracksModel.setProperty(i, "selected", false)
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Item {
                                                        Layout.fillWidth: true
                                                        clip: true
                                                        Layout.preferredHeight: tracksExpanded ? Math.min(detailedTracksModel.count * 40, 400) : 0
                                                        Behavior on Layout.preferredHeight { NumberAnimation { duration: 300; easing.type: Easing.InOutQuad } }
                                                        
                                                        ListView {
                                                            anchors.fill: parent
                                                            model: detailedTracksModel
                                                            clip: true
                                                            interactive: contentHeight > height
                                                            ScrollBar.vertical: ScrollBar { visible: parent.interactive }
                                                            delegate: Item {
                                                                    width: ListView.view.width; height: 40
                                                                    RowLayout {
                                                                        anchors.fill: parent; anchors.leftMargin: 15; anchors.rightMargin: 15; spacing: 15
                                                                        Rectangle { 
                                                                            width: 16; height: 16; radius: 4; color: (cacheContent.clearTracks || model.selected) ? "#44ff44" : "#222"
                                                                            Image { anchors.centerIn: parent; source: "assets/check.svg"; width: 10; height: 10; visible: cacheContent.clearTracks || model.selected; layer.enabled: true; layer.effect: ColorOverlay { color: "black" } }
                                                                        }
                                                                        Text { text: model.id; color: "#aaa"; font.family: "Rubik"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                                                                        Text { text: formatSize(model.size); color: "#666"; font.family: "Rubik"; font.pixelSize: 11 }
                                                                    }
                                                                    MouseArea { 
                                                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                                        onClicked: {
                                                                            if (cacheContent.clearTracks) {
                                                                                cacheContent.clearTracks = false
                                                                                for (var i = 0; i < detailedTracksModel.count; i++) {
                                                                                    detailedTracksModel.setProperty(i, "selected", true)
                                                                                }
                                                                                model.selected = false
                                                                            } else {
                                                                                model.selected = !model.selected
                                                                                var all = true
                                                                                for (var j = 0; j < detailedTracksModel.count; j++) {
                                                                                    if (!detailedTracksModel.get(j).selected) { all = false; break }
                                                                                }
                                                                                if (all) cacheContent.clearTracks = true
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                        }
                                                    }

                                                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#333"; Layout.leftMargin: 15; Layout.rightMargin: 15 }

                                                    Item {
                                                        Layout.fillWidth: true; Layout.preferredHeight: 50
                                                        RowLayout {
                                                            anchors.fill: parent; anchors.margins: 15; spacing: 15
                                                            Rectangle { 
                                                                width: 20; height: 20; radius: 4; color: cacheContent.clearCovers ? "#bb66ff" : "#333"; Layout.alignment: Qt.AlignVCenter
                                                                Image { anchors.centerIn: parent; source: "assets/check.svg"; width: 12; height: 12; visible: cacheContent.clearCovers; layer.enabled: true; layer.effect: ColorOverlay { color: "black" } }
                                                            }
                                                            Text { text: "Cover Cache"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter }
                                                            Text { 
                                                                text: (window.cacheVersion, MorphCache.getCoverCacheCount()) + " covers, " + formatSize((window.cacheVersion, MorphCache.getCoverCacheSize()))
                                                                color: "#888"; font.family: "Rubik"; font.pixelSize: 13; Layout.alignment: Qt.AlignVCenter
                                                            }
                                                            Image {
                                                                source: "assets/chevron-down.svg"; Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                                                rotation: coversExpanded ? 180 : 0
                                                                layer.enabled: true; layer.effect: ColorOverlay { color: "#444" }
                                                                Behavior on rotation { NumberAnimation { duration: 200 } }
                                                            }
                                                        }
                                                        MouseArea { 
                                                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                            onClicked: {
                                                                if (mouse.x > 60) {
                                                                    if (coversExpanded) {
                                                                        coversExpanded = false
                                                                        clearCoversTimer.start()
                                                                    } else {
                                                                        clearCoversTimer.stop()
                                                                        loadDetailedCovers()
                                                                        coversExpanded = true
                                                                    }
                                                                } else {
                                                                    cacheContent.clearCovers = !cacheContent.clearCovers
                                                                    if (!cacheContent.clearCovers) {
                                                                        for (var i = 0; i < detailedCoversModel.count; i++) {
                                                                            detailedCoversModel.setProperty(i, "selected", false)
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Item {
                                                        Layout.fillWidth: true
                                                        clip: true
                                                        Layout.preferredHeight: coversExpanded ? Math.min(detailedCoversModel.count * 40, 400) : 0
                                                        Behavior on Layout.preferredHeight { NumberAnimation { duration: 300; easing.type: Easing.InOutQuad } }

                                                        ListView {
                                                            anchors.fill: parent
                                                            model: detailedCoversModel
                                                            clip: true
                                                            interactive: contentHeight > height
                                                            ScrollBar.vertical: ScrollBar { visible: parent.interactive }
                                                            delegate: Item {
                                                                    width: ListView.view.width; height: 40
                                                                    RowLayout {
                                                                        anchors.fill: parent; anchors.leftMargin: 15; anchors.rightMargin: 15; spacing: 15
                                                                        Rectangle { 
                                                                            width: 16; height: 16; radius: 4; color: (cacheContent.clearCovers || model.selected) ? "#bb66ff" : "#222"
                                                                            Image { anchors.centerIn: parent; source: "assets/check.svg"; width: 10; height: 10; visible: cacheContent.clearCovers || model.selected; layer.enabled: true; layer.effect: ColorOverlay { color: "black" } }
                                                                        }
                                                                        Text { text: model.name; color: "#aaa"; font.family: "Rubik"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideMiddle }
                                                                        Text { text: formatSize(model.size); color: "#666"; font.family: "Rubik"; font.pixelSize: 11 }
                                                                    }
                                                                    MouseArea { 
                                                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                                        onClicked: {
                                                                            if (cacheContent.clearCovers) {
                                                                                cacheContent.clearCovers = false
                                                                                for (var i = 0; i < detailedCoversModel.count; i++) {
                                                                                    detailedCoversModel.setProperty(i, "selected", true)
                                                                                }
                                                                                model.selected = false
                                                                            } else {
                                                                                model.selected = !model.selected
                                                                                var all = true
                                                                                for (var j = 0; j < detailedCoversModel.count; j++) {
                                                                                    if (!detailedCoversModel.get(j).selected) { all = false; break }
                                                                                }
                                                                                if (all) cacheContent.clearCovers = true
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        Button {
                                            id: clearBtn
                                            Layout.fillWidth: true; Layout.preferredHeight: 50
                                            enabled: cacheContent.clearTracks || cacheContent.clearCovers || (function(){
                                                for(var i=0; i<detailedTracksModel.count; i++) if(detailedTracksModel.get(i).selected) return true
                                                for(var j=0; j<detailedCoversModel.count; j++) if(detailedCoversModel.get(j).selected) return true
                                                return false
                                            })()
                                            onClicked: {
                                                var anyCleared = false
                                                var currentTrackDeleted = false
                                                
                                                if (cacheContent.clearTracks) { 
                                                    MorphCache.clearTrackCache()
                                                    anyCleared = true
                                                    currentTrackDeleted = true
                                                    for (var key in streamUrlCache) {
                                                        if (streamUrlCache[key].toString().startsWith("file://")) {
                                                            delete streamUrlCache[key]
                                                        }
                                                    }
                                                } else {
                                                    for(var i=detailedTracksModel.count-1; i>=0; i--) {
                                                        if(detailedTracksModel.get(i).selected) {
                                                            var tid = detailedTracksModel.get(i).id
                                                            if (currentTrack && tid === currentTrack.id) currentTrackDeleted = true
                                                            delete streamUrlCache[tid]
                                                            MorphCache.removeCacheFile(detailedTracksModel.get(i).name, true)
                                                            anyCleared = true
                                                        }
                                                    }
                                                }
                                                
                                                if (currentTrackDeleted && currentTrack) {
                                                    delete streamUrlCache[currentTrack.id]
                                                    if (MorphAudio.isPlaying) {
                                                        lastKnownPosition = MorphAudio.position
                                                        isRecovering = true
                                                        MorphServices.resolve(currentTrack.service, currentTrack.id)
                                                    }
                                                }
                                                
                                                if (cacheContent.clearCovers) { MorphCache.clearCoverCache(); anyCleared = true }
                                                else {
                                                    for(var j=detailedCoversModel.count-1; j>=0; j--) {
                                                        if(detailedCoversModel.get(j).selected) {
                                                            MorphCache.removeCacheFile(detailedCoversModel.get(j).name, false)
                                                            anyCleared = true
                                                        }
                                                    }
                                                }

                                                if (anyCleared) {
                                                    window.cacheVersion++
                                                    refreshDetailedCache()
                                                    cacheContent.showSuccess = true
                                                    successTimer.start()
                                                }
                                            }
                                            background: Rectangle { 
                                                color: clearBtn.enabled ? "#1a1a1a" : "#111"
                                                radius: 10; border.color: clearBtn.enabled ? "#333" : "#222"
                                            }
                                            contentItem: Text { 
                                                text: cacheContent.showSuccess ? "CLEARED SUCCESSFULLY!" : "CLEAR SELECTED DATA"
                                                color: cacheContent.showSuccess ? "#44ff44" : (clearBtn.enabled ? "white" : "#444")
                                                font.family: "Rubik"; font.pixelSize: 14; font.weight: Font.Black
                                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                            }
                                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true; spacing: 10
                                            Text { text: "CACHE SETTINGS"; color: "#444"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Black }
                                            
                                            RowLayout {
                                                Layout.fillWidth: true
                                                Text { text: "Save Track Cache"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; Layout.fillWidth: true }
                                                Switch {
                                                    id: saveTrackSwitch; Layout.alignment: Qt.AlignVCenter; Layout.preferredHeight: 20; padding: 0
                                                    checked: (window.settingsVersion, MorphSettings.getSaveTrackCache())
                                                    onToggled: {
                                                        MorphSettings.setSaveTrackCache(checked)
                                                        window.cacheVersion++
                                                        refreshDetailedCache()
                                                    }
                                                    indicator: Rectangle {
                                                        implicitWidth: 36; implicitHeight: 20; radius: 10
                                                        color: saveTrackSwitch.checked ? "#44ff44" : "#222"
                                                        Behavior on color { ColorAnimation { duration: 150 } }
                                                        Rectangle {
                                                            x: saveTrackSwitch.checked ? parent.width - width - 2 : 2; y: 2
                                                            width: 16; height: 16; radius: 8; color: "white"
                                                            Behavior on x { NumberAnimation { duration: 150 } }
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            RowLayout {
                                                Layout.fillWidth: true
                                                Text { text: "Save Cover Cache"; color: "white"; font.family: "Rubik"; font.pixelSize: 14; Layout.fillWidth: true }
                                                Switch {
                                                    id: saveCoverSwitch; Layout.alignment: Qt.AlignVCenter; Layout.preferredHeight: 20; padding: 0
                                                    checked: (window.settingsVersion, MorphSettings.getSaveCoverCache())
                                                    onToggled: {
                                                        MorphSettings.setSaveCoverCache(checked)
                                                        window.cacheVersion++
                                                        refreshDetailedCache()
                                                    }
                                                    indicator: Rectangle {
                                                        implicitWidth: 36; implicitHeight: 20; radius: 10
                                                        color: saveCoverSwitch.checked ? "#bb66ff" : "#222"
                                                        Behavior on color { ColorAnimation { duration: 150 } }
                                                        Rectangle {
                                                            x: saveCoverSwitch.checked ? parent.width - width - 2 : 2; y: 2
                                                            width: 16; height: 16; radius: 8; color: "white"
                                                            Behavior on x { NumberAnimation { duration: 150 } }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true; spacing: 10
                                            Text { text: "CACHE LIMIT"; color: "#444"; font.family: "Rubik"; font.pixelSize: 11; font.weight: Font.Black }
                                            RowLayout {
                                                spacing: 8
                                                Repeater {
                                                    model: [
                                                        { label: "100MB", value: 104857600 },
                                                        { label: "500MB", value: 524288000 },
                                                        { label: "1GB", value: 1073741824 },
                                                        { label: "5GB", value: 5368709120 },
                                                        { label: "NO LIMITS", value: 0 }
                                                    ]
                                                    Button {
                                                        id: limitBtn
                                                        Layout.fillWidth: true; Layout.preferredHeight: 32
                                                        property bool active: (window.settingsVersion, MorphSettings.getCacheLimit() === modelData.value)
                                                        onClicked: {
                                                            MorphSettings.setCacheLimit(modelData.value)
                                                            window.cacheVersion++
                                                            refreshDetailedCache()
                                                        }
                                                        background: Rectangle { 
                                                            color: limitBtn.active ? "white" : "#1a1a1a"
                                                            radius: 6; border.color: "#333" 
                                                        }
                                                        contentItem: Text { 
                                                            text: modelData.label; color: limitBtn.active ? "black" : "#888"
                                                            font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Bold
                                                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter 
                                                        }
                                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; acceptedButtons: Qt.NoButton }
                                                    }
                                                }
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: "If the cache size exceeds this limit, the oldest unused files will be deleted from the device memory."
                                                color: "#555"; font.family: "Rubik"; font.pixelSize: 11; wrapMode: Text.Wrap
                                            }
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
                                    id: currentTimeLabel
                                    opacity: progressHoverArea.containsMouse ? 1 : 0
                                    Behavior on opacity { NumberAnimation { duration: 150 } }
                                    text: formatTime(MorphAudio.position)
                                    color: "white"; font.family: "Rubik"; font.pixelSize: 10; font.weight: Font.Bold
                                    y: -12
                                    x: {
                                        var desiredX = (progressSlider.visualPosition * parent.width) - (width / 2)
                                        var maxX = progressSlider.width - durationLabel.width - width - 10
                                        return Math.max(0, Math.min(desiredX, maxX))
                                    }
                                }

                                Text {
                                    id: durationLabel
                                    opacity: progressHoverArea.containsMouse ? 1 : 0
                                    Behavior on opacity { NumberAnimation { duration: 150 } }
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
                                            source: currentTrack ? MorphCache.getCachedCover(currentTrack.coverUrl) : ""; Layout.preferredWidth: 48; Layout.preferredHeight: 48; fillMode: Image.PreserveAspectCrop
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
                                            id: bitrateRect
                                            property bool shouldShow: MorphAudio.bitrate > 0 && window.width > 900
                                            opacity: shouldShow ? 1 : 0
                                            visible: opacity > 0
                                            Behavior on opacity { NumberAnimation { duration: 150 } }
                                            
                                            Layout.preferredWidth: shouldShow ? (bitrateText.width + 10) : 0
                                            Behavior on Layout.preferredWidth { NumberAnimation { duration: 150 } }
                                            
                                            height: 16; radius: 4; color: "#1a1a1a"; clip: true
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
                            var view = ListView.view
                            track = view ? view.model.get(index) : (searchModel.count > 0 ? searchModel.get(index) : historyModel.get(index))
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
                            var view = ListView.view
                            m = view ? view.model : (searchModel.count > 0 ? searchModel : historyModel)
                            track = m.get(index)

                            var tObj = { "id": track.id, "title": track.title, "artist": track.artist, "coverUrl": track.coverUrl, "service": track.service, "webUrl": track.webUrl || "", "durationMs": track.durationMs || 0 }
                            MorphSettings.addSearchHistory(tObj)

                            historyModel.clear()
                            var hist = MorphSettings.getSearchHistory()
                            for (var i = 0; i < hist.length; i++) historyModel.append(hist[i])

                            playTrack(tObj, index)
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
                    source: MorphCache.getCachedCover(model.coverUrl || ""); Layout.preferredWidth: 36; Layout.preferredHeight: 36; fillMode: Image.PreserveAspectCrop 
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
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignVCenter
                    
                    Rectangle {
                        width: 6; height: 6; radius: 3; color: "#44ff44"
                        visible: (window.cacheVersion, MorphCache.isTrackCached(model.id))
                    }
                    Text {
                        text: formatTime(model.durationMs || 0)
                        color: "#666"; font.family: "Rubik"; font.pixelSize: 12; visible: (model.durationMs || 0) > 0
                        Layout.preferredWidth: 35
                        horizontalAlignment: Text.AlignRight
                    }
                    Image {
                        source: (window.likesVersion, MorphSettings.isLiked(model.id)) ? "assets/heart.svg" : "assets/heart-outline.svg"; Layout.preferredWidth: 18; Layout.preferredHeight: 18; Layout.leftMargin: 4; layer.enabled: true; layer.effect: ColorOverlay { color: "white" }
                        MouseArea { 
                            anchors.fill: parent; onClicked: MorphSettings.toggleLike({ "id": model.id, "title": model.title, "artist": model.artist, "coverUrl": model.coverUrl, "service": model.service, "album": model.album || "", "webUrl": model.webUrl || "", "durationMs": model.durationMs || 0 }); cursorShape: Qt.PointingHandCursor 
                        }
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
            isSearching = false
            for (var i = 0; i < results.length; i++) {
                var item = results[i]
                item.service = serviceName
                if (item.durationMs === undefined) item.durationMs = 0
                searchModel.append(item)
            }
        }
        function onErrorOccurred(message) {
            isSearching = false
            importPlaylistPopup.isBusy = false
            importPlaylistPopup.errorMsg = message
        }
        function onChartsReady(serviceName, results) {
            if (results.length > 0) chartsModel.clear()
            for (var i = 0; i < results.length; i++) {
                var item = results[i]
                item.service = serviceName
                if (item.durationMs === undefined) item.durationMs = 0
                chartsModel.append(item)
            }
        }
        function onWaveReady(serviceName, results) {
            currentPlaylist = "MY_VIBE"
            libraryModel.clear()
            for (var i = 0; i < results.length; i++) {
                var item = results[i]
                item.service = serviceName
                if (item.durationMs === undefined) item.durationMs = 0
                libraryModel.append(item)
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
                var item = tracks[i]
                if (item.durationMs === undefined) item.durationMs = 0
                libraryModel.append(item)
            }
            
            if (libraryModel.count > 0) {
                playTrack(libraryModel.get(0), 0)
            }

            importPlaylistPopup.isBusy = false
            importPlaylistPopup.close()
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
                for(var i = likes.length - 1; i >= 0; i--) {
                    var item = likes[i]
                    if (item.durationMs === undefined) item.durationMs = 0
                    libraryModel.append(item)
                }
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
                        for (var i = 0; i < tracks.length; i++) {
                            var item = tracks[i]
                            if (item.durationMs === undefined) item.durationMs = 0
                            libraryModel.append(item)
                        }
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
