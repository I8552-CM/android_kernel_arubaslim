#include "bmainit.h"
#include <linux/kernel.h>

#define BLOCK_SIZE 26

unsigned char flag_sensor_backedup;

int CalcCRC8(unsigned char *p_MemAddr, unsigned char BlockSize)
{
	unsigned char CrcReg = 0xFF;
	unsigned char MemByte;
	unsigned char BitNo;

	while (BlockSize) {
		MemByte = *p_MemAddr;
		for (BitNo = 0; BitNo < 8; BitNo++) {
			if ((CrcReg^MemByte) & 0x80)
				CrcReg = (CrcReg << 1) ^ 0x11D;
			else
				CrcReg <<= 1;

			MemByte <<= 1;
		}
		BlockSize--;
		p_MemAddr++;
	}

	CrcReg = ~CrcReg;
	return CrcReg;
}


int backup_or_restore_i2c(struct bma_callback *pCB)
{

	int i2c_id_ii = 0x10;
	int flag_sensor_found = 0;
	unsigned char chip_id = 0;
	unsigned char crc8 = 0;
	unsigned char buf = 0;
	unsigned char regs[BLOCK_SIZE + 1] = {0};
	int ii = 0;
	int write_to_nvm = 0;
	int read_from_nvm = 0;

	if ((pCB == 0)
		|| (pCB->i2c_read == 0)
		|| (pCB->i2c_write == 0)
		|| (pCB->fs_read == 0)
		|| (pCB->fs_write == 0)
		|| (pCB->msleep == 0)
		|| (pCB->sensor_i2c_id == 0)) {
		pr_info("Bad parameter.\n");
		return -1;
	}

	//check sensor_i2c_id first to save time
	pCB->i2c_read(pCB->sensor_i2c_id, 0x00, 1, &chip_id, 1);
	pr_info("[BMA_INIT] 0X%02X : 0X%02X\n", pCB->sensor_i2c_id, chip_id);

	if ((chip_id & 0xFC) == 0xF8) { //BMC150 CHIP_ID
		flag_sensor_found = 1;
#if defined(CONFIG_SENSORS_BMA222E)
		i2c_id_ii = pCB->sensor_i2c_id;
#endif

	} else {
		//scan i2c address 0x10 to 0x1F, check if we can find the chip id
		do {
			pCB->i2c_read(i2c_id_ii, 0x00, 1, &chip_id, 1);
			if ((chip_id & 0xFC) == 0xF8) { //BMC150 CHIP_ID
				flag_sensor_found = 1;
				pr_info("[BMA_INIT] Sensor found\n");
			} else {
				i2c_id_ii++;
				pr_info("[BMA_INIT] i2c_id_ii: 0X%02X, ",
					i2c_id_ii);
			}
		} while ((!flag_sensor_found) && (i2c_id_ii <= 0x1F));
	}


	if (!flag_sensor_found) {
		pr_info("[BMA_INIT] Sensor not found\n");
		return -1;
	} else {
		pr_info("[BMA_INIT] Sensor found address 0x%x\n", i2c_id_ii);
		//set sensor to normal mode
		buf = 0x00;
		pCB->i2c_write(i2c_id_ii, 0x11, 1, &buf, 1);

		//wait 1.8ms
		pCB->msleep(2);

		//open extended memory map
		buf = 0xAA;
		pCB->i2c_write(i2c_id_ii, 0x35, 1, &buf, 1);
		pCB->i2c_write(i2c_id_ii, 0x35, 1, &buf, 1);

		//set the I2C_ID back in reg 0x05
		pCB->i2c_read(i2c_id_ii, 0x05, 1, &buf, 1);
		#if defined(CONFIG_SENSORS_BMA2X2)
		buf = (buf & 0x1F);
		#else
		buf = (buf & 0x1F) | 0x80 ;
		#endif
		pCB->i2c_write(i2c_id_ii, 0x05, 1, &buf, 1);

		//get the flag_sensor_backedup from NVM;  
		pCB->fs_read("/efs/flag_sensor_backedup",
			&flag_sensor_backedup, 1);

		if (flag_sensor_backedup) {
			//restore
			pr_info("[BMA_INIT] flag_sensor_backedup %d\n", flag_sensor_backedup);

			//get backuped data from NVM
			read_from_nvm = pCB->fs_read("/efs/reg_backedup",
				regs, BLOCK_SIZE+1);

			if (!read_from_nvm) {
				pr_info("[BMA_INIT] cannot get saved data from NVM\n");
				return -1;
			}

			for (ii = 0x00; ii <= 0x1A; ii++){
				pCB->i2c_write(pCB->sensor_i2c_id,
					ii, 1, &regs[ii], 1);
			}

			//close extended memory map
			buf = 0x0A;
			pCB->i2c_write(pCB->sensor_i2c_id, 0x35, 1, &buf, 1);

			//clear user offset regs
			buf = 0x00;
			for (ii = 0x38; ii <= 0x3A; ii++){
				pCB->i2c_write(pCB->sensor_i2c_id,
					ii, 1, &buf, 1);
			}
		} else {
			pr_info("[BMA_INIT] flag_sensor_backedup %d\n", flag_sensor_backedup);
			//backup
			for (ii = 0x00; ii <= 0x1A; ii++){
				pCB->i2c_read(pCB->sensor_i2c_id,
					ii, 1, &regs[ii], 1);
			}

			//close extended memory map
			buf = 0x0A;
			pCB->i2c_write(pCB->sensor_i2c_id, 0x35, 1, &buf, 1);

			//clear user offset regs
			buf = 0x00;
			for (ii = 0x38; ii <= 0x3A; ii++){
				pCB->i2c_write(pCB->sensor_i2c_id,
					ii, 1, &buf, 1);
			}

			crc8 = CalcCRC8(regs, BLOCK_SIZE);
			if (crc8 == regs[0x1A]) {
				pr_info("[BMA_INIT] CRC check OK\n");

				//save regs to nvm
				write_to_nvm = (pCB->fs_write(
					"/efs/reg_backedup",
					regs, BLOCK_SIZE+1) == BLOCK_SIZE+1);

				if (write_to_nvm) {
					flag_sensor_backedup = 1;
					pCB->fs_write(
						"/efs/flag_sensor_backedup",
						&flag_sensor_backedup, 1);
				} else {
					pr_info("[BMA_INIT] backup to NVM failed\n");
					return -1;
				}

			} else {
				pr_info("[BMA_INIT] CRCcheckfailed, crc8=0x%x regs[0x1A]=0x%x\n",
					crc8, regs[0x1A]);
				return -1;
			}
		}
	}
	return 0;
}
