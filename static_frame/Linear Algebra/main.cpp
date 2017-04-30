///////////////////////////////////////////////////////////
// Aurthor: Harry Snell - 140342321
// Date: 30/04/17
// Desciption: OpenCL Linear Algebra static frame ray tracer - main C++ 
// File 1 of 2
// N.B. OpenCL and regular C++ have been seperated by a line of '////' where possible 
///////////////////////////////////////////////////////////

#include <iostream>
#include <CL/cl.hpp> // OpenCL header - tested with both the AMD and Intel OpenCL SDKs 
#include <fstream>
#include <ctime>

void savebmp(const char *filename, int w, int h, int dpi, cl_float3 *data) {
	// Function to save image as a bitmap file 
	FILE *fp;
	int area = w*h;
	int s = 4 * area;
	int filesize = 54 + s;

	double factor = 39.375;
	int m = static_cast<int>(factor);

	int ppm = dpi * m;
	unsigned char bmpfileheader[14] = { 'B', 'M' , 0,0,0,0, 0,0,0,0, 54,0,0,0 };
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,24,0 };

	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);

	bmpinfoheader[4] = (unsigned char)(w);
	bmpinfoheader[5] = (unsigned char)(w >> 8);
	bmpinfoheader[6] = (unsigned char)(w >> 16);
	bmpinfoheader[7] = (unsigned char)(w >> 24);

	bmpinfoheader[8] = (unsigned char)(h);
	bmpinfoheader[9] = (unsigned char)(h >> 8);
	bmpinfoheader[10] = (unsigned char)(h >> 16);
	bmpinfoheader[11] = (unsigned char)(h >> 24);

	bmpinfoheader[21] = (unsigned char)(s);
	bmpinfoheader[22] = (unsigned char)(s >> 8);
	bmpinfoheader[23] = (unsigned char)(s >> 16);
	bmpinfoheader[24] = (unsigned char)(s >> 24);

	bmpinfoheader[25] = (unsigned char)(ppm);
	bmpinfoheader[26] = (unsigned char)(ppm >> 8);
	bmpinfoheader[27] = (unsigned char)(ppm >> 16);
	bmpinfoheader[28] = (unsigned char)(ppm >> 24);

	bmpinfoheader[29] = (unsigned char)(ppm);
	bmpinfoheader[30] = (unsigned char)(ppm >> 8);
	bmpinfoheader[31] = (unsigned char)(ppm >> 16);
	bmpinfoheader[32] = (unsigned char)(ppm >> 24);

	fp = fopen(filename, "wb");
	fwrite(bmpfileheader, 1, 14, fp);
	fwrite(bmpinfoheader, 1, 40, fp);

	for (int i = 0; i < area; i++) {

		double red = (data[i].x) * 255;
		double green = (data[i].y) * 255;
		double blue = (data[i].z) * 255;

		unsigned char color[3] = { (int)floor(blue) , (int)floor(green), (int)floor(red) };
		fwrite(color, 1, 3, fp);
	}
	fclose(fp);
	//std::cout << "done" << std::endl;
}
cl_float8 createSphere(float x, float y, float z, float r, float R, float G, float B) {
	// Function to create an eight item float vector representing a sphere 
	// The vector components represents the following:
		// s0 = sphere centre x coordinate
		// s1 = sphere centre y coordinate
		// s2 = sphere centre z coordinate
		// s3 = sphere radius 
		// s4 = sphere colour red component
		// s5 = sphere colour green component
		// s6 = sphere colour blue component
		// s7 = blank
	return { x, y, z, r, R, G, B, 0 };
}

