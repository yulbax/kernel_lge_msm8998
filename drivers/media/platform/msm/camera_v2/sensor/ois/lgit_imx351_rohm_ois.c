/* ================================================================== */
/*  OIS firmware */
/* ================================================================== */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/types.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <soc/qcom/camera2.h>
#include <linux/poll.h>
#include "msm_sd.h"
#include "msm_cci.h"
#include "msm_ois.h"
#include "msm_ois_i2c.h"

#define LAST_UPDATE "17-07-21, 13M LGIT OLAF OIS bu24235"

/*If changed FW, change below FW bin and Checksum information*/
#define JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_BIN_1 "bu24235_dl_program_Joan_LGITAct_ICG1020S_rev0_S_data1.bin"
#define JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_BIN_2 "bu24235_dl_program_Joan_LGITAct_ICG1020S_rev0_S_data2.bin"
#define JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_BIN_1 "bu24235_dl_program_Joan_LGITAct_ICG1020S_rev3_S_data1.bin"
#define JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_BIN_2 "bu24235_dl_program_Joan_LGITAct_ICG1020S_rev3_S_data2.bin"
#define JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_BIN_1 "bu24235_dl_program_Joan_LGITAct_ICG1020S_rev5_S_data1.bin"
#define JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_BIN_2 "bu24235_dl_program_Joan_LGITAct_ICG1020S_rev5_S_data2.bin"

#define JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_CHECKSUM	0x00014555
#define JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_CHECKSUM	0x00014AB9
#define JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_CHECKSUM	0x00014523

/*If changed FW, change above FW bin and Checksum information*/

#define E2P_FIRST_ADDR			(0xA90)
#define E2P_DATA_BYTE_FIRST		(48)
#define E2P_MAP_VER_ADDR        (0xBE2)
#define CAL_VER					(0xAC2)

#define CTL_END_ADDR_FOR_FIRST_E2P_DL	(0x1DC0)
#define CTL_END_ADDR_FOR_SECOND_E2P_DL	(0x1DDA)

#define OIS_START_DL_ADDR		(0xF010)
#define OIS_COMPLETE_DL_ADDR	(0xF006)
#define OIS_READ_STATUS_ADDR	(0x6024)
#define OIS_CHECK_SUM_ADDR		(0xF008)

#define LIMIT_STATUS_POLLING	(15)
#define LIMIT_OIS_ON_RETRY		(5)

#define GYRO_SCALE_FACTOR 175
#define HALL_SCALE_FACTOR 8192
#define GYRO_GAIN_LGIT 6900

#define NUM_GYRO_SAMLPING (10)

static int16_t g_gyro_offset_value_x, g_gyro_offset_value_y;
static bool vcm_check = 0;
static int8_t pwm_prev_mode = -1;
extern uint16_t cal_ver;
extern uint16_t map_ver;

/*If changed FW, change below FW bin and Checksum information*/

static struct ois_i2c_bin_list JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_BIN_DATA = {
	.files = 2,
	.entries = {
		{
			.filename = JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_BIN_1,
			.filesize = 0x0338, //824byte
			.blocks = 1,
			.addrs = {
				{0x0000, 0x0337, 0x0000},
			}
		},
		{
			.filename = JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_BIN_2,
			.filesize = 0x01C0,
			.blocks = 1,
			.addrs = {
				{0x0000, 0x01BF, 0x1C00},
			}
		},
	},
	.checksum = JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_CHECKSUM
};

static struct ois_i2c_bin_list JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_BIN_DATA = {
	.files = 2,
	.entries = {
		{
			.filename = JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_BIN_1,
			.filesize = 0x0338, //824byte
			.blocks = 1,
			.addrs = {
				{0x0000, 0x0337, 0x0000},
			}
		},
		{
			.filename = JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_BIN_2,
			.filesize = 0x01C0,
			.blocks = 1,
			.addrs = {
				{0x0000, 0x01BF, 0x1C00},
			}
		},
	},
	.checksum = JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_CHECKSUM
};

