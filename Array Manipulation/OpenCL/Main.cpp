#include <iostream>
#include <fstream>
#include <CL/cl.hpp>
#include <ctime>

int main() {
	const int width = 5000; // Defines the array width and height. 
	const int height = width;
	float* array_1 = new float[width * height];
	float* array_output = new float[width * height];
	// Populates the input array.
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			array_1[y*width + x] = x;
		}
	}
	std::clock_t start = std::clock();
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	auto platform = platforms.front(); // To use the CPU or intergrated GPU uncomment this line, and commment out the following line.
	//auto platform = platforms.at(1); // To use a discrete GPU uncomment this line, and comment out the proceeding line.
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_CPU, &devices); // To use either a discrete or integrated GPU change "CL_DEVICE_TYPE_CPU" to "CL_DEVICE_TYPE_GPU"
	auto device = devices.front();
	std::ifstream testFile("kernel1.cl");
	std::string src(std::istreambuf_iterator<char>(testFile), (std::istreambuf_iterator<char>()));
	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
	cl::Context context(devices);
	cl::Program program(context, sources);
	auto err = program.build("-cl-std=CL1.1");
	cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(float) * width *  height);
	cl::Buffer inputBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * width * height);
	cl::Kernel kernel(program, "testKernel", &err);
	kernel.setArg(0, outputBuffer);
	kernel.setArg(1, inputBuffer);
	cl::CommandQueue queue(context, device);
	queue.enqueueWriteBuffer(inputBuffer, CL_FALSE, 0, sizeof(float) * width * height, array_1);
	queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
	queue.enqueueReadBuffer(outputBuffer, CL_FALSE, 0, sizeof(float) * width * height, array_output);
	queue.finish();
	std::clock_t finish = std::clock();
	double duration = (finish - start) / (double)CLOCKS_PER_SEC;
	std::cout << duration << std::endl;
	delete array_1;
	delete array_output;
	std::cin.ignore();
	return 0;
}