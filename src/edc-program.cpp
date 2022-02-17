
#include <iostream>
#include "accelerometer.h"

int main() {
	std::cout << "Earthquake Detection Cluster" << std::endl;
	std::cout << "============================" << std::endl;
	Accelerometer* inst = Accelerometer::GetInstance();
	inst->readAcceleration();

	Accelerometer::DestroyInstance();

	return 0;
}

