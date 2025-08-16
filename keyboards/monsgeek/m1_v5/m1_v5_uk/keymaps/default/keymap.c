// Copyright 2024 yangzheng20003 (@yangzheng20003)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// M1 V5 UK Default Keymap
// Features:
// - Windows/Mac system toggle with smart key swapping
// - Gaming mode (WASD → arrow keys)
// - RGB recording and playback
// - Control app mode (Right Ctrl → App key)
// - Mac-specific Siri activation

#include <sys/resource.h>
#include QMK_KEYBOARD_H
#include "rgb_record/rgb_record.h"
#include "config.h"

// RGB record variables
static uint32_t rec_time;
static bool no_record_fg;
HSV start_hsv;
RGB rgb_test_open;

extern void record_rgbmatrix_increase(uint8_t *last_mode);
void rgb_matrix_hs_set_remain_time(uint8_t index, uint8_t remain_time);


// External typedef and variable declarations
typedef union {
    uint32_t raw;
    struct {
        uint8_t flag : 1;
        uint8_t devs : 3;
        uint8_t record_channel : 4;
        uint8_t record_last_mode;
        uint8_t last_btdevs : 3;
        uint8_t dir_flag : 1;
        uint8_t ctrl_app_flag : 1;
        uint8_t is_mac_mode : 1;
        uint8_t swap_lalt_lgui : 1;
        uint8_t swap_ralt_rgui : 1;
    };
} user_config_t;

user_config_t user_config;

// Helper functions for reading/writing user config from EEPROM
void user_config_read(void) {
    // Read the 32-bit config from the beginning of the datablock
    user_config.raw = eeprom_read_dword((uint32_t *)EECONFIG_USER_DATABLOCK);
}

void user_config_write(void) {
    // Write the 32-bit config to the beginning of the datablock
    eeprom_update_dword((uint32_t *)EECONFIG_USER_DATABLOCK, user_config.raw);
}

void rgb_blink_dir(void) {
    rgb_matrix_hs_indicator_set(0xFF, (RGB){0, 0, 0}, 250, 1);
}

enum layers {
    BASE = 0,
    MAC_FN,
    WIN_FN,
    XTRA,
};

enum custom_keycodes {
    // System and user preference keycodes
    SYS_TOG = SAFE_RANGE,  // Toggle Windows/Mac mode
    RL_MOD,                   // Cycle RGB lighting modes
    HS_SIRI,                  // Activate Siri (Mac)

    // RGB recording and playback
    RP_P0,                    // RGB pattern slot 0
    RP_P1,                    // RGB pattern slot 1
    RP_P2,                    // RGB pattern slot 2
    RP_END,                   // Start/stop recording

    // Gaming and accessibility
    HS_DIR,                   // Toggle gaming mode (WASD → arrows)
    HS_CT_A,                  // Control app mode toggle

    // Custom layer switching
    FN_LAYER,                 // Dynamic function layer switch
};

// Custom keycodes are now defined in m1_v5_uk.h

#define _______ KC_TRNS
#define ________ HS_BLACK

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [BASE] = LAYOUT( /* Base */
        KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_DEL,   KC_MUTE,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,  KC_HOME,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,            KC_PGUP,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_BSLS,  KC_ENT,   KC_PGDN,
        KC_LSFT,  KC_NUBS,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  KC_UP,    KC_END,
        KC_LCTL,  KC_LGUI,  KC_LALT,                      KC_SPC,                                 KC_RALT,  FN_LAYER, KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [MAC_FN] = LAYOUT( /* Mac Function */
        _______,  KC_BRID,  KC_BRIU,  KC_MCTL,  HS_SIRI,  _______,  _______,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  RGB_MOD,  _______,
        EE_CLR,   _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  RGB_SPD,  RGB_SPI,  _______,  _______,
        _______,  _______,  HS_DIR,   KC_BT1,   KC_BT2,   KC_BT3,   KC_2G4,   KC_USB,   KC_INS,   _______,  KC_PSCR,  _______,  _______,            _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  RGB_TOG,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  KC_CALC,  _______,  _______,  _______,  KC_MUTE,  KC_VOLD,  KC_VOLU,  _______,  MO(XTRA), RGB_VAI,  _______,
        SYS_TOG,  _______,  _______,                      HS_BATQ,                                _______,  _______,  HS_CT_A,  RGB_SAI,  RGB_VAD,  RGB_SAD),

    [WIN_FN] = LAYOUT( /* Windows Function */
        _______,  KC_MYCM,  KC_MAIL,  KC_WSCH,  KC_WHOM,  KC_MSEL,  KC_MPLY,  KC_MPRV,  KC_MNXT,  _______,  _______,  _______,  _______,  RGB_MOD,  _______,
        EE_CLR,   _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  RGB_SPD,  RGB_SPI,  _______,  _______,
        _______,  _______,  HS_DIR,   KC_BT1,   KC_BT2,   KC_BT3,   KC_2G4,   KC_USB,   KC_INS,   _______,  KC_PSCR,  _______,  _______,            _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  RGB_TOG,  _______,  _______, _______,  _______,  _______,
        _______,  _______,  _______,  _______,  KC_CALC,  _______,  _______,  _______,  KC_MUTE,  KC_VOLD,  KC_VOLU,  _______,  MO(XTRA), RGB_VAI,  _______,
        SYS_TOG,  GU_TOGG,  _______,                      HS_BATQ,                                _______,  _______,  HS_CT_A,  RGB_SAI,  RGB_VAD,  RGB_SAD),

    [XTRA] = LAYOUT( /* Extra */
        QK_BOOT,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  BT_TEST,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,                      _______,                                _______,  _______,  _______,  _______,  _______,  _______)

};

