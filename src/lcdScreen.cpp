#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <unistd.h>

#include "lcdScreen.h"

#define D4_GPIO_NUMBER 66
#define D5_GPIO_NUMBER 69
#define D6_GPIO_NUMBER 115
#define D7_GPIO_NUMBER 48
#define RS_GPIO_NUMBER 68
#define E_GPIO_NUMBER  67

#define NUM_PINS 6
#define NUM_DATABUS_PINS 4

LCDScreen *LCDScreen::instance = nullptr;
std::mutex LCDScreen::mtx;


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Construction and destruction of LCD instance.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */
LCDScreen::LCDScreen()
{
	std::cout << "Initializing LCD..." << std::endl;

	setUpPinToGPIOMapping();
	memset(status, 0, 4 * sizeof(*status));

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
	}

	// Set each data pin and E pin to 0.
	pinWrite(E, gpio_utilities::PinValue::LOW);
	pinWrite(D4, gpio_utilities::PinValue::LOW);
	pinWrite(D5, gpio_utilities::PinValue::LOW);
	pinWrite(D6, gpio_utilities::PinValue::LOW);
	pinWrite(D7, gpio_utilities::PinValue::LOW);

	// Enter command mode to begin initializing.
	setCommandMode();

	/*
	 * The sequence of commands used to initialize the LCD display were taken
	 * from
	 * https://web.alfredstate.edu/faculty/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
	 * under the section "4-Bit Interface, Initialization by Instruction."
	 */

	// Special Function Set 1.
	write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::microseconds(64));
	// Special Function Set 2.
	write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::microseconds(64));
	// Special Function Set 3.
	write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Sets to 4-bit operation.
	write4Bits(0x2); /* 0010 */
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	// Sets to 4-bit operation. Sets 2-line display. Selects 5x8 dot character font.
	write4Bits(0x2); /* 0010 - We can alternatively write 0000 here for 8-bit operation. */
	write4Bits(0x8); /* 1000 - We can alternatively write 1100 here for 5x10 dot font. */
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Display ON/OFF control.
	write4Bits(0x0); /* 0000 */
	write4Bits(0x8); /* 1000 */
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Sets mode to increment cursor position by 1 and shift right when writing to display.
	write4Bits(0x0); /* 0000 */
	write4Bits(0x6); /* 0110 */
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Clear the display.
	write4Bits(0x0); /* 0000 */
	write4Bits(0x1); /* 0001 */
	std::this_thread::sleep_for(std::chrono::microseconds(64));

	// Turns on display. This corresponds to the instruction 0000 1100 in binary.
	// To be able to see the cursor, use 0000 1110.
	// To enable cursor blinking, use 0000 1111.
	write4Bits(0x0); /* 0000 */
	write4Bits(0xC); /* 1100 */
	std::this_thread::sleep_for(std::chrono::microseconds(64));


	// Pull RS up to write data.
	setWriteMode();

	// playInitMessage();

	clearDisplay();
}

LCDScreen::~LCDScreen()
{
	// Turn display off.
	displayOff();
}

LCDScreen *LCDScreen::GetInstance(void)
{
	// Prevent creating multiple instances.
	std::lock_guard<std::mutex> lock(mtx);

	// Create instance if it doesn't exist.
	if (!instance) {
		instance = new LCDScreen();
	}
	return instance;
}

