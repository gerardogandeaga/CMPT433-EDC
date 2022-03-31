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

	// Enter command mode to begin initializing.
	SetCommandMode();

	/*
	 * The sequence of commands used to initialize the LCD display were taken
	 * from
	 * https://web.alfredstate.edu/faculty/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
	 * under the section "4-Bit Interface, Initialization by Instruction."
	 */

	// Special Function Set 1.
	Write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::microseconds(64));
	// Special Function Set 2.
	Write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::microseconds(64));
	// Special Function Set 3.
	Write4Bits(0x03); // 0011
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Sets to 4-bit operation.
	Write4Bits(0x2); /* 0010 */
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	// Sets to 4-bit operation. Sets 2-line display. Selects 5x8 dot character font.
	Write4Bits(0x2); /* 0010 - We can alternatively write 0000 here for 8-bit operation. */
	Write4Bits(0x8); /* 1000 - We can alternatively write 1100 here for 5x10 dot font. */
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Display ON/OFF control.
	Write4Bits(0x0); /* 0000 */
	Write4Bits(0x8); /* 1000 */
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Sets mode to increment cursor position by 1 and shift right when writing to display.
	Write4Bits(0x0); /* 0000 */
	Write4Bits(0x6); /* 0110 */
	std::this_thread::sleep_for(std::chrono::microseconds(128));

	// Clear the display.
	Write4Bits(0x0); /* 0000 */
	Write4Bits(0x1); /* 0001 */
	std::this_thread::sleep_for(std::chrono::microseconds(64));

	// Turns on display. This corresponds to the instruction 0000 1100 in binary.
	// To be able to see the cursor, use 0000 1110.
	// To enable cursor blinking, use 0000 1111.
	Write4Bits(0x0); /* 0000 */
	Write4Bits(0xC); /* 1100 */
	std::this_thread::sleep_for(std::chrono::microseconds(64));


	// Pull RS up to write data.
	SetWriteMode();

	PlayInitMessage();

	ClearDisplay();

	// stop_worker = false;
	// worker_thread = std::thread(&LCDScreen::Worker, this);
	// top_message_rotating = false;
	// top_message_index = 0;
}

void LCDScreen::PlayInitMessage()
{
	std::string init_msg = "    Initializing Earthquake Detection Cluster . . .    ";
	for (int i = 0; i < 54; ++i)
	{
		// Set cursor position to the start of the top line.
		SetCursorPosition(0, 0);
		// Compute the string we want to output.
		int str_start = i;
		int str_length = init_msg.length() - str_start;
		str_length = str_length > 16 ? 16 : str_length;
		std::string output_str = init_msg.substr(str_start, str_length);
		
		// Output string.
		WriteMessage(output_str);

		// Sleep for 200 ms.	
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}

LCDScreen::~LCDScreen()
{
	// Turn display off.
	DisplayOff();

	// stop_worker = true;
	// worker_thread.join();
}

void LCDScreen::SetStatus(std::string severity, bool isMaster)
{
	// On the first line, output the severity.
	SetCursorPosition(0, 0);
	WriteMessage(severity);

	// Show if master node.
	SetCursorPosition(0, 13);
	if (isMaster) {
		WriteMessage("[M]");
	}
	else {
		WriteMessage("[*]");
	}
}

void LCDScreen::DisplayOn()
{
	// Enter command mode.
	SetCommandMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));

	// Instruction to turn on display (without showing cursor) corresponds to
	// 0000 1100 in binary.
	Write4Bits(0x0);
	Write4Bits(0xC);
	std::this_thread::sleep_for(std::chrono::microseconds(50));
	
	// Display stays in write mode by default.
	SetWriteMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));
}

void LCDScreen::DisplayOff()
{
	// Enter command mode.
	SetCommandMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));

	// Instruction to turn off display corresponds to 0000 1000 in binary.
	Write4Bits(0x0);
	Write4Bits(0x8);
	std::this_thread::sleep_for(std::chrono::microseconds(50));
	
	// Display stays in write mode by default.
	SetWriteMode();
	std::this_thread::sleep_for(std::chrono::microseconds(50));
}