const uint16_t PROGMEM rgbrec_default_effects[RGBREC_CHANNEL_NUM][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
       HS_GREEN, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________,
       ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________,
       ________, ________, HS_GREEN, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________,           ________,
       ________, HS_GREEN, HS_GREEN, HS_GREEN, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________,
       ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, HS_GREEN, ________,
       ________, ________, ________,                     ________,                               ________, ________, ________, HS_GREEN, HS_GREEN, HS_GREEN),

    [1] = LAYOUT(
       ________, HS_RED,   HS_RED,   HS_RED,   ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________,
       ________, HS_RED,   HS_RED,   HS_RED,   HS_RED,   HS_RED,   ________, ________, ________, ________, ________, ________, ________, ________, ________,
       HS_RED,   HS_RED,   HS_RED,   HS_RED,   HS_RED,   ________, ________, ________, ________, ________, ________, ________, ________,           ________,
       ________, HS_RED,   HS_RED,   HS_RED,   ________, HS_RED,   ________, ________, ________, ________, ________, ________, ________, ________, ________,
       HS_RED,   ________, ________, ________, ________, HS_RED,   ________, ________, ________, ________, ________, ________, ________, ________, ________,
       HS_RED,   ________, HS_RED,                       ________,                               ________, ________, ________, ________, ________, ________),

    [2] = LAYOUT(
       HS_BLUE,  ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________, ________,
       ________, HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  ________, ________, ________, ________, ________, ________, ________,
       ________, HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  ________, ________, ________, ________, ________, ________, ________,           ________,
       ________, HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  HS_BLUE,  ________, ________, ________, ________, ________, ________, ________, ________, ________,
       ________, ________, ________, HS_BLUE,  HS_BLUE,  ________, ________, ________, ________, ________, ________, ________, ________, ________, ________,
       ________, ________, ________,                     ________,                               ________, ________, ________, ________, ________, ________),
};

// ============================================================================
// KEYMAP STATE VARIABLES
// ============================================================================

static uint32_t hs_ct_time = 0;  // Timer for 3-second activation

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [BASE]   = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MAC_FN] = {ENCODER_CCW_CW(_______, _______)},
    [WIN_FN] = {ENCODER_CCW_CW(_______, _______)},
    [XTRA]   = {ENCODER_CCW_CW(_______, _______)},
};
#endif
// clang-format on

