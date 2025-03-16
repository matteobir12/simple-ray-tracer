#version 450

#define SPHERE_COUNT 2

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba8, binding = 0) uniform image2D imgOutput;
layout(binding = 1) uniform samplerBuffer noiseTex;

// World Data
uniform int SphereCount;

//Camera Data
uniform int Width;
uniform int Height;

// Light Data
uniform vec3 lightDirection;
uniform vec3 lightColor;

//void main() {
//	vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
//	ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
//
//	value.x = float(texelCoord.x) / (gl_NumWorkGroups.x);
//	value.y = float(texelCoord.y) / (gl_NumWorkGroups.y);
//
//	imageStore(imgOutput, texelCoord, value);
//}

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
};

struct HitRecord {
	bool hit;
	vec3 p;
	vec3 normal;
	float t;
	bool frontFace;
};

vec3 SampleSquare(int rayInd) {
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	int index = (coord.y * Height) + coord.x;
	index = (index + rayInd) % (Width * Height);

	vec2 samp = texelFetch(noiseTex, index).xy;
	return vec3(samp.x - 0.5f, samp.y - 0.5f, 0.0);
}

vec3 RayAt(Ray r, float t) {
	return r.origin + r.direction * t;
}

void SetFaceNormal(Ray ray, vec3 outwardNormal, inout HitRecord rec) {
	rec.frontFace = dot(ray.direction, outwardNormal) < 0;
	rec.normal = rec.frontFace ? outwardNormal : -outwardNormal;
}

