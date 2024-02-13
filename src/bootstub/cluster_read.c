/**
 * cluster_read - Minimal cluster-read I/O library.
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "bootstub.h"
#include "cluster_read.h"

#define FF_WF_FAST_CONTIGUOUS_READ 1

/* Drive window management */

static uint32_t drive_window_sector = -1;
static uint8_t drive_window[512];

static uint8_t move_window(uint32_t sector) {
	if (sector != drive_window_sector) {
		uint8_t result = disk_read(0, drive_window, sector, 1);
		if (result)
			return result;
		drive_window_sector = sector;
	}
	return 0;
}

/* Cluster table read */

static inline bool is_valid_cluster(uint32_t cluster) {
	return !(cluster < 2 || cluster >= bootstub_data->fat_entry_count);
}

static uint32_t read_next_cluster(uint32_t cluster) {
	uint32_t result = 0xFFFFFFFF;
	uint16_t i;
	if (!is_valid_cluster(cluster)) {
        return 1;
    }
    switch (bootstub_data->fs_type) {
        case FS_FAT12:
            cluster += cluster >> 1;
            if (move_window(bootstub_data->cluster_table_base + (cluster >> 9))) break;
            i = drive_window[cluster++ & 0x1FF];
            if (move_window(bootstub_data->cluster_table_base + (cluster >> 9))) break;
            i |= drive_window[cluster & 0x1FF] << 4;
            result = (cluster & 1) ? (i >> 4) : (i & 0xFFF);
            break;
        case FS_FAT16:
            if (move_window(bootstub_data->cluster_table_base + (cluster >> 8))) break;
            result = ((uint16_t*) drive_window)[cluster & 0xFF];
            break;
        case FS_FAT32:
            if (move_window(bootstub_data->cluster_table_base + (cluster >> 7))) break;
            result = ((uint32_t*) drive_window)[cluster & 0x7F] & 0x0FFFFFFF;
            break;
    }
	return result;
}

/* Cluster read code */

static uint32_t current_cluster;
static uint32_t current_sector;
static uint32_t current_file_ptr;

static LBA_t clst2sect (	/* !=0:Sector number, 0:Failed (invalid cluster#) */
	DWORD clst		/* Cluster# to be converted */
)
{
	clst -= 2;		/* Cluster number is origin from 2 */
	if (clst >= bootstub_data->fat_entry_count - 2) return 0;		/* Is it invalid cluster number? */
	return bootstub_data->data_base + (LBA_t)bootstub_data->cluster_size * clst;	/* Start sector number of the cluster */
}

void cluster_open(uint32_t cluster) {
    current_cluster = cluster;
	current_sector = clst2sect(current_cluster);
    current_file_ptr = 0;
}

uint8_t cluster_read(void __far* buff, uint16_t btr) {
	DWORD clst;
	LBA_t sect;
	UINT rcnt, cc, csect;
	BYTE __far* rbuff = (BYTE __far*)buff;
#if FF_WF_FAST_CONTIGUOUS_READ
	UINT contiguous_cluster_size;
	DWORD nextclst, prevclst;
#endif

	for ( ; btr > 0; btr -= rcnt, rbuff += rcnt, current_file_ptr += rcnt) {	/* Repeat until btr bytes read */
		if ((current_file_ptr & 0x1FF) == 0) {			/* On the sector boundary? */
			csect = (UINT)((current_file_ptr >> 9) & (bootstub_data->cluster_size - 1));	/* Sector offset in the cluster */
			if (csect == 0) {					/* On the cluster boundary? */
				if (current_file_ptr != 0) {			/* On the top of the file? */
					clst = read_next_cluster(current_cluster);
				} else {
					clst = current_cluster;
				}
				if (clst < 2) return FR_INT_ERR;
				if (clst == 0xFFFFFFFF) return FR_DISK_ERR;
				current_cluster = clst;				/* Update current cluster */
			}
			sect = clst2sect(current_cluster);	/* Get current sector */
			if (sect == 0) return FR_INT_ERR;
			sect += csect;
			cc = btr >> 9;					/* When remaining bytes >= sector size, */
			if (cc > 0) {						/* Read maximum contiguous sectors directly */
#if FF_WF_FAST_CONTIGUOUS_READ
				contiguous_cluster_size = bootstub_data->cluster_size; /* Contiguous cluster size, in sectors */
				nextclst = current_cluster;
				while (csect + cc > contiguous_cluster_size) {
					prevclst = nextclst;
                    nextclst = read_next_cluster(prevclst);	/* Follow cluster chain on the FAT */
					if (nextclst != (prevclst + 1)) break; /* Next cluster must be contiguous */
					contiguous_cluster_size += bootstub_data->cluster_size;
					current_cluster = nextclst;
				}
				if (csect + cc > contiguous_cluster_size) {	/* Clip at cluster boundary */
					cc = contiguous_cluster_size - csect;
				}
#else
				if (csect + cc > bootstub_data->cluster_size) {	/* Clip at cluster boundary */
					cc = bootstub_data->cluster_size - csect;
				}
#endif
				if (disk_read(0, rbuff, sect, cc) != RES_OK) return FR_DISK_ERR;
				rcnt = cc << 9;				/* Number of bytes transferred */
				continue;
			}
            current_sector = sect;
		}
		rcnt = 512 - (current_file_ptr & 0x1FF);	/* Number of bytes remains in the sector */
		if (rcnt > btr) rcnt = btr;					/* Clip it by btr if needed */
		if (move_window(current_sector) != FR_OK) return FR_DISK_ERR;	/* Move sector window */
		_fmemcpy(rbuff, drive_window + (current_file_ptr & 0x1FF), rcnt);	/* Extract partial sector */
	}

    return FR_OK;
}
