__kernel void testKernel(__global float* outputData, __global float* inputData){
	outputData[(get_global_id(1) * get_global_size(0)) + get_global_id(0)] = 
	sqrt((0.5 * inputData[(get_global_id(1) * get_global_size(0)) + get_global_id(0)]) * 
	pow(inputData[(get_global_id(1) * get_global_size(0)) + get_global_id(0)],inputData[(get_global_id(1) * get_global_size(0)) + get_global_id(0)]));
}