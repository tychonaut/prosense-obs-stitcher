#version 330
// **************************************
// FRUSTUM TO EQUIRECT
// **************************************
// Dome-mapping for rectangular 2D content
// (defined by an oriented frustum, 
// represented by a viewProjcetion matrix),
// equirectangularly projected onto a canvas.
// Doing this repeatedly stitches multiple
// images into a panorama.
//
// Markus Schlueter (GEOMAR)
// version 0.1.0
// date: 2021/02/01

// ============================================================================
// ============================================================================
//{ GLSL Fragment Shader data interface



//{
// a bug in ShaderEd demands sampler uniforms on top and outside of structs :C

uniform sampler2D sinkParams_in_backgroundTexture;
uniform sampler2D sourceParams_in_currentPlanarRendering;

uniform sampler2D sourceParams_in_warpLUT;
uniform sampler2D sourceParams_in_blendMask;

//}







struct FS_SinkParams_in {
    int index;
    //sampler2D backgroundTexture;
    //KISS for first iteration:
    ivec2 resolution; 
    
    // How to interpret this orientation:
    // The goal is to re-parametrize the canvas of 
    // a full-sphere projection, so that the sub-canvas
    // filled by e.g. a 21°-tilted hemispheric dome
    // fills the canvas in a way that has desireable 
    // properties.
    // To stick with the above example: If the full 
    // spherical equirectangular canvas would have
    // a (virtual) resolution of 8k*4k, and if we would
    // encode  the dome image naively, 
    // most of the bottom half of the image is black, 
    // and the seam has a wave form due to the tilt.
    // In order to use hardware encoding of Nvidia GPUs
    // without issues (not all GPUs support more than
    // 4k*4k resolution) and to save bandwidth,
    // the tilted dome could be encoded in the 
    // left 4kx4k half of the virtual 8kx4k canvas.
    // Therefore, the whole scene (i.e. each frustum)
    // needs to be rotated by 
    // roll=90.0, pitch=0.0 yaw=21.0,
    // in order to yield a left-only hemisphere
    // only negative x values).
    // 
    // IMPORTANT: The video *PLAYER* software needs
    // to be aware of this transformation and undo it
    //TODO setup in host data structures
    bool useSceneRotationMatrix;
    mat4 scene_RotationMatrix;
    
    // in case of fisheye projection (planned alternative to eqirect):
    // TODO setup in host data structures
    // hacky flip from equirect to fisheye;
    // dependent compilation would be better, but not for this prototype...
    bool useFishEye;
    float fishEyeFOV_angle;
    
    
    // rest of this struct is unused yet
    // resolution the whole 4pi steradian panorama image would have;
    ivec2 resolution_virtual; 
    ivec2 cropRectangle_min;
    ivec2 cropRectangle_max;
    //precalculated from cropRectangle
    ivec2 resolution_effective;
};

uniform FS_SinkParams_in sinkParams_in;


struct FS_SourceParams_in {
    int index;
    //sampler2D currentPlanarRendering;
    
    
    // just for debugging, prefer pre-accumulated versions
    // like frustum_viewProjectionMatrix 
    // or frustum_reorientedViewProjectionMatrix
    
    //  frustum_viewMatrix = transpose(frustum_rotationMatrix)
    mat4 frustum_viewMatrix;
    mat4 frustum_projectionMatrix;

    // accumulated VP matrix:
    // frustum_viewProjectionMatrix =  frustum_projectionMatrix * frustum_viewMatrix;
    mat4 frustum_viewProjectionMatrix;
    
    // accumulated VP matrix, but additionally applied rotation to whole scene
    // (e.g. to fit the hemisphere of a tilted dome into the left half of 
    // an equirect. projection)
    // frustum_reorientedRotationMatrix = scene_RotationMatrix * frustum_rotationMatrix
    // frustum_reorientedViewMatrix = transpose(frustum_reorientedRotationMatrix);
    // frustum_reorientedViewProjectionMatrix = frustum_projectionMatrix * frustum_reorientedViewMatrix
    mat4 frustum_reorientedViewProjectionMatrix;
    
    int decklinkWorkaround_verticalOffset_pixels;
    
    // rest of this struct is unused yet
    
    // currently unused; default false
    bool doImageWarp;
    // currently unused; default false
    bool do3DImageWarp;
    //n.b. warp inversion must be done on the host side
    // by inverting the LUT.
    // n.b. (inverse) mesh warping must be done
    // as a preprocess: render warp mesh and input image
    // to texture, then feed the resulting texture
    // to this shader.
    //sampler2D warpLUT; //2D or 3D
    
    bool doBlending;
    //sampler2D blendMask;
};

uniform FS_SourceParams_in sourceParams_in;



//} GLSL Fragment Shader data interface
// ----------------------------------------------------------------------------