// Simple user functions that can be overridden for keymap-specific behavior
bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    if (rgbrec_is_started()) {
        switch (keycode) {
            case RP_P0:
            case RP_P1:
            case RP_P2:
            case RP_END:
            case RGB_MOD:
            break;
            default:
                if (!IS_QK_MOMENTARY(keycode) && record->event.pressed) {
                    rgbrec_register_record(keycode, record);
                }
                return false;
        }
    }

    switch (keycode) {
        case SYS_TOG:
            if (record->event.pressed) {
                user_config.is_mac_mode = !user_config.is_mac_mode;
                // Auto-swap GUI and Alt keys based on system
                if (user_config.is_mac_mode) {
                    rgb_matrix_hs_set_remain_time(HS_RGB_BLINK_INDEX_WIN, 0);
                    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_MAC, (RGB){RGB_WHITE}, 250, 3);
                    // Mac: Cmd(GUI) should be closer to spacebar
                    keymap_config.swap_lalt_lgui = true;
                    keymap_config.swap_ralt_rgui = true;
                } else {
                    rgb_matrix_hs_set_remain_time(HS_RGB_BLINK_INDEX_MAC, 0);
                    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_WIN, (RGB){RGB_WHITE}, 250, 3);
                    // Windows: Alt should be closer to spacebar
                    keymap_config.swap_lalt_lgui = false;
                    keymap_config.swap_ralt_rgui = false;
                }
                user_config_write();
                rgb_blink_dir();
            }
            return false;

        case NK_TOGG:
            if (record->event.pressed) {
                rgb_matrix_hs_indicator_set(0xFF, (RGB){0x00, 0x6E, 0x00}, 250, 1);
            }
            return true;

        case RP_P0:
            if (record->event.pressed) {
                user_config.record_channel = 0;
                rgbrec_read_current_channel(user_config.record_channel);
                rgbrec_end(user_config.record_channel);
                user_config_write();
                rgbrec_show(user_config.record_channel);
            }
            return false;

        case RP_P1:
            if (record->event.pressed) {
                user_config.record_channel = 1;
                rgbrec_read_current_channel(user_config.record_channel);
                rgbrec_end(user_config.record_channel);
                user_config_write();
                rgbrec_show(user_config.record_channel);
            }
            return false;

        case RP_P2:
            if (record->event.pressed) {
                user_config.record_channel = 2;
                rgbrec_read_current_channel(user_config.record_channel);
                rgbrec_end(user_config.record_channel);
                user_config_write();
                rgbrec_show(user_config.record_channel);
            }
            return false;

        case RP_END:
            if (record->event.pressed) {
                if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_RGBR_PLAY) {

                    return false;
                }
                if (!rgbrec_is_started()) {
                    rgbrec_start(user_config.record_channel);
                    no_record_fg = false;
                    rec_time     = timer_read32();
                    rgbrec_set_close_all(HSV_BLACK);
                } else {
                    rec_time = 0;
                    rgbrec_end(user_config.record_channel);
                }
            }
            return false;
        case RGB_SAI:
            if (record->event.pressed) {
                uint8_t index;
                index = record_color_hsv(true);
                if ((index != 0xFF)) {
                    rgb_blink_dir();
                }

            }
            return false;
       case RGB_SAD:
            if (record->event.pressed) {
                uint8_t index;
                index = record_color_hsv(false);
                if (index != 0xFF) {
                    rgb_blink_dir();
                }
            }
            return false;
        case RGB_MOD:
            if (record->event.pressed) {
                rgb_blink_dir();
                if (rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_RGBR_PLAY) {
                    if (rgbrec_is_started()) {
                        rgbrec_read_current_channel(user_config.record_channel);
                        rgbrec_end(user_config.record_channel);
                        no_record_fg = false;
                    }
                    if (user_config.record_last_mode != 0xFF)
                        rgb_matrix_mode(user_config.record_last_mode);
                    else
                        rgb_matrix_mode(RGB_MATRIX_DEFAULT_MODE);
                    user_config_write();
                    start_hsv = rgb_matrix_get_hsv();
                    return false;
                }
                record_rgbmatrix_increase(&(user_config.record_last_mode));
                user_config_write();
                start_hsv = rgb_matrix_get_hsv();
            }

        case RL_MOD:
            if (record->event.pressed) {
                // Cycle through RGB matrix modes
                uint8_t mode = rgb_matrix_get_mode();
                if (mode < RGB_MATRIX_EFFECT_MAX - 1) {
                    rgb_matrix_mode(mode + 1);
                } else {
                    rgb_matrix_mode(1);
                }
            }
            return false;

        case HS_SIRI:
            if (record->event.pressed) {
                register_code(KC_LCMD);
                register_code(KC_SPC);
                wait_ms(20);
            } else {
                unregister_code(KC_SPC);
                unregister_code(KC_LCMD);
            }
            return false;

        case HS_DIR:
            if (record->event.pressed) {
                user_config.dir_flag = !user_config.dir_flag;
                rgb_test_open     = hsv_to_rgb((HSV){.h = 0, .s = 0, .v = RGB_MATRIX_VAL_STEP * 5});
                rgb_matrix_hs_indicator_set(0xFF, (RGB){rgb_test_open.r, rgb_test_open.g, rgb_test_open.b}, 250, 1);
                user_config_write();
            }
            return false;
        case HS_CT_A:
            if (record->event.pressed) {
                hs_ct_time = timer_read32();
            } else {
                hs_ct_time = 0;
            }
            return false;

        // ============================================================================
        // GAMING MODE: WASD → Arrow keys when dir_flag is active
        // ============================================================================
        case KC_A:
            if (user_config.dir_flag) {
                if (record->event.pressed) {
                    register_code16(KC_LEFT);
                } else {
                    unregister_code16(KC_LEFT);
                }
                return false;
            }
            return true;

        case KC_S:
            if (user_config.dir_flag) {
                if (record->event.pressed) {
                    register_code16(KC_DOWN);
                } else {
                    unregister_code16(KC_DOWN);
                }
                return false;
            }
            return true;

        case KC_D:
            if (user_config.dir_flag) {
                if (record->event.pressed) {
                    register_code16(KC_RGHT);
                } else {
                    unregister_code16(KC_RGHT);
                }
                return false;
            }
            return true;

        case KC_W:
            if (user_config.dir_flag) {
                if (record->event.pressed) {
                    register_code16(KC_UP);
                } else {
                    unregister_code16(KC_UP);
                }
                return false;
            }
            return true;

        // ============================================================================
        // CONTROL APP MODE: Right Ctrl → App key when flag is active
        // ============================================================================
        case KC_RCTL:
            if (user_config.ctrl_app_flag) {
                if (record->event.pressed) {
                    register_code16(KC_APP);
                } else {
                    unregister_code16(KC_APP);
                }
                return false;
            }
            return true;
        case KC_RGUI:
            if (user_config.swap_lalt_lgui) {
                if (record->event.pressed) {
                    register_code16(KC_LEFT_GUI);
                } else {
                    unregister_code16(KC_LEFT_GUI);
                }
                return false;
            }
            return true;
        case KC_RALT:
            if (user_config.swap_ralt_rgui) {
                if (record->event.pressed) {
                    register_code16(KC_LEFT_ALT);
                } else {
                    unregister_code16(KC_LEFT_ALT);
                }
                return false;
            }
            return true;

        case FN_LAYER:
            if (record->event.pressed) {
                if (user_config.is_mac_mode) {
                    layer_on(MAC_FN);
                } else {
                    layer_on(WIN_FN);
                }
            } else {
                layer_off(MAC_FN);
                layer_off(WIN_FN);
            }
            return false;
    }

    return true;
}

