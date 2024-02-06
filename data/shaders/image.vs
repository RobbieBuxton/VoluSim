#version 330 core
layout(location=0) in vec2 vVertex;
layout(location=1) in vec2 aTexCoord;

smooth out vec2 TexCoord;

void main()
{
  gl_Position = vec4(vVertex*2.0-1,0,1);
  TexCoord = aTexCoord;
}