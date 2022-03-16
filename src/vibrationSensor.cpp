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

#define VIBRATION_IN "/sys/class/gpio/gpio22/value"
#define SAMPLE_RATE 5000000 // 5ms
#define PULSE_THRESHOLD 10 // How many consecutive '0' readings needed to end a vibration pulse

static int readLineFromFile(const char* fileName, char* buff, unsigned int maxLength);
static char readValue();

VibrationSensor::VibrationSensor() {
    std::cout << "Creating VibrationSensor singleton..." << std::endl;

    // Export GPIO 22 pin for reading vibration sensor
    system("cd /sys/class/gpio; echo 22 > export;");

    std::cout << "Current SAMPLE_RATE: " << SAMPLE_RATE / 1000000 << "ms" << std::endl;
    std::cout << "Current PULSE_THRESHOLD: " << PULSE_THRESHOLD << std::endl;

    stopWorker = false;
	workerThread = std::thread(&VibrationSensor::worker, this);
}

VibrationSensor::~VibrationSensor() {
	std::cout << "Destroying singleton" << std::endl;
    // wait for the thread to end
	stopWorker = true;
	workerThread.join();
}

// Read vibration sensor and calculate current 'pulse'
void VibrationSensor::readVibration(void) {
    char reading = readValue();
    if (reading == '1') {
        pulse++;
        zeroCount = 0;
    } else {
        // Consecutive zero readings above threshold will end a pulse
        zeroCount++;
        if (zeroCount == PULSE_THRESHOLD) {
            if (pulse > 0) {
                pulse = 0;
            }
            zeroCount = 0;
        }
    }
}

int VibrationSensor::getPulse() {
    return pulse;
}

// Read vibration sensor every 10ms
void VibrationSensor::worker() 
{
	while (!stopWorker) {
		readVibration();
		std::this_thread::sleep_for(std::chrono::nanoseconds(SAMPLE_RATE));
	}
}

VibrationSensor *VibrationSensor::GetInstance() {
	// sync threads so they dont create different instances of the vibration sensor
	// using a lock guard locks the mutex for the duration of the function!
	std::lock_guard<std::mutex> lock(mtx);

	// create a new instance of the vibration object
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
    int bytesRead = readLineFromFile(VIBRATION_IN, buff, 1024);

    if (bytesRead > 0) {
        return buff[0];
    }
    return '\0';
}
