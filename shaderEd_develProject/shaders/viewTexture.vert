#version 330

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_texCoords;

out vec4 texCoords;

void main()
{
    gl_Position =  in_position;
    texCoords = in_texCoords;
}
