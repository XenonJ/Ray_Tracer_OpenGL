#version 330

in vec3 pixelColor;

out vec4 outputColor;

void main()
{
	outputColor = vec4(pixelColor, 1.0);
}