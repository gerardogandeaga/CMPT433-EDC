/*
Code pretty much copied from Acccelerometer. Update as needed.
*/
#ifndef _VIBRATIONSENSOR_H_
#define _VIBRATIONSENSOR_H_

#include <mutex>

class VibrationSensor {

public:
    // delete these functions to enforce singleton behaviour
	VibrationSensor(VibrationSensor &other) = delete;
	void operator=(const VibrationSensor &) = delete;
	
	// this will act like out publicly available contructor
	static VibrationSensor *GetInstance(void);
	static void DestroyInstance(void); // Our manual destructor, should be explicitly called for cleanup!

	// TODO: currently checks for vibrations for 10 secs, printing 0 or 1 depending on if a vibration is detected.
    // Can try measuring the "length" of a vibration by counting the time it takes for vibration to end.
	void readVibration(void);
private:
    // singleton statics
	static VibrationSensor *instance;
	static std::mutex mtx;

    const std::string VIBRATION_IN;

protected:
    VibrationSensor();
    ~VibrationSensor();
};

#endif