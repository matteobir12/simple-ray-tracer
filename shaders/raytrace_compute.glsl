#version 450

#define SPHERE_COUNT 2
#define MAX_LIGHTS 10
#define M_PI 3.1415926535897

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba8, binding = 0) uniform image2D imgOutput;
layout(binding = 1) uniform samplerBuffer noiseTex;

// World Data
uniform int SphereCount;

//Camera Data
uniform int Width;
uniform int Height;

#include <raytrace_types.glsl>
#include <raytrace_utils.glsl>
#include <brdf.glsl>

// Light Data
uniform int lightCount;
layout(std430, binding = 2) buffer lightBuffer {
	Light lights[];
};


//---------------------------------RayTracing---------------------------------//

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
	rec.mat = s.mat;
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

bool CheckLightOccluded(vec3 pos, Light light, Sphere[SPHERE_COUNT] spheres) {
	vec3 dir = normalize(light.position - pos);
	float max = length(light.position - pos);

	Ray lightRay;
	lightRay.origin = pos;
	lightRay.direction = dir;
	HitRecord rec = CheckHit(lightRay, spheres, 0.001, max);
	return rec.hit;
}

vec3 GetRayColor(Camera cam, Ray ray, Sphere[SPHERE_COUNT] spheres, int depth) {
	vec3 dir = normalize(ray.direction);
	float a = 0.5f * (dir.y + 1.0f);
	vec3 color = vec3(0.0);

	float infinity = 1.0 / 0.0;
	float maxDepth = float(depth);
	Light light = lights[0];
	int lightIndex = 0;
	vec3 throughputColor = vec3(1.0);
	vec3 accumColor = vec3(0.0);
	while (true) {
		HitRecord rec = CheckHit(ray, spheres, 0.001, infinity);
		if (rec.hit)
		{
			Light light = lights[lightIndex];
			float shadowMult = CheckLightOccluded(rec.p, light, spheres) ? 0.0 : 1.0;
			color += throughputColor * SampleDirect(rec, -ray.direction, light, shadowMult);

			float pdf;
			bool isDiffuse;
			vec3 direction = SampleNextRay(rec, ray, pdf, isDiffuse);
			if (pdf == 0.0) {
				break;
			}

			throughputColor *= BRDF(rec, ray.direction, direction, isDiffuse) * abs(dot(direction, rec.normal)) / pdf;
			
			ray.direction = direction;
			ray.origin = rec.p;
			depth--;
			lightIndex = (lightIndex + 1) % lightCount;

			if (depth <= 0) {
				float survivalProb = clamp(luminance(throughputColor), 0.1, 1.0);
				if (randFloatSample(rec.p.xy) > survivalProb) break;
				throughputColor /= survivalProb;
			}
		}
		else {
			break;
		}
	}

	color += throughputColor * ((1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0));
	
	return color;
}

void main() {

	// TODO move all this data to uniform buffers
	Material material1;
	Material material2;
	material1.albedo = vec3(0.1, 0.2, 0.2);
	material1.specular = vec3(0.6, 0.8, 0.8);
	material1.roughness = 0.1;
	material2.albedo = vec3(0.8, 0.3, 0.3);
	material2.specular = vec3(0.5, 0.2, 0.4);
	material2.roughness = 0.05;

	Sphere world[SPHERE_COUNT];
	world[1].pos = vec3(0.0, -100.5, -1.0);
	world[1].radius = 100.0;
	world[1].mat = material1;
	world[0].pos = vec3(0.0, 0.0, -1.0);
	world[0].radius = 0.5;
	world[0].mat = material2;

	CameraSettings settings;
	settings.width = Width;
	settings.aspect = float(Width) / float(Height);
	settings.samplesPerPixel = 250;
	settings.maxDepth = 30;
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
