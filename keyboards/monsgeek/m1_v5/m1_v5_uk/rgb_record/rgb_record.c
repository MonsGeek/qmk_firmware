// Copyright 2024 yangzheng20003 (@yangzheng20003)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rgb_record.h"
#include "rgb_matrix.h"
#include "eeprom.h"

#define RGBREC_STATE_ON  1
#define RGBREC_STATE_OFF 0
#define RGBREC_COLOR_NUM (sizeof(rgbrec_hs_lists) / sizeof(rgbrec_hs_lists[0]))

typedef struct {
    uint8_t state;
    uint8_t channel;
    uint16_t value;
} rgbrec_info_t;

typedef uint16_t (*rgbrec_effects_t)[MATRIX_COLS];

rgbrec_effects_t p_rgbrec_effects = NULL;

static uint16_t rgbrec_hs_lists[] = RGB_RECORD_HS_LISTS;
static uint8_t rgbrec_buffer[MATRIX_ROWS * MATRIX_COLS * 2];

//clang-format off

/* Curated RGB effect mode list for MODE+/MODE- cycling.
 * Uses enum constants so the IDs are always correct regardless of which
 * built-in effects are enabled or disabled in keyboard.json.
 * RGBR_PLAY (the custom recording playback mode) is intentionally excluded. */
static const uint8_t rgbmatrix_buff[] = {
    RGB_MATRIX_SOLID_COLOR,
    RGB_MATRIX_ALPHAS_MODS,
    RGB_MATRIX_GRADIENT_UP_DOWN,
    RGB_MATRIX_GRADIENT_LEFT_RIGHT,
    RGB_MATRIX_BREATHING,
    RGB_MATRIX_BAND_SAT,
    RGB_MATRIX_BAND_VAL,
    RGB_MATRIX_BAND_PINWHEEL_SAT,
    RGB_MATRIX_BAND_PINWHEEL_VAL,
    RGB_MATRIX_BAND_SPIRAL_SAT,
    RGB_MATRIX_BAND_SPIRAL_VAL,
    RGB_MATRIX_CYCLE_ALL,
    RGB_MATRIX_CYCLE_LEFT_RIGHT,
    RGB_MATRIX_CYCLE_UP_DOWN,
    RGB_MATRIX_RAINBOW_MOVING_CHEVRON,
    RGB_MATRIX_CYCLE_OUT_IN,
    RGB_MATRIX_CYCLE_OUT_IN_DUAL,
    RGB_MATRIX_CYCLE_PINWHEEL,
    RGB_MATRIX_CYCLE_SPIRAL,
    RGB_MATRIX_DUAL_BEACON,
    RGB_MATRIX_RAINBOW_BEACON,
    RGB_MATRIX_RAINBOW_PINWHEELS,
    RGB_MATRIX_RAINDROPS,
    RGB_MATRIX_JELLYBEAN_RAINDROPS,
    RGB_MATRIX_HUE_BREATHING,
    RGB_MATRIX_HUE_PENDULUM,
    RGB_MATRIX_HUE_WAVE,
    RGB_MATRIX_PIXEL_RAIN,
    RGB_MATRIX_PIXEL_FLOW,
    RGB_MATRIX_PIXEL_FRACTAL,
    RGB_MATRIX_TYPING_HEATMAP,
    RGB_MATRIX_DIGITAL_RAIN,
    RGB_MATRIX_SOLID_REACTIVE_SIMPLE,
    RGB_MATRIX_SOLID_REACTIVE,
    RGB_MATRIX_SOLID_REACTIVE_WIDE,
    RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE,
    RGB_MATRIX_SOLID_REACTIVE_CROSS,
    RGB_MATRIX_SOLID_REACTIVE_MULTICROSS,
    RGB_MATRIX_SOLID_REACTIVE_NEXUS,
    RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS,
    RGB_MATRIX_SPLASH,
    RGB_MATRIX_MULTISPLASH,
    RGB_MATRIX_SOLID_SPLASH,
    RGB_MATRIX_SOLID_MULTISPLASH,
};

