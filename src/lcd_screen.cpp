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

	// Pull RS and EN low
	PinWrite(RS, gpio_utilities::PinValue::LOW);
	PinWrite(E, gpio_utilities::PinValue::LOW);

	//Write4Bits(0x00);
	//Write4Bits(0x01);

	//== Initialization by Instruction ==//
	// Special Function set 1
	Write4Bits(0x03);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	
	// Special Function set 2
	Write4Bits(0x03);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	// Special Function set 3
	Write4Bits(0x03);
	std::this_thread::sleep_for(std::chrono::microseconds(200));

	// Function set to change interface
	Write4Bits(0x02);
	std::this_thread::sleep_for(std::chrono::microseconds(200));

	// Function set (0010NF**) = (00101000)
	Write4Bits(0x02);
	Write4Bits(0x08); // (1000)
	std::this_thread::sleep_for(std::chrono::microseconds(200));

	// Display ON/OFF
	Write4Bits(0x00);
	Write4Bits(0x08);
	std::this_thread::sleep_for(std::chrono::microseconds(200));

	// Clear display
	Write4Bits(0x00);
	Write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	// Entry mode set
	Write4Bits(0x00);
	Write4Bits(0x06);
	std::this_thread::sleep_for(std::chrono::microseconds(200));
	//== Initialization Done ==//


	// Display ON/OFF with blinking cursor
	Write4Bits(0x00);
	Write4Bits(0x0F);
	std::this_thread::sleep_for(std::chrono::microseconds(200));

	// Clear display
	Write4Bits(0x00);
	Write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	/*
	// Return home
	Write4Bits(0x00);
	Write4Bits(0x02);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	*/

	// Pull RS up to write data
	PinWrite(RS, gpio_utilities::PinValue::HIGH);

	// Write Data
	Write4Bits(0x04);
	Write4Bits(0x08);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "Done!" << std::endl;
}

LCDScreen::~LCDScreen()
{

}

void LCDScreen::SetUpPinToGPIOMapping()
{
	// Create a mapping from pin symbols to GPIO pins.
	int gpio_pins[NUM_PINS] = {
		D4_GPIO_NUMBER,
		D5_GPIO_NUMBER,
		D6_GPIO_NUMBER,
		D7_GPIO_NUMBER,
		RS_GPIO_NUMBER,
		E_GPIO_NUMBER
	};

	for (int i = 0; i < NUM_PINS; ++i) {
		pin_map.insert(std::pair<PinSymbol, int>((PinSymbol)i, gpio_pins[i]));
	}
}

void LCDScreen::SetWriteMode()
{
	int RS_gpio_number = pin_map.find(RS)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(RS_gpio_number, 
	                                          gpio_utilities::PinValue::HIGH)) {
		std::cerr << "Failed to set RS pin to HIGH ";
		std::cerr << "in LCDScreen::SetWriteMode." << std::endl;
	}
}

void LCDScreen::SignalEnable()
{
	int E_gpio_number = pin_map.find(E)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(E_gpio_number, 
	                                          gpio_utilities::PinValue::HIGH)) {
		std::cerr << "Failed to set E pin to HIGH ";
		std::cerr << "in LCDScreen::SignalEnable." << std::endl;
	}
}

void LCDScreen::PinWrite(PinSymbol pin, int pinVal)
{
	int gpio_number = pin_map.find(pin)->second;
	//std::cout << "Writing " << pinVal << " to " << gpio_number << std::endl;
	if (!gpio_utilities::WriteToGPIOValueFile(gpio_number, (gpio_utilities::PinValue) pinVal)) {
		std::cerr << "Failed to set gpio " << gpio_number << " to " << pinVal;
		std::cerr << "in LCDScreen::PinWrite." << std::endl;
	}
}

// Derived from Arduino LiquidCrystal library
void LCDScreen::PulseEnable()
{
	PinWrite(E, gpio_utilities::PinValue::LOW);
	std::this_thread::sleep_for(std::chrono::microseconds(200));
	PinWrite(E, gpio_utilities::PinValue::HIGH);
	std::this_thread::sleep_for(std::chrono::microseconds(1000));
	PinWrite(E, gpio_utilities::PinValue::LOW);
	std::this_thread::sleep_for(std::chrono::microseconds(200));
}

// Derived from Arduino LiquidCrystal library
void LCDScreen::Write4Bits(uint8_t value)
{
	for (int i = 0; i < NUM_DATABUS_PINS; ++i) {
		PinWrite((PinSymbol) i, ((value >> i) & 0x01));
	}
	PulseEnable();
}

void LCDScreen::PrintDatabusContents()
{
	std::string result = "Pins initialized.\n";
	result += "Databus contents...\n";
	for (int i = 0; i < NUM_DATABUS_PINS; ++i) {
		result += "D";
		result += std::to_string(i+4);
		result += ": ";

		// Get GPIO number.
		int gpio_number = pin_map.find((PinSymbol)i)->second;
		// Get buffer of contents.
		char *buffer = gpio_utilities::ReadFromGPIOValueFile(gpio_number);
		if (buffer) {
			result += std::string(buffer);
			free(buffer);
			buffer = nullptr;
		}
	}
	result += "Now ready to initialize for commands.\n";
	std::cout << result;
}
