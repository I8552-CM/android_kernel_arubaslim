struct bma_callback {
	int (*i2c_read)(unsigned char chip, unsigned int addr,
		int alen, unsigned char *buffer, int len);
	int (*i2c_write)(unsigned char chip, unsigned int addr,
		int alen, unsigned char *buffer, int len);
	int (*fs_read)(const unsigned char *file_name,
		unsigned char *addr, int count);
	int (*fs_write)(const unsigned char *file_name,
		unsigned char *addr, int count);
	void (*msleep)(unsigned int count);
	unsigned char sensor_i2c_id;
};



int backup_or_restore_i2c(struct bma_callback *pCB);
