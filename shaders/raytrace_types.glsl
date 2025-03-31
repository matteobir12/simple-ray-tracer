
struct Material {
	vec3 albedo;
	vec3 specular;
	float roughness;
	float metalness;
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
	vec3 direction;
};

struct Sphere {
	vec3 pos;
	float radius;
	Material mat;
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
	Material mat;
};

struct BrdfData {
	// Material properties
	vec3 specularF0;
	vec3 diffuseReflectance;

	// Roughnesses
	float roughness;    //< perceptively linear roughness (artist's input)
	float alpha;        //< linear roughness - often 'alpha' in specular BRDF equations
	float alphaSquared; //< alpha squared - pre-calculated value commonly used in BRDF equations

	// Commonly used terms for BRDF evaluation
	vec3 F; //< Fresnel term

	// Vectors
	vec3 V; //< Direction to viewer (or opposite direction of incident ray)
	vec3 N; //< Shading normal
	vec3 H; //< Half vector (microfacet normal)
	vec3 L; //< Direction to light (or direction of reflecting ray)

	float NdotL;
	float NdotV;

	float LdotH;
	float NdotH;
	float VdotH;
};