/* Per-hue saturation ramp table: rgb_sats[hue_index][sat_level].
 * Each row provides 5 saturation values from desaturated (white-ish) to fully
 * saturated, hand-tuned to compensate for this keyboard's green-heavy WS2812
 * LEDs. The LEDs have a disproportionately strong green channel, so:
 *   - Green and Cyan hues start at saturation 0 at the low end (otherwise the
 *     "white" tint looks green instead of neutral).
 *   - Red, Orange, Yellow, and blue-family hues use higher base saturation
 *     values (127–166) to push past the green bias and produce a convincing
 *     desaturated/white tone at the lowest level.
 * These values were determined empirically by visual inspection of the actual
 * hardware output. */
static uint8_t rgb_sats[RGB_HUE_MAX][RGB_SAT_MAX] = {
    {127, 220, 238, 245, 255},  // Red      (hue=0)    white→pure red
    {144, 235, 245, 250, 255},  // Orange   (hue=5)    white→pure orange
    {166, 238, 247, 251, 255},  // Yellow   (hue=10)   white→pure yellow
    {  0, 200, 230, 245, 255},  // Green    (hue=85)   greenish-white→pure green
    {  0, 200, 230, 245, 255},  // Cyan     (hue=128)  greenish-white→pure cyan
    {126, 220, 238, 245, 255},  // Blue     (hue=170)  white→pure blue
    {127, 220, 238, 245, 255},  // BluePurp (hue=191)  white→pure blue-purple
    {127, 220, 238, 245, 255},  // Purple   (hue=213)  white→pure purple
    {127, 220, 238, 245, 255},  // Magenta  (hue=234)  white→pure magenta
};

/* Hue values for the 9-color palette, indexed by hue_index.
 * These are QMK HSV hue values (0-255 range). */
static uint8_t rgb_hues[RGB_HUE_MAX] = {0, 5, 10, 85, 128, 170, 191, 213, 234};

//clang-format on
static rgbrec_info_t rgbrec_info = {
    .state   = RGBREC_STATE_OFF,
    .channel = 0,
    .value   = 0xFF,
};

static uint8_t rgbrec_buffer[MATRIX_ROWS * MATRIX_COLS * 2];
extern const uint16_t PROGMEM rgbrec_default_effects[RGBREC_CHANNEL_NUM][MATRIX_ROWS][MATRIX_COLS];

static bool find_matrix_row_col(uint8_t index, uint8_t *row, uint8_t *col) {
    uint8_t i, j;

    for (i = 0; i < MATRIX_ROWS; i++) {
        for (j = 0; j < MATRIX_COLS; j++) {
            if (g_led_config.matrix_co[i][j] != NO_LED) {
                if (g_led_config.matrix_co[i][j] == index) {
                    *row = i;
                    *col = j;

                    return true;
                }
            }
        }
    }

    return false;
}

static inline RGB hs_to_rgb(uint8_t h, uint8_t s) {

    if ((h == 0) && (s == 0)) {
        return hsv_to_rgb((HSV){0, 0, 0});
    } else if ((h == 1) && (s == 1)) {
        return hsv_to_rgb((HSV){0, 0, rgbrec_info.value});
    } else {
        return hsv_to_rgb((HSV){h, s, rgbrec_info.value});
    }
}

void rgbrec_init(uint8_t channel) {

    p_rgbrec_effects    = (rgbrec_effects_t)rgbrec_buffer;
    rgbrec_info.state   = RGBREC_STATE_OFF;
    rgbrec_info.channel = channel;
    rgbrec_info.value   = rgb_matrix_get_val();

    rgbrec_read_current_channel(rgbrec_info.channel);
}

bool rgbrec_show(uint8_t channel) {

    if (channel >= RGBREC_CHANNEL_NUM) {
        return false;
    }

    rgbrec_info.channel = channel;
    rgbrec_read_current_channel(rgbrec_info.channel);

    if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_RGBR_PLAY) {
        rgb_matrix_mode(RGB_MATRIX_CUSTOM_RGBR_PLAY);
    }

    return true;
}

