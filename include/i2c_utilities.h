namespace i2c_utilities 
{

	void initI2c(void);

	int getI2cBus1DeviceControl(int deviceAddr);

	void endDeviceControl(int i2cFd);

	void writeToI2c(int i2cFd, unsigned char reg, unsigned char val);

}
