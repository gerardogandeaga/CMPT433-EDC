#include <cstring>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "gpio_utilities.h"

#define EXPORT_FILE_PATH "/sys/class/gpio/export"

namespace gpio_utilities {

bool ExportGPIOPin(int gpio_number)
{
	FILE *export_file = fopen(EXPORT_FILE_PATH, "w");
	if (!export_file) {
		std::cerr << "Unable to open " << EXPORT_FILE_PATH;
		std::cerr << " in gpio_utilities::ExportGPIOPin." << std::endl;
		return false;
	}

	std::string gpio_number_str = std::to_string(gpio_number);
	int bytes_written = fprintf(export_file, "%s", gpio_number_str.c_str());
	if (bytes_written <= 0) {
		perror("ERROR");
		fclose(export_file);
		return false;
	}

	fclose(export_file);
	return true;
}

std::string GetGPIODirectory(int gpio_number)
{
	std::string base = "/sys/class/gpio/gpio";
	std::string gpio_number_str = std::to_string(gpio_number);
	std::string result = base + gpio_number_str + "/";
	return result;
}

bool WriteToGPIOValueFile(int gpio_number, PinValue value)
{
	std::string gpio_value_file_path = GetGPIODirectory(gpio_number) + "value";
	FILE *gpio_value_file = fopen(gpio_value_file_path.c_str(), "w");
	if (!gpio_value_file) {
		std::cerr << "Unable to open " << gpio_value_file_path;
		std::cerr << " in gpio_utilities::WriteToGPIOValueFile." << std::endl;
		return false;
	}

	std::string pin_value = std::to_string(value);
	int bytes_written = fprintf(gpio_value_file, "%s", pin_value.c_str());
	if (bytes_written <= 0) {
		perror("ERROR");
		fclose(gpio_value_file);
		return false;
	}

	fclose(gpio_value_file);
	return true;
}

bool WriteToGPIODirectionFile(int gpio_number, PinDirection direction) {
	std::string gpio_direction_file_path = GetGPIODirectory(gpio_number) + "direction";
	FILE *gpio_direction_file = fopen(gpio_direction_file_path.c_str(), "w");
	if (!gpio_direction_file) {
		std::cerr << "Unable to open " << gpio_direction_file_path;
		std::cerr << " in gpio_utilities::WriteToGPIODirectionFile." << std::endl;
		return false;
	}

	// Set direction to `in` if specified direction is IN,
	// or set it to `out` otherwise.
	std::string direction_str = (direction == IN) ?
	                            std::string("in") :
	                            std::string("out");
	int bytes_written = fprintf(gpio_direction_file, "%s", direction_str.c_str());
	if (bytes_written <= 0) {
		perror("ERROR");
		fclose(gpio_direction_file);
		return false;
	}

	fclose(gpio_direction_file);
	return true;
}

char *ReadFromGPIOValueFile(int gpio_number) {
	// Allocate buffer to read into.
	size_t buffer_size = 8;
	char *buffer = (char *)malloc(sizeof(*buffer) * buffer_size);
	memset(buffer, 0, sizeof(*buffer) * buffer_size);

	// Obtain GPIO value file.
	std::string gpio_value_file_path = GetGPIODirectory(gpio_number) + "value";
	FILE *gpio_value_file = fopen(gpio_value_file_path.c_str(), "r");
	if (!gpio_value_file) {
		std::cerr << "Unable to open " << gpio_value_file_path;
		std::cerr << " in gpio_utilities::ReadFromGPIOValueFile." << std::endl;
		return nullptr;
	}

	// Read into buffer.
	int bytes_read = getline(&buffer, &buffer_size, gpio_value_file);
	fclose(gpio_value_file);

	if (bytes_read <= 0) {
		perror("ERROR");
		std::cerr << "Read <= 0 bytes ";
		std::cerr << "in gpio_utilities::ReadFromGPIOValueFile." << std::endl;
		return nullptr;
	}
	return buffer;
}

} // gpio_utilities
