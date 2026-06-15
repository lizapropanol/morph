<h1 align="center">morph</h1>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++" />
  <img src="https://img.shields.io/badge/Qt-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt" />
  <img src="https://img.shields.io/badge/QML-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="QML" />
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL_v3-blue.svg?style=for-the-badge" alt="License" /></a>
  <img src="https://img.shields.io/badge/Platform-Linux-blue?style=for-the-badge" alt="Linux" />
  <a href="https://www.donationalerts.com/r/lizapropanol"><img src="https://img.shields.io/badge/Donate-DonationAlerts-orange?style=for-the-badge&logo=donate&logoColor=white" alt="Donate" /></a>
</p>

**morph** is a sleek, modern desktop music player built with C++ and Qt/QML. It integrates seamlessly with **Yandex Music**, **SoundCloud** and **YouTube Music**, providing a unified and aesthetically pleasing interface for your music library, charts, and personalized radio streams.

---

## Key Features

- **Triple Service Integration**: Search and play tracks from Yandex Music, SoundCloud and YouTube Music in a single interface.
- **My Vibe (Personalized Wave)**: Direct integration with Yandex Music's algorithmic radio, complete with listening telemetry so your recommendations keep improving.
- **Top Charts**: Browse and play the top tracks from Yandex Music with a clean, dual-column ranked layout.
- **Daily Mixes**: Access personalized daily playlists from SoundCloud directly on the home screen.
- **Artist Profiles**: Browse detailed artist profiles displaying a biography, high-resolution avatar, list of albums, and tracks directly from the player.
- **Advanced Playlist Management**:
  - Create, rename, and delete custom playlists.
  - Set custom cover images using external URLs (Imgur, Pinterest, etc.).
  - Easily add tracks from search, charts, or history directly into your playlists.
- **Search History**: Automatically keeps track of your 20 most recently played tracks from searches for quick access.
- **Persistent Caching & Offline Support**: Automatically saves played tracks and viewed covers to local storage, enabling full playback without an internet connection.
- **Advanced Cache Management**: Dedicated storage view with real-time size calculation and granular clearing options for tracks and covers.
- **Real-time Bitrate Display**: Shows current track quality and bitrate information directly in the interface for all supported services.
- **Security & Privacy**:
  - **Hardware-Linked Encryption**: Authentication tokens are protected with dual-layer AES-256-GCM encryption. Keys are derived via PBKDF2-HMAC-SHA-512 (600,000 iterations) from a combination of system-unique identifiers, ensuring your credentials remain secure and unusable if copied to another machine.
- **System Integration**:
  - **Discord Rich Presence**: Show your current track, artist, and playback status directly on your Discord profile.
  - **MPRIS Support**: Full integration with Linux media controllers (playerctl, system tray, etc.) for remote playback control and metadata display.
- **Extensible Styling Engine**:
  - **Config Library**: A built-in library to manage, import, and export custom QML themes and configurations effortlessly.
  - **Live "Run Preview"**: Instantly launch a separate window to test and visualize your style changes in real-time as you edit.
  - **Integrated IDE**: Includes a custom QML syntax highlighter and live preview system for theme development.
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

### Arch User Repository (AUR)
If you are on Arch Linux, you can easily install **morph** using an AUR helper like `yay`:
```bash
yay -S morph
```

### Prerequisites
- CMake (3.16 or higher)
- Qt 6 (Core, Quick, Qml, Network, Multimedia, DBus, Svg, 5Compat)
- C++23 compatible compiler (GCC 12+ / Clang 15+)

### Runtime Dependencies (Arch Linux)
For full functionality (audio codecs and icons), make sure to install:
```bash
sudo pacman -S qt6-base qt6-declarative qt6-multimedia qt6-svg qt6-5compat gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav hicolor-icon-theme yt-dlp ffmpeg openssl
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
- `src/services/`: Specific API implementations (`YandexService`, `SoundCloudService`, `YouTubeService`) and base interfaces.
- `src/utils/`: Cache management, path resolution, and file system utilities.
- `src/network/`: HTTP wrappers for API communication.
- `src/settings/`: JSON-based local storage management for playlists, likes, tokens, and history.
- `src/audio/`: Low-level audio engine integration.
- `ui/`: The frontend layer, containing `style.qml` and associated assets.

---

## License

This project is open-source. Please see the `LICENSE` file in the root directory for more details.

---

## Star History

<a href="https://www.star-history.com/?repos=lizapropanol%2Fmorph&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=lizapropanol/morph&type=date&theme=dark&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=lizapropanol/morph&type=date&theme=dark&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=lizapropanol/morph&type=date&theme=dark&legend=top-left" />
 </picture>
</a>

---

<p align="center">Developed with ❤️ by <a href="https://github.com/lizapropanol">lizapropanol</a></p>