bool rgbrec_start(uint8_t channel) {

    if (channel >= RGBREC_CHANNEL_NUM) {

        return false;
    }

    if (rgbrec_info.state == RGBREC_STATE_OFF) {
        rgbrec_info.state   = RGBREC_STATE_ON;
        rgbrec_info.channel = channel;
        rgbrec_read_current_channel(rgbrec_info.channel);
        if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_RGBR_PLAY) {
            rgb_matrix_mode(RGB_MATRIX_CUSTOM_RGBR_PLAY);
        }

        return true;
    }

    return false;
}

void rgbrec_update_current_channel(uint8_t channel) {
    uint32_t addr = 0;

    if (channel >= RGBREC_CHANNEL_NUM) {
        return;
    }

    addr = (uint32_t)(RGBREC_EECONFIG_ADDR) + (channel * sizeof(rgbrec_buffer));
    eeprom_update_block(rgbrec_buffer, (void *)addr, sizeof(rgbrec_buffer));
}

bool rgbrec_end(uint8_t channel) {

    if (channel >= RGBREC_CHANNEL_NUM) {

        return false;
    }

    if ((rgbrec_info.state == RGBREC_STATE_ON) && (channel == rgbrec_info.channel)) {
        rgbrec_info.state = RGBREC_STATE_OFF;
        rgbrec_update_current_channel(rgbrec_info.channel);

        return true;
    }

    return false;
}

static inline void rgb_matrix_set_hs(int index, uint16_t hs) {

    RGB rgb = hs_to_rgb(HS_GET_H(hs), HS_GET_S(hs));
    rgb_matrix_set_color(index, rgb.r, rgb.g, rgb.b);
}

void rgbrec_play(uint8_t led_min, uint8_t led_max) {
    uint8_t row = 0xFF, col = 0xFF;
    uint16_t hs_color;

    rgbrec_info.value = rgb_matrix_get_val();

    for (uint8_t i = led_min; i < led_max; i++) {
        if (find_matrix_row_col(i, &row, &col)) {
            if (p_rgbrec_effects != NULL) {
                hs_color = p_rgbrec_effects[row][col];
                rgb_matrix_set_hs(i, hs_color);
            }
        }
    }
}

void rgbrec_set_close_all(uint8_t h, uint8_t s, uint8_t v) {

    if (!h && !s && !v) {
        memset(rgbrec_buffer, 0, sizeof(rgbrec_buffer));
    } else {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                rgbrec_buffer[row * MATRIX_COLS * col * 2]       = s;
                rgbrec_buffer[(row * MATRIX_COLS * col * 2) + 1] = v;
            }
        }
    }
}

void rgbrec_read_current_channel(uint8_t channel) {
    uint32_t addr = 0;

    if (channel >= RGBREC_CHANNEL_NUM) {

        return;
    }

    addr = (uint32_t)(RGBREC_EECONFIG_ADDR) + (channel * sizeof(rgbrec_buffer));
    eeprom_read_block(rgbrec_buffer, (void *)addr, sizeof(rgbrec_buffer));
}

bool rgbrec_is_started(void) {

    return (rgbrec_info.state == RGBREC_STATE_ON);
}

static inline void cycle_rgb_next_color(uint8_t row, uint8_t col) {

    if (p_rgbrec_effects == NULL) {

        return;
    }

    for (uint8_t index = 0; index < RGBREC_COLOR_NUM; index++) {
        if (rgbrec_hs_lists[index] == p_rgbrec_effects[row][col]) {
            index                      = ((index + 1) % RGBREC_COLOR_NUM);
            p_rgbrec_effects[row][col] = rgbrec_hs_lists[index];

            return;
        }
    }

    p_rgbrec_effects[row][col] = rgbrec_hs_lists[0];
}

