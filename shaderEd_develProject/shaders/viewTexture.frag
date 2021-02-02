#version 330

out vec4 fragColor;
  
in vec2 texcoords;

uniform sampler2D screenTexture;

void main()
{ 
    fragColor = texture(screenTexture, texcoords);
}