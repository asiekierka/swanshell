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

#ifndef CART_RTC_H_
#define CART_RTC_H_

#include <ws.h>
#include <stdint.h>

#define RTC_DATETIME_SIZE 7
#define RTC_DATETIME_STRING_LEN 19

/**
 * Convert an RTC date/time structure to a string buffer of at least size RTC_DATETIME_STRING_LEN.
 */
void rtc_datetime_to_string(char *text, const ws_cart_rtc_datetime_t *dt);
/**
 * Convert an date/time string to an RTC date/time structure.
 *
 * @param rtc_status RTC status - used for distinguishing 12/24 hour mode.
 */
bool rtc_string_to_datetime(const char *text, ws_cart_rtc_datetime_t *dt, uint8_t rtc_status);
/**
 * Calculate the correct day-of-week from a given RTC date/time structure's day/month/year.
 */
void rtc_correct_day_of_week(ws_cart_rtc_datetime_t *dt);
void rtc_adjust_component(ws_cart_rtc_datetime_t *dt, uint8_t rtc_status, int sel_value, int delta);

#endif /* CART_RTC_H_ */