static struct ois_i2c_bin_list JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_BIN_DATA = {
	.files = 2,
	.entries = {
		{
			.filename = JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_BIN_1,
			.filesize = 0x0338, //824byte
			.blocks = 1,
			.addrs = {
				{0x0000, 0x0337, 0x0000},
			}
		},
		{
			.filename = JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_BIN_2,
			.filesize = 0x01C0,
			.blocks = 1,
			.addrs = {
				{0x0000, 0x01BF, 0x1C00},
			}
		},
	},
	.checksum = JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_CHECKSUM
};

/*If changed FW, change above FW bin and Checksum information*/

static int lgit_imx351_rohm_ois_poll_ready(int limit)
{
	uint8_t ois_status = 0;
	int read_byte = 0;
	int rc = OIS_SUCCESS;

	usleep_range(1000, 1000 + 10); /* wait 1ms */

	while ((ois_status != 0x01) && (read_byte < limit)) {
		rc = RegReadA(OIS_READ_STATUS_ADDR, &ois_status); /* polling status ready */
		if (rc < 0) {
			pr_debug("%s:%d, OIS_I2C_ERROR\n", __func__, __LINE__);
			return OIS_INIT_I2C_ERROR;
		}
		usleep_range(5* 1000, 5* 1000 + 10); /* wait 5ms */
		read_byte++;
	}

	if (ois_status == 0x01) {
		return OIS_SUCCESS;
	}
	else {
		pr_debug("%s:%d, OIS_TIMEOUT_ERROR\n", __func__, __LINE__);
		return OIS_INIT_TIMEOUT;
	}
}

int lgit_imx351_rohm_ois_bin_download(struct ois_i2c_bin_list bin_list)
{
	int rc = 0;
	//int cnt = 0;

	int32_t read_value_32t;

	pr_debug("%s:%d Enter\n", __func__, __LINE__);

	/* check OIS ic is alive */
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	/* Send command ois start DownLoad */
	rc = RegWriteA(OIS_START_DL_ADDR, 0x00);

#if 0 /* request by rohm */
	while (rc < 0 && cnt < LIMIT_STATUS_POLLING) {
		usleep_range(2000, 2010);
		rc = RegWriteA(OIS_START_DL_ADDR, 0x00);
		cnt++;
	}
#endif

	if (rc < 0) {
		pr_debug("%s:%d, OIS_I2C_ERROR\n", __func__, __LINE__);
		rc = OIS_INIT_I2C_ERROR;
		return rc;
	}

	/* OIS program downloading */
	rc = ois_i2c_load_and_write_bin_list(bin_list);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_BIN_DOWNLOAD_FAIL\n", __func__, __LINE__);
		return rc;
	}
#if 1
	/* Check sum value!*/
	RamRead32A(OIS_CHECK_SUM_ADDR, &read_value_32t);
	if (read_value_32t != bin_list.checksum) {
		pr_debug("%s:%d, saved sum = 0x%x, read sum = 0x%x\n",
			__func__, __LINE__, bin_list.checksum, read_value_32t);
		rc = OIS_INIT_CHECKSUM_ERROR;
		return rc;
	}
#else
	RamRead32A(OIS_CHECK_SUM_ADDR, &read_value_32t);
	pr_debug("%s:%d, saved sum = 0x%x, read sum = 0x%x\n",
				__func__, __LINE__, bin_list.checksum, read_value_32t);
#endif
	/* If Change EEPROM MAP, change below */
	rc = ois_i2c_load_and_write_e2prom_data(E2P_FIRST_ADDR, E2P_DATA_BYTE_FIRST, CTL_END_ADDR_FOR_FIRST_E2P_DL);
	if (rc < 0) {
		pr_debug("%s:%d ois_i2c_load_and_write_e2prom_data failed (rc: %d)\n", __func__, __LINE__, rc);
		return rc;
	}
	/* Send command ois complete dl */
	RegWriteA(OIS_COMPLETE_DL_ADDR, 0x00);

	/* Read ois status */
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	pr_debug("%s:%d End\n", __func__, __LINE__);

	return rc;

}

int lgit_imx351_rohm_ois_init_cmd(int limit)
{
	int rc = 0;
	uint8_t ois_status = 0;

	pr_debug("%s:%d Enter\n", __func__, __LINE__);

	RegReadA(0x6020, &ois_status);
	CDBG("%s:%d, 0x6020 status : 0x%d\n", __func__, __LINE__, ois_status);
	if(ois_status != 0x01) {
		RegWriteA(0x6020, 0x01);
	}
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING + limit);
	//do rc check after VCM Init for AF

	//VCM Init Code
	RegWriteA(0x60F1, 0x02);
	vcm_check = 1;

