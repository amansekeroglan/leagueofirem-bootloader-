#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EDK2_DIR="${EDK2_PATH:-/home/efe/edk2}"

if [ ! -d "$EDK2_DIR" ]; then
    echo "Error: EDK2 directory not found at $EDK2_DIR"
    exit 1
fi

ln -sfn "$SCRIPT_DIR" "$EDK2_DIR/BootMgrPkg"

cd "$EDK2_DIR"
source edksetup.sh
build -a X64 -t GCC -p BootMgrPkg/BootMgr.dsc -b RELEASE

cp -f "$EDK2_DIR/Build/BootMgr/RELEASE_GCC/X64/BootMgr.efi" "$SCRIPT_DIR/BootMgr.efi"
echo "Build successful: $SCRIPT_DIR/BootMgr.efi"
