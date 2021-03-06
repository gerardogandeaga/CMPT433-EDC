
#include <iostream>
#include <thread>
#include <chrono>

#include "i2c_utilities.h"
#include "accelerometer.h"
#include "lcdScreen.h"
#include "vibrationSensor.h"
#include "node.h"

int main(int argc, char *argv[]) 
{
	if (argc != 3) {
		std::cout << "Wrong number of arguments, expected 2" << std::endl;
		return -1;
	}

	const char *serverAddr = argv[1];
	int port = atoi(argv[2]);
	
	std::cout << "Earthquake Detection Cluster" << std::endl;
	std::cout << "============================" << std::endl;

	// initialize i2c
	i2c_utilities::initI2c();

	// init the node
	Node::Initialize(serverAddr, port);

	while (1);

	Node::End();

	// Vector prev = accInst->getAcceleration();
	// while (1) {
	// 	Vector acc = accInst->getAcceleration();
	// 	// std::cout << "<" << acc.x << ", " << acc.y << ", " << acc.z << ">" << std::endl;
	// 	std::this_thread::sleep_for(std::chrono::nanoseconds(100000000));
	// 	// std::cout << acc.magnitude() << std::endl;
	// 	std::cout << acc.diff(prev).magnitude() << std::endl;
	// 	prev = acc;
	// }

	// VibrationSensor* inst = VibrationSensor::GetInstance();

	// while(1) {
		// std::cout << "Pulse: " << inst->getPulse() << std::endl;
	// }

	// VibrationSensor::DestroyInstance();

	return 0;
}

