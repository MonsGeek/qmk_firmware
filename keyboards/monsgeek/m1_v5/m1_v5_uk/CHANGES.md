# Monsgeek M1 V5 UK — RGB Fixes and Improvements

## Summary

This changeset fixes several long-standing RGB control issues with the Monsgeek M1 V5 UK ISO keyboard firmware. The core problems are: broken reverse mode cycling, a limited color palette locked to 5 fixed HSV presets, and EEPROM defaults not being synchronized with `keyboard.json`, causing unexpected behavior after flashing or clearing EEPROM.

---

## Bug Fixes

### 1. RGB Mode Reverse Cycling (`RGB_RMOD`) Broken

**Problem:** Pressing MODE+ (`RGB_MOD`) cycles forward through a curated subset of RGB effects. However, pressing MODE- (`RGB_RMOD`) falls through to QMK's default `rgb_matrix_step_reverse()`, which cycles through *all* enabled modes in enum order — a completely different ordering. This means going forward one mode and then backward one mode does not return to the previous mode.

**Root cause:** The firmware intercepts `RGB_MOD` with custom logic that cycles through a curated array (`rgbmatrix_buff`), but `RGB_RMOD` had no custom handler.

**Fix:**
- Added `record_rgbmatrix_decrease()` in `rgb_record.c` that walks the same curated array backward.
- Combined `RGB_MOD` and `RGB_RMOD` into one `case` block in `process_record_kb()`, dispatching to the appropriate increase/decrease function.
- Added `RGB_RMOD` to the recording-mode filter exceptions (same treatment as `RGB_MOD`).

**Files:** `m1_v5_uk.c`, `rgb_record/rgb_record.c`, `rgb_record/rgb_record.h`

### 2. HUE+/HUE- Cycling Out of Sync After EEPROM Clear

**Problem:** The default hue in `keyboard.json` (applied by QMK on EEPROM init) and the custom hue *index* (stored at `CONFINFO_EECONFIG_ADDR + 5`, used by HUE+/HUE-) were completely disconnected. After `EE_CLR` or fresh flash, the keyboard displays the correct default color, but the hue index defaults to 0 (Red). Pressing HUE+ then jumps to Orange instead of the next hue after the configured default.

**Fix:**
- Added `find_hue_index()` and `find_sat_index()` helper functions that look up the correct index from the hue/sat arrays based on the QMK-generated `RGB_MATRIX_DEFAULT_HUE` and `RGB_MATRIX_DEFAULT_SAT` defines.
- `eeconfig_confinfo_default()` now writes the correct hue and saturation indices to EEPROM, derived automatically from `keyboard.json` defaults. No hardcoded magic numbers — changing the defaults in `keyboard.json` automatically propagates.

**Files:** `m1_v5_uk.c`, `rgb_record/rgb_record.c`, `rgb_record/rgb_record.h`

---

## Improvements

### 3. Expanded Color Palette (5 HSV Presets → 9 Named Hues × 5 Saturation Levels)

**Problem:** The original firmware had only 5 hardcoded HSV presets (`rgb_hsvs`): Red, Yellow-green, Cyan, Purple, and White. HUE+/HUE- cycled through these with no independent saturation control.

**Improvement:** Replaced with a proper 9-hue × 5-saturation palette:
- **9 hues:** Red (0), Orange (5), Yellow (10), Green (85), Cyan (128), Blue (170), Blue-Purple (191), Purple (213), Magenta (234)
- **5 saturation levels per hue:** Each hue has hand-tuned saturation ramps from a desaturated/white tone to full saturation, stored in `rgb_sats[9][5]`.
- **Hardware LED correction:** The WS2812 LEDs on this keyboard have a disproportionately strong green channel. All saturation ramps in `rgb_sats` are empirically tuned to compensate for this: Green and Cyan hues start at saturation 0 at the lowest level (so the "desaturated" end reads as white instead of green), while Red, Orange, Yellow, and blue-family hues use higher base saturations (127–166) to push past the green bias and produce a convincing neutral/white tone.
- HUE+/HUE- cycles through the 9 hues; SAT+/SAT- cycles through the 5 saturation levels for the current hue.
- Both indices are independently persisted in EEPROM.