#if 0 //VCM Test Code
	RegWriteB(0x60E4, 0x00); // 0x1FF: 511, 0x200: -512
#endif

	//do rc check after VCM Init for AF
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	if (cal_ver ==  0x0101 || cal_ver == 0x0301) {
		CDBG("%s 1. 0x6023 0x%x \n", __func__, ois_status);
		RegWriteA(0x6023, 0x00);
	}

	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	RegReadA(0x6023, &ois_status);
	CDBG("%s 2. 0x6023 0x%x \n", __func__, ois_status);

	pr_debug("%s:%d End\n", __func__, __LINE__);
	return OIS_SUCCESS;
}

static struct msm_ois_func_tbl lgit_ois_func_tbl;

int32_t lgit_imx351_rohm_ois_mode(struct msm_ois_ctrl_t *o_ctrl,
					   struct msm_ois_set_info_t *set_info)
{
	int cur_mode = lgit_ois_func_tbl.ois_cur_mode;
	uint8_t mode = 0;
	int rc = 0;

	if (copy_from_user(&mode, (void *)set_info->setting, sizeof(uint8_t))) {
		pr_debug("%s:%d failed\n", __func__, __LINE__);
		rc = -EFAULT;
		return rc;
	}
	pr_debug("%s:Enter input mode : %d, current mode : %d \n", __func__, mode, cur_mode);

	if (cur_mode == mode)
		return OIS_SUCCESS;

	if (cur_mode != OIS_MODE_CENTERING_ONLY) {
		RegWriteA(0x6020, 0x01);

		rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
		if (rc < 0) {
			pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
			return rc;
		}
	}

	switch (mode) {
	case OIS_MODE_PREVIEW_CAPTURE:
	case OIS_MODE_CAPTURE:
	case OIS_MODE_VIDEO:
		pr_debug("%s:input mode : %d, current mode : %d \n", __func__, mode, cur_mode);
		//RegWriteA(0x6021, 0x03);
		RegWriteA(0x6021, 0x63); // New STILL Mode for improving drift issue
		usleep_range(10 * 1000, 10 * 1000 + 10); // 20151127 Rohm_LeeDJ_delay 10ms
		RegWriteA(0x6020, 0x02);
		break;
#if 0 //change, ROHM 12S F/W has issue at 0x61
	case OIS_MODE_VIDEO:
		pr_debug("%s:%d, %d video\n", __func__, mode, cur_mode);
		RegWriteA(0x6021, 0x61);
		usleep_range(10 * 1000, 10 * 1000 + 10); // 20151127 Rohm_LeeDJ_delay 10ms
		RegWriteA(0x6020, 0x02);
		break;
#endif
	case OIS_MODE_CENTERING_ONLY:
		pr_debug("%s:%d, %d centering_only\n", __func__, mode, cur_mode);
		RegWriteA(0x6020, 0x01);
		usleep_range(10 * 1000, 10 * 1000 + 10); // 20151127 Rohm_LeeDJ_delay 10ms
		rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
		if (rc < 0) {
			pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
			return rc;
		}
		break;
	case OIS_MODE_CENTERING_OFF:
		pr_debug("%s:%d, %d centering_off\n", __func__, mode, cur_mode);
		RegWriteA(0x6020, 0x00); /* lens centering off */
		usleep_range(10 * 1000, 10 * 1000 + 10); // 20151127 Rohm_LeeDJ_delay 10ms
		rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
		if (rc < 0) {
			pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
			return rc;
		}
		break;
	}

	lgit_ois_func_tbl.ois_cur_mode = mode;
	pr_debug("%s:%d End\n", __func__, __LINE__);

	return OIS_SUCCESS;
}

