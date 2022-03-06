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

#include "vibrationSensor.h"

VibrationSensor *VibrationSensor::instance{nullptr};
std::mutex VibrationSensor::mtx;

std::string VIBRATION_IN = "/sys/class/gpio/gpio22/value";

static int readLineFromFile(const char* fileName, char* buff, unsigned int maxLength);
static char readValue();

VibrationSensor::VibrationSensor() {
    std::cout << "Creating singleton" << std::endl;

    // Export GPIO 22 pin for reading vibration sensor
    system("cd /sys/class/gpio; echo 22 > export;");
}

VibrationSensor::~VibrationSensor() {
	std::cout << "Destroying singleton" << std::endl;
}

void VibrationSensor::readVibration(void) {
    // Poll every 500ms
    struct timespec pollDelay = {0, 500000000};
    for (int i = 0; i < 20; i++) {
        std::cout << "Vibration: " << readValue() << std::endl;
        nanosleep(&pollDelay, (struct timespec*) NULL);
    }
}

VibrationSensor *VibrationSensor::GetInstance() {
	// sync threads so they dont create different instances of the accelerometer
	// using a lock gaurd locks the mutex for the duration of the function!
	std::lock_guard<std::mutex> lock(mtx);

	// create a new instance of the accelerometer object
	if (instance == nullptr)
		instance = new VibrationSensor();

	return instance;
}

void VibrationSensor::DestroyInstance(void) {
	std::lock_guard<std::mutex> lock(mtx);
	delete instance;
	instance = nullptr;
}

static int readLineFromFile(const char* fileName, char* buff, unsigned int maxLength)
{
	FILE *file = fopen(fileName, "r");
	int bytes_read = getline(&buff, (size_t *) &maxLength, file);
	fclose(file);
	return bytes_read;
}

static char readValue() {
    char buff[1024];
    int bytesRead = readLineFromFile(VIBRATION_IN.c_str(), buff, 1024);

    if (bytesRead > 0) {
        return buff[0];
    }
    return '\0';
}
