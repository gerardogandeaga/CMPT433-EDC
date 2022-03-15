#ifndef LCDSCREEN_H
#define LCDSCREEN_H

/*
 * Interface for the Adafruit Standard 16x2 LCD screen on the
 * Beaglebone Green.
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

private:
	LCDScreen();
	~LCDScreen();

};

#endif // LCDSCREEN_H