// ============================================================================
// ============================================================================
// Macros and constants:

// Workaround that may help integrating this shader
// into the OBS Studio Effect-Framework.
#define DEVELOPMENT_CODE 1


 const float c_PI   =3.141592653589793238462643383279502884197169399375105820974944592308;
 const float c_PIH  =c_PI/2.0;
 const float c_2PI  =2.0*c_PI;
 
 // debug brackground color
 const vec4 c_BgColor= vec4( 0, 0, 1, 0);
 
 
 
// ============================================================================
// ============================================================================
// types:













// ============================================================================
// ============================================================================
// function forwards

//-------------------------------------
// main: 
void main();

//-------------------------------------
// little helpers:
float angleToRadians(float angle);
vec3  aziEleToCartesian3D(vec2 aziEle_rads);
vec2 fragCoordsToAziEle_Equirect(FS_SinkParams_in params, vec2 fragCoords);
vec2 fragCoordsToAziEle_FishEye (FS_SinkParams_in params, vec2 fragCoords);
// Transform direction vector, interpreted as position vector
// (this is valid, becaus all frusta are currently expected 
// to sit in the center), from world coords into image-space
// coords of the current frustum: 'shadow mapping-lookup-style':
// warning: can return values outside [0..1]^2, must be checked!
vec2 cartesianDirectionToSourceTexCoords(vec3 dir, mat4 viewProjectionMatrix);




#if DEVELOPMENT_CODE


in vec4 texcoord;

layout(location = 0) out vec4 out_color;


// ============================================================================
// ============================================================================
// function implementations:



// ----------------------------------------------------------------------------
// main function impl.
void main()
{
    // smaple background texture:
    vec2 texCoord_backGroundTexture = gl_FragCoord.xy / sinkParams_in.resolution.xy;  
    vec4 backGroundColor = 
        vec4(texture(sinkParams_in_backgroundTexture,
                     texCoord_backGroundTexture.xy).xyz, 1.0);
                     
    // DEBUG: some non-black debug background color to spot black bars:
    if(all(lessThan(backGroundColor.xyz, vec3(0.1))))
    {
        backGroundColor.xyz = vec3(0.25, 0.0, 0.0);
    }
    
    
    
    //{ fragment coords to azimuth and elevation, either equirect or fishEye
    vec2 aziEle;
    if(sinkParams_in.useFishEye)
    {
        aziEle = fragCoordsToAziEle_FishEye(sinkParams_in, gl_FragCoord.xy);
    }else
    {
        aziEle = fragCoordsToAziEle_Equirect(sinkParams_in, gl_FragCoord.xy);
    }
    //}
    
    // azimuth and elevation to cartesian (xyz) coords:
    vec3 dir_cartesian = aziEleToCartesian3D(aziEle);
    
    vec2 texCoords_0_1 = 
        cartesianDirectionToSourceTexCoords(
            dir_cartesian,  
            sourceParams_in.frustum_reorientedViewProjectionMatrix); 
            //sourceParams_in.frustum_viewProjectionMatrix);
    
    
    //discard fragment if corresponding direction is not covered by the current frustum
    // i.e. image coord are outside  [0..1]^2
    if( any( greaterThan( texCoords_0_1, vec2( 1.0, 1.0) ) )
        || 
        any( lessThan( texCoords_0_1, vec2( 0.0, 0.0) ) )
    )
    {
        discard;
    }

    // correct for the black bar issue in dacklink capture cards:
    // https://forum.blackmagicdesign.com/viewtopic.php?f=4&t=131164
    vec2 tc_offset = vec2(0.0, 
                          float(sourceParams_in.decklinkWorkaround_verticalOffset_pixels)
                          / textureSize(sourceParams_in_currentPlanarRendering, 0).y 
    );
    vec2 tc_corrected = texCoords_0_1 - tc_offset;
    tc_corrected = clamp(tc_corrected, vec2(0.0, 0.0), vec2(1.0, 1.0));
               
    // do the lookup in the source texture:
    vec4 screenPixelColor = texture(sourceParams_in_currentPlanarRendering,
                                    tc_corrected);
        
    out_color = screenPixelColor ; // + backGroundColor + debugColor[sourceParams_in.index] ;
    
    
    // TODO check why blending does not work
    // vec4 debugColor[5] ;
    // debugColor[0] = vec4(1.0, 0.0, 0.0, 0.0);
    // debugColor[1] = vec4(0.0, 1.0, 0.0, 0.0);
    // debugColor[2] = vec4(0.0, 0.0, 1.0, 0.0);
    // debugColor[3] = vec4(1.0, 1.0, 0.0, 0.0);
    // debugColor[4] = vec4(1.0, 0.0, 1.0, 0.0);
}
#endif //DEVELOPMENT_CODE
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Little helpers impl.

