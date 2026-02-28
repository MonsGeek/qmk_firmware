// Copyright 2024 yangzheng20003 (@yangzheng20003)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"
// RGB indicator functions for visual feedback
void rgb_matrix_hs_indicator_set(uint8_t index, RGB rgb, uint32_t interval, uint8_t times);
void rgb_matrix_hs_set_remain_time(uint8_t index, uint8_t remain_time);

// Battery query flag for RGB record functionality
extern bool rk_bat_req_flag;
