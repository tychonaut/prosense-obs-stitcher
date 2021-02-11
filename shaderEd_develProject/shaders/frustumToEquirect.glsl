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
//{ Types and uniforms:


//{
// a bug in ShaderEd demands sampler uniforms on top and outside of structs :C

uniform sampler2D sinkParams_in_backgroundTexture;
uniform sampler2D sourceParams_in_currentPlanarRendering;


// currently unused
uniform sampler2D sourceParams_in_warpLUT;
// currently unused
uniform sampler2D sourceParams_in_blendMask;

//}







struct FS_SinkParams_in {
    int index;
    
    //sampler2D backgroundTexture;
    
    // resolution the whole 4pi steradian panorama image would have;
    vec2 resolution_virtual; 
    vec2 cropRectangle_lowerLeft;
    // *actual* render target resolution!
    // vec2 cropRectangle_extents; // unused    
    
    // in case of fisheye projection (alternative to eqirect):
    // hacky flip from equirect to fisheye;
    // dependent compilation would be better, but not for this prototype...
    bool useFishEye;
    float fishEyeFOV_angle;
};

uniform FS_SinkParams_in sinkParams_in;


struct FS_SourceParams_in {
   
     int index;
   
     //sampler2D currentPlanarRendering;
     
	//  Decklink 'black bar'-bug workaround:
	//  https://forum.blackmagicdesign.com/viewtopic.php?f=4&t=131164&p=711173&hilit=black+bar+quad#p711173
	//  ('VideoSource.decklinkWorkaround_verticalOffset_pixels' in config file)
    int decklinkWorkaround_verticalOffset_pixels;
    
    // accumulated VP matrix, but additionally applied rotation to whole scene
    // (e.g. to fit the hemisphere of a tilted dome into the left half of 
    // an equirect. projection)
    // frustum_reorientedRotationMatrix = sink_rotationMatrix * frustum_rotationMatrix
    // frustum_reorientedViewMatrix = transpose(frustum_reorientedRotationMatrix);
    // frustum_reorientedViewProjectionMatrix = frustum_projectionMatrix * frustum_reorientedViewMatrix
    //
    // How to interpret sink_rotationMatrix
    // (calculated from VideoSink.Projection.Orientation in config file):
    // The goal is to re-parametrize the canvas of 
    // a full-sphere projection, so that the sub-canvas
    // filled by e.g. a 21°-tilted hemispherical dome
    // fills the canvas in a way that has desireable 
    // properties for encoding and streaming.
    // E.g. if the full 
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
    // To achieve this, first undo the -21°tilt:
    // tilt by +21°, resulting in only the upper half 
    // of the canvas to be filled by the dome's hemisphere,
    // then tilt another +90° (=+111° in total), mapping all
    // pixels to the left half.
    //
    // IMPORTANT: The video *PLAYER* software needs
    // to be aware of the sink_rotationMatrix transformation and undo it!
    mat4 frustum_reorientedViewProjectionMatrix;

    
    // ---------------------------------
    
    //{ just for debugging, prefer pre-accumulated versions
    //  like frustum_viewProjectionMatrix 
    //  or frustum_reorientedViewProjectionMatrix:
    //  frustum_viewMatrix = transpose(frustum_rotationMatrix)
    mat4 frustum_viewMatrix;
    mat4 frustum_projectionMatrix;
    // accumulated VP matrix: in OpenGL notation (right-to-left-multiply):
    // frustum_viewProjectionMatrix =  frustum_projectionMatrix * frustum_viewMatrix;
    mat4 frustum_viewProjectionMatrix;
    //}
    
    
    //{ rest of this struct is unused yet
    
    // currently unused; default false
    bool doImageWarp;
    // currently unused; default false
    bool do3DImageWarp;
    // n.b. warp inversion must be done on the host side
    // by inverting the LUT.
    // n.b. (inverse) mesh warping must be done
    // as a preprocess: render warp mesh and input image
    // to texture, then feed the resulting texture
    // to this shader.
    //sampler2D warpLUT; //2D or 3D
    