int lgit_imx351_rohm_ois_calibration(int ver)
{
	int16_t gyro_offset_value_x, gyro_offset_value_y = 0;
	uint8_t ois_status = 0;
	int rc = 0;

	pr_debug("%s: lgit_ois_calibration start\n", __func__);
	/* Gyro Zero Calibration Starts. */
	//20151127 Rohm_LeeDJ
	/* Read ois status */
	RegReadA(0x6020, &ois_status);

	CDBG("%s:%d, 0x6020 status : 0x%d\n", __func__, __LINE__, ois_status);
	if(ois_status != 0x01) {
			RegWriteA(0x6020, 0x01);
			usleep_range(100 * 1000, 100 * 1000 + 10); // 20151127 Rohm_LeeDJ_delay 100ms
	}

	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	/* Gyro On */
	if (cal_ver == 0x0101 || cal_ver == 0x0301) {
		RegWriteA(0x6023, 0x00);
	}

	RegReadA(0x6023, &ois_status);
	CDBG("%s 2. 0x6023 0x%x \n", __func__, ois_status);

	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	/* Select Xch Gyro */
	RegWriteA(0x6088, 0);

	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	/* Read Xch Gyro Offset */
	RegReadB(0x608A, &gyro_offset_value_x);

#if 0
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}
#endif

	/* Select Ych Gyro */
	RegWriteA(0x6088, 1);

	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}


	/* Read Xch Gyro Offset */
	RegReadB(0x608A, &gyro_offset_value_y);

#if 0
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}
#endif

	/* If Change EEPROM MAP, change below */
	/* Cal. Data to eeprom */
	ois_i2c_e2p_write(0xA98, (uint16_t)(0xFFFF & gyro_offset_value_x), 2);
	usleep_range(100 * 1000, 100 * 1000 + 10);
	ois_i2c_e2p_write(0xA9A, (uint16_t)(0xFFFF & gyro_offset_value_y), 2);
	 /* gyro_offset_value_x -> gyro_offset_value_y*/

	/* Cal. Data to OIS Driver */
	RegWriteA(0x609C, 0x00); //Changed rohm 0926 LeeDJ
	RamWriteA(0x609D, gyro_offset_value_x); /* 16 */

	RegWriteA(0x609C, 0x01); //Changed rohm 0926 LeeDJ
	RamWriteA(0x609D, gyro_offset_value_y); /* 16 */

	RegWriteA(0x6023, 0x00); //added rohm 0926 LeeDJ

	/* Gyro Zero Calibration Ends. */
	pr_debug("%s gyro_offset_value_x %d gyro_offset_value_y %d\n", __func__, gyro_offset_value_x, gyro_offset_value_y);
	g_gyro_offset_value_x = gyro_offset_value_x;
	g_gyro_offset_value_y = gyro_offset_value_y;

	pr_debug("%s: lgit_ois_calibration end\n", __func__);
	return OIS_SUCCESS;
}

int32_t lgit_imx351_init_set_rohm_ois(struct msm_ois_ctrl_t *o_ctrl,
						   struct msm_ois_set_info_t *set_info)
{
	int32_t rc = OIS_SUCCESS;
	uint16_t ver = 0;
	pwm_prev_mode = -1;

	pr_debug("%s:%d Enter, %s\n", __func__, __LINE__, LAST_UPDATE);

	if (copy_from_user(&ver, (void *)set_info->setting, sizeof(uint16_t))) {
		pr_debug("%s:%d failed\n", __func__, __LINE__);
		rc = -EFAULT;
		return rc;
	}

	ois_i2c_e2p_read(CAL_VER, &cal_ver, 2);
	ois_i2c_e2p_read(E2P_MAP_VER_ADDR, &map_ver, 1);
	CDBG("%s cal_ver %x, map_ver %x, init ver %d\n", __func__, cal_ver, map_ver, ver);

	switch (cal_ver) {
		case 0xA9E9:
			pr_debug("[CHECK] %s: JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_BIN_DATA, 0s\n", __func__);
			rc = lgit_imx351_rohm_ois_bin_download(JOAN_0616_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_0_BIN_DATA);
			break;
		case 0x0101:
			pr_debug("[CHECK] %s: JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_BIN_DATA, 1M_3S\n", __func__);
			rc = lgit_imx351_rohm_ois_bin_download(JOAN_0630_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_3_BIN_DATA);
			break;
		case 0x0301:
			pr_debug("[CHECK] %s: JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_BIN_DATA, 3M_5S\n", __func__);
			rc = lgit_imx351_rohm_ois_bin_download(JOAN_0719_LGIT_OLAF_ACTUATOR_FIRMWARE_VER_5_BIN_DATA);
			break;
		default:
			pr_debug("[CHECK] %s: Apply Default : No Download BIN_DATA cal_ver:0x%x\n", __func__, cal_ver);
			rc = -EFAULT;
			break;
	}

