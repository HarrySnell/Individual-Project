///////////////////////////////////////////////////////////
// N.B. As this application is very similar to the Geometric Algebra static frame ray tracer,
// only the changes made to the application have been commented. 
///////////////////////////////////////////////////////////

#include <CL/cl.hpp>
#include <iostream>
#include <ctime>
#include <fstream>

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
	return { x, y, z,(float)((x * x + y * y + z * z) / 2.0 - r / 2.0 * r), 1.0, R, G, B };
}

int main() {
	//std::cin.ignore();
	clock_t start = std::clock();
	///////////////////////////////////////////////////////////////
	const int saveFlag = 1;
	const int frames = 750;
	const int height = 1080;
	const int width = 1920;
	const int size = height * width;
	const float verticalFOV = 120;
	const float theta = 0.001; // Defines the angle of rotation per frame 
	///////////////////////////////////////////////////////////////
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	auto platform = platforms.front();
	//auto platform = platforms.at(1);
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	auto device = devices.front();
	std::ifstream testFile("GA_RT_Y_AXIS_ROTATION_kernel.cl"); 
	std::string src(std::istreambuf_iterator<char>(testFile), (std::istreambuf_iterator<char>()));
	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
	//std::cout << src << std::endl;
	cl::Context context(devices);
	cl::Program program(context, sources);
	auto err = program.build("-cl-std=CL1.1");
	///////////////////////////////////////////////////////////////
	cl_float3  cam = { 0,0,0 };
	cl_float3 *outputData = new cl_float3[size];
	float cameraOrigin[6];
	cameraOrigin[1] = cam.x; // e1
	cameraOrigin[2] = cam.y; // e2
	cameraOrigin[3] = cam.z; // e3
	cameraOrigin[4] = (cam.x * cam.x + cam.y * cam.y + cam.z * cam.z) / 2.0; // einf
	cameraOrigin[5] = 1.0; // e0
	cl_float3 cameraLookAt = { 0,0,1 };
	float cameraLookAtMagnitude = sqrt((cameraLookAt.x * cameraLookAt.x) + (cameraLookAt.y * cameraLookAt.y) + (cameraLookAt.z * cameraLookAt.z));
	cl_float3 cameraDirectionNormalised = { (cameraLookAt.x - cam.x) / cameraLookAtMagnitude , (cameraLookAt.y - cam.y) / cameraLookAtMagnitude , (cameraLookAt.z - cam.z) / cameraLookAtMagnitude };
	float distance = (height / 2) / (tan(verticalFOV / 2));
	cl_float3 imagePlaneCentre = { cam.x + (distance * cameraDirectionNormalised.x) ,cam.y + (distance * cameraDirectionNormalised.y) ,cam.z + (distance * cameraDirectionNormalised.z) };
	cl_float3 u = { cameraDirectionNormalised.x , cameraDirectionNormalised.y ,cameraDirectionNormalised.z };
	float vXIntermediate = -u.z;
	float vZIntermediate = u.x;
	float vMagnitude = sqrt((vXIntermediate * vXIntermediate) + (vZIntermediate * vZIntermediate));
	cl_float3 v = { vXIntermediate / vMagnitude ,0,vZIntermediate / vMagnitude };
	cl_float3 w = { (u.y * v.z) - (u.z * v.y) ,(u.z * v.x) - (u.x * v.z) ,(u.x * v.y) - (u.y * v.x) };
	cl_float3 imagePlaneOrigin = { imagePlaneCentre.x - (v.x * (width / 2)) - (w.x * (height / 2)) ,imagePlaneCentre.y - (v.y * (width / 2)) - (w.y * (height / 2)) ,imagePlaneCentre.z - (v.z * (width / 2)) - (w.z * (height / 2)) };
	cl_float4 sphere_ = { 0,0,10,1 };
	
	const int numberOfSpheres = 75;
	cl_float8 spheres[numberOfSpheres]; // e1,e2,e3,einf,e0,r,g,b
	for (int t = 0; t < numberOfSpheres; t++) {
		spheres[t] = createSphere((((t*t)) / 3000) - 1, -(t*t) / 3000, t + 1, 0.5, 0.375, 0.75, 0.75);
	}
	cl_float3 backgroundColour = { 0.25,0.25,0.25 };
	cl_float3 lightPos = { 10,-10,10 };
	float lightPosition[6];
	lightPosition[1] = lightPos.x; // e1
	lightPosition[2] = lightPos.y; // e2
	lightPosition[3] = lightPos.z; // e3
	lightPosition[4] = (lightPos.x * lightPos.x + lightPos.y * lightPos.y + lightPos.z * lightPos.z) / 2.0; // einf
	lightPosition[5] = 1.0; // e0
	cl_float3 lightColour = { 1,1,1 };

	cl_float3 planeColour = { 0,0,1 };
	float plane[6];
	plane[1] = 0; // e1
	plane[2] = -1; // e2
	plane[3] = 0; // e3
	plane[4] = 4; // einf

	// Creates the rotation operator (R) and its reverse (R_reverse)
	cl_float2 R;
	R.s0 = cos(theta / 2.0); // 1.0
	R.s1 = sin(theta / 2.0); // e1 ^ e3
	cl_float2 R_reverse;
	R_reverse.s0 = R.s0; // 1.0
	R_reverse.s1 = (-R.s1); // e1 ^ e3
	

	float *primaryRays_6 = new float[size];
	float *primaryRays_7 = new float[size];
	float *primaryRays_8 = new float[size];
	float *primaryRays_10 = new float[size];
	float *primaryRays_11 = new float[size];
	float *primaryRays_13 = new float[size];
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int id = (y * width) + x;
			float currentPixel[6];
			currentPixel[1] = imagePlaneOrigin.x + (v.x * x) + (w.x * y);
			currentPixel[2] = imagePlaneOrigin.y + (v.y * x) + (w.y * y);
			currentPixel[3] = imagePlaneOrigin.z + (v.z * x) + (w.z * y);
			currentPixel[4] = (((currentPixel[1] * currentPixel[1]) + (currentPixel[2] * currentPixel[2]) + (currentPixel[3] * currentPixel[3]))) / 2.0;
			currentPixel[5] = 1.0;

			primaryRays_6[id] = cameraOrigin[3] + (-currentPixel[3]); // e1 ^ e2
			primaryRays_7[id] = (-(cameraOrigin[2] + (-currentPixel[2]))); // e1 ^ e3
			primaryRays_8[id] = (-(cameraOrigin[2] * currentPixel[3] + (-(cameraOrigin[3] * currentPixel[2])))); // e1 ^ einf
			primaryRays_10[id] = cameraOrigin[1] + (-currentPixel[1]); // e2 ^ e3
			primaryRays_11[id] = cameraOrigin[1] * currentPixel[3] + (-(cameraOrigin[3] * currentPixel[1])); // e2 ^ einf
			primaryRays_13[id] = (-(cameraOrigin[1] * currentPixel[2] + (-(cameraOrigin[2] * currentPixel[1])))); // e3 ^ einf
		}
	}
	///////////////////////////////////////////////////////////////
	cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(cl_float3) * size);
	cl::Buffer cameraOriginBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * 6);
	cl::Buffer lightPositionBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * 6);
	cl::Buffer planeBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * 6);
	cl::Buffer spheresBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(cl_float8) * numberOfSpheres);
	cl::Buffer primaryRays_6_Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * size);
	cl::Buffer primaryRays_7_Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * size);
	cl::Buffer primaryRays_8_Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * size);
	cl::Buffer primaryRays_10_Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * size);
	cl::Buffer primaryRays_11_Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * size);
	cl::Buffer primaryRays_13_Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(float) * size);
	///////////////////////////////////////////////////////////////
	cl::Kernel kernel(program, "testKernel", &err);
	///////////////////////////////////////////////////////////////
	kernel.setArg(0, outputBuffer);
	kernel.setArg(1, primaryRays_6_Buffer);
	kernel.setArg(2, primaryRays_7_Buffer);
	kernel.setArg(3, primaryRays_8_Buffer);
	kernel.setArg(4, primaryRays_10_Buffer);
	kernel.setArg(5, primaryRays_11_Buffer);
	kernel.setArg(6, primaryRays_13_Buffer);
	kernel.setArg(7, spheresBuffer);
	kernel.setArg(8, numberOfSpheres);
	kernel.setArg(9, planeBuffer);
	kernel.setArg(10, planeColour);
	kernel.setArg(11, lightPositionBuffer);
	kernel.setArg(12, lightColour);
	kernel.setArg(13, cameraOriginBuffer);
	kernel.setArg(14, backgroundColour);
	///////////////////////////////////////////////////////////////
	cl::CommandQueue queue(context, device);
	///////////////////////////////////////////////////////////////
	queue.enqueueWriteBuffer(cameraOriginBuffer, CL_FALSE, 0, sizeof(float) * 6, cameraOrigin);
	queue.enqueueWriteBuffer(primaryRays_6_Buffer, CL_FALSE, 0, sizeof(float) * size, primaryRays_6);
	queue.enqueueWriteBuffer(primaryRays_7_Buffer, CL_FALSE, 0, sizeof(float) * size, primaryRays_7);
	queue.enqueueWriteBuffer(primaryRays_8_Buffer, CL_FALSE, 0, sizeof(float) * size, primaryRays_8);
	queue.enqueueWriteBuffer(primaryRays_10_Buffer, CL_FALSE, 0, sizeof(float) * size, primaryRays_10);
	queue.enqueueWriteBuffer(primaryRays_11_Buffer, CL_FALSE, 0, sizeof(float) * size, primaryRays_11);
	queue.enqueueWriteBuffer(primaryRays_13_Buffer, CL_FALSE, 0, sizeof(float) * size, primaryRays_13);
	///////////////////////////////////////////////////////////////
	for (int f = 0; f < frames; f++) {
		//std::cout << "Current frame : " << f << std::endl;
		///////////////////////////////////////////////////////////////
		if (f != 0) {
			// Rotates the scene geometry by theta about the Y axis
			for (int n = 0; n < numberOfSpheres; n++) {
				cl_float4 spheres_;
				spheres_.s0 = (R.s0 * spheres[n].s0 + R.s1 * spheres[n].s2) * R_reverse.s0 + (-((R.s0 * spheres[n].s2 + (-(R.s1 * spheres[n].s0))) * R_reverse.s1)); // e1
				spheres_.s1 = R.s0 * spheres[n].s1 * R_reverse.s0 + (-(R.s1 * spheres[n].s1)) * R_reverse.s1; // e2
				spheres_.s2 = (R.s0 * spheres[n].s0 + R.s1 * spheres[n].s2) * R_reverse.s1 + (R.s0 * spheres[n].s2 + (-(R.s1 * spheres[n].s0))) * R_reverse.s0; // e3
				spheres_.s3 = R.s0 * spheres[n].s3 * R_reverse.s0 + (-(R.s0 * spheres[n].s3 * R_reverse.s1)); // einf
				spheres[n].s0 = spheres_.s0;
				spheres[n].s1 = spheres_.s1;
				spheres[n].s2 = spheres_.s2;
				spheres[n].s3 = spheres_.s3;
			}
			plane[4] = R.s0 * plane[4] * R_reverse.s0 + (-(R.s1 * plane[4] * R_reverse.s1)); // einf
			cl_float4 lightPosition_;
			lightPosition_.s0 = (R.s0 * lightPosition[1] + R.s1 * lightPosition[3]) * R_reverse.s0 + (-((R.s0 * lightPosition[3] + (-(R.s1 * lightPosition[1]))) * R_reverse.s1)); // e1
			lightPosition_.s1 = R.s0 * lightPosition[2] * R_reverse.s0 + (-(R.s1 * lightPosition[2])) * R_reverse.s1; // e2
			lightPosition_.s2 = (R.s0 * lightPosition[1] + R.s1 * lightPosition[3]) * R_reverse.s1 + (R.s0 * lightPosition[3] + (-(R.s1 * lightPosition[1]))) * R_reverse.s0; // e3
			lightPosition_.s3 = R.s0 * lightPosition[4] * R_reverse.s0 + (-(R.s1 * lightPosition[4] * R_reverse.s1)); // einf
			lightPosition[1] = lightPosition_.s0;
			lightPosition[2] = lightPosition_.s1;
			lightPosition[3] = lightPosition_.s2;
			lightPosition[4] = lightPosition_.s3;
		}
		///////////////////////////////////////////////////////////////
		queue.enqueueWriteBuffer(spheresBuffer, CL_FALSE, 0, sizeof(cl_float8) * numberOfSpheres, spheres);
		queue.enqueueWriteBuffer(lightPositionBuffer, CL_FALSE, 0, sizeof(float) * 6, lightPosition);
		queue.enqueueWriteBuffer(planeBuffer, CL_FALSE, 0, sizeof(float) * 6, plane);
		///////////////////////////////////////////////////////////////
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
		///////////////////////////////////////////////////////////////
		queue.enqueueReadBuffer(outputBuffer, CL_FALSE, 0, sizeof(cl_float3) * size, outputData);
		queue.finish();
		///////////////////////////////////////////////////////////////
		if (saveFlag == 1) {
			std::string s = std::to_string(f);
			s.append(".bmp");
			char const *filename = s.c_str();
			savebmp(filename, width, height, 72, outputData);
		}
	}
	///////////////////////////////////////////////////////////////
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