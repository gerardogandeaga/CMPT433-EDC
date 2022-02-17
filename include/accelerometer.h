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

class Accelerometer {

public:
	// delete these functions to enforce singleton behaviour
	Accelerometer(Accelerometer &other) = delete;
	void operator=(const Accelerometer &) = delete;
	
	// this will act like out publicly avaible contructor
	static Accelerometer *GetInstance(void);
	static void DestroyInstance(void); // Our manual destructor, should be explicitly called for cleanup!

	// TODO: write x,y,z values to variables
	void readAcceleration(void);

private:
	// singleton statics
	static Accelerometer *instance;
	static std::mutex mtx;

	// i2c file descriptor for the accelerometer
	int i2cFd;
	// output buffer that is used to write to i2c registers
	unsigned char i2cWriteBuff[2];

protected:
	Accelerometer();
	~Accelerometer();
};

#endif