	if (rc < 0)	{
		pr_debug("%s: init fail\n", __func__);
		return rc;
	}

	switch (ver) {
	case OIS_VER_RELEASE:
		CDBG("%s OIS_VER_RELEASE\n", __func__);
		lgit_imx351_rohm_ois_init_cmd(LIMIT_OIS_ON_RETRY);
		break;
	case OIS_VER_CALIBRATION:
	case OIS_VER_DEBUG:
		CDBG("%s OIS_VER_DEBUG\n", __func__);
		lgit_imx351_rohm_ois_calibration(ver);

		/* OIS ON */
		RegWriteA(0x6021, 0x03);/* LGIT STILL & PAN ON MODE */
		usleep_range(10 * 1000, 10 * 1000 + 10); // 20151127 Rohm_LeeDJ_delay 10ms
		RegWriteA(0x6020, 0x02);/* OIS ON */

		rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
		if (rc < 0) {
			pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
			return rc;
		}
		break;
	}
	lgit_ois_func_tbl.ois_cur_mode = OIS_MODE_CENTERING_ONLY;
	pr_debug("%s:%d End\n", __func__, __LINE__);
	return rc;

}
int32_t	lgit_imx351_rohm_ois_on(struct msm_ois_ctrl_t *o_ctrl,
					 struct msm_ois_set_info_t *set_info)
{
	int32_t rc = OIS_SUCCESS;
	pr_debug("%s:%d Enter\n", __func__, __LINE__);
#if 0
	RegWriteA(0x6020, 0x01);
	RegWriteA(0x6021, 0x79); /* LGIT STILL & PAN ON MODE */
	RegWriteA(0x613F, 0x01);
	RegWriteA(0x6020, 0x02); /* OIS ON */
#endif
	pr_debug("%s:%d End\n", __func__, __LINE__);
	return rc;
}

void lgit_imx351_rohm_write_vcm(int16_t nDAC)
{
	if (vcm_check == 1) {
	CDBG("%s Enter\n", __func__);
	CDBG("[CHECK] nDAC: %d\n", nDAC);
	/*
	if (nDAC > 511) nDAC = 0x1FF;
	if (nDAC < -512) nDAC = 0x200;
	//W 0x60DA //0~1024//usint
	//      uint16_t data[2];
	//       data[1] = nDAC & 0xff;
	//       data[0] = nDAC >> 8;

	RegWriteB(0x60E4, nDAC);
	*/
	RegWriteB(0x612E, nDAC);
	}
}

EXPORT_SYMBOL(lgit_imx351_rohm_write_vcm);

int32_t	lgit_imx351_rohm_ois_off(struct msm_ois_ctrl_t *o_ctrl,
					  struct msm_ois_set_info_t *set_info)
{
	int rc = OIS_SUCCESS;

	pr_debug("%s:%d Enter\n", __func__, __LINE__);

	RegWriteA(0x6020, 0x01);
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	RegWriteA(0x6020, 0x00); //Standby Mode
	vcm_check = 0;
	pr_debug("%s:%d End\n", __func__, __LINE__);
	return rc;
}

int32_t lgit_imx351_rohm_ois_stat(struct msm_ois_ctrl_t *o_ctrl,
					   struct msm_ois_set_info_t *set_info)
{
	struct msm_ois_info_t ois_stat;
	int rc = 0;

	int16_t val_hall_x;
	int16_t val_hall_y;

	/* float gyro_scale_factor_idg2020 = (1.0)/(262.0); */
	int16_t val_gyro_x;
	int16_t val_gyro_y;
	/* Hall Fail Spec. */
	int16_t spec_hall_x_lower = 1467;/* for get +-0.65deg, need 45.0um. */
	int16_t spec_hall_x_upper = 2629;
	int16_t spec_hall_y_lower = 1467;
	int16_t spec_hall_y_upper = 2629;
	uint8_t *ptr_dest = NULL;

