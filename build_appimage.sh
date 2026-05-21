#!/bin/bash
set -e

echo ">>> 1. Compiling the latest code..."
mkdir -p build && cd build
cmake ..
make -j$(nproc)
cd ..

echo ">>> 2. Preparing files..."
cat <<EOF > dockerupdater.desktop
[Desktop Entry]
Name=DockerUpdater
Exec=DockerUpdater
Icon=docker
Type=Application
Categories=Utility;
EOF

rm -f docker.png

if [ ! -f docker.svg ]; then
    wget -q -O docker.svg https://raw.githubusercontent.com/simple-icons/simple-icons/develop/icons/docker.svg
fi

echo ">>> 3. Downloading modern 'linuxdeploy' tools..."
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget -c -nv "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod a+x linuxdeploy-x86_64.AppImage
fi
if [ ! -f linuxdeploy-plugin-qt-x86_64.AppImage ]; then
    wget -c -nv "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod a+x linuxdeploy-plugin-qt-x86_64.AppImage
fi

echo ">>> 4. Building the AppImage..."
rm -rf AppDir 
export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE=qmake6
./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    -e build/DockerUpdater \
    -d dockerupdater.desktop \
    -i docker.svg \
    --plugin qt \
    --output appimage

echo ""
echo "========================================================="
echo ">>> DONE! Your AppImage is ready to be moved to Bazzite."
echo "========================================================="