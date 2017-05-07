///////////////////////////////////////////////////////////
// N.B. As this application is very similar to the linear algebra ray tracer with translations,
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
	const int saveFlag = 1;
	const int width = 1920;
	const int height = 1080;
	const int frames = 2;
	// Creates the axis of rotation through two user defined points
	const cl_float3 axisOrigin = { 1,1,1 };
	const cl_float3 axisDirection_ = { 4,1,2 };
	const float axisDirection_magnitude = sqrt((axisDirection_.x * axisDirection_.x) + (axisDirection_.y * axisDirection_.y) + (axisDirection_.z * axisDirection_.z));
	const cl_float3 axisDirection = { axisDirection_.x/ axisDirection_magnitude,axisDirection_.y / axisDirection_magnitude,axisDirection_.z / axisDirection_magnitude };
	float theta_Z = atan(-axisDirection.y / axisDirection.z);
	cl_float3 axisDirection_Z = { (axisDirection.x * cos(theta_Z)) - (axisDirection.y * sin(theta_Z)),(axisDirection.x * sin(theta_Z)) + (axisDirection.y * cos(theta_Z)),axisDirection.z };
	float theta_Y = atan(axisDirection_Z.x / -axisDirection_Z.z);
	const cl_float theta =  0.2; // Defines the angle of rotation per frame 
	///////////////////////////////////////////////////////////
	std::clock_t start = std::clock();
	///////////////////////////////////////////////////////////
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	auto platform = platforms.front();
	//auto platform = platforms.at(1);
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	auto device = devices.front();
	std::ifstream testFile("LA_RT_ARBITRARY_AXIS_ROTATION_kernel.cl"); // KERNEL NAME HERE !!!!
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
	float distance = (height / 2) / (tan(verticalFOV / 2));// Distance from the camera origin to the image plane!
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
	cl_float3 planePoint = { 0,5000,0 };//0 5000 0
	cl_float3 planeColour{ 0,0,1 };
	cl_float3 lightPosition = { 10,-40,10 }; // 10 10 10
	cl_float3 lightColour = { 1,1,1 };
	cl_float3 backgroundColour = { 0.25,0.25,0.25 };

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
	queue.enqueueWriteBuffer(primaryRaysBuffer, CL_FALSE, 0, sizeof(cl_float3)* height * width, primaryRays);
	///////////////////////////////////////////////////////////
	for (int f = 0; f < frames; f++) {
		std::cout << "Current frame : " << f << std::endl;
		if (f != 0) {
			// Rotates each of the spheres about the defined axis
			// Uses the algorithm described in section five of the dissertation 
			for (int n = 0; n < numberOfSpheres; n++) {
				spheres[n].s0 = spheres[n].s0 - axisOrigin.x;
				spheres[n].s1 = spheres[n].s1 - axisOrigin.y;
				spheres[n].s2 = spheres[n].s2 - axisOrigin.z;
				cl_float3 sphereCentre_Zrotation;
				sphereCentre_Zrotation.s0 = (spheres[n].s0 * cos(theta_Z)) - (spheres[n].s1 * sin(theta_Z));
				sphereCentre_Zrotation.s1 = (spheres[n].s0 * sin(theta_Z)) + (spheres[n].s1 * cos(theta_Z));
				sphereCentre_Zrotation.s2 = spheres[n].s2;
				cl_float3 sphereCentre_Yrotation;
				sphereCentre_Yrotation.s0 = (sphereCentre_Zrotation.s2 * sin(theta_Y)) + (sphereCentre_Zrotation.s0 * cos(theta_Y));
				sphereCentre_Yrotation.s1 = sphereCentre_Zrotation.s1;
				sphereCentre_Yrotation.s2 = (sphereCentre_Zrotation.s2 * cos(theta_Y)) - (sphereCentre_Zrotation.s0 * sin(theta_Y));
				cl_float3 sphereCentre_primaryRotation;
				sphereCentre_primaryRotation.s0 = (sphereCentre_Yrotation.s0 * cos(theta)) - (sphereCentre_Yrotation.s1 * sin(theta));
				sphereCentre_primaryRotation.s1 = (sphereCentre_Yrotation.s0 * sin(theta)) + (sphereCentre_Yrotation.s1 * cos(theta));
				sphereCentre_primaryRotation.s2 = sphereCentre_Yrotation.s2;
				cl_float3 sphereCentre_Yrotation_inverse;
				sphereCentre_Yrotation_inverse.s0 = (sphereCentre_primaryRotation.s2 * sin(-theta_Y)) + (sphereCentre_primaryRotation.s0 * cos(-theta_Y));
				sphereCentre_Yrotation_inverse.s1 = sphereCentre_primaryRotation.s1;
				sphereCentre_Yrotation_inverse.s2 = (sphereCentre_primaryRotation.s2 * cos(-theta_Y)) - (sphereCentre_primaryRotation.s0 * sin(-theta_Y));
				cl_float3 sphereCentre_Zrotation_inverse;
				sphereCentre_Zrotation_inverse.s0 = (sphereCentre_Yrotation_inverse.s0 * cos(-theta_Z)) - (sphereCentre_Yrotation_inverse.s1 * sin(-theta_Z));
				sphereCentre_Zrotation_inverse.s1 = (sphereCentre_Yrotation_inverse.s0 * sin(-theta_Z)) + (sphereCentre_Yrotation_inverse.s1 * cos(-theta_Z));
				sphereCentre_Zrotation_inverse.s2 = sphereCentre_Yrotation_inverse.s2;
				spheres[n].s0 = sphereCentre_Zrotation_inverse.s0 + axisOrigin.x;
				spheres[n].s1 = sphereCentre_Zrotation_inverse.s1 + axisOrigin.y;
				spheres[n].s2 = sphereCentre_Zrotation_inverse.s2 + axisOrigin.z;
			}
		}
		///////////////////////////////////////////////////////////
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