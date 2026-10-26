#!/bin/bash
set -e

echo ">>> 1. Compiling the latest code..."
mkdir -p build && cd build
cmake ..
make -j$(nproc)
cd ..

echo ">>> 2. Preparing desktop file and icons..."
cat <<EOF > dockerupdater.desktop
[Desktop Entry]
Name=DockerUpdater
Exec=DockerUpdater
Icon=docker
Type=Application
Categories=Development;Utility;
Comment=Docker & Podman Multi-Platform Image Deployer and Auto-Updater
EOF

rm -f docker.png

if [ ! -f docker.svg ]; then
    wget -q -O docker.svg https://raw.githubusercontent.com/simple-icons/simple-icons/develop/icons/docker.svg
fi

echo ">>> 3. Downloading 'linuxdeploy' tools..."
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget -c -nv "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod a+x linuxdeploy-x86_64.AppImage
fi
if [ ! -f linuxdeploy-plugin-gtk.sh ]; then
    wget -c -nv "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh" -O linuxdeploy-plugin-gtk.sh
    chmod a+x linuxdeploy-plugin-gtk.sh
fi

echo ">>> 4. Building the AppImage..."
rm -rf AppDir 
export APPIMAGE_EXTRACT_AND_RUN=1
export DEPLOY_GTK_VERSION=4

./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    -e build/DockerUpdater \
    -d dockerupdater.desktop \
    -i docker.svg \
    --plugin gtk \
    --output appimage

echo ""
echo "========================================================="
echo ">>> DONE! Your AppImage is ready."
echo "========================================================="