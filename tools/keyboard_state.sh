#!/bin/bash
#
# Read or write keyboard layer and FN lock via hidraw (Report ID 5, 3 data bytes).
# Byte0: low nibble = layer (0-9 internally, displayed as 1-10), 0x0F = don't change.
# Byte1: bit flags (bit 0 = FN lock).
# Byte2: bit mask for flags to set (bit 0 = set FN lock).
#
# Usage:
#   keyboard_state.sh get              - print current layer (1-10) and fn_lock
#   keyboard_state.sh set --layer N    - set layer 1-10 (keeps FN lock state)
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

cmd() {
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
    write_report $1 $2 $3
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
    # Byte1 = layer (0-9), byte2 = flags (bit 0 = fn_lock), byte3 reserved
    local b1=$((16#${hex:2:2}))
    local b2=$((16#${hex:4:2}))
    local layer_internal=$((b1 & 0x0F))
    local fn_lock=$(( (b2 & 0x01) != 0 ))
    # Convert to 1-based for user (0-9 -> 1-10)
    local layer_user=$((layer_internal + 1))
    echo "layer=$layer_user fn_lock=$([ "$fn_lock" -eq 1 ] && echo on || echo off)"
}

cmd_get() {
    # 0x0F = don't change layer, 0x00 in byte2 = don't change flags -> device echoes current state.
    cmd 15 0 0
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

    # Build command: byte0 = layer (0x0F = don't change), byte1 = flags, byte2 = flag mask
    local b0=15  # 0x0F = don't change layer
    local b1=0   # flags (bit 0 = fn_lock)
    local b2=0   # flag mask (bit 0 = set fn_lock)
    
    if [[ -n "$layer" ]]; then
        # User provides 1-10, convert to 0-9 internally
        if [[ "$layer" -lt 1 || "$layer" -gt 10 ]]; then
            echo "Layer must be 1-10" >&2
            exit 1
        fi
        b0=$(((layer - 1) & 0x0F))
    fi
    
    if [[ -n "$fn_lock" ]]; then
        b2=$((b2 | 0x01))  # set bit 0 in mask
        if [[ "$fn_lock" == "on" ]]; then
            b1=$((b1 | 0x01))  # set bit 0 in flags
        elif [[ "$fn_lock" == "off" ]]; then
            b1=$((b1 & 0xFE))  # clear bit 0 in flags
        else
            echo "fn-lock must be 'on' or 'off'" >&2
            exit 1
        fi
    fi
    
    cmd "$b0" "$b1" "$b2"
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
    set)  shift; cmd_set "$@" ; ;;
    *)
        echo "Usage: $0 get | set [--layer N] [--fn-lock on|off]" >&2
        echo "  Layer N: 1-10 (1-based)" >&2
        exit 1
        ;;
esac
