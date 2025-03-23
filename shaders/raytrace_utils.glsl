
float saturate(float val) {
	return clamp(val, 0.0, 1.0);
}

vec3 saturate(vec3 val) {
	return clamp(val, 0.0, 1.0);
}

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

float randFloatSample(vec2 seed) {
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	int index = (coord.y * Height) + coord.x;

	float rand = randFloat(seed.xy) * Width * Height;
	int randInt = int(rand);
	index = (index + randInt) % (Width * Height);

	float val = texelFetch(noiseTex, index).x;
	return val;// (val + 1.0) / 2.0;
}

float randFloatSampleUniform(vec2 seed) {
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	int index = (coord.y * Height) + coord.x;

	float rand = randFloat(seed.xy) * Width * Height;
	int randInt = int(rand);
	index = (index + randInt) % (Width * Height);

	float val = texelFetch(noiseUniformTex, index).x;
	return val;// (val + 1.0) / 2.0;
}

float randFloatSample(vec2 seed, int index) {
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);

	float rand = randFloat(seed.xy) * Width * Height;
	int randInt = int(rand);
	index = (index + randInt) % (Width * Height);

	float val = texelFetch(noiseTex, index).x;
	return val;
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

	float rand = randFloatSampleUniform(point.xy) * Width * Height;
	int randInt = int(rand);
	index = (index + randInt) % (Width * Height);

	vec3 onSphere = texelFetch(noiseTex, index).xyz;
	if (dot(onSphere, normal) > 0.0)
		return onSphere;
	else
		return -onSphere;
}

vec3 GetCosHemisphereSample(vec3 hitNorm, vec3 point) {
	// Get 2 random numbers to select our sample with
	vec3 rand = randomOnHemisphere(hitNorm, point);

	// Cosine weighted hemisphere
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 bitangent = cross(hitNorm, up);
	vec3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(rand.x);
	float phi = 2.0f * 3.14159265f * rand.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - rand.x);
}

/** Returns a relative luminance of an input linear RGB color in the ITU-R BT.709 color space */
float luminance(vec3 rgb) {
	return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
}

vec3 specularF0(vec3 baseColor, float metalness) {
	return mix(vec3(0.04, 0.04, 0.04), baseColor, metalness);
}

float probabilityToSampleDiffuse(vec3 diffBRDF, vec3 specBRDF) {
	float lumDiffuse = max(0.01f, luminance(diffBRDF));
	float lumSpecular = max(0.01f, luminance(specBRDF));
	return lumDiffuse / (lumDiffuse + lumSpecular);
}

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
vec3 getPerpendicularVector(vec3 u) {
	vec3 a = abs(u);
	uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
	uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, vec3(xm, ym, zm));
}

float shadowedF90(vec3 F0) {
	// This scaler value is somewhat arbitrary, Schuler used 60 in his article. In here, we derive it from MIN_DIELECTRICS_F0 so
	// that it takes effect for any reflectance lower than least reflective dielectrics
	//const float t = 60.0f;
	const float t = (1.0f / 0.04);
	return min(1.0f, t * luminance(F0));
}

SupportedMaterial OBJMatToSupported(MaterialFromOBJ in_mat) {
  SupportedMaterial out_mat;
  out_mat.albedo = in_mat.diffuse;
  out_mat.specular = in_mat.specular;
  // TEMP
  out_mat.roughness = 1 / (in_mat.specular_ex + 0.0000001 /* eps */);
  return out_mat;
}