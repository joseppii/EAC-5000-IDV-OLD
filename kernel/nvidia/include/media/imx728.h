/*
 * imx728_.h - imx728 sensor header
 *
 * Copyright (C) 2023, Leopardimaging Inc.
 * Based on Copyright (c) 2021-2022, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef __IMX728_H__
#define __IMX728_H__

/* imx728 - sensor parameters */

#define IMX728_MIN_FRAME_LENGTH		        (256)
#define IMX728_MAX_FRAME_LENGTH		        (65535)

/* imx728 sensor register address */

#define IMX728_FRAME_LENGTH_ADDR_MSB		0x6140
#define IMX728_FRAME_LENGTH_ADDR_LSB		0x6141

#endif /* __IMX728_H__ */