**Files:** `rgb_record/rgb_record.c`, `rgb_record/rgb_record.h`, `config.h` (`EECONFIG_CONFINFO_USE_SIZE` increased to accommodate new storage)

### 4. All Enabled RGB Modes in Curated Cycling List

**Problem:** The original curated list contained only 16 of 44+ enabled modes. Many interesting effects (gradient, cycle_all, pixel_flow, reactive variants, splash, etc.) were unreachable via MODE+/MODE-.

**Improvement:** The curated list now includes all 44 enabled modes, with Solid Color first. Only modes not enabled in `keyboard.json` are excluded (flower_blooming, starlight variants). The list now uses QMK enum constants (`RGB_MATRIX_SOLID_COLOR`, etc.) instead of hardcoded numeric IDs, so it is always correct regardless of which modes are enabled or disabled — previously, disabling `flower_blooming` shifted all subsequent mode IDs down by one, causing the last entry in the curated list to accidentally point to the custom `RGBR_PLAY` recording playback mode.

**File:** `rgb_record/rgb_record.c`

### 5. Updated Defaults

- **Default mode:** `solid_color` (was `cycle_left_right`)
- **Default hue:** 234 / Magenta (was 0 / Red)
- **Default saturation:** 255 / full (was 255)
- **Default brightness:** 200 (was 104)
- **Max brightness:** 200 (was 104)

**File:** `keyboard.json`

### 6. USB Suspend/Wake Improvements

- Added `USB_SUSPEND_WAKEUP_DELAY 200` (200ms delay on wake) for reliable resume.
- Added `USB_POWER_DOWN_DELAY 2000` to reduce the suspend detection delay from the default 10s to 2s, so LEDs turn off promptly when the PC sleeps.
- Added `HS_LED_BOOSTING_PIN` control in `suspend_power_down_kb()` (pin low) and `suspend_wakeup_init_kb()` (pin high) to properly disable/enable LED voltage boosting during suspend, reducing power draw.

**Files:** `config.h`, `m1_v5_uk.c`

### 7. Default Keymap: RGB Control Keys on Fn Layers

**Improvement:** Updated both `_FL` (Win) and `_MFL` (Mac) Fn layers to expose all RGB controls from the default keymap, removing the need to load a custom VIA keymap for basic RGB control:
- Fn + PgUp → MODE+ (`RGB_MOD`)
- Fn + PgDn → MODE- (`RGB_RMOD`)
- Fn + < (comma) → HUE- (`RGB_HUD`)
- Fn + > (dot) → HUE+ (`RGB_HUI`)
- Fn + Left → SAT- (`RGB_SAD`)
- Fn + Right → SAT+ (`RGB_SAI`)
- Fn + Up → Brightness+ (`RGB_VAI`, unchanged)
- Fn + Down → Brightness- (`RGB_VAD`, unchanged)

Volume up/down keys that previously occupied `<`/`>` positions were displaced. Volume control remains available via the encoder knob and Fn+M (mute).

**File:** `keymaps/default/keymap.c`

---

## Files Changed

| File | Changes |
|------|---------|
| `config.h` | Increased `EECONFIG_CONFINFO_USE_SIZE`, added `USB_SUSPEND_WAKEUP_DELAY`, `USB_POWER_DOWN_DELAY` |
| `keyboard.json` | Updated default animation, hue, sat, val, max_brightness |
| `keymaps/default/keymap.c` | RGB control keys on Fn layers (MODE±, HUE±, SAT±) |
| `m1_v5_uk.c` | `RGB_RMOD` handling, EEPROM default sync, `RGB_HUI`/`RGB_HUD` hue cycling, suspend pin management |
| `rgb_record/rgb_record.c` | New 9-hue palette, `record_rgbmatrix_decrease()`, `find_hue_index()`, `find_sat_index()`, `record_color_hue()`, updated `record_color_hsv()`, curated mode list uses enum constants |
| `rgb_record/rgb_record.h` | New defines (`RGB_SAT_MAX`, `RGB_HUE_MAX`), function declarations |

## After Flashing

Hold **Fn + `** (backtick) for 3 seconds to trigger `EE_CLR` and reset EEPROM to the new synced defaults. This only needs to be done once after updating.
