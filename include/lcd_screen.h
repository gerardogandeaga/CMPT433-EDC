#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

#include <map>
#include <string>
#include "gpio_utilities.h"

/*
 * Interface for the Adafruit Standard 16x2 LCD screen on the
 * Beaglebone Green.
 * https://www.adafruit.com/product/181
 *
 * Author: Kenneth So, Vladimir Mishel
 *
 * The following is a list of Beaglebone pins that were used to connect
 * to each data line of the LCD screen.
 * RS -- P8_10
 * E  -- P8_8
 * D4 -- P8_7
 * D5 -- P8_9
 * D6 -- P9_27
 * D7 -- P9_15
 */

class LCDScreen {
private:
	static LCDScreen *instance;

public:
    inline static void Initialize() { LCDScreen::instance = new LCDScreen(); }
	inline static void Destroy() { delete LCDScreen::instance; }
	inline static LCDScreen *Get() { return instance; }

	LCDScreen(LCDScreen const &) = delete;
	void operator=(LCDScreen const &) = delete;

    // Write a full string to the LCD.
    void WriteMessage(std::string str);

    // Clear the LCD display.
    void ClearDisplay();

    void SetCursorPosition(int row, int col);
    void ShiftCursorRight();

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

    void SetCommandMode();
    void SetWriteMode();

    void SetUpPinToGPIOMapping();

    // Set a pin to HIGH or LOW.
    void PinWrite(PinSymbol pin, gpio_utilities::PinValue value);

    // Set RS pin to HIGH (write data) or LOW (write instruction).
    void SetWriteMode(gpio_utilities::PinValue pinVal);

    // Set E pin to HIGH or LOW.
    void SetEnable(gpio_utilities::PinValue pinVal);

    // Derived from Arduino LiquidCrystal library
    // Write 4 bits to D4, D5, D6, D7 and pulse the LCD to read these values
    void Write4Bits(uint8_t value);

    // Tell the LCD board to read the databus by flashing the 'E' pin from high to low.
    void PulseEnable();

    // Write a single character to the LCD.
    void WriteChar(char c);

    // PinSymbol - GPIO number mapping for easy reference to
    // GPIO files.
    std::map<PinSymbol, int> pin_map;

    // Cursor position tracking variables.
    int cursor_position_row;
    int cursor_position_col;
};

#endif // LCD_SCREEN_H
