#include <iostream>
#include "lcd_screen.h"

int main(void)
{
	LCDScreen::Initialize();

	auto *lcd = LCDScreen::Get();

	lcd->SetTopMessage("This is a few chars.");

	std::this_thread::sleep_for(std::chrono::seconds(100));

	LCDScreen::Destroy();

	return 0;
}