int main() {
	//std::cin.ignore(); // Testing statement - allows energy monitor to be started at the same time
	// Image Parameters: 
	const int saveFlag = 0; // set to 1 to save every frame as a bitmap in the program directory 
	const int width = 1920; // Image width
	const int height = 1080; // Image height
	const int frames = 750; // Number of frames to be rendered
	///////////////////////////////////////////////////////////
	std::clock_t start = std::clock(); // Creates a start time stamp 
	///////////////////////////////////////////////////////////
	// OpenCL initialisation statements. 
	// Creates platform, selects device, reads kernel file, creates context, builds program
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	auto platform = platforms.front(); // Uncomment to use an integrated GPU or CPU. Following line must be commented out.
	//auto platform = platforms.at(1); // Uncomment to use a discrete GPU. Previous line must be commented out. 
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices); // Change to CL_DEVICE_TYPE_CPU to run on the CPU 
	auto device = devices.front();
	std::ifstream testFile("LA_RT_kernel.cl"); 
	std::string src(std::istreambuf_iterator<char>(testFile), (std::istreambuf_iterator<char>()));
	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
	//std::cout << src << std::endl; // debug statement - prints out entire kernel file 
	cl::Context context(devices);
	cl::Program program(context, sources);
	auto err = program.build("-cl-std=CL1.1"); // Restricts OpenCL version to 1.1 
	///////////////////////////////////////////////////////////
	cl_float3 *outputs = new cl_float3[width*height]; // Three item float vector array to store the RGB values of the output image 	
	// Defines the camera location and look at point
	// Creates the image plane and camera basis vectors
	cl_float3 cameraOrigin = { 0,0,0 };
	cl_float3 cameraLookingAt = { 0,0,1 };
	cl_float3 cameraDirection = { cameraOrigin.x - cameraLookingAt.x ,cameraOrigin.y - cameraLookingAt.y , cameraOrigin.z - cameraLookingAt.z };
	float magnitude = sqrt((cameraDirection.x * cameraDirection.x) + (cameraDirection.y * cameraDirection.y) + (cameraDirection.z * cameraDirection.z));
	cl_float3 cameraDirectionNormalised = { ((cameraDirection.x) / magnitude), ((cameraDirection.y) / magnitude), ((cameraDirection.z) / magnitude) };
	const float verticalFOV = 120;
	float distance = (height / 2) / (tan(verticalFOV / 2));// Distance from the camera origin to the image plane!
	cl_float3 imagePlaneCentre = { cameraOrigin.x - (distance * cameraDirectionNormalised.x) ,cameraOrigin.y - (distance * cameraDirectionNormalised.y) ,cameraOrigin.z - (distance * cameraDirectionNormalised.z) };
	cl_float3 u = { cameraDirectionNormalised.x , cameraDirectionNormalised.y ,cameraDirectionNormalised.z };
	float vXIntermediate = -u.z;
	float vZIntermediate = u.x;
	float vMagnitude = sqrt((vXIntermediate * vXIntermediate) + (vZIntermediate * vZIntermediate));
	cl_float3 v = { vXIntermediate / vMagnitude ,0,vZIntermediate / vMagnitude };
	cl_float3 w = { (u.y * v.z) - (u.z * v.y) ,(u.z * v.x) - (u.x * v.z) ,(u.x * v.y) - (u.y * v.x) };
	cl_float3 imagePlaneOrigin = { imagePlaneCentre.x - (v.x * (width / 2)) - (w.x * (height / 2)) ,imagePlaneCentre.y - (v.y * (width / 2)) - (w.y * (height / 2)) ,imagePlaneCentre.z - (v.z * (width / 2)) - (w.z * (height / 2)) };

	// Creates the scene spheres
	const int numberOfSpheres = 75;
	cl_float8 *spheres = new cl_float8[numberOfSpheres];
	for (int t = 0; t < numberOfSpheres; t++) {
		spheres[t] = createSphere(-(((t*t))/3000)+1, -(((t*t)) / 3000) + 0, (t)+1, 0.5, 0.375, 0.75, 0.75);
	}
	// Defines the plane 
	cl_float3 planeNormal = { 0,1, 0 };
	cl_float3 planePoint = { 0,5000,0 };//0 5000 0
	cl_float3 planeColour{ 0,0,1 };
	// Defines the light position and colour 
	cl_float3 lightPosition = { 10,-40,10 }; // 10 10 10
	cl_float3 lightColour = { 1,1,1 };
	// Defines the background colour 
	cl_float3 backgroundColour = { 0.25,0.25,0.25 };
	///////////////////////////////////////////////////////////
	// Creates the memory buffers to be passed to the kernel, and creates the kernel 
	cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(cl_float3)* width * height);
	cl::Buffer spheresBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(cl_float8)*numberOfSpheres);
	cl::Kernel kernel(program, "testKernel", &err);
	///////////////////////////////////////////////////////////
	// Sets kernel input and output arguments 
	kernel.setArg(0, outputBuffer);
	kernel.setArg(1, v);
	kernel.setArg(2, w);
	kernel.setArg(3, imagePlaneOrigin);
	kernel.setArg(4, cameraOrigin);
	kernel.setArg(5, lightPosition);
	kernel.setArg(6, lightColour);
	kernel.setArg(7, backgroundColour);
	kernel.setArg(8, planeNormal);
	kernel.setArg(9, planePoint);
	kernel.setArg(10, planeColour);
	kernel.setArg(11, spheresBuffer);
	kernel.setArg(12, numberOfSpheres);
	///////////////////////////////////////////////////////////
	cl::CommandQueue queue(context, device); // Sets the command queue
	// Writes the array of spheres to the corresponding memory buffer. 
	queue.enqueueWriteBuffer(spheresBuffer, CL_FALSE, 0, sizeof(cl_float8)*numberOfSpheres, spheres);
	///////////////////////////////////////////////////////////
	for (int f = 0; f < frames; f++) {
		// Runs the kernel the number of times defined by the 'frames' variable 
		//std::cout << "Current frame: " << f << std::endl; // Debug statment, uncommenting this will affect performance! 
		///////////////////////////////////////////////////////////
		// Runs Kernel and retreives the output buffer containing the RGB data for the current frame. 
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
		queue.enqueueReadBuffer(outputBuffer, CL_FALSE, 0, sizeof(cl_float3) * width * height, outputs);
		queue.finish();
		///////////////////////////////////////////////////////////
		// Saves output image as a bitmap in the active directory if the save flag is 1. 
		if (saveFlag == 1) {
			std::string s = std::to_string(f);
			s.append(".bmp");
			char const *filename = s.c_str();
			savebmp(filename, width, height, 72, outputs);
		}
	}
	///////////////////////////////////////////////////////////
	// Creates an end time stamp 
	// Prints out test data 
	std::clock_t finish = std::clock();
	double duration = (finish - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Number of spheres: " << std::endl;
	std::cout << "Number of frames: " << frames << std::endl;
	std::cout << "Resolution: " << width << " by " << height << std::endl;
	std::cout << "Duration: " << duration << " seconds" << std::endl;
	std::cout << (double)frames / duration << " frames per second" << std::endl;
	if (saveFlag == 1) { std::cout << "Frames saved" << std::endl; }
	else { std::cout << "Frames not saved" << std::endl; }
	std::cout << std::endl;
	std::cout << "Press enter to exit" << std::endl;
	std::cin.ignore();
	///////////////////////////////////////////////////////////
	return 0;
}