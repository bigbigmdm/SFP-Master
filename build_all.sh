#!/bin/bash

if [[ "$EUID" -ne 0 ]] && [[ "$OSTYPE" != "darwin"* ]]
  then echo "Please run as root! (sudo ./build_all.sh)"
  exit
fi
[[ "$OSTYPE" == "darwin"* ]] && export C_INCLUDE_PATH=/usr/local/opt/libusb/include
(

rm -rf build/
mkdir build/
# For using Qt6
cmake -S . -B build/
# For using Qt5
#cmake -S . -B build/ -DFORCE_QT5=ON
cmake --build build/ --parallel 2
cmake --install build/
rm -rf build/
)

# Reloading the USB rules or creating the app bundles for macOS
[[ "$OSTYPE" != "darwin"* ]] && udevadm control --reload-rules || ./create_macos_appbundles.sh
# Updating system icon cache and desktop database in Linux systems
if [ "$(uname)" = "Linux" ]; then
    
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -f -t /usr/share/icons/hicolor
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q
    fi
fi