void LCDScreen::SetCursorPosition(int row, int column)
{
	// Enter command mode.
	SetCommandMode();

	if ((0 <= row && row <= 1) && (0 <= column && column <= 15)) {
		// Instruction corresponding to setting the location of the cursor
		// is given by 1xxx xxxx in binary, where the bits denoted by x 
		// tells us the location. Thus, 0x80 = 1000 0000 corresponds to the
		// start of the first line, and 1100 0000 corresponds to the start
		// of the second line.
		uint8_t val = (row == 0) ? 0x80 : 0xC0;
		val += column;
		Write4Bits(val >> 4);
		Write4Bits(val & 0xf);
	}
	else {
		std::cerr << "Cannot set cursor position in LCDScreen::SetCursorPosition." << std::endl;
		std::cerr << "Must have: " << std::endl;
		std::cerr << "\t0 <= row <= 1;" << std::endl;
		std::cerr << "\t0 <= col <= 15." << std::endl;
	}

	// Display stays in write mode by default.
	SetWriteMode();
}

void LCDScreen::Worker()
{
	while (!stop_worker) {
		// First, clear the screen.
		ClearDisplay();

		// // Output the top message.
		// OutputTopMessage();

		// // Output the bottom message.
		// OutputBottomMessage();

		// Sleep for 200 ms.	
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

/*
void LCDScreen::SetTopMessage(std::string message)
{
	// Lock messages for concurrency.
	std::lock_guard<std::mutex> lock(lcd_mutex);
	top_message = std::string(message);

	// Check if we have a long message.
	if (top_message.length() > 16) {
		// Add whitespace for clarity when displaying the message.
		top_message += "    ";
		top_message_rotating = true;
		top_message_index = 0;
	}
	else {
		top_message_rotating = false;
	}
}

void LCDScreen::OutputTopMessage() {
	// Set cursor position to the start of the top line.
	SetCursorPosition(0, 0);

	// Lock messages for concurrency.
	std::lock_guard<std::mutex> lock(lcd_mutex);

	std::string output_str = top_message;
	if (top_message_rotating) {
		// Compute the string we want to output.
		int str_start = top_message_index;
		int str_length = top_message.length() - str_start;
		str_length = str_length > 16 ? 16 : str_length;
		output_str = top_message.substr(str_start, str_length) + top_message.substr(0, 16 - str_length);
	}
	
	// Output string.
	for (char ch : output_str) {
		WriteChar(ch);
	}

	top_message_index = (top_message_index + 1) % top_message.length();
}

void LCDScreen::SetBottomMessage(std::string message)
{
	// Lock messages for concurrency.
	std::lock_guard<std::mutex> lock(lcd_mutex);
	bottom_message = std::string(message);
}

void LCDScreen::OutputBottomMessage()
{
	// Set cursor position to the start of the bottom line.
	SetCursorPosition(1, 0);

	// Lock messages for concurrency.
	std::lock_guard<std::mutex> lock(lcd_mutex);
	for (char ch : bottom_message) {
		WriteChar(ch);
	}
}
*/
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

void LCDScreen::ClearDisplay()
{
	SetCommandMode();

	// Instruction to clear display.
	Write4Bits(0x00);
	Write4Bits(0x01);
	std::this_thread::sleep_for(std::chrono::microseconds(50));

	SetWriteMode();
}

void LCDScreen::WriteMessage(std::string message)
{
	for (char ch : message) {
		WriteChar(ch);
	}
}

void LCDScreen::WriteChar(char c)
{
	unsigned int upper_bits = (c >> 4);
	unsigned int lower_bits = c & 0xF;
	Write4Bits(upper_bits);
	Write4Bits(lower_bits);
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
	PinWrite(E, gpio_utilities::PinValue::HIGH);
	std::this_thread::sleep_for(std::chrono::microseconds(600));
	PinWrite(E, gpio_utilities::PinValue::LOW);
	std::this_thread::sleep_for(std::chrono::microseconds(600));
}

void LCDScreen::PinWrite(PinSymbol pin, gpio_utilities::PinValue value)
{
	int gpio_number = pin_map.find(pin)->second;
	if (!gpio_utilities::WriteToGPIOValueFile(gpio_number, value)) {
		std::cerr << "Failed to set gpio" << gpio_number << " to " << value;
		std::cerr << "in LCDScreen::PinWrite." << std::endl;
	}
}
