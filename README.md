# Docker Auto Deployer & Updater

A lightweight, modern Linux desktop application to manage, rebuild, and securely publish single and multi-architecture Docker and Podman container images.

Built for developers, homelabbers, and sysadmins who want to maintain published container images patched against base OS vulnerabilities without manually constructing complex multi-platform build commands.

## Overview
When container images are published to registries like Docker Hub, underlying base images (Debian, Alpine, Ubuntu) and runtime packages continuously receive security updates.

This tool simplifies the maintenance workflow:
1. Fetch existing repositories directly from Docker Hub via API.
2. Select local directory containing a `Dockerfile` or `Containerfile`.
3. Configure multi-platform target architectures (`x86_64`, `arm64`, `arm/v7`, `arm/v6`, `386`, `riscv64`, or custom targets).
4. Pull base images automatically (`--pull`) to ensure security patches are incorporated.
5. Build and push multi-architecture manifests directly to Docker Hub using Buildx or Podman.

---

## Features
- **Native GNOME Interface:** Built with Libadwaita and GTK4, supporting system light and dark themes seamlessly.
- **Docker Buildx & Multi-Platform:** Build and push images targeting:
  - `linux/amd64` (Standard 64-bit x86, Intel / AMD)
  - `linux/arm64` (ARM 64-bit, Apple Silicon, Raspberry Pi 4/5, AWS Graviton)
  - `linux/arm/v7` (ARM 32-bit v7)
  - `linux/arm/v6` (ARM 32-bit v6)
  - `linux/386` (32-bit x86)
  - `linux/riscv64` (RISC-V 64-bit)
  - *Custom architectures* (e.g. `linux/ppc64le`, `linux/s390x`)
- **Docker & Podman Support:** Switch between standard Docker daemon and Podman with a single control.
- **Live Console Streaming:** Real-time log output with monospace terminal formatting and auto-scrolling.
- **Security Updates by Default:** Includes `--pull` flag to ensure base images are always up to date.
- **Cancel Build:** Safely terminate running build subprocesses at any time.
- **AppImage Ready:** Package into a standalone AppImage for immutable distributions such as Bazzite and Fedora Silverblue.

---

## Prerequisites
Ensure your workstation is authenticated with Docker Hub or target registry:

**Docker:**
```bash
docker login docker.io
```
**Podman:**
```bash
podman login docker.io
```

---

## Build from Source

### Dependencies
* **Fedora / RHEL:**
  ```bash
  sudo dnf install gcc-c++ cmake libadwaita-devel gtk4-devel pkgconf-pkg-config curl moby-engine
  ```
* **Ubuntu / Debian:**
  ```bash
  sudo apt install build-essential cmake libadwaita-1-dev libgtk-4-dev pkg-config curl docker.io
  ```
* **Arch Linux:**
  ```bash
  sudo pacman -S base-devel cmake libadwaita gtk4 pkgconf curl docker
  ```

### Compilation
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Execution
```bash
./DockerUpdater
```

---

## Packaging AppImage (Immutable OS / Bazzite)
On immutable operating systems like Bazzite, Fedora Silverblue, or openSUSE MicroOS, you can compile inside a container (e.g. Distrobox) and package as an AppImage:

```bash
chmod +x build_appimage.sh
./build_appimage.sh
```

### Podman Short-Name Resolution
If using Podman on Fedora or Bazzite, configure unqualified search registries to default to Docker Hub:

```bash
mkdir -p ~/.config/containers
echo 'unqualified-search-registries = ["docker.io"]' > ~/.config/containers/registries.conf
```

---

## License
MIT License.