void LCDScreen::DestroyInstance(void)
{
	// Prevent multiple frees.
	std::lock_guard<std::mutex> lock(mtx);

	if (instance) {
		delete instance;
	}
	instance = nullptr;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Initialization message shown during initialization.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void LCDScreen::playInitMessage()
{
	std::string init_msg = "    Initializing Earthquake Detection Cluster . . .    ";
	for (int i = 0; i < 54; ++i)
	{
		// Set cursor position to the start of the top line.
		setCursorPosition(0, 0);
		// Compute the string we want to output.
		int str_start = i;
		int str_length = init_msg.length() - str_start;
		str_length = str_length > 16 ? 16 : str_length;
		std::string output_str = init_msg.substr(str_start, str_length);
		
		// Output string.
		writeMessage(output_str);

		// Sleep for 200 ms.
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Show status of calling node.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void LCDScreen::setStatus(bool is_master,
                          int num_nodes,
						  int node_magnitude,
						  int consensus_magnitude)
{
	int new_status[4] = {(int)is_master, num_nodes, node_magnitude, consensus_magnitude};

	// First, make sure the status has changed.
	if (memcmp(status, new_status, 4 * sizeof(*status)) != 0) {
		std::lock_guard<std::mutex> lock(mtx);
		
		// Clear display before updating.
		clearDisplay();

		// On the first line, output the severity.
		setCursorPosition(0, 0);
		switch (consensus_magnitude) {
			case 2:
				writeMessage("Weak");
				break;
			case 3:
				writeMessage("Light");
				break;
			case 4:
				writeMessage("Moderate");
				break;
			case 5:
				writeMessage("Strong");
				break;
			case 6:
				writeMessage("Very strong");
				break;
			case 7:
				writeMessage("Severe");
				break;
			case 8:
				writeMessage("Violent");
				break;
			case 9:
				writeMessage("Extreme");
				break;
			default:
				writeMessage("Not felt");
		}

		// Show if master node in the top left.
		setCursorPosition(0, 13);
		if (is_master) {
			writeMessage("[M]");
		}
		else {
			writeMessage("[*]");
		}

		// Show node and consensus magnitudes.
		setCursorPosition(1, 0);
		writeMessage(std::to_string(node_magnitude));
		writeMessage(" -- ");
		writeMessage(std::to_string(consensus_magnitude));

		// Show number of connected nodes.
		setCursorPosition(1, 13);
		writeMessage("(" + std::to_string(num_nodes) + ")");

		// Update status.
		memcpy(status, new_status, 4 * sizeof(*status));
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Miscellaneous private helper methods.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void LCDScreen::setCommandMode()
{
	// Set RS pin to LOW.
	pinWrite(RS, gpio_utilities::PinValue::LOW);
}

void LCDScreen::setWriteMode()
{
	// Set RS pin to HIGH.
	pinWrite(RS, gpio_utilities::PinValue::HIGH);
}

void LCDScreen::setUpPinToGPIOMapping()
{
	// Create a mapping from pin symbols to GPIO pins.
	pin_map.insert(std::pair<PinSymbol, int>(D4, D4_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(D5, D5_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(D6, D6_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(D7, D7_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(RS, RS_GPIO_NUMBER));
	pin_map.insert(std::pair<PinSymbol, int>(E, E_GPIO_NUMBER));
}


void LCDScreen::writeChar(char c)
{
	unsigned int upper_bits = (c >> 4);
	unsigned int lower_bits = c & 0xF;
	write4Bits(upper_bits);
	write4Bits(lower_bits);
}

void LCDScreen::write4Bits(uint8_t value)
{
	for (int i = 0; i < NUM_DATABUS_PINS; ++i) {
		gpio_utilities::PinValue pin_value = (value >> i) & 0x01 ?
		                                     gpio_utilities::PinValue::HIGH :
											 gpio_utilities::PinValue::LOW;
		pinWrite((PinSymbol)i, pin_value);
	}
	pulseEnable();
}

void LCDScreen::pinWrite(PinSymbol pin, gpio_utilities::PinValue value)
{
	int gpio_number = pin_map.find(pin)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(gpio_number, value)) {
		std::cerr << "Failed to set gpio" << gpio_number << " to " << value;
		std::cerr << "in LCDScreen::PinWrite." << std::endl;
	}
}

void LCDScreen::pulseEnable()
{
	pinWrite(E, gpio_utilities::PinValue::HIGH);
	std::this_thread::sleep_for(std::chrono::microseconds(600));
	pinWrite(E, gpio_utilities::PinValue::LOW);
	std::this_thread::sleep_for(std::chrono::microseconds(600));
}

void LCDScreen::clearDisplay()
{
	setCommandMode();

	// Instruction to clear display.
	write4Bits(0x00);
	write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::microseconds(50));

	setWriteMode();
}

void LCDScreen::displayOn()
{
	// Enter command mode.
	setCommandMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));

	// Instruction to turn on display (without showing cursor) corresponds to
	// 0000 1100 in binary.
	write4Bits(0x0);
	write4Bits(0xC);
	std::this_thread::sleep_for(std::chrono::microseconds(50));
	
	// Display stays in write mode by default.
	setWriteMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));
}

void LCDScreen::displayOff()
{
	// Enter command mode.
	setCommandMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));

	// Instruction to turn off display corresponds to 0000 1000 in binary.
	write4Bits(0x0);
	write4Bits(0x8);
	std::this_thread::sleep_for(std::chrono::microseconds(50));
	
	// Display stays in write mode by default.
	setWriteMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));
}

void LCDScreen::setCursorPosition(int row, int column)
{
	// Enter command mode.
	setCommandMode();

	if ((0 <= row && row <= 1) && (0 <= column && column <= 15)) {
		// Instruction corresponding to setting the location of the cursor
		// is given by 1xxx xxxx in binary, where the bits denoted by x 
		// tells us the location. Thus, 0x80 = 1000 0000 corresponds to the
		// start of the first line, and 1100 0000 corresponds to the start
		// of the second line.
		uint8_t val = (row == 0) ? 0x80 : 0xC0;
		val += column;
		write4Bits(val >> 4);
		write4Bits(val & 0xf);
	}
	else {
		std::cerr << "Cannot set cursor position in LCDScreen::SetCursorPosition." << std::endl;
		std::cerr << "Must have: " << std::endl;
		std::cerr << "\t0 <= row <= 1;" << std::endl;
		std::cerr << "\t0 <= col <= 15." << std::endl;
	}

	// Display stays in write mode by default.
	setWriteMode();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Output method for debugging.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void LCDScreen::writeMessage(std::string message)
{
	for (char ch : message) {
		writeChar(ch);
	}
}
