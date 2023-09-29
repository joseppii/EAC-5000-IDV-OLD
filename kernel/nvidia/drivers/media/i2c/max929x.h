#ifndef __MAX929X_H__
#define __MAX929X_H__

#define AR0239_TABLE_END 0xffff

struct max929x_reg {
	u16 slave_addr;
	u16 reg;
	u16 val;
};

static struct max929x_reg max9296_LINKA_Dser_Ser_init[] = {
	{0x48, 0x0010, 0x31}, // Apply Reset Oneshot for changes
	{0x62, 0x0010, 0x21},

	{0x62, 0x02c1, 0x80},
	{0x62, 0x0570, 0x0c},
	{0x62, 0x03f1, 0x09},
	{0x62, 0x03f0, 0x59},
	{0x48, 0x0051, 0x02},
	{0x48, 0x0052, 0x01},
	{0x48, 0x0333, 0x1b},
	{0x48, 0x0320, 0x2c},
	{0x62, 0x0318, 0x5e},
	{0x62, 0x02c1, 0x90},
	{0x62, 0x0330, 0x00}, // Set SER to 1x4 mode (phy_config = 0)
	{0x62, 0x0332, 0xee},
	{0x62, 0x0333, 0xe4}, // Additional lane map
	{0x62, 0x0331, 0x31}, // Set 4 lanes for serializer (ctrl1_num_lanes = 3)
	{0x62, 0x0311, 0x20}, // Start video from both port A and port B.
	{0x62, 0x0308, 0x62}, // Enable info lines. Additional start bits for Port A and B. Use data from port B for all pipelines.
	{0x62, 0x0314, 0x22}, // Route 16bit DCG (DT = 0x30) to VIDEO_X (Bit 6 enable)
	{0x62, 0x0316, 0x5e}, // Route 12bit RAW (DT = 0x2C) to VIDEO_Y (Bit 6 enable)
	{0x62, 0x0318, 0x22}, // Route EMBEDDED8 to VIDEO_Z (Bit 6 enable)
	{0x62, 0x031A, 0x22}, // Unused VIDEO_U
	{0x62, 0x0002, 0x22}, // Make sure all pipelines start transmission (VID_TX_EN_X/Y/Z/U = 1)
	{0x48, 0x0330, 0x04}, // Set MIPI Phy Mode: 2x(1x4) mode
	{0x48, 0x0333, 0x4E}, // lane maps - all 4 ports mapped straight
	{0x48, 0x0334, 0xE4}, // Additional lane map
	{0x48, 0x040A, 0x00}, // lane count - 0 lanes striping on controller 0 (Port A slave in 2x1x4 mode).
	{0x48, 0x044A, 0xd0}, // lane count - 4 lanes striping on controller 1 (Port A master in 2x1x4 mode).
	{0x48, 0x048A, 0xd0}, // lane count - 4 lanes striping on controller 2 (Port B master in 2x1x4 mode).
	{0x48, 0x04CA, 0x00}, // lane count - 0 lanes striping on controller 3 (Port B slave in 2x1x4 mode).
	{0x48, 0x031D, 0x2f}, // MIPI clock rate - 1.5Gbps from controller 0 clock (Port A slave in 2x1x4 mode).
	{0x48, 0x0320, 0x2f}, // MIPI clock rate - 1.5Gbps from controller 1 clock (Port A master in 2x1x4 mode).
	{0x48, 0x0323, 0x2f}, // MIPI clock rate - 1.5Gbps from controller 2 clock (Port B master in 2x1x4 mode).
	{0x48, 0x0326, 0x2f}, // MIPI clock rate - 1.5Gbps from controller 2 clock (Port B slave in 2x1x4 mode).
	{0x48, 0x0050, 0x00}, // Route data from stream 0 to pipe X
	{0x48, 0x0051, 0x01}, // Route data from stream 0 to pipe Y
	{0x48, 0x0052, 0x02}, // Route data from stream 0 to pipe Z
	{0x48, 0x0053, 0x03}, // Route data from stream 0 to pipe U
	{0x48, 0x0332, 0xF0}, // Enable all PHYS.
	{0x62, 0x03F1, 0x89}, // Output RCLK to sensor.
	{0x48, 0x0005, 0x00}, //need disable pixel clk out inb order to use MFP1
};

#ifdef MAX929X_FRAME_SYNC
static struct max929x_reg max9296_enable_trigger[] = {
	{0x48, 0x03EF, 0xC0},	// AUTO_FS_LINKS = 0, FS_USE_XTAL = 1, FS_LINK_[3:0] = 0
	{0x48, 0x03E2, 0x00},	// Turn off auto master link selection
	{0xff, 0x0000, 0x60},
	{0x48, 0x03EA, 0x00},	// OVLP window = 0
	{0x48, 0x03EB, 0x00},
	{0x48, 0x0005, 0x00},    //need disable pixel clk out inb order to use MFP1
	{0x48, 0x03E5, 0x9F},	// 10Hz FSYNC
	{0x48, 0x03E6, 0x25},
	{0x48, 0x03E7, 0x26},
	{0x62, 0x02D6, 0x84},	// Enable RX on Ser MFP8
	{0x62, 0x02D8, 0x08},	// MFP8 receive id 8
	{0x48, 0x03F1, 0x40},	// FSYNC TX ID [7:3] is 8
};
#endif
#endif
