#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>
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

    // Set top and bottom messages of the LCD screen.
    void SetTopMessage(std::string message);
    void SetBottomMessage(std::string message);

    void WriteMessageBottomLine(std::string str);

    // Clear the LCD display.
    void ClearDisplay();

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

    void Worker();

    void OutputTopMessage();
    void OutputBottomMessage();

    // Set modes for either entering commands, or writing to the LCD screen.
    void SetCommandMode();
    void SetWriteMode();

    void SetUpPinToGPIOMapping();

    // Set a pin to HIGH or LOW.
    void PinWrite(PinSymbol pin, gpio_utilities::PinValue value);

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

    // Cursor control.
    void CursorHome();
    void SetCursorPosition(int row, int col);
    void ShiftCursorRight();

    // Messages which are currently being displayed on the top and bottom line.
    std::string top_message;
    std::string bottom_message;

    std::thread worker_thread;
    std::atomic<bool> stop_worker;
    std::mutex lcd_mutex;
};

#endif // LCD_SCREEN_H
