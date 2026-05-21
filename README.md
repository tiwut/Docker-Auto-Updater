# 🐳 Docker Auto Updater (Qt6 GUI)

A lightweight, blazing-fast C++ Qt application that allows you to manage, rebuild, and securely update your Docker Hub images with **zero CLI usage**. 

Built for developers, self-hosters, and homelabbers who want to keep their published Docker images free of security vulnerabilities without having to remember tedious `docker build` and `docker push` commands.

<img width="650" height="587" alt="image" src="https://github.com/user-attachments/assets/6eb34411-2ff0-4fcc-86ee-17e2201d0cb4" />

## ✨ Why does this exist?
When you publish Docker images to Docker Hub, the base OS (Alpine, Ubuntu, Debian) and underlying software (Nginx, PHP, Node) quickly become outdated, exposing your users to security vulnerabilities. 

This tool provides a 1-click solution to:
1. Fetch all your repositories from Docker Hub.
2. Select a local `Dockerfile`.
3. Automatically pull the newest, most secure base image (`--pull`).
4. Rebuild the image and push it to Docker Hub.

## 🚀 Features
- **Visual Interface:** No more typing `docker build -t myname/myrepo:latest .`
- **Live Console Output:** Matrix-style green terminal output directly in the GUI.
- **Podman & Bazzite Support:** Works flawlessly on immutable OSes (like Bazzite or Fedora Silverblue) using Podman.
- **AppImage Ready:** Can be packaged into a standalone AppImage to keep your host OS completely clean.

## 🛠️ Prerequisites
Before using this tool, make sure your computer is logged into your container registry:

**For standard Linux (Docker):**
```bash
docker login docker.io
```
**For Bazzite / Fedora Immutable (Podman):**
```bash
podman login docker.io
```

---

## 💻 How to Build from Source (Native Linux)
If you want to build and run this directly on Ubuntu, Debian, Arch, or Fedora:

1. **Install Dependencies:**
   * **Ubuntu/Debian:** `sudo apt install build-essential cmake qt6-base-dev libqt6network6 docker.io`
   * **Fedora:** `sudo dnf install gcc-c++ cmake qt6-qtbase-devel moby-engine`
   * **Arch:** `sudo pacman -S base-devel cmake qt6-base docker`

2. **Compile:**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

3. **Run:**
   ```bash
   ./DockerUpdater
   ```

---

## 📦 How to Build an AppImage (For Immutable OS / Bazzite)
If you are using an immutable OS like Bazzite, you should not install development tools on your host. Instead, use a container (like Distrobox) to compile the app into a standalone `.AppImage`.

1. Open a Distrobox container (e.g., Debian or Ubuntu).
2. Clone this repository.
3. Run the included AppImage builder script:
   ```bash
   chmod +x build_appimage.sh
   ./build_appimage.sh
   ```
4. Move the resulting `DockerUpdater-x86_64.AppImage` to your main OS, make it executable, and run it natively!

### ⚠️ Podman "Short-Name" Fix (Important for Bazzite users)
If you are using **Podman**, you might get an `Error: creating build container: short-name resolution enforced` when clicking Rebuild. To fix this, tell Podman to default to Docker Hub:

```bash
mkdir -p ~/.config/containers
echo 'unqualified-search-registries = ["docker.io"]' > ~/.config/containers/registries.conf
```

## 🤝 Contributing
Feel free to open issues or submit pull requests! Ideas for future features:
- Dark/Light mode toggle.
- "Save last used folder path" functionality.
- Batch update (Update multiple repos at once).

## 📄 License
MIT License. Do whatever you want with it!
