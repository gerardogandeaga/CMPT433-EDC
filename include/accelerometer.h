/*
Author: Gerardo Gandeaga
Created: Feb 15, 2021

Description: This is a threadsafe Singleton implementation of an Accelerometer class.
This class will be used for reading accelerometer values. There should not exist 
more than 1 instance of this class at any given time. 

Use GetInstance() to get a pointer to the instance of Accelerometer.

Singleton Implementation Reference: https://refactoring.guru/design-patterns/singleton/cpp/example#example-1

Edits: 
	- [Name] and [Edit]
*/
#ifndef _ACCELEROMETER_H_
#define _ACCELEROMETER_H_

#include <mutex>
#include <thread>

struct Vector {
	double x;
	double y;
	double z;

	Vector();
	Vector(double x, double y, double z);
	Vector(const Vector &other);

	inline double magnitude();
};

class Accelerometer 
{
public:
	// delete these functions to enforce singleton behaviour
	Accelerometer(Accelerometer &other) = delete;
	void operator=(const Accelerometer &) = delete;
	
	// this will act like out publicly avaible contructor
	static Accelerometer *GetInstance(void);
	static void DestroyInstance(void); // Our manual destructor, should be explicitly called for cleanup!

	// creates and returns an averaged acceleration vector
	Vector readAcceleration(void);

private:
	// singleton statics
	static Accelerometer *instance;
	static std::mutex mtx;

	// thread handler
	bool active;
	std::thread workerThread;

	// i2c file descriptor for the accelerometer
	int i2cFd;
	// output buffer that is used to write to i2c registers
	unsigned char i2cWriteBuff[2];

	// Vector buffer
	// TODO: is this buffer system worth it? check if we can avoid it
	Vector *movementSamples;
	int sampleLocation;
	int sampleSize;
	std::mutex samplesMtx;

	void writeToI2CReg(unsigned char reg, unsigned char val);
	void sampleAcceleration(void);
	double accSample2Value(short lsb, short msb);

protected:
	/*
	Sets up the I2C device to read accelerometer input */
	Accelerometer();
	~Accelerometer();

	// threading
	bool stopWorker;
	void worker();
};

#endif
