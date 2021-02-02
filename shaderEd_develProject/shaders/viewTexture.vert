#version 330

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_texcoords;

out vec4 texcoords;

void main()
{
    gl_Position =  in_position;
    texcoords = in_texcoords;
}