// Transform direction vector, interpreted as position vector
// (this is valid, becaus all frusta are currently expected 
// to sit in the center), from world coords into image-space
// coords of the current frustum: 'shadow mapping-lookup-style':
// warning: can return values outside [0..1]^2, must be checked!
vec2 cartesianDirectionToSourceTexCoords(vec3 dir, mat4 viewProjectionMatrix)
{
    vec4 texCoords_projected  = 
        viewProjectionMatrix
        * vec4(dir.xyz, 1.0);
        
        
    // filter geometry behind frustum:
    // the negative z component in 'frustum camera coords' is found in the w component.
    // If -z is negative, then z is positive in cam coords, positive z coords 
    // lie behind the cam. we have to filter them out
    if(texCoords_projected.w <= 0.0)
    {  
        discard;
    }
        
    // Clip coords to Normalized device coords (NCD): Div by homogeneous (W) coord
    vec3 texCoords_NDC = texCoords_projected.xyz / texCoords_projected.w;



    
    // NDC [-1 .. +1] --> image space[0..1]       
    vec2 texCoords_0_1 =  (texCoords_NDC.xy * 0.5) + vec2(0.5, 0.5);

    return texCoords_0_1;
}



// ----------------------------------------------------------------------------
float angleToRadians(float angle)
{
	return angle / 180.0f * c_PI ;
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//takes a pixel's azi/ele and calculates unit length 3D coordinate 
// Coordinate system is described in comments to texCoordsToAziEle()
vec3 aziEleToCartesian3D(vec2 aziEle_rads)
{
	
	// just for readability:
	// float r = s.domeRadius;
    float r = 1.0f;
	float azi = aziEle_rads.x;
	float ele = aziEle_rads.y;
	
	//ele+= angleToRadians(s.domeTilt);
	
	vec3 ret = vec3(
		        r * sin(ele) * cos(azi),
		        r * cos(ele),
		        r * sin(ele) * sin(azi)
	);
	
	//return ret;
	
	ret = normalize(ret);
	
	return ret;
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
vec2 fragCoordsToAziEle_Equirect(FS_SinkParams_in params, vec2 fragCoords)
{
    float azi_0_1 = fragCoords.x / params.resolution.x;  
    float ele_0_1 = fragCoords.y / params.resolution.y;
    
    float azi_0_2pi = azi_0_1 * c_2PI;
    
    // elevation in [-pi/2 .. +pi/2]
    // elevation == +pi/2 --> north or +y axis, respectively
    // elevation ==  0    --> equator
    // elevation == -pi/2 --> south or -y axis, respectively
    //float ele_pmPih = (ele_0_1 * c_PI) - c_PIH;
    //vec2 aziEle = vec2(azi_0_2pi, ele_pmPih);
    
    // elevation in [0 .. +pi]
    // elevation ==  0  --> north or +y axis, respectively
    // elevation ==  pi/2    --> equator
    // elevation == -pi --> south or -y axis, respectively    
    float ele_0_pi = (1.0 - ele_0_1) * c_PI 
        //+ c_PIH //DEBUG
        ;
    vec2 aziEle = vec2(azi_0_2pi, ele_0_pi);
    
    return aziEle;
}
// ----------------------------------------------------------------------------



    // old code; maybe useful for fisheye projection support later --------------------------



// texture coords in [0..1]^2 to azimuth and elevation radians,
// in local dome coordinates (i.e. no tilt))
// azi: west  (+x axis): 00   radians 
//      north (+z axis): pi/2 radians 
//      east  (-x axis): pi   radians 
//      south (-z axis): 2pi  radians 
// ele: top   (+y axis): 0 radians
//      horizon (xz-plane): pi/2 radians
vec2 fragCoordsToAziEle_FishEye (FS_SinkParams_in params, vec2 fragCoords)
//vec2 oldCode_FishEyeTexCoordsToAziEle_radians(OldCode_FS_input s, vec2 tc)
{
    //[img size]^2 --> [0..1]^2;
    vec2 texCoord_ViewPort = fragCoords.xy / params.resolution.xy;

	//[0..1]^2 --> [-1..-1]^2
	vec2 tc_centered_0_1 = (texCoord_ViewPort - vec2(0.5f,0.5f)) * 2.0f;

	float r = length(tc_centered_0_1);

	//http://paulbourke.net/dome/fisheye/
	float azi = 0.0f;
	if(r != 0.0f)
	{
		if( (tc_centered_0_1.x >= 0.0f))
		{
			azi = asin(tc_centered_0_1.y / r);
		}
		else
		{
			azi = c_PI - asin(tc_centered_0_1.y / r);
		}
	}
	
	float ele = r * angleToRadians(params.fishEyeFOV_angle)/2.0f;
	
	return vec2(azi,ele);
}




