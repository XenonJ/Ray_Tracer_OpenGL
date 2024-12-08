#version 330

in vec3 myNormal;
in vec3 myPosition;

void main()
{
	vec3 tmpPos = myPosition * 2.1;

	gl_Position = vec4(tmpPos, 1.0);
}

