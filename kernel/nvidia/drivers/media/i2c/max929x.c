/*
 * max929x.c - max929x IO Expander driver
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

#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <media/camera_common.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <media/gmsl-link.h>
#include "max929x.h"

#define MAX9296_INDEX 	0
#define MAX96712_INDEX 	1
struct max929x {
	struct i2c_client *i2c_client;
	struct regmap *regmap;
	struct gpio_desc *pwdn_gpio;
	struct gpio_desc *poc_en_gpio;
	bool linka_en;
	bool linkb_en;
	bool streaming_en;
	u32 deser_index;
};
struct max929x *extenal_priv[4];

static int max929x_write_reg(struct max929x *priv, u8 slave_addr, u16 reg, u8 val)
{
	struct i2c_client *i2c_client = priv->i2c_client;
	int err;

	i2c_client->addr = slave_addr;
	dev_dbg(&i2c_client->dev, "%s: slave_addr 0x%02x, reg 0x%04x, val 0x%02x\n",
				__func__, slave_addr, reg, val);
	err = regmap_write(priv->regmap, reg, val);
	if (err)
		dev_err(&i2c_client->dev, "%s:slave_addr 0x%x i2c write failed, 0x%x = %x\n",
				__func__, slave_addr, reg, val);

	return err;
}

static int max929x_read_reg(struct max929x *priv, u8 slave_addr, u16 reg, u8 *val)
{
	struct i2c_client *i2c_client = priv->i2c_client;
	int err;
	u32 reg_val;

	i2c_client->addr = slave_addr;

	err = regmap_read(priv->regmap, reg, &reg_val);
	if (err) {
		dev_err(&i2c_client->dev, "%s:i2c read failed, 0x%x\n",
			__func__, reg);
	}
	*val = reg_val & 0xFF;
	return err;
}

int max929x_switch_channel(int channel, int phy_num, bool val)
{
	struct max929x *priv = extenal_priv[channel];
	u8 reg_value;

	if (channel > 2 || channel < 0)
		return -1;

	max929x_read_reg(priv, 0x90, 0x332, &reg_value);
	phy_num = 0x10 << phy_num;
	if (val == true) {
		reg_value = reg_value | phy_num;
	} else {
		reg_value = reg_value & (~phy_num);
	}

	max929x_write_reg(priv, 0x90, 0x332, reg_value);
	return 0;
}
EXPORT_SYMBOL(max929x_switch_channel);

static int max929x_write_reg_list(struct max929x *priv, struct max929x_reg *table, int size)
{
	int err = 0, i;
	u16 reg;
	u8 slave_addr;
	u8 val, retry;

	for (i = 0; i < size; i++) {
		slave_addr = table[i].slave_addr;
		reg = table[i].reg;
		val = table[i].val;
		retry = 3;
		/*retry 3 times*/
		while (retry) {
			err = max929x_write_reg(priv, slave_addr, reg, val);
			if (!err)
				break;
			usleep_range(1000, 1010);
			retry--;
		}
		if (!retry)
			return EBUSY;
		if (reg == 0x0010 || reg == 0x0000)
			msleep(200);
	}
	return 0;
}

int max929x_setup_streaming(u32 deser_index)
{
	struct max929x *priv = extenal_priv[deser_index];
	struct device dev = priv->i2c_client->dev;
	u8 err;
	dev_err(&dev, "%s: Setup Streamingggggggggg Link\n", __func__);
	err = max929x_write_reg_list(priv, max9296_LINKA_Dser_Ser_init,
			sizeof(max9296_LINKA_Dser_Ser_init)/sizeof(struct max929x_reg));
	dev_err(&dev, "%s: Setup Streamingggg\n", __func__);		
	/* Power Down Sensor and ISP */		
    msleep(50);
    max929x_write_reg(priv, 0x62, 0x02d3, 0x80);
    msleep(50);
    max929x_write_reg(priv, 0x62, 0x02d4, 0xa0);
     msleep(50);
    max929x_write_reg(priv, 0x62, 0x02be, 0x80);
    msleep(50);
    max929x_write_reg(priv, 0x62, 0x02bf, 0xa0);

    /* Power UP Sensor and ISP */		
    msleep(50);
    max929x_write_reg(priv, 0x62, 0x02d3, 0x90);
    msleep(50);
    max929x_write_reg(priv, 0x62, 0x02d4, 0x60);
     msleep(50);
    max929x_write_reg(priv, 0x62, 0x02be, 0x90);
    msleep(50);
    max929x_write_reg(priv, 0x62, 0x02bf, 0x60);	
	msleep(200);
	priv->streaming_en = true;
	max929x_write_reg(priv, 0x62, 0x02C7, 0x80);
	msleep(200);
	max929x_write_reg(priv, 0x62, 0x02C7, 0x90);
#ifdef MAX929X_FRAME_SYNC
	err = max929x_write_reg_list(priv, max9296_enable_trigger,
			sizeof(max9296_enable_trigger)/sizeof(struct max929x_reg));
#endif
	return 0;
}
EXPORT_SYMBOL(max929x_setup_streaming);

static  struct regmap_config max929x_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
};

static int max929x_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct device dev = client->dev;
	struct device_node *np;
	struct max929x *priv;
	int gpio;

	dev_err(&dev, "%s: enteringgggggggggggggg\n", __func__);

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	priv->i2c_client = client;
	priv->regmap = devm_regmap_init_i2c(priv->i2c_client, &max929x_regmap_config);
	if (IS_ERR(priv->regmap)) {
		dev_err(&client->dev,
			"regmap init failed: %ld\n", PTR_ERR(priv->regmap));
		return -ENODEV;
	}

	np = dev.of_node;
	if (!np)
		return -EINVAL;

	gpio = of_get_named_gpio(np, "pwdn-gpios", 0);
	if (gpio < 0) {
		dev_err(&client->dev, "pwdn-gpios not in DT\n");
	} else {
		gpio_set_value(gpio, 0);
		msleep(10);
		gpio_set_value(gpio, 1);
		msleep(200);
	}

	msleep(200);

	priv->deser_index = 0;
	device_property_read_u32(&client->dev, "deser-index", &priv->deser_index);
	dev_err(&client->dev, "Deser Index %d\n", priv->deser_index);
	priv->streaming_en = false;
	priv->linka_en = priv->linkb_en = false;
	extenal_priv[priv->deser_index] = priv;

	max929x_setup_streaming(priv->deser_index);
	dev_dbg(&dev, "%s: success\n", __func__);

	return 0;
}

static int max929x_remove(struct i2c_client *client)
{
	struct device dev = client->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return 0;
}

static const struct i2c_device_id max929x_id[] = {
	{ "max929x", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, max929x_id);

const struct of_device_id max929x_of_match[] = {
	{ .compatible = "nvidia,max929x", },
	{ },
};
MODULE_DEVICE_TABLE(of, max929x_of_match);

static struct i2c_driver max929x_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "max929x",
		.of_match_table = of_match_ptr(max929x_of_match),
	},
	.probe = max929x_probe,
	.remove = max929x_remove,
	.id_table = max929x_id,
};

static int __init max929x_init(void)
{
	return i2c_add_driver(&max929x_i2c_driver);
}

static void __exit max929x_exit(void)
{
	i2c_del_driver(&max929x_i2c_driver);
}

module_init(max929x_init);
module_exit(max929x_exit);

MODULE_DESCRIPTION("IO Expander driver max929x");
MODULE_AUTHOR("Yi Xu <yix@leopardimaging.com>");
MODULE_LICENSE("GPL v2");
