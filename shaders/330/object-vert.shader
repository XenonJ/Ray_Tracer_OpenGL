#version 330

in vec3 myNormal;
in vec3 myPosition;

void main()
{
	gl_Position = vec4(myPosition, 1.0);
}

