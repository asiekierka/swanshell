/**
 * cluster_read - Minimal cluster-based file I/O library.
 *
 * Based on code from ChaN's FatFs.
 *
 * Copyright (C) 2022, ChaN, all right reserved.
 *
 * FatFs module is an open source software. Redistribution and use of FatFs in
 * source and binary forms, with or without modification, are permitted provided
 * that the following condition is met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this condition and the following disclaimer.
 *
 * This software is provided by the copyright holder and contributors "AS IS"
 * and any warranties related to this software are DISCLAIMED.
 * The copyright owner or contributors be NOT LIABLE for any damages caused
 * by use of this software.
 */

#ifndef _CLUSTER_READ_H_
#define _CLUSTER_READ_H_

#include <stdint.h>
#include <wonderful.h>

void cluster_open(uint32_t cluster);
uint8_t cluster_read(void __far* buf, uint16_t len);

#endif /* _CLUSTER_READ_H_ */
