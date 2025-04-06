#version 450
#extension GL_ARB_bindless_texture : require

#define SPHERE_COUNT 5
#define MAX_LIGHTS 10
#define M_PI 3.1415926535897
#define DIFFUSE_BRDF 1
#define SPECULAR_BRDF 2
#define SHOW_SPHERES true
#define SHOW_MODELS false

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba8, binding = 0) writeonly uniform image2D imgOutput;
layout(binding = 1) uniform samplerBuffer noiseTex;
layout(binding = 2) uniform samplerBuffer noiseUniformTex;
layout(rgba32f, binding = 3) uniform image2D accumBuffer;

//Camera Data
uniform int Width;
uniform int Height;

// Accumulation Data
uniform int accumFrames;
uniform bool resetAccumBuffer;

uniform vec3 cameraOrigin;
uniform vec3 cameraDirection;
uniform vec3 cameraUp;
uniform vec3 cameraRight;

#include <raytrace_types.glsl>
#include <ray_intersects.glsl>
#include <raytrace_utils.glsl>
#include <brdf.glsl>

// Light Data
uniform int lightCount;
layout(std430, binding = 4) buffer lightBuffer {
	Light lights[];
};


//---------------------------------RayTracing---------------------------------//

Camera GetCamera(CameraSettings settings) {
    Camera camera;
    camera.width = settings.width;
    camera.height = int(settings.width / settings.aspect);
    camera.height = (camera.height < 1) ? 1 : camera.height;

    camera.pixelSamplesScale = 1.0 / settings.samplesPerPixel;

    camera.center = cameraOrigin;

    // Use the passed camera vectors
    camera.w = -cameraDirection;
    camera.u = cameraRight;
    camera.v = cameraUp;

    vec3 viewU = camera.u * settings.focusDist;
    vec3 viewV = camera.v * settings.focusDist;

    camera.pixelDeltaU = viewU / float(settings.width);
    camera.pixelDeltaV = viewV / float(camera.height);

    vec3 viewLowerLeft = camera.center - (settings.focusDist * camera.w) - viewU / 2.0 - viewV / 2.0;
    camera.pixel00Loc = viewLowerLeft + 0.5 * (camera.pixelDeltaU + camera.pixelDeltaV);

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

  ray.intersection_distance = max;
  if (SHOW_SPHERES) {
	  for (int i = 0; i < SPHERE_COUNT; i++)
	  {
		  if (SphereHit(ray, spheres[i], min, ray.intersection_distance, rec))
		  {
			  rec.hit = true;
			  ray.intersection_distance = rec.t;
		  }
	  }
  }

  if (SHOW_MODELS) {
    for (uint i = 0; i < bvh_count; i++) {
      // transform ray into model's space
      vec4 trans_origin = bvhs[i].frame * vec4(ray.origin, 1.);
      vec4 trans_direction = bvhs[i].frame * vec4(ray.direction, 0.);
      // ray.intersection_distance is inout here, think this means it will be updated as expected
      vec3 tri_norm;
      uint hit = Intersects(bvhs[i].first_index, trans_origin.xyz, trans_direction.xyz, ray.intersection_distance, tri_norm);

      if (hit != uint(-1)) {
        rec.hit = true;
        // I think?
        rec.p = (ray.intersection_distance * ray.direction) + ray.origin;
        vec3 model_p = (ray.intersection_distance * trans_direction.xyz) + trans_origin.xyz;
        rec.normal = tri_norm;
        rec.t = ray.intersection_distance;
        TriangleToSupportedMat(triangles[hit], model_p, rec.mat);
      }
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

bool SampleLights(HitRecord hit, out float sampleWeight, out Light selectedLight) {
	float totalWeights = 0.0;
	float samplePdf = 0.0;
	Light sLight;
	bool selected = false;

	for (int i = 0; i < lightCount; i++) {
		int randLightIndex = int(round(randFloatSampleUniform(hit.p.xy) * lightCount));
		float lightWeight = float(lightCount);
		Light light = lights[randLightIndex];
		float falloff = GetLightFalloff(hit, light);
		float intensity = light.intensity * falloff;

		float lightPdf = luminance(vec3(intensity));
		float lightRISWeight = lightPdf * lightWeight;
		
		totalWeights += lightRISWeight;
		float rand = randFloatSampleUniform(vec2(hit.p.y + i, hit.p.z + i));
		if (rand < (lightRISWeight / totalWeights)) {
			sLight = light;
			samplePdf = lightPdf;
			selected = true;
		}
	}
	sampleWeight = (totalWeights / float(lightCount)) / max(0.001, samplePdf);
	selectedLight = sLight;
	return selected;
}

vec3 GetRayColor(Camera cam, Ray ray, Sphere[SPHERE_COUNT] spheres, int maxDepth) {
	vec3 dir = normalize(ray.direction);
	float a = 0.5f * (dir.y + 1.0f);

	float infinity = 1.0 / 0.0;
	Light light = lights[0];
	int lightIndex = 0;
	int randIndex = 0;
	int depth = maxDepth;
	
	// Use this for dark grey
	vec3 skyColor = vec3(0.05);
	//vec3 skyColor = ((1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0));
	vec3 throughputColor = vec3(1.0, 1.0, 1.0);
	vec3 color = vec3(0.0, 0.0, 0.0);

	while (true) {
		HitRecord rec = CheckHit(ray, spheres, 0.001, infinity);
		if (rec.hit)
		{
			float lightSampleWeight;
			Light light;
			bool sampledLight = SampleLights(rec, lightSampleWeight, light);
			vec3 V = -ray.direction;

			if (sampledLight) {
				//Get Direct color
				float shadowMult = CheckLightOccluded(rec.p, light, spheres) ? 0.0 : 1.0;
				vec3 L = getLightData(light, rec.p);

				if (rec.mat.useSpec) {
					color += throughputColor * SampleDirect(rec, -ray.direction, light, shadowMult) * lightSampleWeight;
				}
				else {
					float falloff = GetLightFalloff(rec, light);
					vec3 lightIntensity = light.color * falloff * light.intensity * lightSampleWeight;
					color += throughputColor * SampleDirectNew(rec, -ray.direction, L) * shadowMult * lightIntensity;
				}
			}

			int brdfType;
			if (rec.mat.metalness == 1.0 && rec.mat.roughness == 0.0) {
				brdfType = SPECULAR_BRDF;
			}
			else {
				float brdfProb = GetBrdfProbability(rec.mat, V, rec.normal);
				float rand = randFloatSampleUniform(vec2(rec.p.x + depth, rec.p.y + depth));

				if (rand < brdfProb) {
					brdfType = SPECULAR_BRDF;
					throughputColor /= brdfProb;
				}
				else {
					brdfType = DIFFUSE_BRDF;
					throughputColor /= (1.0 - brdfProb);
				}
			}

			if (depth <= 0) {
				float survivalProb = clamp(luminance(throughputColor), 0.1, 1.0);
				if (randFloatSampleUniform(vec2(rec.p.x + randIndex, rec.p.y + randIndex)) > survivalProb) break;
				throughputColor /= survivalProb;
				randIndex++;
			}
			else {
				depth--;
			}

			vec3 direction;
			vec3 brdfWeight;
			if (!SampleIndirectNew(rec, rec.normal, V, rec.mat, brdfType, direction, brdfWeight)) {
				break;
			}

			throughputColor *= brdfWeight;
			
			ray.direction = direction;
			ray.origin = rec.p;
		}
		else {
			break;
		}
	}
	
	color += throughputColor * skyColor;
	return color;
}

void main() {

	// TODO move all this data to uniform buffers
	Material material1;
	Material material2;
	Material material3;
	Material material4;
	Material material5;

	// Ground Material
	material1.albedo = vec3(0.2, 0.8, 0.8);
	material1.specular = vec3(0.2, 0.4, 0.4);
	material1.roughness = 0.01;
	material1.metalness = 0.99;
	material1.useSpec = false;

	// Red Material
	material2.albedo = vec3(0.8, 0.3, 0.3);
	material2.specular = vec3(0.9, 0.7, 0.7);
	material2.roughness = 0.1;
	material2.metalness = 0.5;
	material2.useSpec = true;

	// Green Material
	material3.albedo = vec3(0.2, 0.9, 0.3);
	material3.specular = vec3(0.2, 0.9, 0.9);
	material3.roughness = 0.3;
	material3.metalness = 0.95;
	material3.useSpec = true;

	// Blue Material
	material4.albedo = vec3(0.2, 0.4, 1.0);
	material4.specular = vec3(0.8, 0.8, 0.9);
	material4.roughness = 0.01;
	material4.metalness = 0.9;
	material4.useSpec = false;

	// Yellow Material
	material5.albedo = vec3(0.9, 0.8, 0.1);
	material5.specular = vec3(0.3, 0.3, 0.1);
	material5.roughness = 0.7;
	material5.metalness = 0.3;
	material5.useSpec = false;

	Sphere world[SPHERE_COUNT];
	// Ground
	world[1].pos = vec3(0.0, -100.5, -1.0);
	world[1].radius = 100.0;
	world[1].mat = material1;

	// Blue Sphere
	world[0].pos = vec3(1.8, 0.0, -2.0);
	world[0].radius = 0.5;
	world[0].mat = material4;

	// Green Sphere
	world[2].pos = vec3(0.55, 0.0, -2.0);
	world[2].radius = 0.5;
	world[2].mat = material3;

	// Red Sphere
	world[3].pos = vec3(-0.55, 0.0, -2.0);
	world[3].radius = 0.5;
	world[3].mat = material2;

	// Yellow Sphere
	world[4].pos = vec3(-1.8, 0.0, -2.0);
	world[4].radius = 0.5;
	world[4].mat = material5;

	CameraSettings settings;
	settings.width = Width;
	settings.aspect = float(Width) / float(Height);
	settings.samplesPerPixel = 1; // DON'T USE THIS!!
	settings.maxDepth = 5;
	settings.vFov = 90.0;
	if (!SHOW_MODELS) {
		settings.samplesPerPixel = 100;
		settings.origin = vec3(0.0, 0.0, 0.0);
		settings.lookAt = vec3(0.0, 0.0, -1.0);
	}
	else {
		settings.samplesPerPixel = 5;
		settings.origin = vec3(0.0, 20.0, 20.0);
		settings.lookAt = vec3(0.0, 1.0, -1.0);
	}
	settings.vUp = vec3(0.0, 1.0, 0.0);
	settings.defocusAngle = 0.0;
	settings.focusDist = 1.0;

	Camera camera = GetCamera(settings);

	ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);

	if (resetAccumBuffer) {
		imageStore(accumBuffer, texelCoord, vec4(0.0, 0.0, 0.0, 1.0));
		return;
	}

	vec3 value = vec3(0.0, 0.0, 0.0);
	value.x = float(texelCoord.x) / (gl_NumWorkGroups.x);
	value.y = float(texelCoord.y) / (gl_NumWorkGroups.y);
	vec3 pixelColor = vec3(0.0, 0.0, 0.0);

	int samp = accumFrames % (Width * Height);
	Ray ray = GetRay(camera, settings, texelCoord.x, texelCoord.y, samp);
	pixelColor += GetRayColor(camera, ray, world, settings.maxDepth);

	vec3 prevColor = imageLoad(accumBuffer, texelCoord).xyz;
	vec3 accumColor = prevColor + pixelColor;
	imageStore(accumBuffer, texelCoord, vec4(accumColor, 1.0));

	if (isnan(pixelColor.x) || isnan(pixelColor.y) || isnan(pixelColor.z)) {
		pixelColor = vec3(0.0, 1.0, 0.0);
	}

	vec4 outColor = vec4(linearToSrgb(accumColor / accumFrames)/*pixelColor*/, 1.0);
	imageStore(imgOutput, texelCoord, outColor);
}
