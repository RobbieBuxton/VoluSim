#version 330 core
layout (location=0) out vec4 vFragColor;

smooth in vec2 TexCoord;
uniform sampler2D textureMap;
void main()
{
  vFragColor = texture(textureMap, TexCoord);
}