#version 330

//uniform mat4 modelMat;
//uniform mat4 vp;

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_texcoords;

out vec4 texcoord;

void main()
{
    gl_Position =  in_position;
    texcoord = in_texcoords;
}