	memset(&ois_stat, 0, sizeof(ois_stat));
	snprintf(ois_stat.ois_provider, ARRAY_SIZE(ois_stat.ois_provider), "LGIT_ROHM");

	/* Gyro Read by reg */
	RegWriteA(0x609C, 0x02); //Changed rohm 0926 LeeDJ
	RamReadA(0x609D, &val_gyro_x);
	RegWriteA(0x609C, 0x03); //Changed rohm 0926 LeeDJ
	RamReadA(0x609D, &val_gyro_y);

	//set_info->ois_info.gyro[0] = (int16_t)val_gyro_x;
	//set_info->ois_info.gyro[1] = (int16_t)val_gyro_y;

	/* Hall Fail */
	/* Read Hall X */
	RegWriteA(0x6060, 0x00);
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	RegReadB(0x6062, &val_hall_x);

	/* Read Hall Y */
	RegWriteA(0x6060, 0x01);
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	RegReadB(0x6062, &val_hall_y);

	ois_stat.gyro[0] = (int16_t)val_gyro_x;
	ois_stat.gyro[1] = (int16_t)val_gyro_y;
	ois_stat.hall[0] = (int16_t)val_hall_x;
	ois_stat.hall[1] = (int16_t)val_hall_y;
	ois_stat.is_stable = 1;

	ptr_dest = (uint8_t *)set_info->ois_info;
	if (copy_to_user(ptr_dest, &ois_stat, sizeof(ois_stat))) {
		pr_debug("%s:%d failed copy_to_user result\n", __func__, __LINE__);
	}

	pr_debug("%s val_hall_x : (%d), val_gyro_x : (0x%x), g_gyro_offset_value_x (%d)\n",
		__func__, val_hall_x, val_gyro_x, g_gyro_offset_value_x);
	pr_debug("%s val_hall_y : (%d), val_gyro_y : (0x%x), g_gyro_offset_value_y (%d)\n",
		__func__, val_hall_y, val_gyro_y, g_gyro_offset_value_y);


	if (abs(val_gyro_x) > (5 * GYRO_SCALE_FACTOR) ||
		abs(val_gyro_y) > (5 * GYRO_SCALE_FACTOR)) {
		pr_debug("Gyro Offset X is FAIL!!! (%d)\n", val_gyro_x);
		pr_debug("Gyro Offset Y is FAIL!!! (%d)\n", val_gyro_y);
		ois_stat.is_stable = 0;
	}

	/* Hall Spec. Out? */
	if (val_hall_x > spec_hall_x_upper || val_hall_x < spec_hall_x_lower) {
		pr_debug("val_hall_x is FAIL!!! (%d) 0x%x\n", val_hall_x,
			 val_hall_x);
		ois_stat.is_stable = 0;
	}

	if (val_hall_y > spec_hall_y_upper || val_hall_y < spec_hall_y_lower) {
		pr_debug("val_hall_y is FAIL!!! (%d) 0x%x\n", val_hall_y,
			 val_hall_y);
		ois_stat.is_stable = 0;
	}

	return 0;
}

int32_t lgit_imx351_rohm_ois_move_lens(struct msm_ois_ctrl_t *o_ctrl,
							struct msm_ois_set_info_t *set_info)
{

	int8_t hallx = 0;/* target_x / HALL_SCALE_FACTOR; */
	int8_t hally = 0;/* target_y / HALL_SCALE_FACTOR; */
	uint8_t result = 0;
	int32_t rc = OIS_SUCCESS;
	int16_t offset[2];
	if (copy_from_user(&offset[0], (void *)set_info->setting, sizeof(int16_t) * 2)) {
		pr_debug("%s:%d failed\n", __func__, __LINE__);
		rc = -EFAULT;
		return rc;
	}

	switch (cal_ver) {
		/* LGIT Actuator */
		case 0x0101:
		case 0x0301:
			hallx =  offset[0] * GYRO_GAIN_LGIT / HALL_SCALE_FACTOR;
			hally =  offset[1] * GYRO_GAIN_LGIT / HALL_SCALE_FACTOR;
			break;
		default:
			pr_debug("[CHECK] %s: OIS NOT SUPPORTED\n", __func__);
			rc = OIS_FAIL;
			break;
	}

