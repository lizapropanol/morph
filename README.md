<h1 align="center">morph</h1>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++" />
  <img src="https://img.shields.io/badge/Qt-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt" />
  <img src="https://img.shields.io/badge/QML-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="QML" />
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL_v3-blue.svg?style=for-the-badge" alt="License" /></a>
  <img src="https://img.shields.io/badge/Platform-Linux-blue?style=for-the-badge" alt="Linux" />
  <a href="https://www.donationalerts.com/r/lizapropanol"><img src="https://img.shields.io/badge/Donate-DonationAlerts-orange?style=for-the-badge&logo=donate&logoColor=white" alt="Donate" /></a>
</p>

**morph** is a sleek, modern desktop music player built with C++ and Qt/QML. It integrates seamlessly with **Yandex Music** and **SoundCloud**, providing a unified and aesthetically pleasing interface for your music library, charts, and personalized radio streams.

---

## Key Features

- **Dual Service Integration**: Search and play tracks from both Yandex Music and SoundCloud in a single interface.
- **My Vibe (Personalized Wave)**: Direct integration with Yandex Music's algorithmic radio, complete with listening telemetry so your recommendations keep improving.
- **Top Charts**: Browse and play the top tracks from Yandex Music with a clean, dual-column ranked layout.
- **Daily Mixes**: Access personalized daily playlists from both Yandex Music and SoundCloud directly on the home screen.
- **Advanced Playlist Management**:
  - Create, rename, and delete custom playlists.
  - Set custom cover images using external URLs (Imgur, Pinterest, etc.).
  - Easily add tracks from search, charts, or history directly into your playlists.
- **Search History**: Automatically keeps track of your 20 most recently played tracks from searches for quick access.
- **Persistent Caching & Offline Support**: Automatically saves played tracks and viewed covers to local storage, enabling full playback without an internet connection.
- **Advanced Cache Management**: Dedicated storage view with real-time size calculation and granular clearing options for tracks and covers.
- **Modern UI/UX**:
  - Dark, minimalist aesthetic utilizing smooth gradients and QML animations.
  - Circular reveal splash screen synchronized with logo expansion.
  - Time-aware greetings with active service status indicators.
  - Real-time visual indicators (green dots) for cached offline tracks.
  - Master-Detail navigation flow for intuitive library browsing.
  - Interactive elements with consistent hover states and hand-cursor feedback.
- **Reactive Settings**: Automatic persistence of authentication tokens and audio preferences upon input.
- **Lightweight & Fast**: Powered by C++ backend logic for minimal resource usage while delivering a premium Qt Quick interface.

---

## Installation & Setup

### Prerequisites
- CMake (3.16 or higher)
- Qt 6 (Core, Quick, Qml, Network, Multimedia, DBus, Svg, 5Compat)
- C++23 compatible compiler (GCC 12+ / Clang 15+)

### Runtime Dependencies (Arch Linux)
For full functionality (audio codecs and icons), make sure to install:
```bash
sudo pacman -S qt6-svg qt6-5compat gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/lizapropanol/morph.git
cd morph

# Create a build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run the application
./morph
```

---

## Configuration

To unlock full functionality, you need to provide your authentication tokens in the **Settings** tab. Changes are saved automatically.

### Yandex Music Token
1. Use a browser extension like **yandex-music-token** (available for Chrome/Firefox).
2. Log in to your Yandex account.
3. Copy the generated token and paste it into the "Yandex Music Token" field in morph's settings.

### SoundCloud OAuth Token
1. Log in to your account at [soundcloud.com](https://soundcloud.com).
2. Open Developer Tools (**F12**) and go to the **Application** tab (Chrome/Edge) or **Storage** tab (Firefox).
3. In the sidebar, expand **Cookies** and select `https://soundcloud.com`.
4. Find the cookie named `oauth_token` and copy its value (it starts with `2-`).
5. Paste this token into the "SoundCloud Client ID" field in morph's settings.

*Note: Using a personal OAuth token is required for personalized features like **Daily Mixes**.*

---

## Project Structure

- `src/core/`: Application initialization and service management.
- `src/services/`: Specific API implementations (`YandexService`, `SoundCloudService`) and base interfaces.
- `src/utils/`: Cache management, path resolution, and file system utilities.
- `src/network/`: HTTP wrappers for API communication.
- `src/settings/`: JSON-based local storage management for playlists, likes, tokens, and history.
- `src/audio/`: Low-level audio engine integration.
- `ui/`: The frontend layer, containing `style.qml` and associated assets.

---

## License

This project is open-source. Please see the `LICENSE` file in the root directory for more details.

---

<p align="center">Developed with ❤️ by <a href="https://github.com/lizapropanol">lizapropanol</a></p>
