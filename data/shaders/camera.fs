#version 330 core
out vec4 FragColor;

in vec4 colourAPos;

void main()
{
	FragColor = vec4(0.5 + colourAPos.x/2.5, 0.5 + colourAPos.y/2.5, 0.5 + colourAPos.z/5.0, 2.5);
}