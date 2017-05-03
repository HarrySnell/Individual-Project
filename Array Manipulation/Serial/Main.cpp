#include <iostream>
#include <ctime>
#include <math.h>

int main() {
	const int width = 26000; // Defines array width and height 
	const int height = width;
	float* array_1 = new float[width * height];
	float* array_output = new float[width * height];
	// Initialises the input array 
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			array_1[y*width + x] = x;
		}
	}
	// Manipulates the input array as shown in section 14.3, and times the duration. 
	std::clock_t start = std::clock(); 
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			array_output[y*width + x] = sqrt((0.5 * sqrt(array_1[y*width + x])) * pow(array_1[y*width + x], array_1[y*width + x]));
		}
	}
	std::clock_t finish = std::clock();
	double time = (finish - start) / (double)CLOCKS_PER_SEC;
	std::cout << time << std::endl;
	delete array_1;
	delete array_output;
	std::cin.ignore();
	return 0;
}