void matrix_scan_user(void) {
    // Handle HS_CT_A timeout - toggle ctrl_app_flag after 3 seconds
    if (timer_elapsed32(hs_ct_time) > 3000 && hs_ct_time) {
        user_config.ctrl_app_flag = !user_config.ctrl_app_flag;
        rgb_test_open          = hsv_to_rgb((HSV){.h = 0, .s = 0, .v = RGB_MATRIX_VAL_STEP * 5});
        rgb_matrix_hs_indicator_set(0xFF, (RGB){rgb_test_open.r, rgb_test_open.g, rgb_test_open.b}, 250, 1);
        user_config_write();
        hs_ct_time = 0;
    }
}

void keyboard_post_init_user(void) {
    user_config_read();
    // Initialize RGB recording system
    rgbrec_init(user_config.record_channel);
    start_hsv = rgb_matrix_get_hsv();
}

bool hs_reset_settings_user(void) {
    // Reset all keymap-specific settings to defaults
    user_config.is_mac_mode = false;
    user_config.dir_flag = false;
    user_config.ctrl_app_flag = false;
    hs_ct_time = 0;

    // Reset key swapping to Windows defaults
    user_config.swap_lalt_lgui = false;
    user_config.swap_ralt_rgui = false;
    user_config_write();

    return true;
}

bool dip_switch_update_mask_user(uint32_t state) {
    // Check switch position based on both pins
    // Mac mode: C15 (bit 0) high AND C14 (bit 1) low
    bool win_pin = state & (1UL << 0);  // C15
    bool mac_pin = state & (1UL << 1);  // C14

    user_config.is_mac_mode = win_pin && !mac_pin;

    if (user_config.is_mac_mode) {
        // Mac mode: swap GUI and Alt (Cmd closer to spacebar)
        user_config.swap_lalt_lgui = true;
        user_config.swap_ralt_rgui = true;
        rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_MAC, (RGB){RGB_WHITE}, 250, 3);
    } else {
        // Windows mode: normal layout (Alt closer to spacebar)
        user_config.swap_lalt_lgui = false;
        user_config.swap_ralt_rgui = false;
        rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_WIN, (RGB){RGB_WHITE}, 250, 3);
    }

    user_config_write();

    return true;
}
