namespace gpio_utilities {

enum PinValue {
    LOW,   /* 0 */
    HIGH   /* 1 */
};

enum PinDirection {
	IN,    /* 0 */
	OUT    /* 1 */
};

// Export GPIO pin specified by `gpio_number`.
// Returns true on export success, false on
// export failure.
bool ExportGPIOPin(int gpio_number);

// Returns the directory for the GPIO pin specified by `gpio_number`.
std::string GetGPIODirectory(int gpio_number);

// Writes `value` to the GPIO value file specified by `gpio_number`.
// Returns true on success, false on failure.
bool WriteToGPIOValueFile(int gpio_number, PinValue value);

// Writes `direction` to the GPIO direction file specified by
// `gpio_number`.
// Returns true on success, false on failure.
bool WriteToGPIODirectionFile(int gpio_number, PinDirection direction);

// Reads from GPIO value file specified by `gpio_number` into a 
// character buffer.
// Returns pointer to said buffer upon success, and nullptr
// upon failure.
// NOTE: Must call free on returned pointer at some point!
char *ReadFromGPIOValueFile(int gpio_number);

} // gpio_utilities
