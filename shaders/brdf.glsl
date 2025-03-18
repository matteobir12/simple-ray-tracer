
void getLightData(Light light, vec3 hitPos, out vec3 toLight) {
	vec3 lightDist = light.position - hitPos;
	toLight = normalize(lightDist);
}

float ggxNormalDistribution(float NdotH, float roughness) {
	float a2 = roughness * roughness;
	float d = ((NdotH * a2 - NdotH) * NdotH + 1);
	return a2 / max(0.001f, (d * d * M_PI));
}

// This from Schlick 1994, modified as per Karas in SIGGRAPH 2013 "Physically Based Shading" course
float ggxSchlickMaskingTerm(float NdotL, float NdotV, float roughness) {
	// Karis notes they use alpha / 2 (or roughness^2 / 2)
	float k = roughness * roughness / 2;

	// Compute G(v) and G(l).  These equations directly from Schlick 1994
	float g_v = NdotV / (NdotV * (1 - k) + k);
	float g_l = NdotL / (NdotL * (1 - k) + k);

	// Return G(v) * G(l)
	return g_v * g_l;
}

// Traditional Schlick approximation to the Fresnel term (also from Schlick 1994)
vec3 schlickFresnel(vec3 f0, float u) {
	return f0 + (vec3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - u, 5.0f);
}

float GetPdf() {
	return 1.0;
}

// Get a cosine-weighted random vector centered around a specified normal direction.
vec3 SampleDiffuse(vec3 point, vec3 hitNorm) {

	return randomOnHemisphere(hitNorm, point);
	// Get 2 random numbers to select our sample with
	//vec3 randVal = randomOnHemisphere(hitNorm, point);

	//// Cosine weighted hemisphere sample from RNG
	//vec3 bitangent = getPerpendicularVector(hitNorm);
	//vec3 tangent = cross(bitangent, hitNorm);
	//float r = sqrt(randVal.x);
	//float phi = 2.0f * M_PI * randVal.y;

	//// Get our cosine-weighted hemisphere lobe sample direction
	//return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

// GGX microfacet sample
vec3 SampleSpecular(vec3 point, float roughness, vec3 hitNorm) {
	// Get our uniform random numbers
	vec3 randVal = randomOnHemisphere(hitNorm, point);

	// Get an orthonormal basis from the normal
	vec3 B = getPerpendicularVector(hitNorm);
	vec3 T = cross(B, hitNorm);

	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cosThetaH = sqrt(max(0.0f, (1.0 - randVal.x) / ((a2 - 1.0) * randVal.x + 1)));
	float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
	float phiH = randVal.y * M_PI * 2.0f;

	// Get our GGX NDF sample (i.e., the half vector)
	return T * (sinThetaH * cos(phiH)) + B * (sinThetaH * sin(phiH)) + hitNorm * cosThetaH;
}

void GetAllBRDFValues(HitRecord hit, vec3 V, vec3 L, out float NdotL, out float NdotH, out float LdotH, out float NdotV, out float D, out float G, out vec3 F) {
	// Compute (N dot L)
	vec3 N = hit.normal;
	NdotL = saturate(dot(N, L));

	// Compute half vectors and additional dot products for GGX
	vec3 H = normalize(V + L);
	NdotH = saturate(dot(N, H));
	LdotH = saturate(dot(L, H));
	NdotV = saturate(dot(N, V));

	// Evaluate terms for our GGX BRDF model
	float rough = hit.mat.roughness;
	vec3 spec = hit.mat.specular;
	D = ggxNormalDistribution(NdotH, rough);
	G = ggxSchlickMaskingTerm(NdotL, NdotV, rough);
	F = schlickFresnel(spec, LdotH);
}

vec3 SampleDirect(HitRecord hit, vec3 V, Light light, float shadowMult) {
	// Query the scene to find info about the randomly selected light
	vec3 L;
	vec3 lightIntensity = light.intensity;
	getLightData(light, hit.p, L);

	float NdotL;
	float NdotH;
	float LdotH;
	float NdotV;
	float D;
	float G;
	vec3 F;
	GetAllBRDFValues(hit, V, L, NdotL, NdotH, LdotH, NdotV, D, G, F);

	// Evaluate the Cook-Torrance Microfacet BRDF model
	//     Cancel out NdotL here & the next eq. to avoid catastrophic numerical precision issues.
	vec3 ggxTerm = D * G * F / (4 * NdotV /* * NdotL */);

	// Compute our final color (combining diffuse lobe plus specular GGX lobe)
	return shadowMult * lightIntensity * ( /* NdotL * */ ggxTerm + NdotL * hit.mat.albedo / M_PI);
}

vec3 SampleNextRay(HitRecord rec, Ray inRay, inout float pdf) {
	float defuseProb = probabilityToSampleDiffuse(rec.mat.albedo, rec.mat.specular);
	float rand = randFloat(rec.p.xy);// randFloatSample(inRay.direction.xy);
	float NdotL;
	float NdotH;
	float LdotH;
	float NdotV;
	float D;
	float G;
	vec3 F;
	//vec3 newDir;

	vec3 L;
	
	//if (rand < defuseProb) {
		L = SampleDiffuse(rec.p, rec.normal);
		GetAllBRDFValues(rec, -inRay.direction, L, NdotL, NdotH, LdotH, NdotV, D, G, F);
		pdf = (NdotL / M_PI);
	//}
	//else {
	//	vec3 halfVec = SampleSpecular(rec.p, rec.mat.roughness, rec.normal);
	//	newDir = reflect(inRay.direction, halfVec);
	//	GetAllBRDFValues(rec, -newDir, L, NdotL, NdotH, LdotH, NdotV, D, G, F);
	//	pdf = D * NdotH / (4 * LdotH);
	//}
	
	return L;
}

vec3 BRDF(HitRecord hit, vec3 inDir, vec3 outDir) {
	float NdotL;
	float NdotH;
	float LdotH;
	float NdotV;
	float D;
	float G;
	vec3 F;
	GetAllBRDFValues(hit, inDir, outDir, NdotL, NdotH, LdotH, NdotV, D, G, F);
	
	vec3 ggxTerm = D * G * F / (4 * NdotV /* * NdotL */);
	vec3 diffuseTerm = NdotL * hit.mat.albedo / M_PI;
	return diffuseTerm;
	float diffuseProb = probabilityToSampleDiffuse(hit.mat.albedo, hit.mat.specular);

	return (1.0 - diffuseProb) * ggxTerm + diffuseProb * diffuseTerm;
}