	/* check ois mode & change to suitable mode */
	RegReadA(0x6020, &result);
	if (result != 0x01) {
		RegWriteA(0x6020, 0x01);
		rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
		if (rc < 0) {
			pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
			return rc;
		}
	}

	pr_debug("%s target : %d(0x%x), %d(0x%x)\n", __func__,
		   hallx, hallx, hally, hally);

	RegWriteA(0x6026, 0xFF & hallx); /* target x position input */ //Changed rohm 0926 LeeDJ
	RegWriteA(0x6027, 0xFF & hally); /* target y position input */ //Changed rohm 0926 LeeDJ
	RegWriteA(0x6020, 0x03);/* order to move. */ //Changed rohm 0926 LeeDJ

	/* wait 100ms */
	usleep_range(100000, 100010); //added rohm 0926 LeeDJ
	rc = lgit_imx351_rohm_ois_poll_ready(LIMIT_STATUS_POLLING);
	if (rc < 0) {
		pr_debug("%s:%d, OIS_POLLING_ERROR\n", __func__, __LINE__);
		return rc;
	}

	RegReadA(0x609B, &result);

	RegWriteA(0x6020, 0x01); //added rohm 0926 LeeDJ

	if (result == 0x03)
		return  OIS_SUCCESS;

	pr_debug("%s move fail : 0x%x\n", __func__, result);
	return OIS_FAIL;
}

