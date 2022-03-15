
#include <iostream>
#include <thread>
#include <chrono>
#include "accelerometer.h"
#include "vibrationSensor.h"

int main() 
{
	std::cout << "Earthquake Detection Cluster" << std::endl;
	std::cout << "============================" << std::endl;

	VibrationSensor* inst = VibrationSensor::GetInstance();

	while(1) {
		std::cout << "Pulse: " << inst->getPulse() << std::endl;
	}

	VibrationSensor::DestroyInstance();

	return 0;
}