bool rgbrec_register_record(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;

    if (rgbrec_info.state == RGBREC_STATE_ON) {
        cycle_rgb_next_color(record->event.key.row, record->event.key.col);

        return true;
    }

    return false;
}

void eeconfig_init_user_datablock(void) {
    uint32_t addr = 0;

    addr = (uint32_t)(RGBREC_EECONFIG_ADDR);
    eeprom_update_block(rgbrec_default_effects, (void *)addr, sizeof(rgbrec_default_effects));
}

uint8_t find_index(void) {

    for (uint8_t index = 0; index < (sizeof(rgbmatrix_buff) / sizeof(rgbmatrix_buff[0])); index++) {
        if (rgb_matrix_get_mode() == rgbmatrix_buff[index]) {

            return index;
        }
    }

    return 0;
}

/* Read the current saturation index from EEPROM (CONFINFO_EECONFIG_ADDR + 4).
 * Returns 0 if the stored value is out of range (e.g. blank EEPROM). */
uint8_t record_color_read_data(void) {
    const uint8_t *ptr = (const uint8_t *)((uint32_t)CONFINFO_EECONFIG_ADDR + 4);
    uint8_t hs_c       = eeprom_read_byte(ptr);

    if (hs_c >= RGB_SAT_MAX) {
        return 0;
    } else {
        return hs_c;
    }
}

/* Cycle forward through rgbmatrix_buff[]. Preserves the current HSV color
 * across mode changes. Updates *last_mode for EEPROM persistence. */
void record_rgbmatrix_increase(uint8_t *last_mode) {
    uint8_t index;
    HSV current_hsv = rgb_matrix_get_hsv();

    index = find_index();
    if (rgbrec_info.state != RGBREC_STATE_ON) {
        index = (index + 1) % (sizeof(rgbmatrix_buff) / sizeof(rgbmatrix_buff[0]));
    }
    *last_mode = rgbmatrix_buff[index];
    rgb_matrix_mode(rgbmatrix_buff[index]);
    rgb_matrix_sethsv(current_hsv.h, current_hsv.s, current_hsv.v);
}

/* Cycle backward through rgbmatrix_buff[]. Mirror of record_rgbmatrix_increase()
 * so that MODE- reverses MODE+ through the same curated list. */
void record_rgbmatrix_decrease(uint8_t *last_mode) {
    uint8_t index;
    uint8_t count = sizeof(rgbmatrix_buff) / sizeof(rgbmatrix_buff[0]);
    HSV current_hsv = rgb_matrix_get_hsv();

    index = find_index();
    if (rgbrec_info.state != RGBREC_STATE_ON) {
        index = (index + count - 1) % count;
    }
    *last_mode = rgbmatrix_buff[index];
    rgb_matrix_mode(rgbmatrix_buff[index]);
    rgb_matrix_sethsv(current_hsv.h, current_hsv.s, current_hsv.v);
}

/* Cycle saturation level up (status=true) or down (status=false) within the
 * current hue's saturation ramp (rgb_sats[hue][0..4]). Uses the hardware-
 * corrected saturation values rather than linear steps. Returns 0xFF if
 * already at min/max. Persists the new sat index to EEPROM. */
uint8_t record_color_hsv(bool status) {
    uint8_t rgb_sat_index = record_color_read_data();
    uint8_t rgb_hue_index = record_color_hue_read_data();

    if (status) {
        if (rgb_sat_index < (RGB_SAT_MAX - 1))
            rgb_sat_index = (rgb_sat_index + 1);
        else
            return 0xFF;
    } else {
        if (rgb_sat_index)
            rgb_sat_index = (rgb_sat_index - 1);
        else
            return 0xFF;
    }

    rgb_matrix_sethsv(rgb_matrix_get_hsv().h, rgb_sats[rgb_hue_index][rgb_sat_index], rgb_matrix_get_val());

    uint8_t *ptr = (uint8_t *)((uint32_t)CONFINFO_EECONFIG_ADDR + 4);
    eeprom_write_byte(ptr, rgb_sat_index);
    return rgb_sat_index;
}

