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
#define ACC_SMOOTHING_WEIGHT 0.1
// Data
#define SAMPLE_RATE 10000000 // 10ms

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

Vector::Vector(const Vector *other) 
{
	x = other->x;
	y = other->y;
	z = other->z;
}

double Vector::magnitude() 
{
	return sqrt(x*x + y*y + z*z);
}

Vector Vector::diff(Vector &other)
{
	return Vector(x - other.x, y - other.y, z - other.z);
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
	smoothedAccVector = new Vector(); 
	readRawAcceleration(&smoothedAccVector->x, &smoothedAccVector->y, &smoothedAccVector->z);

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
	delete smoothedAccVector;
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

void Accelerometer::readRawAcceleration(double *x, double *y, double *z)
{
	char reg[1] = {0x00};
	write(i2cFd, reg, 1);
	char data[7] = {0};

	if(read(i2cFd, data, 7) != 7) {
		std::cerr << "Could not read 7bytes of acceleration data!" << std::endl;
	}
	else {
		std::lock_guard<std::mutex> lock(valueMtx);

		// get data samples
		*x = accSample2Value(data[2], data[1]);
		*y = accSample2Value(data[4], data[3]);
		*z = accSample2Value(data[6], data[5]);
	}
}

void Accelerometer::sampleAcceleration(void) 
{
	double xAcc, yAcc, zAcc;
	readRawAcceleration(&xAcc, &yAcc, &zAcc);

	// exponential smoothing lambda
	static auto expSmooth = [](double val, double avg) -> double {
		return ACC_SMOOTHING_WEIGHT * val + (1.0 - ACC_SMOOTHING_WEIGHT) * avg;
	};

	// update the smoothed acceleration vector
	smoothedAccVector->x = expSmooth(xAcc, smoothedAccVector->x);
	smoothedAccVector->y = expSmooth(yAcc, smoothedAccVector->y);
	smoothedAccVector->z = expSmooth(zAcc, smoothedAccVector->z);
}

double Accelerometer::accSample2Value(short lsb, short msb)
{
	int val = ((msb << 8) | lsb) >> 4;
	if (val > ACC_G * ACC_MG - 1) {
		val -= 4096; // 2^12
	}
	return (double)val / 1024;
}

Vector Accelerometer::getAcceleration(void)
{
	std::lock_guard<std::mutex> lock(valueMtx);
	// return a copy of the smoothed vector
	return Vector(smoothedAccVector);
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


