#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "i2c_utilities.h"


namespace i2c_utilities 
{
	#define I2C_BUS_1 "/dev/i2c-1"

	void initI2c(void) 
	{
		// configure pins to i2c
		system("config-pin P9_17 i2c");
		system("config-pin P9_18 i2c");
	}

	int getI2cBus1DeviceControl(int deviceAddr) 
	{
		int i2cFd = open(I2C_BUS_1, O_RDWR);

		if (i2cFd < 0) {
			std::cerr << "Could not open " << I2C_BUS_1 << "!" << std::endl;
			exit(EXIT_FAILURE);
		}

		if (ioctl(i2cFd, I2C_SLAVE_FORCE, deviceAddr) < 0) {
			std::cerr << "Unable to set I2C device to slave address: " << deviceAddr << std::endl;
			exit(EXIT_FAILURE);
		}

		return i2cFd;
	}

	void endDeviceControl(int i2cFd)
	{
		close(i2cFd);
	}

	void writeToI2c(int i2cFd, unsigned char reg, unsigned char val)
	{
		unsigned char buff[2];
		buff[0] = reg;
		buff[1] = val;
		if (write(i2cFd, buff, 2) != 2) {
			std::cerr << "Unable to write to i2c register: " << reg << std::endl;
			exit(EXIT_FAILURE);
		}
	}
} 
