 
///////////////////////////////////////////////////////////
// N.B. As this application is very similar to the linear algebra static frame ray tracer,
// only the changes made to the application have been commented. 
///////////////////////////////////////////////////////////

#include <iostream>
#include <CL/cl.hpp>
#include <fstream>
#include <ctime>

void savebmp(const char *filename, int w, int h, int dpi, cl_float3 *data) {
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
	return { x, y, z, r, R, G, B, 0 };
}

int main() {
	//std::cin.ignore();
	std::clock_t start = std::clock();
	///////////////////////////////////////////////////////////
	const int saveFlag = 1;
	const int width = 1920;
	const int height = 1080;
	const int frames = 750;
	const cl_float3 baseTranslationVector = { 0.001,0,0 }; // Defines the vector for which the scene geometry is translated by per frame.
	///////////////////////////////////////////////////////////
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	auto platform = platforms.front();
	//auto platform = platforms.at(1);
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	auto device = devices.front();
	std::ifstream testFile("LA_RT_TRANSLATIONS_kernel.cl"); 
	std::string src(std::istreambuf_iterator<char>(testFile), (std::istreambuf_iterator<char>()));
	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
	//std::cout << src << std::endl;
	cl::Context context(devices);
	cl::Program program(context, sources);
	auto err = program.build("-cl-std=CL1.1");
	///////////////////////////////////////////////////////////
	cl_float3 *outputs = new cl_float3[width*height];
	cl_float3 cameraOrigin = { 0,0,0 };
	cl_float3 cameraLookingAt = { 0,0,1 };
	cl_float3 cameraDirection = { cameraOrigin.x - cameraLookingAt.x ,cameraOrigin.y - cameraLookingAt.y , cameraOrigin.z - cameraLookingAt.z };
	float magnitude = sqrt((cameraDirection.x * cameraDirection.x) + (cameraDirection.y * cameraDirection.y) + (cameraDirection.z * cameraDirection.z));
	cl_float3 cameraDirectionNormalised = { ((cameraDirection.x) / magnitude), ((cameraDirection.y) / magnitude), ((cameraDirection.z) / magnitude) };
	const float verticalFOV = 120;
	float distance = (height / 2) / (tan(verticalFOV / 2));
	cl_float3 imagePlaneCentre = { cameraOrigin.x - (distance * cameraDirectionNormalised.x) ,cameraOrigin.y - (distance * cameraDirectionNormalised.y) ,cameraOrigin.z - (distance * cameraDirectionNormalised.z) };
	cl_float3 u = { cameraDirectionNormalised.x , cameraDirectionNormalised.y ,cameraDirectionNormalised.z };
	float vXIntermediate = -u.z;
	float vZIntermediate = u.x;
	float vMagnitude = sqrt((vXIntermediate * vXIntermediate) + (vZIntermediate * vZIntermediate));
	cl_float3 v = { vXIntermediate / vMagnitude ,0,vZIntermediate / vMagnitude };
	cl_float3 w = { (u.y * v.z) - (u.z * v.y) ,(u.z * v.x) - (u.x * v.z) ,(u.x * v.y) - (u.y * v.x) };
	cl_float3 imagePlaneOrigin = { imagePlaneCentre.x - (v.x * (width / 2)) - (w.x * (height / 2)) ,imagePlaneCentre.y - (v.y * (width / 2)) - (w.y * (height / 2)) ,imagePlaneCentre.z - (v.z * (width / 2)) - (w.z * (height / 2)) };

	const int numberOfSpheres = 75;
	cl_float8 *spheres = new cl_float8[numberOfSpheres];
	for (int t = 0; t < numberOfSpheres; t++) {
		spheres[t] = createSphere(-(((t*t)) / 3000) + 1, -(((t*t)) / 3000) + 0, (t)+1, 0.5, 0.375, 0.75, 0.75);
	}
	cl_float3 planeNormal = { 0,1, 0 };
	cl_float3 planePoint = { 0,5000,0 };
	cl_float3 planeColour{ 0,0,1 };
	cl_float3 lightPosition = { 10,-40,10 }; 
	cl_float3 lightColour = { 1,1,1 };
	cl_float3 backgroundColour = { 0.25,0.25,0.25 };


	// As the camera is fixed, the same primary rays are used for all frames.
	// The primary rays are calculated in series and stored in arrays to be passed to the Kernel 
	cl_float3 *primaryRays = new cl_float3[height * width];
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int id = (y*width) + x;
			cl_float3 currentPixel;
			currentPixel.x = imagePlaneOrigin.x + (v.x * x) + (w.x * y);
			currentPixel.y = imagePlaneOrigin.y + (v.y * x) + (w.y * y);
			currentPixel.z = imagePlaneOrigin.z + (v.z * x) + (w.z * y);
			primaryRays[id] = { currentPixel.x - cameraOrigin.x,currentPixel.y - cameraOrigin.y, currentPixel.z - cameraOrigin.z };
			float primaryRayMagnitude = sqrt((primaryRays[id].x * primaryRays[id].x) + (primaryRays[id].y * primaryRays[id].y) + (primaryRays[id].z * primaryRays[id].z));
			primaryRays[id] = { primaryRays[id].x / primaryRayMagnitude, primaryRays[id].y / primaryRayMagnitude, primaryRays[id].z / primaryRayMagnitude };
		}
	}
	
	///////////////////////////////////////////////////////////
	cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(cl_float3)* width * height);
	cl::Buffer spheresBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(cl_float8)*numberOfSpheres);
	cl::Buffer primaryRaysBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(cl_float3) * width * height);
	cl::Kernel kernel(program, "testKernel", &err);
	///////////////////////////////////////////////////////////
	kernel.setArg(0, outputBuffer);
	kernel.setArg(1, primaryRaysBuffer);
	kernel.setArg(2, spheresBuffer);
	kernel.setArg(3, numberOfSpheres);
	kernel.setArg(4, planeNormal);
	kernel.setArg(5, planePoint);
	kernel.setArg(6, planeColour);
	kernel.setArg(7, lightPosition);
	kernel.setArg(8, lightColour);
	kernel.setArg(9, cameraOrigin);
	kernel.setArg(10, backgroundColour);
	///////////////////////////////////////////////////////////
	cl::CommandQueue queue(context, device);
	///////////////////////////////////////////////////////////
	// Writes all buffers whose data set does not change to the kernel 
	queue.enqueueWriteBuffer(primaryRaysBuffer, CL_FALSE, 0, sizeof(cl_float3)* height * width, primaryRays);
	///////////////////////////////////////////////////////////
	for (int f = 0; f < frames; f++) {
		//std::cout << "Current frame : " << f << std::endl;
		if (f != 0) {
			// translates the scene geometry by the basis translation vector
			for (int n = 0; n < numberOfSpheres; n++) {
				spheres[n].s0 = spheres[n].s0 + baseTranslationVector.x;
				spheres[n].s1 = spheres[n].s1 + baseTranslationVector.y;
				spheres[n].s2 = spheres[n].s2 + baseTranslationVector.z;
			}
			planePoint = { planePoint.x + baseTranslationVector.x, planePoint.y + baseTranslationVector.y, planePoint.z + baseTranslationVector.z };
			lightPosition = { lightPosition.x + baseTranslationVector.x, lightPosition.y + baseTranslationVector.y, lightPosition.z + baseTranslationVector.z };
		}
		// Writes buffers containing scene geometry to the kernel 
		// N.B. As the plane and light position are stored as vectors and not arrays, they do not require to be manually updated. 
		queue.enqueueWriteBuffer(spheresBuffer, CL_FALSE, 0, sizeof(cl_float8)* numberOfSpheres, spheres);
		///////////////////////////////////////////////////////////
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
		queue.enqueueReadBuffer(outputBuffer, CL_FALSE, 0, sizeof(cl_float3) * width * height, outputs);
		queue.finish();
		///////////////////////////////////////////////////////////
		if (saveFlag == 1) {
			std::string s = std::to_string(f);
			s.append(".bmp");
			char const *filename = s.c_str();
			savebmp(filename, width, height, 72, outputs);
		}
	}
	///////////////////////////////////////////////////////////
	std::clock_t finish = std::clock();
	double duration = (finish - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Number of frames: " << frames << std::endl;
	std::cout << "Resolution: " << width << " by " << height << std::endl;
	std::cout << "Duration: " << duration << " seconds" << std::endl;
	std::cout << (double)frames / duration << " frames per second" << std::endl;
	if (saveFlag == 1) { std::cout << "Frames saved" << std::endl; }
	else { std::cout << "Frames not saved" << std::endl; }
	std::cout << std::endl;
	std::cout << "Press enter to exit" << std::endl;
	std::cin.ignore();
	return 0;
}