
#include <iostream>
#include <thread>
#include <chrono>
#include "accelerometer.h"

int main() 
{
	std::cout << "Earthquake Detection Cluster" << std::endl;
	std::cout << "============================" << std::endl;
	Accelerometer* inst = Accelerometer::GetInstance();

	Vector v = inst->readAcceleration();
	std::cout << "<" << v.x << ", " << v.y << ", " << v.z << ">" << std::endl;
	std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));
	v = inst->readAcceleration();
	std::cout << "<" << v.x << ", " << v.y << ", " << v.z << ">" << std::endl;
	std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));
	v = inst->readAcceleration();
	std::cout << "<" << v.x << ", " << v.y << ", " << v.z << ">" << std::endl;
	std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));
	v = inst->readAcceleration();
	std::cout << "<" << v.x << ", " << v.y << ", " << v.z << ">" << std::endl;
	std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));
	v = inst->readAcceleration();
	std::cout << "<" << v.x << ", " << v.y << ", " << v.z << ">" << std::endl;
	Accelerometer::DestroyInstance();

	return 0;
}

