#version 330 core
out vec4 FragColor;

in vec4 colourAPos;

void main()
{
	FragColor = vec4(0.3, colourAPos.y/2.0, colourAPos.z/2.0, 1.0);
}