#include <chrono>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <unistd.h>

#include "lcd_screen.h"

#define D4_GPIO_NUMBER 66
#define D5_GPIO_NUMBER 69
#define D6_GPIO_NUMBER 115
#define D7_GPIO_NUMBER 48
#define RS_GPIO_NUMBER 68
#define E_GPIO_NUMBER  67

#define NUM_PINS 6
#define NUM_DATABUS_PINS 4

LCDScreen *LCDScreen::instance = nullptr;

LCDScreen::LCDScreen()
{
	std::cout << "Initializing LCD..." << std::endl;

	SetUpPinToGPIOMapping();

	// First, export the necessary GPIO pins.
	for (const auto symbol_pin_pair : pin_map) {
		gpio_utilities::ExportGPIOPin(symbol_pin_pair.second);
	}

	// Sleep for 300 ms after exporting.
	std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // 300 ms per pin

	// For each pin, set direction to `out`
	for (int i = 0; i < NUM_PINS; ++i) {
		int gpio_number = pin_map.find((PinSymbol)i)->second;
		gpio_utilities::WriteToGPIODirectionFile(gpio_number, 
		                                         gpio_utilities::PinDirection::OUT);
	}

	// Set each data pin to 0
	PinWrite(D4, gpio_utilities::PinValue::LOW);
	PinWrite(D5, gpio_utilities::PinValue::LOW);
	PinWrite(D6, gpio_utilities::PinValue::LOW);
	PinWrite(D7, gpio_utilities::PinValue::LOW);

	// Pull RS and E low
	SetWriteMode(gpio_utilities::PinValue::LOW);
	SetEnable(gpio_utilities::PinValue::LOW);

	//== Initialization by Instruction ==//
	// Special Function Set 1
	Write4Bits(0x03);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	
	// Special Function Set 2
	Write4Bits(0x03);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	// Special Function Set 3
	Write4Bits(0x03);
	std::this_thread::sleep_for(std::chrono::microseconds(150));

	// Initial Function Set to change interface
	Write4Bits(0x02);
	std::this_thread::sleep_for(std::chrono::microseconds(150));

	// Function Set (0010NF**)
	Write4Bits(0x02); // (0010)
	Write4Bits(0x08); // (1000)
	std::this_thread::sleep_for(std::chrono::microseconds(100));

	// Display ON/OFF Control 
	// (DO NOT CONFIGURE DCB HERE)
	Write4Bits(0x00); // (0000)
	Write4Bits(0x08); // (1000)
	std::this_thread::sleep_for(std::chrono::microseconds(100));

	// Clear Display
	Write4Bits(0x00);
	Write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	// Entry Mode Set
	Write4Bits(0x00); // (0000)
	Write4Bits(0x06); // (0110)
	std::this_thread::sleep_for(std::chrono::microseconds(100));
	//== Initialization Done ==//

	// Display ON/OFF Control
	Write4Bits(0x00); // (0000)
	Write4Bits(0x0D); // (1100)
	std::this_thread::sleep_for(std::chrono::microseconds(100));

	// Clear Display
	Write4Bits(0x00);
	Write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	// Pull RS up to write data
	SetWriteMode(gpio_utilities::PinValue::HIGH);
}

LCDScreen::~LCDScreen()
{

}


void LCDScreen::SetUpPinToGPIOMapping()
{
	// Create a mapping from pin symbols to GPIO pins.
	pin_map.insert(std::pair<PinSymbol, int>(D4, D4_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(D5, D5_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(D6, D6_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(D7, D7_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(RS, RS_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(E, E_GPIO_NUMBER));
}

void LCDScreen::SetWriteMode(gpio_utilities::PinValue pinVal)
{
	int RS_gpio_number = pin_map.find(RS)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(RS_gpio_number, pinVal)) {
		std::cerr << "Failed to set RS pin to HIGH ";
		std::cerr << "in LCDScreen::SetWriteMode." << std::endl;
	}
}

void LCDScreen::SetEnable(gpio_utilities::PinValue pinVal)
{
	int E_gpio_number = pin_map.find(E)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(E_gpio_number, pinVal)) {
		std::cerr << "Failed to set E pin to HIGH ";
		std::cerr << "in LCDScreen::SignalEnable." << std::endl;
	}
}

void LCDScreen::ClearDisplay()
{
	SetWriteMode(gpio_utilities::PinValue::LOW);
	// Clear Display
	Write4Bits(0x00);
	Write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	SetWriteMode(gpio_utilities::PinValue::HIGH);
}


void LCDScreen::WriteMessage(std::string str)
{
	for (std::string::iterator it=str.begin(); it != str.end(); ++it) {
		WriteChar(*it);
	}
}

void LCDScreen::WriteChar(char c)
{
	unsigned int upperBits = (c >> 4) & 0xF;
	unsigned int lowerBits = c & 0xF;
	Write4Bits(upperBits);
	Write4Bits(lowerBits);
}

void LCDScreen::Write4Bits(uint8_t value)
{
	for (int i = 0; i < NUM_DATABUS_PINS; ++i) {
		PinWrite((PinSymbol) i, ((value >> i) & 0x01));
	}
	PulseEnable();
}

void LCDScreen::PulseEnable()
{
	SetEnable(gpio_utilities::PinValue::HIGH);
	std::this_thread::sleep_for(std::chrono::microseconds(1000));
	SetEnable(gpio_utilities::PinValue::LOW);
	std::this_thread::sleep_for(std::chrono::microseconds(400));
}


void LCDScreen::PinWrite(PinSymbol pin, int pinVal)
{
	int gpio_number = pin_map.find(pin)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(gpio_number, (gpio_utilities::PinValue) pinVal)) {
		std::cerr << "Failed to set gpio " << gpio_number << " to " << pinVal;
		std::cerr << "in LCDScreen::PinWrite." << std::endl;
	}
}
