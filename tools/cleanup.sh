#!/bin/sh
# tools/cleanup.sh
# Clean build artifacts and old golddrive mount entries from the registry

# clean build artifacts
git clean -dfx

# clean old golddrive MountPoints2 registry entries
echo "Cleaning old golddrive registry entries..."
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2" 2>/dev/null \
| grep -i "golddrive" \
| while read -r key; do
    echo "  Deleting: $key"
    reg delete "$key" /f 2>/dev/null
done

# clean old golddrive persistent net use entries (HKCU\Network\{Letter})
for letter in G H I J K L M N O P Q R S T U V W X Y Z; do
    remote=$(reg query "HKCU\\Network\\$letter" /v RemotePath 2>/dev/null | grep -i "golddrive")
    if [ -n "$remote" ]; then
        echo "  Deleting persistent mount: $letter"
        reg delete "HKCU\\Network\\$letter" /f 2>/dev/null
    fi
done

# clean old golddrive drive icon entries
for letter in G H I J K L M N O P Q R S T U V W X Y Z; do
    icon=$(reg query "HKCU\\Software\\Classes\\Applications\\Explorer.exe\\Drives\\$letter\\DefaultIcon" 2>/dev/null | grep -i "golddrive")
    if [ -n "$icon" ]; then
        echo "  Deleting drive icon: $letter"
        reg delete "HKCU\\Software\\Classes\\Applications\\Explorer.exe\\Drives\\$letter" /f 2>/dev/null
    fi
done

echo "Cleanup complete."
