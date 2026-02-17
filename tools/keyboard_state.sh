#!/bin/bash
#
# Read or write keyboard layer and FN lock via hidraw (Report ID 5, 3 data bytes).
# Byte0: low nibble = layer (0-14), 0x0F = don't change; bit 4 = FN lock; 0xF0 in high = don't change FN.
# Byte1, byte2: reserved (use 0).
#
# Usage:
#   keyboard_state.sh get              - print current layer and fn_lock
#   keyboard_state.sh set --layer N    - set layer 0-14 (keeps FN lock state)
#   keyboard_state.sh set --fn-lock on|off
#   keyboard_state.sh set --layer N --fn-lock on|off
#

set -e

DEVICE_VID="1eaf"
DEVICE_PID="0024"
REPORT_ID='\x05'

find_hidraw() {
    for d in /sys/class/hidraw/hidraw*; do
        [[ -d "$d" ]] || continue
        [[ -e "$d/device" ]] || continue
        dev="/dev/$(basename "$d")"
        p=$(readlink -f "$d/device" 2>/dev/null) || p="$d/device"
        while [[ -n "$p" && "$p" != "/" ]]; do
            if [[ -f "$p/idVendor" && -f "$p/idProduct" ]]; then
                ven=$(cat "$p/idVendor" 2>/dev/null | tr 'A-F' 'a-f')
                prod=$(cat "$p/idProduct" 2>/dev/null | tr 'A-F' 'a-f')
                if [[ "$ven" == "$DEVICE_VID" && "$prod" == "$DEVICE_PID" ]]; then
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

# Send 4 bytes (Report ID + 3 data bytes) and optionally read reply
write_report() {
    local b0="$1" b1="${2:-0}" b2="${3:-0}"
    printf '%b%b%b%b' "$REPORT_ID" "$(printf '\\x%02x' "$b0")" "$(printf '\\x%02x' "$b1")" "$(printf '\\x%02x' "$b2")" | sudo tee "$HIDRAW" >/dev/null
}

cmd_get() {
    # 0xFF = don't change layer, don't change FN -> device echoes current state.
    # Start reader first (device sends reply after our write), then write.
    # Loop: read one report per iteration with short timeout; cat always waits until timeout,
    # so many short reads (0.05s) are faster than one long wait. Skip reports with ID != 0x05.
    local tmp
    tmp=$(mktemp)
    (
        for _ in {1..50}; do
            block=$(sudo timeout 0.05 cat "$HIDRAW" 2>/dev/null | head -c 16 | xxd -p -c 16)
            [[ -z "$block" ]] && continue
            if [[ ${#block} -ge 8 && "${block:0:2}" == "05" ]]; then
                echo "${block:0:8}" > "$tmp"
                break
            fi
        done
    ) &
    local pid=$!
    sleep 0.02
    write_report 255 0 0
    wait "$pid" 2>/dev/null
    local hex
    hex=$(cat "$tmp" 2>/dev/null)
    rm -f "$tmp"
    if [[ -z "$hex" || ${#hex} -lt 8 ]]; then
        echo "Failed to read from device" >&2
        return 1
    fi
    # First byte must be Report ID 5 (vendor report)
    local report_id="${hex:0:2}"
    if [[ "${report_id,,}" != "05" ]]; then
        echo "Unexpected report ID 0x${report_id} (expected 0x05). Another report type was received." >&2
        return 1
    fi
    # Byte1 = layer|fn_lock, bytes 2-3 reserved
    local b1=$((16#${hex:2:2}))
    local layer=$((b1 & 0x0F))
    local fn_lock=$(( (b1 & 0x10) != 0 ))
    echo "layer=$layer fn_lock=$([ "$fn_lock" -eq 1 ] && echo on || echo off)"
}

cmd_set() {
    local layer="" fn_lock=""
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --layer)   layer="$2"; shift 2 ;;
            --fn-lock) fn_lock="$2"; shift 2 ;;
            *) echo "Unknown option: $1" >&2; exit 1 ;;
        esac
    done

    # Build byte0: 0x0F = don't change layer, 0xF0 = don't change FN
    local b0=255  # 0xFF = no change to both
    if [[ -n "$layer" ]]; then
        if [[ "$layer" -lt 0 || "$layer" -gt 14 ]]; then
            echo "Layer must be 0-14" >&2
            exit 1
        fi
        b0=$(( (b0 & 0xF0) | (layer & 0x0F) ))   # set low nibble, keep high
    fi
    if [[ -n "$fn_lock" ]]; then
        if [[ "$fn_lock" == "on" ]]; then
            b0=$(( (b0 & 0x0F) | 0x10 ))
        elif [[ "$fn_lock" == "off" ]]; then
            b0=$(( b0 & 0x0F ))
        else
            echo "fn-lock must be 'on' or 'off'" >&2
            exit 1
        fi
    fi
    write_report "$b0" 0 0
    echo "OK"
}

# Main
if [[ "$(uname -s)" != "Linux" ]]; then
    echo "This script runs on Linux only (hidraw)." >&2
    exit 1
fi

HIDRAW=$(find_hidraw)
if [[ -z "$HIDRAW" ]]; then
    echo "Keyboard not found (${DEVICE_VID}:${DEVICE_PID}). Connect the device." >&2
    exit 1
fi

case "${1:-}" in
    get)  cmd_get ;;
    set)  shift; cmd_set "$@" ;;
    *)
        echo "Usage: $0 get | set [--layer N] [--fn-lock on|off]" >&2
        exit 1
        ;;
esac