/* Read the current hue index from EEPROM (CONFINFO_EECONFIG_ADDR + 5).
 * Returns 0 if the stored value is out of range (e.g. blank EEPROM). */
uint8_t record_color_hue_read_data(void) {
    const uint8_t *ptr = (const uint8_t *)((uint32_t)CONFINFO_EECONFIG_ADDR + 5);
    uint8_t hs_c       = eeprom_read_byte(ptr);

    if (hs_c >= RGB_HUE_MAX) {
        return 0;
    } else {
        return hs_c;
    }
}

/* Cycle hue forward (status=true) or backward (status=false) through
 * rgb_hues[]. Also applies the matching saturation from rgb_sats[][] so
 * that switching hues keeps the same perceptual saturation level.
 * Persists the new hue index to EEPROM. */
uint8_t record_color_hue(bool status) {
    uint8_t rgb_hue_index = record_color_hue_read_data();
    uint8_t rgb_sat_index = record_color_read_data();

    if (status) {
        rgb_hue_index = (rgb_hue_index + 1) % RGB_HUE_MAX;
    } else {
        rgb_hue_index = (rgb_hue_index + RGB_HUE_MAX - 1) % RGB_HUE_MAX;
    }

    rgb_matrix_sethsv(rgb_hues[rgb_hue_index], rgb_sats[rgb_hue_index][rgb_sat_index], rgb_matrix_get_val());

    uint8_t *ptr = (uint8_t *)((uint32_t)CONFINFO_EECONFIG_ADDR + 5);
    eeprom_write_byte(ptr, rgb_hue_index);
    return rgb_hue_index;
}

/* Look up the index in rgb_hues[] matching the given QMK hue value.
 * Used by eeconfig_confinfo_default() to sync EEPROM with keyboard.json.
 * Returns 0 (Red) if no exact match is found. */
uint8_t find_hue_index(uint8_t hue) {
    for (uint8_t i = 0; i < RGB_HUE_MAX; i++) {
        if (rgb_hues[i] == hue) return i;
    }
    return 0;
}

/* Find the highest saturation index in rgb_sats[hue_index][] whose value
 * does not exceed the given target sat. Used to map a continuous QMK
 * saturation value to the closest discrete step in our hardware-corrected
 * ramp. Returns 0 if no entry qualifies. */
uint8_t find_sat_index(uint8_t hue_index, uint8_t sat) {
    for (int8_t i = RGB_SAT_MAX - 1; i >= 0; i--) {
        if (rgb_sats[hue_index][i] <= sat) return (uint8_t)i;
    }
    return 0;
}

bool rk_bat_req_flag;

void query(void) {
    if (rk_bat_req_flag) {
#ifdef RGBLIGHT_ENABLE
        for (uint8_t i = 0; i < (RGB_MATRIX_LED_COUNT - RGBLED_NUM); i++) {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
#else
        rgb_matrix_set_color_all(0x00, 0x00, 0x00);
#endif
        for (uint8_t i = 0; i < 10; i++) {
            uint8_t mi_index[10] = RGB_MATRIX_BAT_INDEX_MAP;
            if ((i < (*md_getp_bat() / 10)) || (i < 1)) {
                if (*md_getp_bat() >= (IM_BAT_REQ_LEVEL1_VAL)) {
                    rgb_matrix_set_color(mi_index[i], IM_BAT_REQ_LEVEL1_COLOR);
                } else if (*md_getp_bat() >= (IM_BAT_REQ_LEVEL2_VAL)) {
                    rgb_matrix_set_color(mi_index[i], IM_BAT_REQ_LEVEL2_COLOR);
                } else {
                    rgb_matrix_set_color(mi_index[i], IM_BAT_REQ_LEVEL3_COLOR);
                }
            } else {
                rgb_matrix_set_color(mi_index[i], 0x00, 0x00, 0x00);
            }
        }
    }
}
