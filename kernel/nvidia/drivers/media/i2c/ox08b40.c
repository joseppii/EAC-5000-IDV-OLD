/*
 * ox08b40.c - ox08b40 sensor driver
 *
 * Copyright (c) 2020-2023, Leopard CORPORATION.  All rights reserved.
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

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include <media/tegra_v4l2_camera.h>
#include <media/tegracam_core.h>
#include "ox08b40_mode_tbls.h"
#define CREATE_TRACE_POINTS
#include <trace/events/ox08b40.h>

#define OX08B40_MIN_FRAME_LENGTH	(1125)
#define OX08B40_MAX_FRAME_LENGTH	(0x1FFFF)
#define OX08B40_MIN_SHS1_1080P_HDR	(5)
#define OX08B40_MIN_SHS2_1080P_HDR	(82)
#define OX08B40_MAX_SHS2_1080P_HDR	(OX08B40_MAX_FRAME_LENGTH - 5)
#define OX08B40_MAX_SHS1_1080P_HDR	(OX08B40_MAX_SHS2_1080P_HDR / 16)

#define OX08B40_FRAME_LENGTH_ADDR_MSB		0x301A
#define OX08B40_FRAME_LENGTH_ADDR_MID		0x3019
#define OX08B40_FRAME_LENGTH_ADDR_LSB		0x3018
#define OX08B40_COARSE_TIME_SHS1_ADDR_MSB	0x3022
#define OX08B40_COARSE_TIME_SHS1_ADDR_MID	0x3021
#define OX08B40_COARSE_TIME_SHS1_ADDR_LSB	0x3020
#define OX08B40_COARSE_TIME_SHS2_ADDR_MSB	0x3025
#define OX08B40_COARSE_TIME_SHS2_ADDR_MID	0x3024
#define OX08B40_COARSE_TIME_SHS2_ADDR_LSB	0x3023
#define OX08B40_GAIN_ADDR					0x3014
#define OX08B40_GROUP_HOLD_ADDR				0x3001
#define OX08B40_SW_RESET_ADDR			0x3003
// extern int max929x_switch_channel(int channel, int phy_num, bool val);
extern int max929x_setup_streaming(u32 channel);

static const struct of_device_id ox08b40_of_match[] = {
	{ .compatible = "nvidia,ox08b40",},
	{ },
};
MODULE_DEVICE_TABLE(of, ox08b40_of_match);

static const u32 ctrl_cid_list[] = {
	TEGRA_CAMERA_CID_GAIN,
	TEGRA_CAMERA_CID_EXPOSURE,
	TEGRA_CAMERA_CID_FRAME_RATE,
	TEGRA_CAMERA_CID_HDR_EN,
	TEGRA_CAMERA_CID_SENSOR_MODE_ID,
};

struct ox08b40 {
	struct i2c_client	*i2c_client;
	struct v4l2_subdev	*subdev;
	u32				frame_length;
	s64 last_wdr_et_val;
	struct camera_common_data	*s_data;
	struct tegracam_device		*tc_dev;
	u32 channel;
	int phy_num;
};

static const struct regmap_config sensor_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
	#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	.use_single_rw = true,
#else
	.use_single_read = true,
	.use_single_write = true,
#endif
};

static inline int ox08b40_read_reg(struct camera_common_data *s_data,
				u16 addr, u8 *val)
{
	int err = 0;
	u32 reg_val = 0;

	err = regmap_read(s_data->regmap, addr, &reg_val);
	*val = reg_val & 0xFF;

	return err;
}

static int ox08b40_write_reg(struct camera_common_data *s_data,
				u16 addr, u8 val)
{
	int err;
	struct device *dev = s_data->dev;

	err = regmap_write(s_data->regmap, addr, val);
	if (err)
		dev_err(dev, "%s: i2c write failed, 0x%x = %x\n",
			__func__, addr, val);

	return err;
}

static int ox08b40_write_table(struct ox08b40 *priv,
				const ox08b40_reg table[])
{
	struct camera_common_data *s_data = priv->s_data;

	return regmap_util_write_table_8(s_data->regmap,
					 table,
					 NULL, 0,
					 OX08B40_TABLE_WAIT_MS,
					 OX08B40_TABLE_END);
}

static int ox08b40_set_group_hold(struct tegracam_device *tc_dev, bool val)
{
	return 0;
}

static int ox08b40_set_gain(struct tegracam_device *tc_dev, s64 val)
{
	return 0;
}

static int ox08b40_set_frame_rate(struct tegracam_device *tc_dev, s64 val)
{
	return 0;
}

static int ox08b40_set_exposure(struct tegracam_device *tc_dev, s64 val)
{
	return 0;
}

static struct tegracam_ctrl_ops ox08b40_ctrl_ops = {
	.numctrls = ARRAY_SIZE(ctrl_cid_list),
	.ctrl_cid_list = ctrl_cid_list,
	.set_gain = ox08b40_set_gain,
	.set_exposure = ox08b40_set_exposure,
	.set_frame_rate = ox08b40_set_frame_rate,
	.set_group_hold = ox08b40_set_group_hold,
};

static int ox08b40_power_on(struct camera_common_data *s_data)
{
	int err = 0;
	struct camera_common_power_rail *pw = s_data->power;
	struct camera_common_pdata *pdata = s_data->pdata;
	struct device *dev = s_data->dev;

	dev_dbg(dev, "%s: power on\n", __func__);
	if (pdata && pdata->power_on) {
		err = pdata->power_on(pw);
		if (err)
			dev_err(dev, "%s failed.\n", __func__);
		else
			pw->state = SWITCH_ON;
		return err;
	}

	pw->state = SWITCH_ON;
	return 0;

}

static int ox08b40_power_off(struct camera_common_data *s_data)
{
	int err = 0;
	struct camera_common_power_rail *pw = s_data->power;
	struct camera_common_pdata *pdata = s_data->pdata;
	struct device *dev = s_data->dev;

	dev_dbg(dev, "%s: power off\n", __func__);

	if (pdata && pdata->power_off) {
		err = pdata->power_off(pw);
		if (!err)
			goto power_off_done;
		else
			dev_err(dev, "%s failed.\n", __func__);
		return err;
	}

power_off_done:
	pw->state = SWITCH_OFF;

	return 0;
}

static int ox08b40_power_get(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	struct camera_common_data *s_data = tc_dev->s_data;
	struct camera_common_power_rail *pw = s_data->power;
	struct camera_common_pdata *pdata = s_data->pdata;
	const char *mclk_name;
	struct clk *parent;
	int err = 0;

	mclk_name = pdata->mclk_name ?
		    pdata->mclk_name : "extperiph1";
	pw->mclk = devm_clk_get(dev, mclk_name);
	if (IS_ERR(pw->mclk)) {
		dev_err(dev, "unable to get clock %s\n", mclk_name);
		return PTR_ERR(pw->mclk);
	}

	parent = devm_clk_get(dev, "pllp_grtba");
	if (IS_ERR(parent))
		dev_err(dev, "devm_clk_get failed for pllp_grtba");
	else
		clk_set_parent(pw->mclk, parent);


	pw->state = SWITCH_OFF;
	return err;
}

static int ox08b40_power_put(struct tegracam_device *tc_dev)
{
	struct camera_common_data *s_data = tc_dev->s_data;
	struct camera_common_power_rail *pw = s_data->power;

	if (unlikely(!pw))
		return -EFAULT;

	return 0;
}

static struct camera_common_pdata *ox08b40_parse_dt(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	struct device_node *np = dev->of_node;
	struct camera_common_pdata *board_priv_pdata;
	const struct of_device_id *match;
	int err;

	if (!np)
		return NULL;

	match = of_match_device(ox08b40_of_match, dev);
	if (!match) {
		dev_err(dev, "Failed to find matching dt id\n");
		return NULL;
	}

	board_priv_pdata = devm_kzalloc(dev,
					sizeof(*board_priv_pdata), GFP_KERNEL);
	if (!board_priv_pdata)
		return NULL;

	err = of_property_read_string(np, "mclk",
				      &board_priv_pdata->mclk_name);
	if (err)
		dev_err(dev, "mclk not in DT\n");

	return board_priv_pdata;
}

static int ox08b40_set_mode(struct tegracam_device *tc_dev)
{
	struct ox08b40 *priv = (struct ox08b40 *)tegracam_get_privdata(tc_dev);
	struct camera_common_data *s_data = tc_dev->s_data;
	struct device *dev = tc_dev->dev;
	const struct of_device_id *match;
	int err;

	match = of_match_device(ox08b40_of_match, dev);
	if (!match) {
		dev_err(dev, "Failed to find matching dt id\n");
		return -EINVAL;
	}

	err = ox08b40_write_table(priv, mode_table[s_data->mode_prop_idx]);
	if (err)
		return err;

	return 0;
}

static int ox08b40_start_streaming(struct tegracam_device *tc_dev)
{
	struct ox08b40 *priv = (struct ox08b40 *)tegracam_get_privdata(tc_dev);
	int err;

	err = ox08b40_write_table(priv,
		mode_table[OX08B40_MODE_START_STREAM]);
	if (err)
		return err;

	return 0;
}

static int ox08b40_stop_streaming(struct tegracam_device *tc_dev)
{
	// struct camera_common_data *s_data = tc_dev->s_data;
	struct ox08b40 *priv = (struct ox08b40 *)tegracam_get_privdata(tc_dev);
	int err;

	err = ox08b40_write_table(priv, mode_table[OX08B40_MODE_STOP_STREAM]);
	if (err)
		return err;

	usleep_range(1000, 1010);

	return 0;
}


static struct camera_common_sensor_ops ox08b40_common_ops = {
	.numfrmfmts = ARRAY_SIZE(ox08b40_frmfmt),
	.frmfmt_table = ox08b40_frmfmt,
	.power_on = ox08b40_power_on,
	.power_off = ox08b40_power_off,
	.write_reg = ox08b40_write_reg,
	.read_reg = ox08b40_read_reg,
	.parse_dt = ox08b40_parse_dt,
	.power_get = ox08b40_power_get,
	.power_put = ox08b40_power_put,
	.set_mode = ox08b40_set_mode,
	.start_streaming = ox08b40_start_streaming,
	.stop_streaming = ox08b40_stop_streaming,
};

static int ox08b40_board_setup(struct ox08b40 *priv)
{
	struct camera_common_data *s_data = priv->s_data;
	struct device *dev = s_data->dev;
	int err = 0;

	dev_dbg(dev, "%s++\n", __func__);

	err = camera_common_mclk_enable(s_data);
	if (err) {
		dev_err(dev,
			"Error %d turning on mclk\n", err);
		return err;
	}

	err = ox08b40_power_on(s_data);
	if (err) {
		dev_err(dev,
			"Error %d during power on sensor\n", err);
		return err;
	}

	ox08b40_power_off(s_data);
	camera_common_mclk_disable(s_data);
	return err;
}

static int ox08b40_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	dev_dbg(&client->dev, "%s:\n", __func__);

	return 0;
}

static const struct v4l2_subdev_internal_ops ox08b40_subdev_internal_ops = {
	.open = ox08b40_open,
};

static int ox08b40_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct tegracam_device *tc_dev;
	struct ox08b40 *priv;
	int err;

	dev_info(dev, "probing v4l2 sensor\n");

	if (!IS_ENABLED(CONFIG_OF) || !client->dev.of_node)
		return -EINVAL;

	priv = devm_kzalloc(dev,
			sizeof(struct ox08b40), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	tc_dev = devm_kzalloc(dev,
			sizeof(struct tegracam_device), GFP_KERNEL);
	if (!tc_dev)
		return -ENOMEM;

	priv->i2c_client = tc_dev->client = client;
	tc_dev->dev = dev;
	strncpy(tc_dev->name, "ox08b40", sizeof(tc_dev->name));
	tc_dev->dev_regmap_config = &sensor_regmap_config;
	tc_dev->sensor_ops = &ox08b40_common_ops;
	tc_dev->v4l2sd_internal_ops = &ox08b40_subdev_internal_ops;
	tc_dev->tcctrl_ops = &ox08b40_ctrl_ops;

	err = tegracam_device_register(tc_dev);
	if (err) {
		dev_err(dev, "tegra camera driver registration failed\n");
		return err;
	}
	priv->tc_dev = tc_dev;
	priv->s_data = tc_dev->s_data;
	priv->subdev = &tc_dev->s_data->subdev;
	tegracam_set_privdata(tc_dev, (void *)priv);

	err = ox08b40_board_setup(priv);
	if (err) {
		tegracam_device_unregister(tc_dev);
		dev_err(dev, "board setup failed\n");
		return err;
	}

	err = tegracam_v4l2subdev_register(tc_dev, true);
	if (err) {
		dev_err(dev, "tegra camera subdev registration failed\n");
		return err;
	}

	dev_info(dev, "Detected OX08B40 sensor\n");

	return 0;
}

static int
ox08b40_remove(struct i2c_client *client)
{
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct ox08b40 *priv = (struct ox08b40 *)s_data->priv;

	tegracam_v4l2subdev_unregister(priv->tc_dev);
	tegracam_device_unregister(priv->tc_dev);

	return 0;
}

static const struct i2c_device_id ox08b40_id[] = {
	{ "ox08b40", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ox08b40_id);

static struct i2c_driver ox08b40_i2c_driver = {
	.driver = {
		.name = "OX08b40",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ox08b40_of_match),
	},
	.probe = ox08b40_probe,
	.remove = ox08b40_remove,
	.id_table = ox08b40_id,
};

module_i2c_driver(ox08b40_i2c_driver);

MODULE_DESCRIPTION("Media Controller driver for Sony OX08B40");
MODULE_AUTHOR("Yi Xu <yix@leopardimaging.com>");
MODULE_LICENSE("GPL v2");
