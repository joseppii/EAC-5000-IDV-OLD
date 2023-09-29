/*
 * ox08b40_mode_tbls.h - ox08b40 sensor mode tables
 *
 * Copyright (c) 2020-2022, Leopard CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __OX08B40_I2C_TABLES__
#define __OX08B40_I2C_TABLES__

#include <media/camera_common.h>
#include <linux/miscdevice.h>

#define OX08B40_TABLE_WAIT_MS	0
#define OX08B40_TABLE_END	1
#define OX08B40_MAX_RETRIES	3
#define OX08B40_WAIT_MS_STOP	1
#define OX08B40_WAIT_MS_START	30
#define OX08B40_WAIT_MS_STREAM	210
#define OX08B40_GAIN_TABLE_SIZE 255

/* #define INIT_ET_INSETTING 1 */

#define ox08b40_reg struct reg_8

static ox08b40_reg ox08b40_start[] = {

	{0xCC02, 0x10},
	{OX08B40_TABLE_WAIT_MS, OX08B40_WAIT_MS_START},
	{OX08B40_TABLE_WAIT_MS, OX08B40_WAIT_MS_STREAM},
	{ OX08B40_TABLE_END, 0x00 }
};

static ox08b40_reg ox08b40_stop[] = {

	{OX08B40_TABLE_WAIT_MS, OX08B40_WAIT_MS_STOP},
	{OX08B40_TABLE_END, 0x00 }
};

static  ox08b40_reg ox08b40_1920x1020_crop_30fps[] = {

	{OX08B40_TABLE_END, 0x00}
};

enum {
	OX08B40_MODE_1920X1020_CROP_30FPS,
	OX08B40_MODE_START_STREAM,
	OX08B40_MODE_STOP_STREAM,
};

static ox08b40_reg *mode_table[] = {
	[OX08B40_MODE_1920X1020_CROP_30FPS] = ox08b40_1920x1020_crop_30fps,
	[OX08B40_MODE_START_STREAM] = ox08b40_start,
	[OX08B40_MODE_STOP_STREAM] = ox08b40_stop,
};

static const int ox08b40_30fps[] = {
	30,
};

/*
 * WARNING: frmfmt ordering need to match mode definition in
 * device tree!
 */
static const struct camera_common_frmfmt ox08b40_frmfmt[] = {
	{{3840, 2160}, ox08b40_30fps, 1, 0, OX08B40_MODE_1920X1020_CROP_30FPS},
	/* Add modes with no device tree support after below */
};
#endif /* __OX08B40_I2C_TABLES__ */
