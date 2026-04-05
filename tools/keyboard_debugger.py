#!/usr/bin/env python3
"""
Read all IN reports from uConsole keyboard via hidraw and decode by Report ID.

Discovery matches tools/keyboard_state.sh: walk sysfs from hidraw to USB idVendor/idProduct.

Report layout follows USB_DEVICE/App/usbd_custom_hid_if.c (IDs 1–5).
"""

from __future__ import annotations

import argparse
import glob
import os
import struct
import sys
import time
import struct

DEVICE_VID = "1eaf"
DEVICE_PID = "0024"

# USB HID keyboard modifier bits (order in first byte of report 1)
MODIFIER_NAMES = (
    "LCtrl",
    "LShift",
    "LAlt",
    "LGui",
    "RCtrl",
    "RShift",
    "RAlt",
    "RGui",
)

# Common USB HID key usages (page 0x07) for report 1 key array
HID_KEY_NAMES: dict[int, str] = {
    0x00: "(none)",
    0x04: "A",
    0x05: "B",
    0x06: "C",
    0x07: "D",
    0x08: "E",
    0x09: "F",
    0x0A: "G",
    0x0B: "H",
    0x0C: "I",
    0x0D: "J",
    0x0E: "K",
    0x0F: "L",
    0x10: "M",
    0x11: "N",
    0x12: "O",
    0x13: "P",
    0x14: "Q",
    0x15: "R",
    0x16: "S",
    0x17: "T",
    0x18: "U",
    0x19: "V",
    0x1A: "W",
    0x1B: "X",
    0x1C: "Y",
    0x1D: "Z",
    0x1E: "1",
    0x1F: "2",
    0x20: "3",
    0x21: "4",
    0x22: "5",
    0x23: "6",
    0x24: "7",
    0x25: "8",
    0x26: "9",
    0x27: "0",
    0x28: "Enter",
    0x29: "Esc",
    0x2A: "Backspace",
    0x2B: "Tab",
    0x2C: "Space",
    0x2D: "-",
    0x2E: "=",
    0x2F: "[",
    0x30: "]",
    0x31: "\\",
    0x33: "'",
    0x34: "`",
    0x35: ",",
    0x36: ".",
    0x37: "/",
    0x39: "CapsLock",
    0x3A: "F1",
    0x3B: "F2",
    0x3C: "F3",
    0x3D: "F4",
    0x3E: "F5",
    0x3F: "F6",
    0x40: "F7",
    0x41: "F8",
    0x42: "F9",
    0x43: "F10",
    0x44: "F11",
    0x45: "F12",
    0x4F: "Right",
    0x50: "Left",
    0x51: "Down",
    0x52: "Up",
    0xE0: "LCtrl",
    0xE1: "LShift",
    0xE2: "LAlt",
    0xE3: "LGui",
    0xE4: "RCtrl",
    0xE5: "RShift",
    0xE6: "RAlt",
    0xE7: "RGui",
}

CONSUMER_BITS = (
    "Volume+",
    "Volume-",
    "Mute",
    "Brightness+",
    "Brightness-",
    "ScanNext",
    "ScanPrev",
    "Stop",
    "PlayPause",
)


def find_hidraw_dev(vid: str, pid: str) -> str | None:
    """Return /dev/hidrawN for matching USB VID:PID (lowercase hex, no 0x)."""
    vid_l = vid.lower()
    pid_l = pid.lower()
    for hid_sys in sorted(glob.glob("/sys/class/hidraw/hidraw*")):
        dev_link = os.path.join(hid_sys, "device")
        if not os.path.exists(dev_link):
            continue
        p = os.path.realpath(dev_link)
        while p and p != "/":
            vfile = os.path.join(p, "idVendor")
            pfile = os.path.join(p, "idProduct")
            if os.path.isfile(vfile) and os.path.isfile(pfile):
                with open(vfile, encoding="ascii") as f:
                    ven = f.read().strip().lower()
                with open(pfile, encoding="ascii") as f:
                    prod = f.read().strip().lower()
                if ven == vid_l and prod == pid_l:
                    return "/dev/" + os.path.basename(hid_sys)
                break
            p = os.path.dirname(p)
    return None


def hex_line(data: bytes) -> str:
    return data.hex(" ")


