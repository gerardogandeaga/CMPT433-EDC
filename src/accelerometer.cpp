#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "accelerometer.h"

// TODO: Clean up this messy cpp file

Accelerometer *Accelerometer::instance{nullptr};
std::mutex Accelerometer::mtx;

Accelerometer::Accelerometer() {
	std::cout << "Creating singleton" << std::endl;

	// enable i2c on pins
	system("config-pin P9_17 i2c");
	system("config-pin P9_18 i2c");
	
	// enable device control
	i2cFd = open("/dev/i2c-1", O_RDWR);
	if (ioctl(i2cFd, I2C_SLAVE_FORCE, 0x1c) < 0) {
		std::cerr << "I2C: Unable to set I2C device to slave address\n";
		exit(1);
	}

	// Turn on the accelerometer
	i2cWriteBuff[0] = 0x2A;
	i2cWriteBuff[1] = 0x00; // value to turn on device
	if (write(i2cFd, i2cWriteBuff, 2) != 2) {
		std::cerr << "I2C: Uanble to write i2c register he.\n";
		exit(1);
	}

	i2cWriteBuff[0] = 0x2A;
	i2cWriteBuff[1] = 0x01; // value to turn on device
	if (write(i2cFd, i2cWriteBuff, 2) != 2) {
		std::cerr << "I2C: Uanble to write i2c register.\n";
		exit(1);
	}

	// Source: http://www.beaglebone.net/code/beaglebone-and-mma8452-sensor-example.php
	// Select configuration register(0x0E)
	// Set range to +/- 2g(0x00)
	i2cWriteBuff[0] = 0x0E;
	i2cWriteBuff[1] = 0x00;
	if (write(i2cFd, i2cWriteBuff, 2) != 2) {
		std::cerr << "I2C: Uanble to write i2c register.\n";
		exit(1);
	}
}

Accelerometer::~Accelerometer() {
	std::cout << "Destroying singleton" << std::endl;
	close(i2cFd);
}

void Accelerometer::readAcceleration(void) {
	// Read 7 bytes of data(0x00)
	// staus, xAccl msb, xAccl lsb, yAccl msb, yAccl lsb, zAccl msb, zAccl lsb
	char reg[1] = {0x00};
	write(i2cFd, reg, 1);
	char data[7] = {0};
	if(read(i2cFd, data, 7) != 7) {
		printf("Error : Input/Output error \n");
	}
	else {
		// Convert the data to 12-bits
		int xAccl = ((data[1] * 256) + data[2]) / 16;
		if(xAccl > 2047) {
			xAccl -= 4096;
		}
 
		int yAccl = ((data[3] * 256) + data[4]) / 16;
		if(yAccl > 2047) {
			yAccl -= 4096;
		}
 
		int zAccl = ((data[5] * 256) + data[6]) / 16;
		if(zAccl > 2047) {
			zAccl -= 4096;
		}
 
		// Output data to screen
		std::cout << "Acceleration in X-Axis : " << xAccl << std::endl;
		std::cout << "Acceleration in Y-Axis : " << yAccl << std::endl;
		std::cout << "Acceleration in Z-Axis : " << zAccl << std::endl;
	}
}

Accelerometer *Accelerometer::GetInstance() {
	// sync threads so they dont create different instances of the accelerometer
	// using a lock gaurd locks the mutex for the duration of the function!
	std::lock_guard<std::mutex> lock(mtx);

	// create a new instance of the accelerometer object
	if (instance == nullptr)
		instance = new Accelerometer();

	return instance;
}

void Accelerometer::DestroyInstance(void) {
	std::lock_guard<std::mutex> lock(mtx);
	delete instance;
	instance = nullptr;
}
