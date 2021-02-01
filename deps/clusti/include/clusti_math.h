#ifndef CLUSTI_MATH_H
#define CLUSTI_MATH_H


#include <stdbool.h> // bool

//-----------------------------------------------------------------------------
// Macros

#define CLUSTI_MATH_PI (3.14159265358979323846)

#define CLUSTI_MATH_EPSILON (0.0001f)
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Type forwards

struct Clusti_Params_Projection;
typedef struct Clusti_Params_Projection Clusti_Params_Projection;
struct Clusti_Params_FrustumFOV;
typedef struct Clusti_Params_FrustumFOV Clusti_Params_FrustumFOV;

// needed for conversion functions
struct _graphene_matrix_t;
typedef struct _graphene_matrix_t graphene_matrix_t;

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Math function forwards

float clusti_math_DegreesToRadians(float angle_degrees);

// Calculate and assign Clusti_Params_Projection::planar_viewProjectionMatrix
// from Clusti_Params_Projection::orientation and
//      Clusti_Params_Projection::planar_FrustumFOV.
// @return true on sucess, false otherwise (e.g. if the projection is
//      not planar and th frustum is not symmetric
//      (support for this is future work))
bool clusti_math_ViewProjectionMatrixFromFrustumAndOrientation(
	Clusti_Params_Projection *planarProjection_inOut);

bool clusti_math_FOVAnglesAreSymmetric(Clusti_Params_FrustumFOV const *fovs);

// out must be at least 16*fizeof(float)
float *clusti_math_grapheneMatrixToColumnMajorFloatArray(graphene_matrix_t const* in,
	float* out);

//-----------------------------------------------------------------------------


#endif // CLUSTI_MATH_H
