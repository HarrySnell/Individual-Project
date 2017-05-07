
///////////////////////////////////////////////////////////
// N.B. As this file is almost identical to the kernel for the linear algebra static geometry ray tracer, 
// this file has not been commented.  
///////////////////////////////////////////////////////////

__kernel void testKernel(
	__global float3* outputData,
	__global float3* primaryRays,
	__global float8* spheres,
	int numberOfSpheres,
	float3 planeNormal, 
	float3 planePoint, 
	float3 planeColour, 
	float3 lightPosition, 
	float3 lightColour, 
	float3 cameraOrigin, 
	float3 backgroundColour
){
	size_t id = (get_global_id(1) * get_global_size(0)) + get_global_id(0);
	float ambient = 0.1;
	
	float sphereLambda = INFINITY;
	float3 sphereTempColour;
	int closestSphereIndex;
	for(int t = 0; t < numberOfSpheres; t++){
		float a_ = (primaryRays[id].x * primaryRays[id].x) + (primaryRays[id].y * primaryRays[id].y) + (primaryRays[id].z * primaryRays[id].z);
		float b_ = (2 * primaryRays[id].x * (cameraOrigin.x - spheres[t].s0)) + (2 * primaryRays[id].y * (cameraOrigin.y - spheres[t].s1)) + (2 * primaryRays[id].z * (cameraOrigin.z - spheres[t].s2)) ;
		float c_ = ((cameraOrigin.x - spheres[t].s0) * (cameraOrigin.x - spheres[t].s0)) + ((cameraOrigin.y - spheres[t].s1) * (cameraOrigin.y - spheres[t].s1)) + ((cameraOrigin.z - spheres[t].s2) * (cameraOrigin.z - spheres[t].s2)) - (spheres[t].s3*spheres[t].s3);
		float primarySphereIntersectDiscriminate_ = (b_  *  b_) - (4 * a_ * c_);
		float sphereLambda_current;
		if(primarySphereIntersectDiscriminate_ >= 0){
			if(primarySphereIntersectDiscriminate_ == 0){
				sphereLambda_current = (-b_ + sqrt(primarySphereIntersectDiscriminate_))/(2*a_);
			}
			else{
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
				sphereLambda = sphereLambda_current;
				sphereTempColour.x = spheres[t].s4;
				sphereTempColour.y = spheres[t].s5;
				sphereTempColour.z = spheres[t].s6;
				closestSphereIndex = t;
			}
		}
	}

	float primaryPlaneIntersectDenominator = (primaryRays[id].x * planeNormal.x) + (primaryRays[id].y * planeNormal.y) + (primaryRays[id].y * planeNormal.y);// D.N
	float planeLambda = INFINITY;
	
	if(primaryPlaneIntersectDenominator > 0){
		planeLambda = (((planePoint.x - cameraOrigin.x) * planeNormal.x )+((planePoint.y - cameraOrigin.y) * planeNormal.y ) + ((planePoint.z - cameraOrigin.z) * planeNormal.z ))/primaryPlaneIntersectDenominator;
	}
	
	if(planeLambda == INFINITY && sphereLambda == INFINITY){
		outputData[id].x = backgroundColour.x;
		outputData[id].y = backgroundColour.y;
		outputData[id].z = backgroundColour.z;
	}
	else{
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
		float3 primaryIntersectionPosition;
		primaryIntersectionPosition.x = cameraOrigin.x + (tempLambda * primaryRays[id].x);
		primaryIntersectionPosition.y = cameraOrigin.y + (tempLambda * primaryRays[id].y);
		primaryIntersectionPosition.z = cameraOrigin.z + (tempLambda * primaryRays[id].z);
			
		float3 shadowRayDirection;
		shadowRayDirection.x = lightPosition.x - primaryIntersectionPosition.x;
		shadowRayDirection.y = lightPosition.y - primaryIntersectionPosition.y;
		shadowRayDirection.z = lightPosition.z - primaryIntersectionPosition.z;
		float shadowRayMagnitude = sqrt((shadowRayDirection.x*shadowRayDirection.x)+(shadowRayDirection.y*shadowRayDirection.y)+(shadowRayDirection.z*shadowRayDirection.z));
		float3 shadowRayDirectionNormalised;
		shadowRayDirectionNormalised.x = shadowRayDirection.x  / shadowRayMagnitude;
		shadowRayDirectionNormalised.y = shadowRayDirection.y  / shadowRayMagnitude;
		shadowRayDirectionNormalised.z = shadowRayDirection.z  / shadowRayMagnitude;
		
		int shadowFlag = 0;
		for(int t = 0; t < numberOfSpheres; t++){
			float a_1 = (shadowRayDirectionNormalised.x * shadowRayDirectionNormalised.x) + (shadowRayDirectionNormalised.y * shadowRayDirectionNormalised.y) + (shadowRayDirectionNormalised.z * shadowRayDirectionNormalised.z);
			float b_1 = (2 * shadowRayDirectionNormalised.x * (primaryIntersectionPosition.x - spheres[t].s0)) + (2 * shadowRayDirectionNormalised.y * (primaryIntersectionPosition.y - spheres[t].s1)) + (2 * shadowRayDirectionNormalised.z * (primaryIntersectionPosition.z - spheres[t].s2)) ;
			float c_1 = ((primaryIntersectionPosition.x - spheres[t].s0) * (primaryIntersectionPosition.x - spheres[t].s0)) + ((primaryIntersectionPosition.y - spheres[t].s1) * (primaryIntersectionPosition.y - spheres[t].s1)) + ((primaryIntersectionPosition.z - spheres[t].s2) * (primaryIntersectionPosition.z - spheres[t].s2)) - (spheres[t].s3*spheres[t].s3);
			float shadowRaySphereDiscriminate = (b_1 * b_1) - (4 * a_1 * c_1);
			if(shadowRaySphereDiscriminate >= 0){
				if(sphereFlag == 0){
					//shadowFlag = 1;
				}
				else{
					if( t != closestSphereIndex){shadowFlag = 1;}
					else{
						if(shadowRaySphereDiscriminate > 0){
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
			
		if(shadowFlag == 1){
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
			}
			cosTheta = cosTheta;
			if(cosTheta > 1){cosTheta = 1;}
			if(cosTheta < 0){cosTheta = 0;}
			//cosTheta=1;
			outputData[id].x = cosTheta*(tempColour.x + lightColour.x)/2 + (ambient * tempColour.x);
			outputData[id].y = cosTheta*(tempColour.y + lightColour.y)/2 +(ambient * tempColour.y);
			outputData[id].z = cosTheta*(tempColour.z + lightColour.z)/2 +(ambient * tempColour.z);
		}
	}
	if(outputData[id].x > 1){outputData[id].x = 1;}
	if(outputData[id].y > 1){outputData[id].y = 1;}
	if(outputData[id].z > 1){outputData[id].z = 1;}
	if(outputData[id].x < 0){outputData[id].x = 0;}
	if(outputData[id].y < 0){outputData[id].y = 0;}
	if(outputData[id].z < 0){outputData[id].z = 0;}
}