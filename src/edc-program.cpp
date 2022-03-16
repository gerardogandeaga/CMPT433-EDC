
#include <iostream>
#include <thread>
#include <chrono>
#include "accelerometer.h"

int main() 
{
	std::cout << "Earthquake Detection Cluster" << std::endl;
	std::cout << "============================" << std::endl;
	Accelerometer* inst = Accelerometer::GetInstance();

	Vector prev, v, dv;
	prev = inst->getAcceleration();
	for (int i = 0; i < 15; i++) {
		v = inst->getAcceleration();
		dv = prev.diff(v);
		
		std::cout << "<" << v.x << ", " << v.y << ", " << v.z << "> ";
		std::cout << "[diff: <" << dv.x << ", " << dv.y << ", " << dv.z << "> ";
		std::cout << " mag: " << dv.magnitude() << "]" << std::endl;
		std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));
	}

	Accelerometer::DestroyInstance();

	return 0;
}

