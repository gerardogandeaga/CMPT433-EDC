
#include <iostream>
#include <thread>
#include <chrono>

#include "accelerometer.h"
#include "lcd_screen.h"
#include "vibrationSensor.h"
#include "node.h"

int main() 
{
	std::cout << "Earthquake Detection Cluster" << std::endl;
	std::cout << "============================" << std::endl;

	// init the 
	Node::GetInstance();

	while (1);

	Node::DestroyInstance();


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

	LCDScreen *LCD = LCDScreen::Get();

	std::string str = "Hello World!";

	while (1) {
		std::cout << "Displaying message \"" << str << "\" to LCD..." << std::endl;
		LCD->ClearDisplay();
		LCD->WriteMessage(str);
		std::cout << "Write a message to LCD: " << std::endl;
		std::getline(std::cin >> std::ws, str);
	}

	return 0;
}

