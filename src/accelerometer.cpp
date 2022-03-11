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
#include <math.h>
#include <chrono>

#include "accelerometer.h"

// Accelerometer
#define LINUX_BUS_I2C_1 "/dev/i2c-1"
#define I2C_ACC_ADDR 0x1C
#define ACC_ACTIVE_REG 0x2A
#define ACC_CONFIG_REG 0x0E
#define ACC_ON 0x01
#define ACC_OFF 0x00
#define ACC_G 2
#define ACC_MG 1024
// Data
#define SAMPLE_RATE 10000000 // 10ms
#define MAX_SAMPLE_SIZE 300

Accelerometer *Accelerometer::instance{nullptr};
std::mutex Accelerometer::mtx;

// ======================= Vector Functions =======================
Vector::Vector() {}

Vector::Vector(double x, double y, double z) : x(x), y(y), z(z) {}

Vector::Vector(const Vector &other) 
{
	x = other.x;
	y = other.y;
	z = other.z;
}

inline double Vector::magnitude() 
{
	return sqrt(x*x + y*y + z*z);
}

// ==================== Accelerometer Functions ===================

Accelerometer::Accelerometer() 
{
	// enable i2c on pins
	system("config-pin P9_17 i2c");
	system("config-pin P9_18 i2c");
	
	// enable device control
	i2cFd = open(LINUX_BUS_I2C_1, O_RDWR);

	if (i2cFd < 0) {
		std::cerr << "Could not open " << LINUX_BUS_I2C_1 << "!" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (ioctl(i2cFd, I2C_SLAVE_FORCE, I2C_ACC_ADDR) < 0) {
		std::cerr << "Unable to set I2C device to slave address\n";
		exit(EXIT_FAILURE);
	}

	// activate the accelerometer
	writeToI2CReg(ACC_ACTIVE_REG, ACC_OFF);
	writeToI2CReg(ACC_ACTIVE_REG, ACC_ON);
	writeToI2CReg(ACC_CONFIG_REG, 0x00);

	// create the Vector buffer
	movementSamples = new Vector[MAX_SAMPLE_SIZE];
	sampleLocation = 0;
	sampleSize = 0;

	// launch the worker thread
	stopWorker = false;
	workerThread = std::thread(&Accelerometer::worker, this);
}

Accelerometer::~Accelerometer() 
{
	// wait for the thread to end
	stopWorker = true;
	workerThread.join();

	close(i2cFd);
	delete[] movementSamples;
}

void Accelerometer::worker() 
{
	// collect data samples
	while (!stopWorker) {
		sampleAcceleration();
		std::this_thread::sleep_for(std::chrono::nanoseconds(SAMPLE_RATE));
	}
}

void Accelerometer::writeToI2CReg(unsigned char reg, unsigned char val) 
{
	i2cWriteBuff[0] = reg;
	i2cWriteBuff[1] = val;
	if (write(i2cFd, i2cWriteBuff, 2) != 2) {
		std::cerr << "Unable to write to i2c register" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Accelerometer::sampleAcceleration(void) 
{
	std::lock_guard<std::mutex> lock(samplesMtx);

	char reg[1] = {0x00};
	write(i2cFd, reg, 1);
	char data[7] = {0};

	if(read(i2cFd, data, 7) != 7) {
		std::cerr << "Could not read 7bytes of acceleration data!" << std::endl;
	}
	else {
		// get data samples
		double xAcc = accSample2Value(data[2], data[1]);
		double yAcc = accSample2Value(data[4], data[3]);
		double zAcc = accSample2Value(data[6], data[5]);

		// add new vector sample to list
		movementSamples[sampleLocation] = Vector(xAcc, yAcc, zAcc);
		sampleLocation = (sampleLocation + 1) % MAX_SAMPLE_SIZE;
		
		if (sampleSize < MAX_SAMPLE_SIZE)
			sampleSize++;
	}
}

double Accelerometer::accSample2Value(short lsb, short msb)
{
	int val = ((msb << 8) | lsb) >> 4;
	if (val > ACC_G * ACC_MG - 1) {
		val -= 4096; // 2^12
	}
	return (double)val / 1024;
}

Vector Accelerometer::readAcceleration(void)
{
	// copy vector to a new buffer
	// Vector samplesCopy = new Vector[MAX_SAMPLE_SIZE];
	// int location, size;
	
	// { 
	// 	// use a new scope for the gaurd_lock
	// 	std::lock_guard<std::mutex> lock(samplesMtx);

	// 	for (int i = 0; i < MAX_SAMPLE_SIZE; i++) {
	// 		samplesCopy[i] = Vector(movementSamples);
	// 	}
	// 	location = sampleLocation;
	// 	size = sampleSize;
	// }

	// process the samples and spit out an averaged accelerat

	return movementSamples[0];
}

Accelerometer *Accelerometer::GetInstance() 
{
	// sync threads so they dont create different instances of the accelerometer
	// using a lock gaurd locks the mutex for the duration of the function!
	std::lock_guard<std::mutex> lock(mtx);

	// create a new instance of the accelerometer object
	if (instance == nullptr)
		instance = new Accelerometer();

	return instance;
}

void Accelerometer::DestroyInstance(void) 
{
	std::lock_guard<std::mutex> lock(mtx);
	delete instance;
	instance = nullptr;
}


