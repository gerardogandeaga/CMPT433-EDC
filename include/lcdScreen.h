#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

#include <map>
#include <mutex>
#include <string>
#include "gpio_utilities.h"

#define NUM_STATUS_PARAMETERS 3

/*
 * Interface for the Adafruit Standard 16x2 LCD screen on the
 * Beaglebone Green.
 * https://www.adafruit.com/product/181
 *
 * Authors: Kenneth So, Vladimir Mishel
 *
 * The following is a list of Beaglebone pins that were used to connect
 * to each data line of the LCD screen.
 * RS -- P8_10
 * E  -- P8_8
 * D4 -- P8_7
 * D5 -- P8_9
 * D6 -- P9_27
 * D7 -- P9_15
 * 
 * Notes:
 * - In our implementation, the LCD is set to entry mode by default. That is,
 * after execution of any method in this class, the LCD is not in command entry
 * mode.
 */

class LCDScreen {
private:
	static LCDScreen *instance;

public:
    static LCDScreen *GetInstance();
	static void DestroyInstance();

	LCDScreen(LCDScreen const &) = delete;
	void operator=(LCDScreen const &) = delete;

    // Sets status of the calling node.
    void setStatus(int num_nodes, int node_magnitude, int consensus_magnitude);

    // Debugging.
    void writeMessage(std::string);

private:
    enum PinSymbol {
        D4,    /* 0 */
        D5,    /* 1 */
        D6,    /* 2 */
        D7,    /* 3 */
        RS,    /* 4 */
        E      /* 5 */
    };

	LCDScreen();
	~LCDScreen();

    // Outputs initialization message on the LCD screen.
    void playInitMessage();

    // Sets modes for either entering commands, or writing to the LCD screen.
    void setCommandMode();
    void setWriteMode();

    void setUpPinToGPIOMapping();

    // Sets a pin to HIGH or LOW.
    void pinWrite(PinSymbol pin, gpio_utilities::PinValue value);

    // Writes a single character to the LCD.
    void writeChar(char c);

    // Derived from Arduino LiquidCrystal library.
    // Writes 4 bits to D4, D5, D6, D7 and pulse the LCD to read these values
    void write4Bits(uint8_t value);

    // Tells the LCD board to read the databus by flashing the 'E' pin from high to low.
    void pulseEnable();

    // Clears the LCD display.
    void clearDisplay();

    // Display ON/OFF control.
    void displayOn();
    void displayOff();

    // Cursor control.
    void setCursorPosition(int row, int column);

    // PinSymbol - GPIO number mapping for easy reference to GPIO files.
    std::map<PinSymbol, int> pin_map;
    // LCD mutex.
    static std::mutex mtx;
    // LCD status array for data caching.
    int status[NUM_STATUS_PARAMETERS];
};

#endif // LCD_SCREEN_H