def decode_report_1_keyboard(payload: bytes) -> str:
    # ID + 1 modifier + 1 reserved + 6 keycodes = 9 bytes
    if len(payload) < 9:
        return f"short ({len(payload)} B): {hex_line(payload)}"
    mods = payload[1]
    keys = payload[3:9]
    active = [MODIFIER_NAMES[i] for i in range(8) if mods & (1 << i)]
    mod_str = ",".join(active) if active else "(none)"
    key_parts = []
    for k in keys:
        if k == 0:
            continue
        key_parts.append(HID_KEY_NAMES.get(k, f"0x{k:02X}"))
    key_str = ",".join(key_parts) if key_parts else "(none)"
    return f"modifiers=[{mod_str}] keys=[{key_str}]"


def decode_report_2_mouse(payload: bytes) -> str:
    # ID + btn(5)+pad(3) + X + Y + wheel + pan = 6 bytes
    if len(payload) < 6:
        return f"short ({len(payload)} B): {hex_line(payload)}"
    b0 = payload[1]
    buttons = [str(i + 1) for i in range(5) if b0 & (1 << i)]
    btn_s = ",".join(buttons) if buttons else "(none)"
    x, y, wh, pan = struct.unpack_from("bbbb", payload, 2)
    return f"buttons=[{btn_s}] X={x:+d} Y={y:+d} wheel={wh:+d} pan={pan:+d}"


def decode_report_3_consumer(payload: bytes) -> str:
    # ID + 2 bytes (9 usage bits + padding)
    if len(payload) < 3:
        return f"short ({len(payload)} B): {hex_line(payload)}"
    u16 = payload[1] | (payload[2] << 8)
    active = [CONSUMER_BITS[i] for i in range(9) if u16 & (1 << i)]
    return f"bits=[{','.join(active) if active else '(none)'}] raw=0x{u16:03X}"


def decode_report_4_gamepad(payload: bytes) -> str:
    # ID + X + Y + 8 buttons in one byte
    if len(payload) < 4:
        return f"short ({len(payload)} B): {hex_line(payload)}"
    x, y = struct.unpack_from("bb", payload, 1)
    b = payload[3]
    btns = [str(i + 1) for i in range(8) if b & (1 << i)]
    btn_s = ",".join(btns) if btns else "(none)"
    return f"X={x:+d} Y={y:+d} buttons=[{btn_s}]"


def decode_report_5_vendor(payload: bytes) -> str:
    payload = payload[1:]
    return " ".join([f"{payload[i]:02X}" for i in range(len(payload))])


DECODERS = {
    0x01: ("Keyboard", decode_report_1_keyboard),
    0x02: ("Mouse", decode_report_2_mouse),
    0x03: ("Consumer", decode_report_3_consumer),
    0x04: ("Gamepad", decode_report_4_gamepad),
    0x05: ("Vendor", decode_report_5_vendor),
}


def decode_packet(data: bytes) -> str:
    if not data:
        return "(empty)"
    rid = data[0]
    if rid in DECODERS:
        name, fn = DECODERS[rid]
        return f"ID={rid:#04x} ({name}) {fn(data)}"
    return f"ID={rid:#04x} (unknown) raw={hex_line(data)}"


def main() -> int:
    ap = argparse.ArgumentParser(description="Decode hidraw IN reports from uConsole keyboard.")
    ap.add_argument(
        "--device",
        "-d",
        metavar="PATH",
        help="hidraw device (default: auto-detect by VID/PID)",
    )
    ap.add_argument(
        "--vid",
        default=DEVICE_VID,
        help=f"USB vendor id hex without 0x (default: {DEVICE_VID})",
    )
    ap.add_argument(
        "--pid",
        default=DEVICE_PID,
        help=f"USB product id hex without 0x (default: {DEVICE_PID})",
    )
    ap.add_argument(
        "--timestamp",
        "-t",
        action="store_true",
        help="prefix each line with monotonic time",
    )
    args = ap.parse_args()

    if sys.platform != "linux":
        print("This tool expects Linux hidraw.", file=sys.stderr)
        return 1

    dev = args.device or find_hidraw_dev(args.vid, args.pid)
    if not dev:
        print(
            f"Keyboard not found ({args.vid}:{args.pid}). Connect device or use --device.",
            file=sys.stderr,
        )
        return 1

    print(f"Reading {dev} (Ctrl+C to stop)", file=sys.stderr)

    # One report per read(); buffer large enough for longest report (keyboard 9 B).
    bufsize = 64

    try:
        with open(dev, "rb", buffering=0) as f:
            while True:
                chunk = f.read(bufsize)
                if not chunk:
                    break
                prefix = f"{time.monotonic():.3f} " if args.timestamp else ""
                print(prefix + decode_packet(chunk))
    except KeyboardInterrupt:
        print("", file=sys.stderr)
        return 0
    except OSError as e:
        print(f"{dev}: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
