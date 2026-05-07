<h1 align="center">morph</h1>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++" />
  <img src="https://img.shields.io/badge/Qt-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt" />
  <img src="https://img.shields.io/badge/QML-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="QML" />
  <img src="https://img.shields.io/badge/Platform-Linux-blue?style=for-the-badge" alt="Linux" />
</p>

**morph** is a sleek, modern desktop music player built with C++ and Qt/QML. It integrates seamlessly with **Yandex Music** and **SoundCloud**, providing a unified and aesthetically pleasing interface for your music library, charts, and personalized radio streams.

---

## Key Features

- **Dual Service Integration**: Search and play tracks from both Yandex Music and SoundCloud in a single interface.
- **My Vibe (Personalized Wave)**: Direct integration with Yandex Music's algorithmic radio, complete with listening telemetry so your recommendations keep improving.
- **Top Charts**: Browse and play the top 100 tracks from Yandex Music with a clean, dual-column ranked layout.
- **Advanced Playlist Management**:
  - Create, rename, and delete custom playlists.
  - Set custom cover images using external URLs (Imgur, Pinterest, etc.).
  - Easily add tracks from search, charts, or history directly into your playlists.
- **Search History**: Automatically keeps track of your 20 most recently played tracks from searches for quick access.
- **Modern UI/UX**:
  - Dark, minimalist aesthetic utilizing smooth gradients and QML animations.
  - Master-Detail navigation flow for intuitive library browsing.
  - Interactive elements with consistent hover states and hand-cursor feedback.
- **Lightweight & Fast**: Powered by C++ backend logic for minimal resource usage while delivering a premium Qt Quick interface.

---

## Installation & Setup

### Prerequisites
- CMake (3.16 or higher)
- Qt 5 (Core, Quick, Qml, Network, Multimedia, DBus)
- C++17 compatible compiler (GCC/Clang)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/lizapropanol/morph.git
cd morph

# Create a build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build . -j$(nproc)

# Run the application
./morph
```

---

## Configuration

To unlock full functionality, you need to provide your authentication tokens in the **Settings** tab.

### Yandex Music Token
1. Use a browser extension like **yandex-music-token** (available for Chrome/Firefox).
2. Log in to your Yandex account.
3. Copy the generated token and paste it into the "Yandex Music Token" field in morph's settings.

### SoundCloud Client ID
1. Open [soundcloud.com](https://soundcloud.com) in your browser.
2. Open Developer Tools (F12) and go to the Network tab.
3. Filter by `api-v2` and reload the page.
4. Click on any API request and look for the `Authorization` header.
5. Copy the token (excluding the "OAuth " prefix) and paste it into the "SoundCloud Client ID" field in morph.

---

## Project Structure

- `src/core/`: Application initialization and service management.
- `src/services/`: Specific API implementations (`YandexService`, `SoundCloudService`) and base interfaces.
- `src/network/`: HTTP wrappers for API communication.
- `src/settings/`: JSON-based local storage management for playlists, likes, tokens, and history.
- `src/audio/`: (Optional/In-progress) Audio playback abstractions.
- `ui/`: The frontend layer, containing `style.qml` and associated assets.

---

## License

This project is open-source. Please see the `LICENSE` file in the root directory for more details.