    bool doBlending;
    //sampler2D blendMask;
    
    //}
};

uniform FS_SourceParams_in sourceParams_in;

//} Types and uniforms
// ============================================================================



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
    vec2 fragCoords_cropCorrected = gl_FragCoord.xy 
                                   + sinkParams_in.cropRectangle_lowerLeft.xy;
                                   
    // sample background texture:
    vec2 texCoords_backGroundTexture = 
        fragCoords_cropCorrected
        /
        sinkParams_in.resolution_virtual;
    //    gl_FragCoord.xy 
    //    / sinkParams_in.resolution.xy;
        
    vec4 backGroundColor = 
        vec4(texture(sinkParams_in_backgroundTexture,
                     texCoords_backGroundTexture.xy).xyz, 1.0);
                     
    // DEBUG: some non-black debug background color to spot black bars:
    if(all(lessThan(backGroundColor.xyz, vec3(0.1))))
    {
        backGroundColor.xyz = vec3(0.25, 0.0, 0.0);
    }
    
    
    
    //{ fragment coords to azimuth and elevation, either equirect or fishEye
    vec2 aziEle;
    if(sinkParams_in.useFishEye)
    {
        aziEle = fragCoordsToAziEle_FishEye(sinkParams_in,
                                            fragCoords_cropCorrected.xy);
    }else
    {
        aziEle = fragCoordsToAziEle_Equirect(sinkParams_in,
                                             fragCoords_cropCorrected.xy);
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
    vec4 debugColor[5] ;
    debugColor[0] = vec4(1.0, 0.0, 0.0, 0.0);
    debugColor[1] = vec4(0.0, 1.0, 0.0, 0.0);
    debugColor[2] = vec4(0.0, 0.0, 1.0, 0.0);
    debugColor[3] = vec4(1.0, 1.0, 0.0, 0.0);
    debugColor[4] = vec4(0.0, 1.0, 1.0, 0.0);
    
     out_color = screenPixelColor  
                 // + backGroundColor 
                 + 0.25 * debugColor[sourceParams_in.index] ;
}
#endif //DEVELOPMENT_CODE
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Little helpers impl.


// ----------------------------------------------------------------------------
float angleToRadians(float angle)
{
	return angle / 180.0f * c_PI ;
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
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
	
	vec3 ret = vec3(
		        r * sin(ele) * cos(azi),
		        r * cos(ele),
		        r * sin(ele) * sin(azi)
	);
	
    //obsolete and/or unneccessary, but to be sure:	
	ret = normalize(ret);
	return ret;
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
vec2 fragCoordsToAziEle_Equirect(FS_SinkParams_in params, vec2 fragCoords)
{
    float azi_0_1 = fragCoords.x / params.resolution_virtual.x;  
    float ele_0_1 = fragCoords.y / params.resolution_virtual.y;
    
    float azi_0_2pi = azi_0_1 * c_2PI;
    
    // elevation in [0 .. +pi]
    // elevation ==  0  --> north or +y axis, respectively
    // elevation ==  pi/2    --> equator
    // elevation == -pi --> south or -y axis, respectively    
    float ele_0_pi = (1.0 - ele_0_1) * c_PI;
    
    vec2 aziEle = vec2(azi_0_2pi, ele_0_pi);
    
    return aziEle;
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// texture coords in [0..1]^2 to azimuth and elevation radians,
// in local dome coordinates (i.e. no tilt))
// azi: west  (+x axis): 00   radians 
//      north (+z axis): pi/2 radians 
//      east  (-x axis): pi   radians 
//      south (-z axis): 2pi  radians 
// ele: top   (+y axis): 0 radians
//      horizon (xz-plane): pi/2 radians
vec2 fragCoordsToAziEle_FishEye (FS_SinkParams_in params, vec2 fragCoords)
{
    //[img size]^2 --> [0..1]^2;
    vec2 texCoord_ViewPort = fragCoords.xy / params.resolution_virtual.xy;

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
// ----------------------------------------------------------------------------



