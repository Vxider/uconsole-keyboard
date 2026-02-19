#!/bin/bash

# KDE layout switcher for uConsole keyboard
# ------------------------------------------
# Listens for KDE keyboard layout changes (via D-Bus org.kde.KeyboardLayouts) and sends
# the appropriate HID report to the keyboard. Used to automatically enable the double "з"
# to "х" feature when Russian layout is active, and disable it for other layouts.
#
# Requirements:
#   - KDE Plasma (uses org.kde.keyboard D-Bus interface).
#   - Run under your user account, not root.
#   - Copy 70-uconsole-keyboard.rules to /etc/udev/rules.d/ and run:
#     sudo udevadm control --reload-rules && sudo udevadm trigger
#
# Autostart in KDE:
#   Option 1 - .desktop file (recommended):
#     Create ~/.config/autostart/layout_switcher.desktop with:
#
#       [Desktop Entry]
#       Type=Application
#       Name=Layout Switcher (uConsole)
#       Exec=/path/to/layout_switcher.sh
#       X-GNOME-Autostart-enabled=true
#
#     Replace /path/to/ with the actual path to this script.
#
#   Option 2 - KDE System Settings:
#     System Settings → Startup and Shutdown → Autostart → Add Script,
#     then choose this script.
#
#   Option 3 - Run from session startup:
#     Add the script to ~/.config/autostart-scripts/ (KDE runs executables there
#     at login) or launch it from "Autostart" in System Settings as above.

DEVICE_VID="1eaf"
DEVICE_PID="0024"
REPORT_ID='\x05'
ENABLE_DOUBLE_P_TO_BRACE_LEFT='\xff\x80\x80'
DISABLE_DOUBLE_P_TO_BRACE_LEFT='\xff\x00\x80'

# Assume that Russian layout is the second one (index 1),
# change it if you have a different layout order.
RUSSIAN_LAYOUT_ID=1

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

dbus-monitor --profile --session "interface='org.kde.KeyboardLayouts'" |
while read -r line; do
    if [[ "$line" == *"layoutChanged"* ]]; then
        r=$(gdbus call --session \
            --dest org.kde.keyboard \
            --object-path /Layouts \
            --method org.kde.KeyboardLayouts.getLayout)
        layout_id=$(echo $r | grep -oP '\(uint32 \K[^,]+')
        if [ -n "$layout_id" ]; then
            HIDRAW=$(find_hidraw)
            if [ ! -z "$HIDRAW" ]; then
                if [ "$layout_id" -ne "$RUSSIAN_LAYOUT_ID" ]; then
                    # EN
                    (echo -ne $REPORT_ID$DISABLE_DOUBLE_P_TO_BRACE_LEFT > $HIDRAW) >/dev/null 2>&1
                    sleep 0.1
                    (echo -ne $REPORT_ID$DISABLE_DOUBLE_P_TO_BRACE_LEFT > $HIDRAW) >/dev/null 2>&1
                    sleep 0.1
                    (echo -ne $REPORT_ID$DISABLE_DOUBLE_P_TO_BRACE_LEFT > $HIDRAW) >/dev/null 2>&1
                else
                    # RU
                    (echo -ne $REPORT_ID$ENABLE_DOUBLE_P_TO_BRACE_LEFT > $HIDRAW) >/dev/null 2>&1
                    sleep 0.1
                    (echo -ne $REPORT_ID$ENABLE_DOUBLE_P_TO_BRACE_LEFT > $HIDRAW) >/dev/null 2>&1
                    sleep 0.1
                    (echo -ne $REPORT_ID$ENABLE_DOUBLE_P_TO_BRACE_LEFT > $HIDRAW) >/dev/null 2>&1
                fi
            else
                echo "Keyboard not found."
                sleep 1
            fi      
        else
            echo "layout_id not found"
        fi
    fi
done
