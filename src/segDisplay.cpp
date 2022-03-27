#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <chrono>

#include "segDisplay.h"
#include "node.h"
#include "i2c_utilities.h"
#include "gpio_utilities.h"


#define SEG_DISP_GPIO_1 61
#define SEG_DISP_GPIO_2 44

// device address
#define SEG_DISP_I2C_ADDR 0x20
// left and right digit addresses
#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_OUTA 0x14
#define REG_OUTB 0x15

// helper function
static void setDigit(int digit, unsigned char *low, unsigned char *high);

SegDisplay *SegDisplay::instance{nullptr};
std::mutex SegDisplay::mtx;

SegDisplay::SegDisplay()
{
    setup();
    stopWorker = false;
	workerThread = std::thread(&SegDisplay::worker, this);
}

SegDisplay::~SegDisplay() 
{
	stopWorker = true;
	workerThread.join();

    gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_1, gpio_utilities::LOW);
    gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_2, gpio_utilities::LOW);

    i2c_utilities::endDeviceControl(i2cFd);
}

void SegDisplay::worker() 
{
    while (!stopWorker) {
        displayLevel();
    }
}

void SegDisplay::setup(void)
{
    // export gpio pins
    gpio_utilities::ExportGPIOPin(SEG_DISP_GPIO_1);
    gpio_utilities::ExportGPIOPin(SEG_DISP_GPIO_2);

    // set directions
    gpio_utilities::WriteToGPIODirectionFile(SEG_DISP_GPIO_1, gpio_utilities::OUT);
    gpio_utilities::WriteToGPIODirectionFile(SEG_DISP_GPIO_2, gpio_utilities::OUT);

    // get i2c device control
    i2cFd = i2c_utilities::getI2cBus1DeviceControl(SEG_DISP_I2C_ADDR);
    i2c_utilities::writeToI2c(i2cFd, REG_DIRA, 0x00);
    i2c_utilities::writeToI2c(i2cFd, REG_DIRB, 0x00);
}

void SegDisplay::displayLevel()
{
    static int delay = 5000000;

    Node *nodeRef = Node::GetInstanceIfExits();
    if (nodeRef != nullptr) {
        // shake level
        int nodeShakeLevel = nodeRef->getNodeQuakeMagnitude();

        // set the digit values
        unsigned char lLow, lHigh, rLow, rHigh;
        setDigit(nodeShakeLevel / 10, &lLow, &lHigh);
        setDigit(nodeShakeLevel % 10, &rLow, &rHigh);

        // reset
        gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_1, gpio_utilities::LOW);
        gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_2, gpio_utilities::LOW);

        // write digits
        i2c_utilities::writeToI2c(i2cFd, REG_OUTA, lLow);
        i2c_utilities::writeToI2c(i2cFd, REG_OUTB, lHigh);
        gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_1, gpio_utilities::HIGH);

        std::this_thread::sleep_for(std::chrono::nanoseconds(delay));

        // reset
        gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_1, gpio_utilities::LOW);
        gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_2, gpio_utilities::LOW);

        i2c_utilities::writeToI2c(i2cFd, REG_OUTA, rLow);
        i2c_utilities::writeToI2c(i2cFd, REG_OUTB, rHigh);
        gpio_utilities::WriteToGPIOValueFile(SEG_DISP_GPIO_2, gpio_utilities::HIGH);
        std::this_thread::sleep_for(std::chrono::nanoseconds(delay));
    }
}


SegDisplay *SegDisplay::GetInstance()
{
	std::lock_guard<std::mutex> lock(mtx);

	if (instance == nullptr)
		instance = new SegDisplay();

	return instance;
}

void SegDisplay::DestroyInstance(void)
{
	std::lock_guard<std::mutex> lock(mtx);
	delete instance;
	instance = nullptr;
}

static void setDigit(int digit, unsigned char *low, unsigned char *high)
{
	switch (digit)
	{
	case 0:
		*low = 0xA1;
		*high = 0x86;
		break;

	case 1:
		*low = 0x80;
		*high = 0x12;
		break;

	case 2:
		*low = 0x31;
		*high = 0xE;
		break;

	case 3:
		*low = 0xB0;
		*high = 0x6;
		break;

	case 4:
		*low = 0x90;
		*high = 0x8A;
		break;

	case 5:
		*low = 0xB0;
		*high = 0x8C;
		break;

	case 6:
		*low = 0xB1;
		*high = 0x8C;
		break;

	case 7:
		*low = 0x4;
		*high = 0x14;
		break;

	case 8:
		*low = 0xB1;
		*high = 0x8E;
		break;

	case 9:
		*low = 0x90;
		*high = 0x8E;
		break;

	default:
		*low = 0x00;
		*high = 0x00;
		break;
	}
}