float randFloat(vec2 seed) {
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 randomUnitVec() {
	vec2 x = vec2(float(gl_GlobalInvocationID.x), float(gl_GlobalInvocationID.y));
	vec2 y = vec2(float(gl_GlobalInvocationID.x) * 0.25, float(gl_GlobalInvocationID.y) * 0.25);
	vec2 z = vec2(float(gl_GlobalInvocationID.x) * 0.5, float(gl_GlobalInvocationID.y) * 0.5);

	vec3 rand = vec3(randFloat(x), randFloat(y), randFloat(z));
	return normalize(rand);
}

vec3 randomOnHemisphere(vec3 normal, vec3 point) {
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	int index = (coord.y * Height) + coord.x;

	float rand = randFloat(point.xy) * Width * Height;
	int randInt = int(rand);
	index = (index + randInt) % (Width * Height);

	vec3 onSphere = texelFetch(noiseTex, index).xyz;
	if (dot(onSphere, normal) > 0.0)
		return onSphere;
	else
		return -onSphere;
}

Camera GetCamera(CameraSettings settings) {
	Camera camera;
	camera.width = settings.width;
	camera.height = int(settings.width / settings.aspect);
	camera.height = (camera.height < 1) ? 1 : camera.height;

	camera.pixelSamplesScale = 1.0 / settings.samplesPerPixel;

	camera.center = settings.origin;

	// Camera Values
	float theta = radians(settings.vFov);
	float h = tan(theta / 2.0);
	float viewHeight = 2.0 * h * settings.focusDist;
	float viewWidth = viewHeight * (float(settings.width) / camera.height);

	camera.w = normalize(settings.origin - settings.lookAt);
	camera.u = normalize(cross(settings.vUp, camera.w));
	camera.v = cross(camera.w, camera.u);

	vec3 viewU = viewWidth * camera.u;
	vec3 viewV = viewHeight * camera.v;

	camera.pixelDeltaU = viewU / float(settings.width);
	camera.pixelDeltaV = viewV / float(camera.height);

	vec3 viewLowerLeft = camera.center - (settings.focusDist * camera.w) - viewU / 2.0 - viewV / 2.0;
	camera.pixel00Loc = viewLowerLeft + 0.5 * (camera.pixelDeltaU + camera.pixelDeltaV);

	// Camera Defocus
	float defocusRadius = settings.focusDist = tan(radians(settings.defocusAngle / 2.0));
	camera.defocusDiskU = camera.u * defocusRadius;
	camera.defocusDiskV = camera.v * defocusRadius;

	return camera;
}

Ray GetRay(Camera cam, CameraSettings settings, int i, int j, int samp) {
	Ray r;
	vec3 offset = SampleSquare(samp);
	vec3 pixelSample = cam.pixel00Loc + ((i + offset.x) * cam.pixelDeltaU) + ((j + offset.y) * cam.pixelDeltaV);

	//vec3 rayOrigin = (settings.defocusAngle <= 0) ? cam.center : defocusDiskSample();
	vec3 rayOrigin = cam.center;
	vec3 rayDirection = pixelSample - rayOrigin;

	r.origin = rayOrigin;
	r.direction = rayDirection;
	return r;
}

bool SphereHit(Ray ray, Sphere s, float min, float max, inout HitRecord rec) {
	vec3 oc = s.pos - ray.origin;
	float a = pow(length(ray.direction), 2.0);
	float h = dot(ray.direction, oc);
	float c = pow(length(oc), 2.0) - (s.radius * s.radius);
	float discriminant = h * h - a * c;

	if (discriminant < 0.0) {
		return false;
	}

	float sqrtd = sqrt(discriminant);
	float root = (h - sqrtd) / a;
	if (!(min < root && root < max))
	{
		root = (h + sqrtd) / a;
		if (!(min < root && root < max))
			return false;
	}

	rec.t = root;
	rec.p = RayAt(ray, rec.t);
	vec3 outwardNormal = (rec.p - s.pos) / s.radius;
	SetFaceNormal(ray, outwardNormal, rec);

	return true;
}

HitRecord CheckHit(Ray ray, Sphere[SPHERE_COUNT] spheres, float min, float max) {
	HitRecord rec;
	rec.hit = true;
	rec.p = vec3(0.0);
	rec.normal = vec3(0.0);
	rec.t = 0;
	rec.frontFace = true;
	rec.hit = false;

	float closest = max;

	for (int i = 0; i < SPHERE_COUNT; i++)
	{
		if (SphereHit(ray, spheres[i], min, closest, rec))
		{
			rec.hit = true;
			closest = rec.t;
		}
	}

	return rec;
}

vec3 GetRayColor(Camera cam, Ray ray, Sphere[SPHERE_COUNT] spheres, int depth) {
	vec3 dir = normalize(ray.direction);
	float a = 0.5f * (dir.y + 1.0f);
	vec3 color = (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);

	float infinity = 1.0 / 0.0;
	float maxDepth = float(depth);
	while (true) {
		HitRecord rec = CheckHit(ray, spheres, 0.001, infinity);
		if (rec.hit)
		{
			if (depth <= 0)
				return vec3(0.0, 0.0, 0.0);

            vec3 lightDir = normalize(-lightDirection);
            float diff = max(dot(rec.normal, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            vec3 viewDir = normalize(-ray.direction);
            vec3 reflectDir = reflect(-lightDir, rec.normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
            vec3 specular = spec * lightColor;

            vec3 ambient = 0.1 * lightColor; 
            vec3 lighting = ambient + diffuse + specular;

            color = lighting * vec3(1.0, 1.0, 1.0); // assumption of white material temporarily

            vec3 direction = randomOnHemisphere(rec.normal, rec.p);
            ray.direction = direction;
            ray.origin = rec.p;
            depth--;
		}
		else {
			break;
		}
	}
	
	return color;
}

void main() {

	// TODO move all this data to uniform buffers
	Sphere world[SPHERE_COUNT];
	world[1].pos = vec3(0.0, -100.5, -1.0);
	world[1].radius = 100.0;
	world[0].pos = vec3(0.0, 0.0, -1.0);
	world[0].radius = 0.5;

	CameraSettings settings;
	settings.width = Width;
	settings.aspect = float(Width) / float(Height);
	settings.samplesPerPixel = 100;
	settings.maxDepth = 10;
	settings.vFov = 90.0;
	settings.origin = vec3(0.0, 0.0, 0.0);
	settings.lookAt = vec3(0.0, 0.0, -1.0);
	settings.vUp = vec3(0.0, 1.0, 0.0);
	settings.defocusAngle = 0.0;
	settings.focusDist = 1.0;

	Camera camera = GetCamera(settings);

	ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	vec3 value = vec3(0.0, 0.0, 0.0);
	value.x = float(texelCoord.x) / (gl_NumWorkGroups.x);
	value.y = float(texelCoord.y) / (gl_NumWorkGroups.y);
	vec3 pixelColor = vec3(0.0, 0.0, 0.0);

	for (int s = 0; s < settings.samplesPerPixel; s++)
	{
		Ray ray = GetRay(camera, settings, texelCoord.x, texelCoord.y, s);
		pixelColor += GetRayColor(camera, ray, world, settings.maxDepth);
	}

	pixelColor *= camera.pixelSamplesScale;
	vec4 outColor = vec4(pixelColor, 1.0);
	imageStore(imgOutput, texelCoord, outColor);
}
