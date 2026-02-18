#!/bin/bash

# Keyboard USB IDs (must match project: usbd_conf / .ioc)
DEVICE_VID="1eaf"
DEVICE_PID="0024"
DFU_VID_PID="${DEVICE_VID}:0003"
DEVICE_VID_PID="${DEVICE_VID}:${DEVICE_PID}"

# Report ID 5, bootloader magic (matches firmware: jump_to_bootloader)
BOOTLOADER_CMD='\x05\xFE\x55\xAA'

# Find /dev/hidrawN for the given USB VID:PID (sysfs uses lowercase hex)
find_hidraw_by_vid_pid() {
    for d in /sys/class/hidraw/hidraw*; do
        [ -d "$d" ] || continue
        [ -e "$d/device" ] || continue
        dev="/dev/$(basename "$d")"
        # Resolve device symlink so we walk the real path to find idVendor/idProduct
        p=$(readlink -f "$d/device" 2>/dev/null) || p="$d/device"
        while [ -n "$p" ] && [ "$p" != "/" ]; do
            if [ -f "$p/idVendor" ] && [ -f "$p/idProduct" ]; then
                ven=$(cat "$p/idVendor" 2>/dev/null | tr 'A-F' 'a-f')
                prod=$(cat "$p/idProduct" 2>/dev/null | tr 'A-F' 'a-f')
                if [ "$ven" = "$DEVICE_VID" ] && [ "$prod" = "$DEVICE_PID" ]; then
                    echo "$dev"
                    return 0
                fi
                break
            fi
            p=$(dirname "$p")
        done
    done
    return 1
}

bootloader_via_hidraw() {
    echo "Putting keyboard into bootloader via hidraw..."
    hidraw=$(find_hidraw_by_vid_pid)
    if [ -z "$hidraw" ]; then
        echo "No hidraw device found for ${DEVICE_VID}:${DEVICE_PID}"
        return 1
    fi
    echo "Using $hidraw"
    if ! printf '%b' "$BOOTLOADER_CMD" | sudo tee "$hidraw" 2>&1 >/dev/null; then
        echo "Failed to write to $hidraw (try sudo)"
        return 1
    fi
    return 0
}

wait_for_bootloader() {
    TIMEOUT=30
    while ! lsusb | grep -qi "$DFU_VID_PID"; do
        # send bootloader magic again
        [ -e "$hidraw" ] && printf '%b' "$BOOTLOADER_CMD" | sudo tee "$hidraw" 2>&1 >/dev/null
        # failsafe, just in case if we created a file in the previous step
        [ -f "$hidraw" ] && sudo rm -f "$hidraw"
        echo -n "."
        sleep 1
        TIMEOUT=$((TIMEOUT - 1))
        if [ "$TIMEOUT" -eq 0 ]; then
            echo
            echo "Timeout waiting for DFU mode"
            exit 1
        fi
    done    
    echo
    exit 0
}

# Only run on Linux
if [ "$(uname -s)" != "Linux" ]; then
    echo "Can't put the keyboard into DFU mode automatically on this platform, please put it into DFU mode manually"
    exit 0
fi

if lsusb | grep -q "$DEVICE_VID_PID"; then
    if bootloader_via_hidraw; then
        sleep 1
        wait_for_bootloader
    else
        exit 1
    fi
else
    echo "Keyboard not found (${DEVICE_VID_PID}). Connect it and put it into DFU mode, or connect and run this script again."
    wait_for_bootloader
fi
