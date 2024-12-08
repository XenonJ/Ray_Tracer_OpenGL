#version 330

in vec3 pixelColor;
in vec3 rayOrigin;
in vec3 rayDirection;

const float PI = 3.14159265359;
const float stepSize = 0.25;
vec3 worldPosition = rayOrigin + rayDirection * 10000;
// const float width
#define bottom -2
#define top 2
#define width 4

out vec4 outputColor;

vec4 renderCloud(vec3 cameraPosition, vec3 worldPosition) {
    vec3 viewDirection = normalize(worldPosition - cameraPosition);
    vec3 step = viewDirection * stepSize;
    vec4 colorSum = vec4(0);
    vec3 point = cameraPosition;

    // ray marching
    for(int i=0; i<100; i++) {
        point += step;
        if(bottom>point.y || point.y>top || -width>point.x || point.x>width || -width>point.z || point.z>width) {
            continue;
        }
        
        float density = 0.05;
        vec4 color = vec4(1.0, 1.0, 1.0, 1.0) * density;    
        colorSum = colorSum + color * float(1.0 - colorSum.a);

		if(colorSum.a > 0.95) {
			break;
		}
	
    }

    return colorSum;
}

void main()
{
	outputColor = mix(vec4(pixelColor, 1.0), renderCloud(rayOrigin,  rayOrigin + rayDirection * 1000), 0.5);
}