/**
 * Copyright (c) 2025 Adrian Siekierka
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

#include <nile/mcu.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include <ws/system.h>
#include "errors.h"
#include "main.h"
#include "cart/mcu.h"
#include "settings.h"
#include "util/math.h"

#define SOH 1
#define EOT 4
#define ACK 6
#define NAK 21
#define CAN 24
#define TIMEOUT_TICKS (75 * 10)
#define TIMEOUT_PACKET_SECONDS 60

static bool mcu_native_cdc_read_block_sync(void __wf_cram* buffer, uint16_t buflen, uint16_t timeout_ticks) {
    volatile uint16_t target_ticks = vbl_ticks + timeout_ticks;

    while (buflen && ((int16_t) (vbl_ticks - target_ticks)) < 0) {
        int bytes_read = nile_mcu_native_cdc_read_sync(buffer, MIN(buflen, 256));
        if (bytes_read > 0) {
            buflen -= bytes_read;
            buffer += bytes_read;
        }
    }

    return !buflen;
}

static int16_t mcu_native_cdc_read_sync(void __wf_cram* buffer, uint16_t buflen, uint16_t timeout_ticks) {
    volatile uint16_t target_ticks = vbl_ticks + timeout_ticks;

    while (((int16_t) (vbl_ticks - target_ticks)) < 0) {
        int bytes_read = nile_mcu_native_cdc_read_sync(buffer, MIN(buflen, 256));
        if (bytes_read > 0)
            return bytes_read;
    }

    return -1;
}

static bool mcu_native_cdc_write_block_sync(const void __wf_cram* buffer, uint16_t buflen, uint16_t timeout_ticks) {
    volatile uint16_t target_ticks = vbl_ticks + timeout_ticks;

    while (buflen && ((int16_t) (vbl_ticks - target_ticks)) < 0) {
        int bytes_written = nile_mcu_native_cdc_write_sync(buffer, MIN(buflen, 256));
        if (bytes_written > 0) {
            buflen -= bytes_written;
            buffer += bytes_written;
        }
    }

    return !buflen;
}

#define SEND_DATA(count) \
    if (!mcu_native_cdc_write_block_sync(data, (count), TIMEOUT_TICKS)) \
        { result = ERR_DATA_TRANSFER_TIMEOUT; goto finish; }

#define RECV_DATA(count) \
    if (!mcu_native_cdc_read_block_sync(data, (count), TIMEOUT_TICKS)) \
        { result = ERR_DATA_TRANSFER_TIMEOUT; goto finish; }

extern uint8_t xmodem_checksum(uint8_t *data);

int xmodem_recv_start(uint32_t *size) {
    uint8_t data[132];
    uint8_t eot_step = 0;
    uint8_t xmodem_idx = 0x01;

    uint16_t bank = 0;
    uint16_t offset = 0;
    int result = 0;
    bool received_soh = false;

    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);
    nile_mcu_native_cdc_clear_sync();
    mcu_native_enter_speed(settings.mcu_spi_speed);

    uint16_t timeout_ticks = 0;
    data[0] = NAK; SEND_DATA(1);
    *size = 0;

    while (true) {
        int bytes_read = mcu_native_cdc_read_sync(data, 132, 75);
        if (bytes_read < 1) {
            timeout_ticks++;
            if (timeout_ticks >= TIMEOUT_PACKET_SECONDS) {
                result = ERR_DATA_TRANSFER_TIMEOUT; goto finish;
            }
            if (!received_soh) {
                data[0] = NAK; SEND_DATA(1);
            }
            continue;
        }

        for (int i = 0; i < bytes_read; i++) {
            if (data[i] == SOH) {
                timeout_ticks = 0;
                received_soh = true;

                // SOH: receive block
                if (i > 0) {
                    // slow path in case SOH was not the first byte; this should never happen
                    memmove(data, data + i, bytes_read - i);
                    bytes_read -= i;
                }

                if (bytes_read < 132) {
                    if (!mcu_native_cdc_read_block_sync(data + bytes_read, 132 - bytes_read, TIMEOUT_TICKS)) {
                        result = ERR_DATA_TRANSFER_TIMEOUT; goto finish;
                    }
                }

                if ((data[1] ^ data[2]) == 0xFF) {
                    if (data[1] == xmodem_idx) {
                        if (data[131] == xmodem_checksum(data + 3)) {
                            // respond with ACK
                            data[0] = ACK; nile_mcu_native_cdc_write_async_start(data, 1);

                            // read next 128 bytes into PSRAM
                            outportw(WS_CART_EXTBANK_RAM_PORT, bank);
                            outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);

                            memcpy(MK_FP(0x1000, offset), data + 3, 128);

                            if (nile_mcu_native_cdc_write_async_finish() != 1) {
                                SEND_DATA(1);
                            }

                            offset += 128;
                            if (!offset) {
                                bank++;
                                if (bank >= 192) {
                                    data[0] = CAN; SEND_DATA(1);
                                    result = ERR_FILE_TOO_LARGE; goto finish;
                                }
                            }

                            xmodem_idx++;
                            break;
                        }
                    } else {
                        // Valid XMODEM packet received, but for the wrong packet.
                        // Give the communication time to resynchronize.
                        received_soh = false;
                        break;
                    }
                }

                // on failure, respond with NAK
                data[0] = NAK; SEND_DATA(1); break;
            } else if (data[i] == CAN) {
                // CAN: cancel transfer
                result = ERR_DATA_TRANSFER_CANCEL; goto finish;
            } else if (data[i] == EOT) {
                // EOT: end of transfer
                // respond with NAK first, then ACK
                data[0] = ((eot_step++) == 0) ? NAK : ACK;
                SEND_DATA(1);
                if (eot_step == 2) goto finish;
            } else {
                // TODO: handle invalid characters
            }
        }
    }

finish:
    if (!result) {
        *size = ((uint32_t)bank << 16) + offset;

        uint16_t rom0_bank = ws_bank_rom0_save(bank - 1);
        uint16_t rom1_bank = ws_bank_rom1_save(bank);

        // access the final 144 bytes of the ROM
        const uint8_t __far *last_bytes = MK_FP(WS_ROM1_SEGMENT + (offset >> 4) - (256 >> 4), 112);

        // the ROM might not have been aligned to 128 bytes
        // try to adjust for this by locating byte 0xEA and maintenance
        int hdr_offset = 128;
        while (hdr_offset >= 0 && (last_bytes[hdr_offset] != 0xEA || (last_bytes[hdr_offset + 5] & 0xF))) hdr_offset--;
        if (hdr_offset >= 0) {
            *size -= (128 - hdr_offset);
        }

        ws_bank_rom0_restore(rom0_bank);
        ws_bank_rom1_restore(rom1_bank);
    }

    outportb(WS_SYSTEM_CTRL_COLOR_PORT, inportb(WS_SYSTEM_CTRL_COLOR_PORT) & ~WS_SYSTEM_CTRL_COLOR_CART_FAST_CLOCK);
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);

    mcu_native_exit_speed();
    mcu_native_finish();

    return result;
}
