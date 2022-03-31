#include <iostream>
#include "lcd_screen.h"

int main(void)
{
	LCDScreen::Initialize();

	auto *lcd = LCDScreen::Get();

	lcd->SetStatus("Not felt", true);

	std::this_thread::sleep_for(std::chrono::seconds(5));

	LCDScreen::Destroy();

	return 0;
}
