#version 330

out vec4 fragColor;
  
in vec4 texCoords;

uniform sampler2D textureToShow;
uniform float renderScale;
// pixel coordinates of lower left corner;
// can be negative;
uniform ivec2 offset_pixels;

void main()
{ 
    const vec4 backgroundColor = vec4(0.0, 0.0, 1.0, 1.0);


    vec2 texCoords_scaled = 
        (gl_FragCoord.xy - offset_pixels.xy ) 
        / vec2(textureSize(textureToShow,0).xy) 
        / renderScale;
    
    bvec2 greaterThan1 = greaterThan(texCoords_scaled, vec2(1.0, 1.0));
    bvec2 lessThan0    = lessThan   (texCoords_scaled, vec2(0.0, 0.0));
    
    if(any(greaterThan1) || any(lessThan0) ) {
        fragColor = backgroundColor;
    } else {
        fragColor = texture(textureToShow, texCoords_scaled.xy).xyzw;
    }    

}