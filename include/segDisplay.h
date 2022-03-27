/*
Code pretty much copied from Acccelerometer. Update as needed.
*/
#ifndef _SEG_DISPLAY_H_
#define _SEG_DISPLAY_H_

#include <mutex>
#include <thread>

class SegDisplay {

public:
	SegDisplay(SegDisplay &other) = delete;
	void operator=(const SegDisplay &) = delete;

	static SegDisplay *GetInstance(void);
	static void DestroyInstance(void); 

	void displayLevel();

private:
	static SegDisplay *instance;
	static std::mutex mtx;

	std::thread workerThread;

	int i2cFd;

	// setup the i2c devices
	void setup(void);

protected:
    SegDisplay();
    ~SegDisplay();

	bool stopWorker;
	void worker();
};

#endif