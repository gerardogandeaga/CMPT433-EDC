#include <chrono>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <unistd.h>

#include "gpio_utilities.h"
#include "lcd_screen.h"

#define D0_GPIO_NUMBER 69
#define D1_GPIO_NUMBER 66
#define D2_GPIO_NUMBER 112
#define D3_GPIO_NUMBER 115
#define D4_GPIO_NUMBER 117
#define D5_GPIO_NUMBER 49
#define D6_GPIO_NUMBER 48
#define D7_GPIO_NUMBER 20
#define RS_GPIO_NUMBER 67
#define E_GPIO_NUMBER  68

#define NUM_PINS 10
#define NUM_RAM_PINS 7

LCDScreen *LCDScreen::instance = nullptr;

LCDScreen::LCDScreen() {
	SetUpPinToGPIOMapping();

	// First, export the necessary GPIO pins.
	for (const auto symbol_pin_pair : pin_map) {
		gpio_utilities::ExportGPIOPin(symbol_pin_pair.second);
	}

	// Sleep for 300 ms after exporting.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// For each RAM pin, set direction to `out`.
	for (int i = 0; i < NUM_RAM_PINS; i++) {
		std::map<PinSymbol, int>::iterator it = pin_map.find((PinSymbol)i);
		if (it != pin_map.end()) {
			gpio_utilities::WriteToGPIODirectionFile(it->second,
			                                         gpio_utilities::PinDirection::OUT);
		}
	}

	SignalEnable();
	SetWriteMode();
}

LCDScreen::~LCDScreen() {

}

void LCDScreen::SetUpPinToGPIOMapping() {
	// Create a mapping from pin symbols to GPIO pins.
	int gpio_pins[NUM_PINS] = {
		69,
		66,
		112,
		115,
		117,
		49,
		48,
		20,
		67,
		68
	};

	for (int i = 0; i < NUM_PINS; i++) {
		pin_map.insert(std::pair<PinSymbol, int>((PinSymbol)i, gpio_pins[i]));
	}
}

void LCDScreen::SetWriteMode() {
	int RS_gpio_number = pin_map.find(RS)->second;
	std::cout << RS_gpio_number << std::endl;
	if (!gpio_utilities::WriteToGPIOValueFile(RS_gpio_number,
		                                      gpio_utilities::PinValue::HIGH))
	{
		std::cerr << "Failed to set RS pin to HIGH ";
		std::cerr << "in LCDScreen::SetWriteMode." << std::endl;
	}
}

void LCDScreen::SignalEnable() {
	int E_gpio_number = pin_map.find(E)->second;
	std::cout << E_gpio_number << std::endl;
	if (!gpio_utilities::WriteToGPIOValueFile(E_gpio_number,
		                                      gpio_utilities::PinValue::HIGH))
	{
		std::cerr << "Failed to set E pin to HIGH ";
		std::cerr << "in LCDScreen::SignalEnable." << std::endl;
	}
}

void PrintDatabusContents() {
	
}
