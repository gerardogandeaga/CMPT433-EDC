/*
Code pretty much copied from Acccelerometer. Update as needed.
*/
#ifndef _VIBRATIONSENSOR_H_
#define _VIBRATIONSENSOR_H_

#include <mutex>
#include <thread>

#define PULSE_BUFFER_SIZE 10

class VibrationSensor {

public:
    // delete these functions to enforce singleton behaviour
	VibrationSensor(VibrationSensor &other) = delete;
	void operator=(const VibrationSensor &) = delete;
	
	// this will act like out publicly available contructor
	static VibrationSensor *GetInstance(void);
	static void DestroyInstance(void); // Our manual destructor, should be explicitly called for cleanup!

	// Returns latest pulse value
	int getPulse(void);

private:
    // singleton statics
	static VibrationSensor *instance;
	static std::mutex mtx;

	// thread handler
	bool active;
	std::thread workerThread;

	int zeroCount;
	int pulse;

	// Reads digital value of vibration detection and updates pulse
	void readVibration(void);

protected:
    VibrationSensor();
    ~VibrationSensor();

	// Threading
	bool stopWorker;
	void worker();

};

#endif