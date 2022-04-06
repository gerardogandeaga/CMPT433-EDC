
#include <iostream>
#include <thread>
#include <chrono>

#include "i2c_utilities.h"
#include "accelerometer.h"
#include "lcd_screen.h"
#include "vibrationSensor.h"
#include "node.h"

void lcd_test()
{
	LCDScreen *LCD = LCDScreen::Get();

	std::string str = "Hello World!";

	while (1) {
		std::cout << "Displaying message \"" << str << "\" to LCD..." << std::endl;
		LCD->ClearDisplay();
		// LCD->WriteMessageTopLine(str);
		std::cout << "Write a message to LCD: " << std::endl;
		std::getline(std::cin >> std::ws, str);
	}
}

int main() 
{
	std::cout << "Earthquake Detection Cluster" << std::endl;
	std::cout << "============================" << std::endl;

	// initialize i2c
	i2c_utilities::initI2c();

	// init the node
	Node::Initialize();

	while (1);

	Node::End();

	//lcd_test();

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

