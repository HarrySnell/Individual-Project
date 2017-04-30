///////////////////////////////////////////////////////////
// Aurthor: Harry Snell - 140342321
// Date: 30/04/17
// Desciption: OpenCL Linear Algebra static frame ray tracer - OpenCL kernel  
// File 2 of 2 
///////////////////////////////////////////////////////////

__kernel void testKernel(
	__global float3* outputData, 
	float3 v, 
	float3 w, 
	float3 imagePlaneOrigin, 
	float3 cameraOrigin,
	float3 lightPosition, 
	float3 lightColour, 
	float3 backgroundColour, 
	float3 planeNormal, 
	float3 planePoint, 
	float3 planeColour,
	__global float8* spheres, 
int numberOfSpheres
){
		// Converts the current image plane x and y coordinates into a 1 dimensional array identifier 
		size_t id = (get_global_id(1) * get_global_size(0)) + get_global_id(0);
		
		// Creates current Pixel Location 
		float3 currentPixelLocation;
		currentPixelLocation.x = imagePlaneOrigin.x + (v.x * get_global_id(0)) + (w.x * get_global_id(1));
		currentPixelLocation.y = imagePlaneOrigin.y + (v.y * get_global_id(0)) + (w.y * get_global_id(1));
		currentPixelLocation.z = imagePlaneOrigin.z + (v.z * get_global_id(0)) + (w.z * get_global_id(1));
		
		// Create a line from the camera to the current Pixel - PRIMARY RAY!!
		float3 primaryRayDirection;
		primaryRayDirection.x = currentPixelLocation.x - cameraOrigin.x;
		primaryRayDirection.y = currentPixelLocation.y - cameraOrigin.y;
		primaryRayDirection.z = currentPixelLocation.z - cameraOrigin.z;
		float primaryRayMagnitude = sqrt((primaryRayDirection.x * primaryRayDirection.x)+(primaryRayDirection.y * primaryRayDirection.y)+(primaryRayDirection.z * primaryRayDirection.z));
		float3 primaryRayDirectionNormalised;
		primaryRayDirectionNormalised.x = primaryRayDirection.x / primaryRayMagnitude;
		primaryRayDirectionNormalised.y = primaryRayDirection.y / primaryRayMagnitude;
		primaryRayDirectionNormalised.z = primaryRayDirection.z / primaryRayMagnitude;

		float sphereLambda = INFINITY;
		float3 sphereTempColour;
		int closestSphereIndex;
		
		// Tests each sphere for intersection with the primary ray 
		for(int t = 0; t < numberOfSpheres; t++){
			float a_ = (primaryRayDirectionNormalised.x * primaryRayDirectionNormalised.x) + (primaryRayDirectionNormalised.y * primaryRayDirectionNormalised.y) + (primaryRayDirectionNormalised.z * primaryRayDirectionNormalised.z);
			float b_ = (2 * primaryRayDirectionNormalised.x * (cameraOrigin.x - spheres[t].s0)) + (2 * primaryRayDirectionNormalised.y * (cameraOrigin.y - spheres[t].s1)) + (2 * primaryRayDirectionNormalised.z * (cameraOrigin.z - spheres[t].s2)) ;
			float c_ = ((cameraOrigin.x - spheres[t].s0) * (cameraOrigin.x - spheres[t].s0)) + ((cameraOrigin.y - spheres[t].s1) * (cameraOrigin.y - spheres[t].s1)) + ((cameraOrigin.z - spheres[t].s2) * (cameraOrigin.z - spheres[t].s2)) - (spheres[t].s3*spheres[t].s3);
			float primarySphereIntersectDiscriminate_ = (b_  *  b_) - (4 * a_ * c_);
			float sphereLambda_current;
			if(primarySphereIntersectDiscriminate_ >= 0){
				if(primarySphereIntersectDiscriminate_ == 0){
					// Where one intersection exists, the quadratic is solved 
					sphereLambda_current = (-b_ + sqrt(primarySphereIntersectDiscriminate_))/(2*a_);
				}
				else{
					// Where two intersections exist, both solutions to the quadratic are found 
					// and the closest to the camera is chosen 
					float sphereLambda1 = (-b_ + sqrt(primarySphereIntersectDiscriminate_))/(2*a_);
					float sphereLambda2 = (-b_ - sqrt(primarySphereIntersectDiscriminate_))/(2*a_);
					if (sphereLambda2 > sphereLambda1){
						sphereLambda_current = sphereLambda1;
					}
					else{
						sphereLambda_current = sphereLambda2;
					}
				}
				if(sphereLambda_current < sphereLambda){
					// If the intersection point for the current sphere is closer to the camera than the 
					// intersection point for the previously tested sphere, its information is stored 
					sphereLambda = sphereLambda_current;
					sphereTempColour.x = spheres[t].s4;
					sphereTempColour.y = spheres[t].s5;
					sphereTempColour.z = spheres[t].s6;
					closestSphereIndex = t;
				}
			}
		}
		
		// Tests the primary ray for intersection with the plane. 
		float primaryPlaneIntersectDenominator = (primaryRayDirection.x * planeNormal.x) + (primaryRayDirection.y * planeNormal.y) + (primaryRayDirection.y * planeNormal.y);// D.N
		float planeLambda = INFINITY;
		if(primaryPlaneIntersectDenominator > 0){
			// If a valid intersection point exists, the equation is solved for lambda (the distance from the camera to the
			// primary ray / plane intersection point. 
			planeLambda = (((planePoint.x - cameraOrigin.x) * planeNormal.x )+((planePoint.y - cameraOrigin.y) * planeNormal.y ) + ((planePoint.z - cameraOrigin.z) * planeNormal.z ))/primaryPlaneIntersectDenominator;
		}
		
		if(planeLambda == INFINITY && sphereLambda == INFINITY){
			// If the primary ray has no valid intersections with either the plane or the sphere,
			// the current pixel colour is set to the background colour,
			// and the kernel instance for the current work item / id terminates here. 
			outputData[id].x = backgroundColour.x;
			outputData[id].y = backgroundColour.y;
			outputData[id].z = backgroundColour.z;
		}
		else{
			// If the primary ray intersects at least one of the scene objects, 
			// the closest intersection point to the camera is chosen, and the corresponding object colour is stored. 
			// N.B. The closest intersection point must be brought fractionaly closer to the camera to prevent self shadowing! 
			float3 tempColour;
			float tempLambda;
			int sphereFlag;
			if(planeLambda < sphereLambda){
				tempColour = planeColour;
				tempLambda = planeLambda-0.0001;
				sphereFlag = 0;
			}
			else{
				tempColour = sphereTempColour;
				tempLambda = sphereLambda-0.03;
				sphereFlag = 1;
			}
			// Calculated the coordinates of the closest point of intersection 
			float3 primaryIntersectionPosition;
			primaryIntersectionPosition.x = cameraOrigin.x + (tempLambda * primaryRayDirectionNormalised.x);
			primaryIntersectionPosition.y = cameraOrigin.y + (tempLambda * primaryRayDirectionNormalised.y);
			primaryIntersectionPosition.z = cameraOrigin.z + (tempLambda * primaryRayDirectionNormalised.z);
			// Creates a line from the closest primary intersection to the light source 
			// 
			float3 shadowRayDirection;
			shadowRayDirection.x = lightPosition.x - primaryIntersectionPosition.x;
			shadowRayDirection.y = lightPosition.y - primaryIntersectionPosition.y;
			shadowRayDirection.z = lightPosition.z - primaryIntersectionPosition.z;
			float shadowRayMagnitude = sqrt((shadowRayDirection.x*shadowRayDirection.x)+(shadowRayDirection.y*shadowRayDirection.y)+(shadowRayDirection.z*shadowRayDirection.z));
			float3 shadowRayDirectionNormalised;
			shadowRayDirectionNormalised.x = shadowRayDirection.x  / shadowRayMagnitude;
			shadowRayDirectionNormalised.y = shadowRayDirection.y  / shadowRayMagnitude;
			shadowRayDirectionNormalised.z = shadowRayDirection.z  / shadowRayMagnitude;
			
			
			// Tests the shadow ray for intersection with all the scene sphere
			// N.B. it is assumed that the plane will always be in the background and will never cast shadow! 
			// If the shadow ray intersects at least one of the spheres, the object to which the primary ray intersection point belongs is in shadow 
			int shadowFlag = 0;
			for(int t = 0; t < numberOfSpheres; t++){
				float a_1 = (shadowRayDirectionNormalised.x * shadowRayDirectionNormalised.x) + (shadowRayDirectionNormalised.y * shadowRayDirectionNormalised.y) + (shadowRayDirectionNormalised.z * shadowRayDirectionNormalised.z);
				float b_1 = (2 * shadowRayDirectionNormalised.x * (primaryIntersectionPosition.x - spheres[t].s0)) + (2 * shadowRayDirectionNormalised.y * (primaryIntersectionPosition.y - spheres[t].s1)) + (2 * shadowRayDirectionNormalised.z * (primaryIntersectionPosition.z - spheres[t].s2)) ;
				float c_1 = ((primaryIntersectionPosition.x - spheres[t].s0) * (primaryIntersectionPosition.x - spheres[t].s0)) + ((primaryIntersectionPosition.y - spheres[t].s1) * (primaryIntersectionPosition.y - spheres[t].s1)) + ((primaryIntersectionPosition.z - spheres[t].s2) * (primaryIntersectionPosition.z - spheres[t].s2)) - (spheres[t].s3*spheres[t].s3);
				float shadowRaySphereDiscriminate = (b_1 * b_1) - (4 * a_1 * c_1);
				if(shadowRaySphereDiscriminate >= 0){
					if(sphereFlag == 0){shadowFlag = 1;}
					else{
						if( t != closestSphereIndex){shadowFlag = 1;}
						else{
							// The current sphere is the primary ray intersection point sphere 
							// I.E. the shadow ray intersects the sphere twice,
							// If the shadow ray intersection point that is not the primary ray intersection point is closer to the camera than 
							// the primary ray intersection point, the nthe sphere is in shadow 
							if(shadowRaySphereDiscriminate > 0){
								// if SR magnitude > distance from srip_ to ls  -> shadow == true!!!
								float sr_lambda_1 = (-b_1 + sqrt(shadowRaySphereDiscriminate)) / (2 * a_1);
								float sr_lambda_2 = (-b_1 - sqrt(shadowRaySphereDiscriminate)) / (2 * a_1);
								if((sr_lambda_1 >= 0) || (sr_lambda_2 >= 0)){
									shadowFlag = 1;
								}
							}
						}
					}
				}
			}
			float ambient = 0.1;
			if(shadowFlag == 1){
				// If the object is in shadow, current pixel colour = object colour * ambient light 
				// and the kernel instance for the current work item / id terminates here. 
				outputData[id].x = tempColour.x * ambient;
				outputData[id].y = tempColour.y * ambient;
				outputData[id].z = tempColour.z * ambient;
			}
			else if (shadowFlag == 0){
				float cosTheta = 1;
				float3 prip_normalDirection;
				if(sphereFlag == 1){
					
					prip_normalDirection.x = primaryIntersectionPosition.x - spheres[closestSphereIndex].s0;
					prip_normalDirection.y = primaryIntersectionPosition.y - spheres[closestSphereIndex].s1;
					prip_normalDirection.z = primaryIntersectionPosition.z - spheres[closestSphereIndex].s2;
					float prip_normalMagnitude = sqrt((prip_normalDirection.x * prip_normalDirection.x)+(prip_normalDirection.y * prip_normalDirection.y)+(prip_normalDirection.z * prip_normalDirection.z));
					prip_normalDirection.x = prip_normalDirection.x / prip_normalMagnitude;
					prip_normalDirection.y = prip_normalDirection.y / prip_normalMagnitude;
					prip_normalDirection.z = prip_normalDirection.z / prip_normalMagnitude;
					cosTheta = (prip_normalDirection.x * shadowRayDirectionNormalised.x) + (prip_normalDirection.y * shadowRayDirectionNormalised.y) + (prip_normalDirection.z * shadowRayDirectionNormalised.z);
				}
				else{
					// Calculates the angle between the shadow ray and the plane normal 
					// Always returns erroneous values, so removed from all GA and LA kernels 
				//	cosTheta = ((planeNormal.x * shadowRayDirectionNormalised.x)+(planeNormal.y * shadowRayDirectionNormalised.y)+(planeNormal.z * shadowRayDirectionNormalised.z));
				//	cosTheta = cosTheta * 10000;
				//	cosTheta = 1;
				}
				if(cosTheta > 1){cosTheta = 1;}
				if(cosTheta < 0){cosTheta = 0;}
				
				outputData[id].x = cosTheta*(tempColour.x + lightColour.x)/2 + (ambient * tempColour.x);
				outputData[id].y = cosTheta*(tempColour.y + lightColour.y)/2 +(ambient * tempColour.y);
				outputData[id].z = cosTheta*(tempColour.z + lightColour.z)/2 +(ambient * tempColour.z);
			}
		}
		// Restricts the output RGB data between 0 and 1 
		if(outputData[id].x > 1){outputData[id].x = 1;}
		if(outputData[id].y > 1){outputData[id].y = 1;}
		if(outputData[id].z > 1){outputData[id].z = 1;}
		if(outputData[id].x < 0){outputData[id].x = 0;}
		if(outputData[id].y < 0){outputData[id].y = 0;}
		if(outputData[id].z < 0){outputData[id].z = 0;}
}