/* If changed Image Sensor Setting, change below settings (151017 copy from P+) */
/* IMX351 Module . PWM frequency adjustment setting . About 3MHz*/
int32_t lgit_imx351_rohm_ois_pwm_mode(struct msm_ois_ctrl_t *o_ctrl,
							struct msm_ois_set_info_t *set_info)
{
	uint8_t mode = 0;
	if (copy_from_user(&mode, (void *)set_info->setting, sizeof(uint8_t))) {
	        pr_debug("%s:%d failed to get mode\n", __func__, __LINE__);
	        return OIS_FAIL;
	}
	if (pwm_prev_mode != mode) {
		pwm_prev_mode = mode;
		if(cal_ver == 0x0301){
			switch (mode) {
				case OIS_IMG_SENSOR_REG_A:
					/* RES_0(Reg_A1) Full-resolution 16M (4656x3492), 30fps */
					pr_debug("%s OIS_IMG_SENSOR_REG_A(Res 0)_h4\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x48);
					RegWriteA(0x60D3, 0x00);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_B:
					/* RES_1(Reg_A2) 16:9 Full-resolution (4656x2620), 30fps 4MHz*/
					pr_debug("%s OIS_IMG_SENSOR_REG_B(Res 1)_h4\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x64);
					RegWriteA(0x60D3, 0x00);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_C:
					/* RES_2(Reg_A10) Bin(HV 2/2)_30fps_2328x1744*/
					pr_debug("%s OIS_IMG_SENSOR_REG_C(Res 2)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x32);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_D:
					/* RES_3(Reg_A11) Bin(HV 2/2)_30fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_D(Res 3)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x32);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_E:
					/* RES_4(Reg_B1) Bin(HV 1/2)_60fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_E(Res 4)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x02);
					RegWriteA(0x60D2, 0x34);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_F:
					/* RES_5(Reg_B2) Bin(HV 1/2)_120fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_F(Res 5)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x02);
					RegWriteA(0x60D2, 0x30);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_G:
					/* RES_6(Reg_A7_1) Bin(HV 2/2)_240fps_2020x1136_LowRate */
					pr_debug("%s OIS_IMG_SENSOR_REG_G(Res 6)_h4\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x03);
					RegWriteA(0x60D2, 0x68);
					RegWriteA(0x60D3, 0x00);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_H:
					/* RES_7(Reg_A2_HDR) Full_30fps_Cropped_4656x2620_HDR (Cinema)*/
					pr_debug("%s OIS_IMG_SENSOR_REG_H(Res 7)_h4\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x64);
					RegWriteA(0x60D3, 0x00);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_I:
					/* RES_8(Reg_A10) Bin(HV 2/2)_30fps_2328x1744*/
					pr_debug("%s OIS_IMG_SENSOR_REG_I(Res 8)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x32);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_J:
					/* RES_9(Reg_A11) Bin(HV 2/2)_30fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_J(Res 9)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x32);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				default:
					pr_debug("%s:not support %d, cal_ver = 0x%x \n", __func__, mode, cal_ver);
					break;
					}
			} else {
			switch (mode) {
				case OIS_IMG_SENSOR_REG_A:
					/* RES_0(Reg_A1) Full-resolution 16M (4656x3492), 30fps */
					pr_debug("%s OIS_IMG_SENSOR_REG_A(Res 0)_h3\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x36);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_B:
					/* RES_1(Reg_A2) 16:9 Full-resolution (4656x2620), 30fps 4MHz*/
					pr_debug("%s OIS_IMG_SENSOR_REG_B(Res 1)_h4\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x64);
					RegWriteA(0x60D3, 0x00);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_C:
					/* RES_2(Reg_A10) Bin(HV 2/2)_30fps_2328x1744*/
					pr_debug("%s OIS_IMG_SENSOR_REG_C(Res 2)_h3\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x4A);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_D:
					/* RES_3(Reg_A11) Bin(HV 2/2)_30fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_D(Res 3)_h3\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x4A);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_E:
					/* RES_4(Reg_B1) Bin(HV 1/2)_60fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_E(Res 4)_h3\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x02);
					RegWriteA(0x60D2, 0x50);
					RegWriteA(0x60D3, 0x00);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_F:
					/* RES_5(Reg_B2) Bin(HV 1/2)_120fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_F(Res 5)_h3\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x02);
					RegWriteA(0x60D2, 0x48);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_G:
					/* RES_6(Reg_A7_1) Bin(HV 2/2)_240fps_2020x1136_LowRate */
					pr_debug("%s OIS_IMG_SENSOR_REG_G(Res 6)_h3\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x03);
					RegWriteA(0x60D2, 0x58);
					RegWriteA(0x60D3, 0x00);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_H:
					/* RES_7(Reg_A2_HDR) Full_30fps_Cropped_4656x2620_HDR (Cinema)*/
					pr_debug("%s OIS_IMG_SENSOR_REG_H(Res 7)_h3\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x48);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x0A);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_I:
					/* RES_8(Reg_A10) Bin(HV 2/2)_30fps_2328x1744*/
					pr_debug("%s OIS_IMG_SENSOR_REG_I(Res 8)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x32);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				case OIS_IMG_SENSOR_REG_J:
					/* RES_9(Reg_A11) Bin(HV 2/2)_30fps_2328x1312*/
					pr_debug("%s OIS_IMG_SENSOR_REG_J(Res 9)_h2\n", __func__);
					RegWriteA(0x60D0, 0x00);
					RegWriteA(0x60D4, 0x07);
					RegWriteA(0x60D1, 0x01);
					RegWriteA(0x60D2, 0x32);
					RegWriteA(0x60D3, 0x01);
					RegWriteA(0x60D4, 0x09);
					RegWriteA(0x60D5, 0x00);
					RegWriteA(0x60D0, 0x01);
					break;
				default:
					pr_debug("%s:not support %d\n", __func__, mode);
					break;
					}
			}
			}
	return OIS_SUCCESS;
}

void lgit_imx351_rohm_ois_init(struct msm_ois_ctrl_t *msm_ois_t)
{
	lgit_ois_func_tbl.ini_set_ois = lgit_imx351_init_set_rohm_ois;
	lgit_ois_func_tbl.enable_ois = lgit_imx351_rohm_ois_on;
	lgit_ois_func_tbl.disable_ois = lgit_imx351_rohm_ois_off;
	lgit_ois_func_tbl.ois_mode = lgit_imx351_rohm_ois_mode;
	lgit_ois_func_tbl.ois_stat = lgit_imx351_rohm_ois_stat;
	lgit_ois_func_tbl.ois_move_lens = lgit_imx351_rohm_ois_move_lens;
	lgit_ois_func_tbl.ois_pwm_mode = lgit_imx351_rohm_ois_pwm_mode;
	lgit_ois_func_tbl.ois_cur_mode = OIS_MODE_CENTERING_ONLY;

	msm_ois_t->sid_ois = 0x7C >> 1;
	msm_ois_t->i2c_client.cci_client->i2c_freq_mode = I2C_FAST_PLUS_MODE;
	msm_ois_t->func_tbl = &lgit_ois_func_tbl;
}
