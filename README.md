# uConsole Keyboard Firmware by Alexey Cluster

This is a clean port of the official uConsole keyboard firmware from [ClockworkPi's repository](https://github.com/clockworkpi/uConsole/tree/master/Code/uconsole_keyboard), rewritten to use **pure STM32 HAL** instead of the messy Arduino/STM32duino framework.


## Why This Exists

The original firmware relies on STM32duino and various Arduino libraries with complex dependencies. This makes it difficult to:
- Edit and compile the firmware directly on the uConsole itself
- Customize key mappings and functionality
- Understand what's actually happening under the hood
- Maintain and debug the code

This port completely eliminates all that Arduino nonsense and uses clean STM32 HAL code. Now you can easily edit, compile, and flash the keyboard firmware right on your uConsole, remapping keys and customizing functionality however you want.


## What's Included

**Fully ported features:**
- Full keyboard scanning
- Trackball handling
- Backlight brightness control via PWM with auto-dimming
- USB HID keyboard, mouse, and gamepad support
- USB Consumer controls (volume, brightness)

**What's changed/added compared to the original firmware:**
- **Layer system** - up to 10 configurable keyboard layouts, switchable via `LeftCtrl`+`RightCtrl`+`Number`
- **Horizontal scrolling** - Fn + Trackball scrolls horizontally as well as vertically
- **Keyboard backlight dimming after inactivity** - configurable in `config.h`
- **Trackball acceleration curve** - improved mouse control
- **Fn Lock** - toggle to permanently invert Fn for F1–F12 keys (press `Fn`+`Esc` to toggle; backlight blinks to confirm)


## Configuration

### Layers

The firmware supports up to **10 keyboard layers**. A layer is a complete set of key bindings and trackball settings. Think of it as separate keyboard profiles that you can switch between on the fly.

**Why is this useful?** You can have one layer for everyday typing, another optimized for gaming with gamepad button mappings and faster trackball, a third for a specific application, and so on — all without reflashing the firmware. Each layer can override any key, Fn combination, and trackball speed/acceleration independently.

**How to switch layers:** press `LeftCtrl`+`RightCtrl`+`Number` (e.g. `LeftCtrl`+`RightCtrl`+`1` for layer 1, `LeftCtrl`+`RightCtrl`+`2` for layer 2, `LeftCtrl`+`RightCtrl`+`3` for layer 3, etc.). The keyboard backlight will blink to indicate the layer number.

**Inheritance:** you only need to define what is *different* on each layer. Any key or setting not explicitly set on a layer is automatically inherited from layer 1. This means layer 2 can override just a handful of buttons and everything else will work exactly as on layer 1.

#### Default bindings

The firmware ships with three pre-configured layers. Layer 1 is the main layer with a standard QWERTY layout. Most keys are mapped as you would expect (Q→Q, W→W, etc.), so only the non-obvious bindings are listed below.

##### Layer 1 — non-standard key assignments

| Key | Action | Notes |
|-----|--------|-------|
| Select | Keypad − | Handy for file managers (MC, far2l) |
| Start | Keypad + | Handy for file managers (MC, far2l) |
| Volume | Volume Down | |
| Shift + Volume | Volume Up | |
| Trackball click | Mouse Middle | |
| GP A | Enter | |
| GP B | Left Meta/Cmd | Convenient for Meta+key shortcuts |
| GP X | Mouse Right Click | Use the mouse with one hand |
| GP Y | Mouse Left Click | Use the mouse with one hand |

##### Layer 1 — Fn combinations

| Fn + Key | Action | Notes |
|----------|--------|-------|
| Fn + Volume | Mute | |
| Fn + Esc | Fn Lock | Toggle F1–F12 without holding Fn |
| Fn + Tab | Caps Lock | |
| Fn + 1–0 | F1–F10 | |
| Fn + − | F11 | |
| Fn + = | F12 | |
| Fn + U | Page Up | |
| Fn + I | Insert | |
| Fn + H | Home | |
| Fn + J | End | |
| Fn + K | Page Down | |
| Fn + Backspace | Delete | |
| Fn + Space | Toggle backlight | |
| Fn + , | Brightness Down | |
| Fn + . | Brightness Up | |
| Fn + ↑ | Page Up | |
| Fn + ↓ | Page Down | |
| Fn + ← | Home | |
| Fn + → | End | |
| Fn + Select | Print Screen | |
| Fn + Start | Pause | |
| Fn + Left Alt | Application/Menu | |
| Fn + GP X | Mouse Forward | Useful in browsers and IDEs |
| Fn + GP Y | Mouse Back | Useful in browsers and IDEs |
| Fn + GP A | Keypad * | Handy for file managers (MC, far2l) |
| Fn + GP B | Keypad / | Handy for file managers (MC, far2l) |

##### Layer 2 — Game layer

Trackball speed is increased and **acceleration is disabled** on this layer (linear cursor movement). The four face buttons send **numeric keypad keys** in a numpad-like cluster (7 / 9 on the top row, 1 / 3 on the bottom), which pairs well with games or tools that expect keypad input. Fn combinations for these keys are **not** redefined here; they stay the same as on layer 1 unless you add `BIND_FN` overrides in `layers.h`.

| Key | Action |
|-----|--------|
| GP A | Keypad 9 |
| GP B | Keypad 7 |
| GP X | Keypad 3 |
| GP Y | Keypad 1 |

All other keys on layer 2 are inherited from layer 1.

##### Layer 3 — Gamepad layer

Same trackball tuning as layer 2 (higher speed, no acceleration). The following buttons are remapped to **virtual gamepad** controls; Fn combinations for these keys are disabled on this layer. The advantage of a virtual gamepad over keyboard keys is that there is **no limit on simultaneous button presses** — all gamepad buttons can be held at the same time, which is critical for gaming.

| Key | Action |
|-----|--------|
| Select | Gamepad Button 5 |
| Start | Gamepad Button 6 |
| ↑ ↓ ← → | Gamepad D-Pad |
| GP A | Gamepad Button 1 |
| GP B | Gamepad Button 2 |
| GP X | Gamepad Button 3 |
| GP Y | Gamepad Button 4 |

Switch to this layer with `LeftCtrl`+`RightCtrl`+`3`. All other keys on layer 3 are inherited from layer 1.

#### Customizing layers

All layer configuration lives in a single file: `layers.h`. To change bindings or add new layers, edit this file and rebuild the firmware.

**Basic syntax:**

```c
LAYER(1);                               // Start defining layer 1

BIND(BUTTON_Q, KEY_Q);                  // Q key produces 'Q'
BIND(BUTTON_GAMEPAD_A, MOUSE_LEFT);     // GP A button produces mouse left click
BIND_FN(BUTTON_1, KEY_F1);              // Fn+1 produces F1
BIND_FN(BUTTON_ESC, SK_KEYBOARD_LOCK);  // Fn+Esc toggles Fn Lock
```

**Trackball settings (per layer):**

```c
TRACKBALL_SPEED(50);                          // Cursor speed divisor (higher = faster)
TRACKBALL_ACCELERATION(0.3f);                 // Acceleration exponent (0 = linear, higher = more exponential)
TRACKBALL_SCROLL_VERTICAL_SPEED(100);         // Vertical scroll speed
TRACKBALL_SCROLL_VERTICAL_ACCELERATION(0.3f); // Vertical scroll acceleration
TRACKBALL_SCROLL_HORIZONTAL_SPEED(100);       // Horizontal scroll speed
TRACKBALL_SCROLL_HORIZONTAL_ACCELERATION(0.3f); // Horizontal scroll acceleration
```

**Adding a new layer** — just add a `LAYER(N)` block at the end of the file. You only need to specify the keys and settings that differ from layer 1:

```c
LAYER(4);
TRACKBALL_SPEED(200);                   // Faster cursor on this layer
BIND(BUTTON_SPACE, KEY_ENTER);          // Space produces Enter
BIND_FN(BUTTON_SPACE, KEY_NONE);        // Disable Fn+Space on this layer
```

**Available key codes:** see `Core/Inc/hid_keyboard.h` for keyboard keys, `Core/Inc/hid_gamepad.h` for gamepad buttons, and `Core/Inc/keymaps.h` for button names. Mouse buttons: `MOUSE_LEFT`, `MOUSE_RIGHT`, `MOUSE_MIDDLE`, `MOUSE_BACK`, `MOUSE_FORWARD`. Consumer keys: `CONSUMER_VOLUME_UP`, `CONSUMER_VOLUME_DOWN`, `CONSUMER_MUTE`, `CONSUMER_BRIGHTNESS_UP`, `CONSUMER_BRIGHTNESS_DOWN`. Special keys: `SK_FN_KEY`, `SK_KEYBOARD_LOCK` (Fn Lock), `SK_KEYBOARD_LIGHT` (toggle backlight). Use `KEY_NONE` to explicitly disable a key.

**Fn combination fallback:** if an Fn combination is not defined for a key, pressing Fn+key will simply produce the normal (non-Fn) action of that key.


##### Keyboard hardware layout

Keep in mind that the uConsole keyboard has two types of keys:

**Matrix keys (8x8 = 64 keys):** all letter, number, and symbol keys (Q, W, E, ..., 1, 2, 3, ..., Esc, Tab, Space, Enter, Backspace, etc.), as well as Select, Start, Volume, Fn Left, and Fn Right. These are wired in a matrix — the MCU scans rows and columns to detect which key is pressed. This is an efficient way to connect many keys with fewer GPIO pins, but it has a downside: when three or more matrix keys in certain combinations are pressed simultaneously, the controller may register a "phantom" key press that wasn't actually pressed. This is known as **ghosting**. In practice, this is rarely a problem during normal typing.

**Non-matrix keys (17 keys):** these are wired directly to individual GPIO pins on the MCU — each key has its own dedicated pin. This means they can all be pressed simultaneously without any ghosting or conflicts. The non-matrix keys are:

| Key | Notes |
|-----|-------|
| Shift Left, Shift Right | Modifier keys |
| Ctrl Left, Ctrl Right | Modifier keys |
| Alt Left, Alt Right | Modifier keys |
| ↑ ↓ ← → | Arrow keys / D-pad |
| GP A, GP B, GP X, GP Y | Gamepad buttons (top right side of the device) |
| Trackball click | Pressing the trackball |
| Mouse L, Mouse R | Mouse buttons (near the D-pad) |

This hardware design ensures that modifier keys (Shift, Ctrl, Alt) and gaming controls (D-pad, GP buttons) always register correctly, no matter how many of them are held at the same time.


### Other hardware and behavior settings: `config.h`

| Setting | Description |
|---------|-------------|
| `KEYBOARD_BACKLIGHT_OFF_TIME` | Seconds of inactivity before backlight dims (0 = never) |
| `KEYBOARD_BACKLIGHT_DIM_OUT_DURATION` | Dim-out animation duration in ms |
| `KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL` | Resume backlight on trackball movement (0/1) |
| `GLIDER_ENABLED` | Cursor inertia: 1 = coast to stop, 0 = stop instantly |
| `GLIDER_SUSTAIN_MAX_MS` | Max ms at full speed before braking |
| `GLIDER_SUSTAIN_SPEED_SCALE` | Sustain scales with speed (higher = fast flicks coast longer) |
| `GLIDER_DECAY_FACTOR_PER_MS` | Braking speed (0.80 = fast stop, 0.95 = long slide) |
| `GLIDER_SPEED_EPSILON` | Speed threshold to stop completely |


## Building

Simply use `make`:

```bash
# Build the firmware
make all

# Clean build files
make clean

# Flash the firmware
make flash
```

**Important for first-time flashing:**

For the **first firmware flash**, you need to use one of these methods:

1. **Official flashing script**: Use the official ClockworkPi flashing script from [uconsole_keyboard_flash.tar.gz](https://github.com/clockworkpi/uConsole/raw/master/Bin/uconsole_keyboard_flash.tar.gz) (replace `uconsole_keyboard.ino.bin` with the new firmware)

2. **Hardware switch**: Use the switch on the back of the keyboard to switch it to update/flashing mode

After the first flash, you can use `make flash`. The command will wait for you to manually enter DFU mode by pressing Fn+X combination on the keyboard.


## License

Based on the original uConsole keyboard firmware by ClockworkPi.


## Support the Developer and the Project

* [GitHub Sponsors](https://github.com/sponsors/ClusterM)
* [Buy Me A Coffee](https://www.buymeacoffee.com/cluster)
* [Sber](https://messenger.online.sberbank.ru/sl/Lnb2OLE4JsyiEhQgC)
* [Donation Alerts](https://www.donationalerts.com/r/clustermeerkat)
* [Boosty](https://boosty.to/cluster)
