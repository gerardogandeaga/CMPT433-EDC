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
	cursor_position_row = 0;
	cursor_position_col = 0;

	// First, export the necessary GPIO pins.
	for (const auto symbol_pin_pair : pin_map) {
		if (!gpio_utilities::ExportGPIOPin(symbol_pin_pair.second)) {
			std::cerr << "Failed to export gpio" << symbol_pin_pair.second;
			std::cerr << " in LCDScreen constructor." << std::endl;
			abort(); 
		}

		// Sleep for 300 ms seconds after exporting each pin.
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		if (!gpio_utilities::WriteToGPIODirectionFile(symbol_pin_pair.second,
		                                              gpio_utilities::PinDirection::OUT)) {
			std::cerr << "Failed to set gpio" << symbol_pin_pair.second << " direction to OUT";
			std::cerr << " in LCDScreen constructor." << std::endl;
			abort();
		}
		// do we want to crash if the LCD doesn't initialize?
	}

	// Set each data pin and E pin to 0.
	PinWrite(E, gpio_utilities::PinValue::LOW);
	PinWrite(D4, gpio_utilities::PinValue::LOW);
	PinWrite(D5, gpio_utilities::PinValue::LOW);
	PinWrite(D6, gpio_utilities::PinValue::LOW);
	PinWrite(D7, gpio_utilities::PinValue::LOW);

	SetCommandMode();

	/*
	 * The sequence of commands used to initialize the LCD display were taken
	 * from
	 * https://web.alfredstate.edu/faculty/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
	 * under the section "4-Bit Interface, Initialization by Instruction."
	 */

	// Special Function Set 1
	Write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	
	// Special Function Set 2
	Write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	// Special Function Set 3
	Write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::microseconds(150));

	// Initial Function Set to change interface
	Write4Bits(0x02); // 0011
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
	SetWriteMode();
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

void LCDScreen::SetCommandMode()
{
	// Set RS pin to LOW.
	PinWrite(RS, gpio_utilities::PinValue::LOW);
}

void LCDScreen::SetWriteMode()
{
	// Set RS pin to HIGH.
	PinWrite(RS, gpio_utilities::PinValue::HIGH);
}

void LCDScreen::SetEnable(gpio_utilities::PinValue pin_value)
{
	PinWrite(E, pin_value);
}

void LCDScreen::ClearDisplay()
{
	SetCommandMode();
	// Clear Display
	Write4Bits(0x00);
	Write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	SetWriteMode();
}


void LCDScreen::WriteMessage(std::string str)
{
	for (std::string::iterator it=str.begin(); it != str.end(); ++it) {
		WriteChar(*it);
	}
}

void LCDScreen::WriteChar(char c)
{
	unsigned int upperBits = (c >> 4);
	unsigned int lowerBits = c & 0xF;
	Write4Bits(upperBits);
	Write4Bits(lowerBits);
}

void LCDScreen::Write4Bits(uint8_t value)
{
	for (int i = 0; i < NUM_DATABUS_PINS; ++i) {
		gpio_utilities::PinValue pin_value = (value >> i) & 0x01 ?
		                                     gpio_utilities::PinValue::HIGH :
											 gpio_utilities::PinValue::LOW;
		PinWrite((PinSymbol)i, pin_value);
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


void LCDScreen::PinWrite(PinSymbol pin, gpio_utilities::PinValue value)
{
	int gpio_number = pin_map.find(pin)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(gpio_number, value)) {
		std::cerr << "Failed to set gpio" << gpio_number << " to " << value;
		std::cerr << "in LCDScreen::PinWrite." << std::endl;
	}
}

void LCDScreen::SetCursorPosition(int row, int col)
{
	if ((0 <= row && row <= 15) && (0 <= col && col <= 1)) {
		for (int i = 0; i < row; ++i) {
			ShiftCursorRight();
		}
	}
	else {
		std::cerr << "Cannot set cursor position in LCDScreen::SetCursorPosition." << std::endl;
		std::cerr << "Must have: " << std::endl;
		std::cerr << "\t0 <= row <= 15;" << std::endl;
		std::cerr << "\t0 <= col <= 1." << std::endl;
	}
}

void LCDScreen::ShiftCursorRight()
{
	SetCommandMode();

	uint8_t val = 0x14;

	unsigned int upperBits = val >> 4;
	unsigned int lowerBits = val & 0xF;
	Write4Bits(upperBits);
	Write4Bits(lowerBits);

	SetWriteMode();
}
