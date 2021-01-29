

#include "clusti_math.h"

#include "clusti_types_priv.h"


#include <math.h> // sin, cos, tan, atan ...
#include <stdio.h>  // printf

#include <assert.h> // assert
#include <stdbool.h> // bool


//-----------------------------------------------------------------------------
// Macros

#define CLUSTI_MATH_PI (3.14159265358979323846)

#define CLUSTI_MATH_EPSILON (0.0001f)


//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Math function impls.

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
	graphene_matrix_t viewMatrix;
	graphene_euler_to_matrix(eulerAnglesPtr, &viewMatrix);

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
		float hFOV_rad_symm = clusti_math_DegreesToRadians(fovs->left_degrees);
		float vFOV_rad_symm =
			clusti_math_DegreesToRadians(fovs->up_degrees);
		float aspectRatio_symm = tanf(hFOV_rad_symm) / tanf(vFOV_rad_symm);

		graphene_matrix_init_perspective(&projMatrix, vFOV_rad_symm,
						 aspectRatio_symm, 0.01f, 100.0f);
	}

	
	// create viewProjection matrix: vpm = p * v;
	// store it directlyy into the target struct
	graphene_matrix_multiply(
		&projMatrix, &viewMatrix,
		&(planarProjection_inOut->planar_viewProjectionMatrix));


	graphene_matrix_print(&viewMatrix);
	graphene_matrix_print(&projMatrix);
	graphene_matrix_print(
		&(planarProjection_inOut->planar_viewProjectionMatrix));


	return true;

	// TODO test 
}
//-----------------------------------------------------------------------------
