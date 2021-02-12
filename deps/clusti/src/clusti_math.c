
//-----------------------------------------------------------------------------
// includes

#include "clusti_math.h" // clusti_math_FOVAnglesAreSymmetric

#include "clusti_types_priv.h"
#include "clusti_status_priv.h" // clusti_status_declareError

#include <math.h> // sin, cos, tan, atan ...
#include <stdio.h>  // printf

#include <assert.h> // assert
#include <stdbool.h> // bool







//-----------------------------------------------------------------------------
// Math function impls.


//float *
//clusti_math_grapheneMatrixToColumnMajorFloatArray(graphene_matrix_t const *in,
//						  float *out)
//{
//	assert(out);
//
//	graphene_matrix_t mat_transposed;
//	// graphene's row major to column major representation:
//	graphene_matrix_transpose(in, &mat_transposed);
//	graphene_matrix_to_float(&mat_transposed, out);
//
//	return out;
//}




float clusti_math_DegreesToRadians(float angle_degrees)
{
	return angle_degrees * (float) (CLUSTI_MATH_PI / 180.0);
}

bool clusti_math_FOVAnglesAreSymmetric(Clusti_Params_FrustumFOV const *fovs)
{
	return (fabsf(fovs->left_degrees - fovs->right_degrees) <
		CLUSTI_MATH_EPSILON) &&
	       (fabsf(fovs->up_degrees - fovs->down_degrees) <
		CLUSTI_MATH_EPSILON);
}

// Create ViewProjectionMatrix from planar projection's
// frustum and orientation params:
// Calculate and assign Clusti_Params_Projection::planar_viewProjectionMatrix
// From Clusti_Params_Projection::orientation and
//      Clusti_Params_Projection::planar_FrustumFOV.
bool clusti_math_ViewProjectionMatrixFromFrustumAndOrientation(
	Clusti_Params_Projection *planarProjection_inOut)
{
	// conistency checks
	assert(planarProjection_inOut);
	assert(planarProjection_inOut->type ==
	       CLUSTI_ENUM_PROJECTION_TYPE_PLANAR);

	// create view matrix from euler angles
	graphene_euler_t *eulerAnglesPtr =
		&(planarProjection_inOut->orientation);

	graphene_matrix_t rotationMatrix;
	graphene_euler_to_matrix(eulerAnglesPtr, &rotationMatrix);
	/*
	TEST scene rotation:
	Put the rendered pixels of the hemisphere of our -21° tilted  dome
	into the left half of the 2:1 rectangular canvas
	(so 8*4k can be redcuced to 4k*4k):
	tilt by +21°, resulting in the upper half of the canvas to be filled
	by the dome's hemisphere),then tilt another +90° (=+111° in total),
	putting all canvas contents into the left half.
	For angle conventions, see functions fragCoordsToAziEle_Equirect()
	and fragCoordsToAziEle_FishEye() in fragment shader
	(frustumToEquirect.frag)
	(works):
	//graphene_matrix_t sceneRotMat;
	//graphene_matrix_init_rotate(&sceneRotMat, 111.0f, graphene_vec3_x_axis());
	//graphene_matrix_t tmpMat;
	//graphene_matrix_multiply(&rotationMatrix, &sceneRotMat, & tmpMat);
	//rotationMatrix = tmpMat;
	*/
	graphene_matrix_t viewMatrix;
	graphene_matrix_transpose(&rotationMatrix, &viewMatrix);
	planarProjection_inOut->planar_viewMatrix = viewMatrix;


	// create projection matrix from frustum opening angles
	// (currently only symmetric frusta, asymmetric frusta
	// need mor trig code that is not needed for a first prototype)
	Clusti_Params_FrustumFOV *fovs =
		&(planarProjection_inOut->planar_FrustumFOV);

	graphene_matrix_t projMatrix;
	// init not nececcary, but good style
	graphene_matrix_init_identity(&projMatrix);

	if (!clusti_math_FOVAnglesAreSymmetric(fovs)) {
		clusti_status_declareError("Frustum is not symmetric."
			"Only symmetric frusta are supported yet, sorry!");
		return false;
	} else {
		float hFOV_rad_symm =
			clusti_math_DegreesToRadians(fovs->left_degrees);
		float vFOV_rad_symm =
			clusti_math_DegreesToRadians(fovs->up_degrees);
		float aspectRatio_symm =
			tanf(hFOV_rad_symm) / tanf(vFOV_rad_symm);

		graphene_matrix_init_perspective(&projMatrix,
						 // double the half-angle FOV
						 2.0f * fovs->up_degrees,
						 aspectRatio_symm,
						 0.1f, 1000.0f);

		planarProjection_inOut->planar_projectionMatrix = projMatrix;
	}

	
	// create viewProjection matrix:
	// in OpenGL   convention: p_proj   =  P_gl * V_gl * p;
	// in Graphene convention: p_proj^T = (P_gl * V_gl * p)^T;
	//				    = p^T * V_gl^T * P_gl^T
	// V_gl^T corresponds to V_gr
	// (they are transposes of each other)
	// -->                     p_proj^T = p^T * V_gr * P_gr
	// --> viewProjectionMatrix_graphene =   V_gr * P_gr
	// --> left to right multiply in graphene!
	graphene_matrix_multiply(
		// view left, proj right
		&viewMatrix,  &projMatrix,
		// store it directlyy into the target struct
		&(planarProjection_inOut->planar_viewProjectionMatrix));


	return true;
}



// The "frustum rotation matrix" is multiplied by the sink's
// "scene rotation matrix", the result is transposed to a view matrix
//  and then mult. with the frustum's projection matrix:
// This can be useful to render tilted hemispherical content
// in a way to only fill the left half of the full-spherical
// canvas, allowing the right haft to be cropped
// without loss of information.
//
// Construct on the fly, to decouple sink from source data
// (if we need more sinks later);
//
// In graphene notation
// (multiply row vectors by matrices to the right):
// point'^T = point^T * VP
// Wanted: VP
// VP = V*P
// V = (R)^T
// R = R_Frustum * R_Scene
// --> VP = (R_Frustum * R_Scene)^T * P
graphene_matrix_t clusti_math_reorientedViewProjectionMatrix(
	graphene_euler_t const *sinkSceneOrientation,
	Clusti_Params_Projection const *sourcePlanarProjection)
{
	// Sink's Euler angles to rotation matrix
	graphene_matrix_t sceneRotMat;
	graphene_euler_to_matrix(sinkSceneOrientation,
				 &sceneRotMat);

	// Source's Euler angles to rotation matrix:
	graphene_matrix_t frustumRotMat;
	graphene_euler_to_matrix(&sourcePlanarProjection->orientation,
				 &frustumRotMat);

	graphene_matrix_t finalRotMat;
	graphene_matrix_multiply(&frustumRotMat, &sceneRotMat, &finalRotMat);

	graphene_matrix_t finalViewMat;
	graphene_matrix_transpose(&finalRotMat, &finalViewMat);

	graphene_matrix_t sceneRotatedViewProjectionMatrix;
	graphene_matrix_multiply(
		&finalViewMat,
		&sourcePlanarProjection->planar_projectionMatrix,
		&sceneRotatedViewProjectionMatrix);

	return sceneRotatedViewProjectionMatrix;
}


//-----------------------------------------------------------------------------
