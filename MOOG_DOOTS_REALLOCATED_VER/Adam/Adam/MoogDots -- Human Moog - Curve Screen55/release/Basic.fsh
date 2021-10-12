#version 410

uniform sampler2D sTexture;
uniform float vertexesColorX;
uniform float vertexesColorY;
uniform float vertexesColorZ;

layout(location = 3) in vec4 vertexColor;
layout(location = 4) in vec2 TexCoord;

out vec4 fragmentColor;

void main()
{
    fragmentColor =  vec4(vertexesColorX , vertexesColorY , vertexesColorZ , 1.0); 
}
