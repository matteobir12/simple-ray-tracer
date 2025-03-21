
struct Material {
	vec3 albedo;
	vec3 specular;
	float roughness;
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
	vec3 intensity;
};

struct HitRecord {
	bool hit;
	vec3 p;
	vec3 normal;
	float t;
	bool frontFace;
	Material mat;
};