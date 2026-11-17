#!/bin/bash
rm -rf /usr/bin/SFP-Master
rm -rf /usr/share/applications/io.github.bigbigmdm.sfp-master.desktop
rm -rf /usr/share/sfp-master
rm -rf /usr/share/man/man1/SFP-Master.1.gz
rm -rf /usr/share/metainfo/io.github.bigbigmdm.sfp-master.metainfo.xml
rm -rf /usr/share/doc/sfp-master

# Removing hicolor icons (only for Linux)
if [[ "$OSTYPE" != "darwin"* ]]; then
    icon_sizes=("16x16" "22x22" "24x24" "32x32" "48x48" "64x64" "128x128" "256x256" "512x512" "scalable")
    for size in "${icon_sizes[@]}"; do
        apps_dir="${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${size}/apps"
        if [ -d "$apps_dir" ]; then
            while IFS= read -r -d '' icon; do
                files+=("$icon")
            done < <(find "$apps_dir" -maxdepth 1 \( -name "io.github.bigbigmdm.imsprog.*" -o -name "io.github.bigbigmdm.imsprog.editor.*" -o -name "io.github.bigbigmdm.imsprog.database_update.*" \) -print0)
        fi
    done
fi

# Linux icon cashe update
if [[ "$OSTYPE" != "darwin"* ]] && command -v gtk-update-icon-cache &> /dev/null; then
    sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor &> /dev/null
fi
