#version 410

uniform float vertexesColorX;
uniform float vertexesColorY;
uniform float vertexesColorZ;

out vec4 fragmentColor;

void main()
{
    fragmentColor =  vec4(vertexesColorX,vertexesColorY, vertexesColorZ, 1.0); 
}
