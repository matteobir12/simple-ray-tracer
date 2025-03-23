
struct SupportedMaterial {
	vec3 albedo;
	vec3 specular;
	float roughness;
};

// TODO: texture support
// needs to match asset_utils/types.h Material
// TODO replace specular_ex with roughness on load where 
// float roughness = sqrt(2.0f / (specular_ex + 2.0f));
// roughness = clamp(roughness, 0.0f, 1.0f);
//
// For now temp sol: roughness ~ 1 / (specular_ex + eps)
struct MaterialFromOBJ {
  vec3 diffuse;
  vec3 specular;
  float specular_ex;
  uint use_texture; // bool
};

// describes the start and len of each BVH in the BVHNodeBuffer
// as well as the coordinate frame the bvh is in (world frame to model frame)
// I'm pretty sure this is also inverse traditional model matrix
struct BVH {
  uint first_index;
  uint count;
  uint _pad0;
  uint _pad1;
  mat4 frame;
};

// we're currently going with 2 children, so at first_child and first_child + 1
struct BVHNode {
  vec3 min_bounds;
  vec3 max_bounds;
  uint first_child_or_prim_index;
  uint prim_count;
};

// data for each triangle in buffer
// each member points to another buffer
struct Triangle {
  uint v0_idx;
  uint v1_idx;
  uint v2_idx;
  uint material_idx;
};

// data for each vertex in buffer
struct VertexData {
  vec3 vertex;
  vec2 texture;
};

struct CameraSettings {
	float		aspect;
	int			width;
	int			samplesPerPixel;
	int			maxDepth;
	float		vFov;
	vec3		origin;
	vec3		lookAt;
	vec3		vUp;
	float		defocusAngle;
	float		focusDist;
};

struct Camera {
	uint			width;
	uint			height;
	float			pixelSamplesScale;
	vec3			center;
	vec3			pixel00Loc;
	vec3			pixelDeltaU;
	vec3			pixelDeltaV;
	vec3			u, v, w; //Camera basis vectors
	vec3			defocusDiskU;
	vec3			defocusDiskV;
};

struct Ray {
  vec3 origin;
  float _pad0;
  vec3 direction;
  float intersection_distance;
};

struct Sphere {
	vec3 pos;
	float radius;
	SupportedMaterial mat;
};

struct Light {
	vec3 position;
	float pad0;
	vec3 intensity;
	float pad1;
};

struct HitRecord {
	bool hit;
	vec3 p;
	vec3 normal;
	float t;
	bool frontFace;
	SupportedMaterial mat;
};