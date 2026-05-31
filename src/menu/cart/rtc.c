/**
 * Copyright (c) 2025, 2026 Adrian Siekierka
 *
 * swanshell is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * swanshell is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with swanshell. If not, see <https://www.gnu.org/licenses/>.
 */

#include "rtc.h"
#include "util/util.h"
#include <wsx/bcd.h>

static uint8_t correct_hour_12_to_24(uint8_t value) {
    uint8_t am_pm = value & 0x80;
    uint8_t hour = value & 0x7F;
    if (hour < 0x12 && am_pm) {
        hour = wsx_int_to_bcd8(wsx_bcd8_to_int(hour) + 12);
    }
    return hour;
}

static uint8_t correct_hour_24_to_12(uint8_t value, uint8_t rtc_status) {
    if (value >= 0x12) {
        if (rtc_status & WS_CART_RTC_STATUS_24_HOUR) {
            return value | 0x80;
        } else {
            return wsx_int_to_bcd8(wsx_bcd8_to_int(value) - 12) | 0x80;
        }
    } else {
        return value;
    }
}

void rtc_datetime_to_string(char *text, const ws_cart_rtc_datetime_t *dt) {
    // Handle AM/PM correction in 12-hour mode
    uint8_t hour = correct_hour_12_to_24(dt->time.hour);

    text[0]  = '2';
    text[1]  = '0';
    text[2]  = util_hex_chars[dt->date.year >> 4];
    text[3]  = util_hex_chars[dt->date.year & 0xF];
    text[4]  = '-';
    text[5]  = util_hex_chars[dt->date.month >> 4];
    text[6]  = util_hex_chars[dt->date.month & 0xF];
    text[7]  = '-';
    text[8]  = util_hex_chars[dt->date.day >> 4];
    text[9]  = util_hex_chars[dt->date.day & 0xF];
    text[10] = ' ';
    text[11] = util_hex_chars[hour >> 4];
    text[12] = util_hex_chars[hour & 0xF];
    text[13] = ':';
    text[14] = util_hex_chars[dt->time.minute >> 4];
    text[15] = util_hex_chars[dt->time.minute & 0xF];
    text[16] = ':';
    text[17] = util_hex_chars[dt->time.second >> 4];
    text[18] = util_hex_chars[dt->time.second & 0xF];
    text[19] = 0;
}

static int16_t extract_bcd_value(const char *text, int len) {
    int16_t value = 0;
    while (len--) {
        char c = *(text++);
        if (c < '0' || c > '9')
            return -1;
        value = (value << 4) | (uint8_t)(c - '0');
    }
    return value;
}

static const uint8_t __far days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

bool rtc_string_to_datetime(const char *text, ws_cart_rtc_datetime_t *dt, uint8_t rtc_status) {
    // TODO: Support more date formats
    if (strlen(text) != RTC_DATETIME_STRING_LEN || text[4] != '-' || text[7] != '-' || text[10] != ' ' || text[13] != ':' || text[16] != ':')
        return false;

    int16_t year = extract_bcd_value(text, 4);
    int16_t month = extract_bcd_value(text + 5, 2);
    int16_t day = extract_bcd_value(text + 8, 2);
    int16_t hour = extract_bcd_value(text + 11, 2);
    int16_t minute = extract_bcd_value(text + 14, 2);
    int16_t second = extract_bcd_value(text + 17, 2);

    if (year < 0x2000 || year > 0x2099)
        return false;
    if (month < 0x1 || month > 0x12)
        return false;
    if (day < 0x1 || day > ((month == 2) ? 0x29 : wsx_int_to_bcd8(days_in_month[month - 1])))
        return false;
    if (hour < 0 || hour >= 0x24)
        return false;
    if (minute < 0 || minute >= 0x60)
        return false;
    if (second < 0 || second >= 0x60)
        return false;

    hour = correct_hour_24_to_12(hour, rtc_status);

    dt->date.year = year;
    dt->date.month = month;
    dt->date.day = day;
    dt->time.hour = hour;
    dt->time.minute = minute;
    dt->time.second = second;
    rtc_correct_day_of_week(dt);
    return true;
}

void rtc_correct_day_of_week(ws_cart_rtc_datetime_t *dt) {
    uint8_t year = wsx_bcd8_to_int(dt->date.year);
    uint8_t month = wsx_bcd8_to_int(dt->date.month);
    uint8_t day = wsx_bcd8_to_int(dt->date.day);

    if (year > 99) year = 99;
    if (month > 12) month = 12;
    if (month < 1) month = 1;
    if (day < 1) day = 1;

    // 0 days in 2000, 366 days in 2001, 731 days in 2002, ...
    uint16_t days_before_year = (365 * year) + ((year + 3) / 4);
    uint16_t days_in_year = 0;
    for (int i = 0; i < month - 1; i++)
        days_in_year += days_in_month[i];
    if (month > 2 && !(year & 3)) days_in_year++;
    days_in_year += day - 1;

    // January 1st, 2000 is a Saturday
    uint8_t day_of_week = (days_before_year + days_in_year + 6) % 7;
    dt->date.wday = day_of_week;
}

void rtc_adjust_component(ws_cart_rtc_datetime_t *dt, uint8_t rtc_status, int sel_value, int delta) {
    int min = 0;
    int max = 99;
    uint8_t hour_adj;
    uint8_t *ptr = 0;
    bool high_digit = !(sel_value & 1);

    switch (sel_value >> 1) {
    case 0: ptr = &dt->date.year; break;
    case 1: ptr = &dt->date.month; min = 1; max = 12; break;
    case 2: ptr = &dt->date.day; min = 1; max = 31; break;
    case 3: ptr = &hour_adj; hour_adj = correct_hour_12_to_24(dt->time.hour); max = 23; break;
    case 4: ptr = &dt->time.minute; max = 59; break;
    case 5: ptr = &dt->time.second; max = 59; break;
    }
    if ((sel_value >> 1) == 2) {
        // Day: handle days in month and leap years
        uint8_t month = wsx_bcd8_to_int(dt->date.month);
        uint8_t year = wsx_bcd8_to_int(dt->date.year);
        if (month == 2 && !(year & 3))
            max = 29;
        else if (month >= 1 && month <= 12)
            max = days_in_month[month - 1];
    }

    int old_value = wsx_bcd8_to_int(*ptr);
    if (delta == -999) {
        if (high_digit) {
            *ptr = wsx_int_to_bcd8(min);
            return;
        } else if ((old_value % 10) > 0) {
            delta = -(old_value % 10);
        } else {
            delta = -10;
        }
    } else if (delta == 999) {
        if (high_digit) {
            *ptr = wsx_int_to_bcd8(max);
            return;
        } else if ((old_value % 10) < 9) {
            delta = 9 - (old_value % 10);
        } else {
            delta = 10;
        }
    }

    if (high_digit) {
        delta *= 10;
    }

    int new_value = old_value + delta;
    if (high_digit) {
        if (new_value < min) {
            new_value = (max - (max % 10)) + (old_value % 10);
            while (new_value > max) new_value -= 10;
            if (new_value == old_value) new_value = max;
        } else if (new_value > max) {
            new_value = (min - (min % 10)) + (old_value % 10);
            if (new_value < min) new_value = min;
            if (new_value == old_value) new_value = min;
        }
    } else {
        if (new_value < min) new_value = max;
        else if (new_value > max) new_value = min;
    }

    if (ptr == &hour_adj) {
        dt->time.hour = correct_hour_24_to_12(wsx_int_to_bcd8(new_value), rtc_status);
    } else {
        *ptr = wsx_int_to_bcd8(new_value);
    }
}
