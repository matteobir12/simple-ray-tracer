
vec3 getLightData(Light light, vec3 hitPos) {
	vec3 lightDist = light.position - hitPos;
	return length(lightDist) > 0 ? normalize(lightDist) : lightDist;
}

// Mostly from "A Gentle Introduction To DirectX Raytracing" by Chris Wyman
float ggxNormalDistribution(float NdotH, float roughness) {
	float a2 = roughness * roughness;
	float d = ((NdotH * a2 - NdotH) * NdotH + 1);
	return a2 / max(0.001f, (d * d * M_PI));
}

// Inspired by the Smith G1 term in "Crash Course in BRDF Implementation" by Jakub Boksansky
float ggxNormalDistributionNew(float NdotH, float alphaSquared) {
	float b = ((alphaSquared - 1.0f) * NdotH * NdotH + 1.0f);
	return alphaSquared / max(0.001, (M_PI * b * b));
}

// This from Schlick 1994, modified as per Karas in SIGGRAPH 2013 "Physically Based Shading" course
float ggxSchlickMaskingTerm(float NdotL, float NdotV, float roughness) {
	// Karis notes they use alpha / 2 (or roughness^2 / 2)
	float k = roughness * roughness / 2;

	// Compute G(v) and G(l).  These equations directly from Schlick 1994
	float g_v = NdotV / max(0.001, (NdotV * (1 - k) + k));
	float g_l = NdotL / max(0.001, (NdotL * (1 - k) + k));

	// Return G(v) * G(l)
	return abs(g_v * g_l);
}

// Traditional Schlick approximation to the Fresnel term (also from Schlick 1994)
vec3 schlickFresnel(vec3 f0, float u) {
	return f0 + (vec3(1.0f, 1.0f, 1.0f) - f0) * pow(max(0.001, 1.0f - u), 5.0f);
}

// Inspired by the "Crash Course in BRDF Implementation" by Jakub Boksansky
vec3 FresnelSchlickNew(vec3 f0, float f90, float NdotS) {
	return f0 + (f90 - f0) * pow(1.0f - NdotS, 5.0f);
}

// Inspired by the "Crash Course in BRDF Implementation" by Jakub Boksansky
float SmithGAlpha(float alpha, float NdotS) {
	return NdotS / (max(0.0001, alpha) * sqrt(1.0 - min(0.99999, NdotS * NdotS)));
}

float SmithGLambdaGGX(float a) {
	return (-1.0 + sqrt(1.0 + (1.0 / max(0.001, a * a)))) * 0.5;
}

// Inspired by the "Crash Course in BRDF Implementation" by Jakub Boksansky
float Smith_G2_Height_Correlated(float alpha, float NdotL, float NdotV) {
	float aL = SmithGAlpha(alpha, NdotL);
	float aV = SmithGAlpha(alpha, NdotV);
	return 1.0f / (1.0f + SmithGLambdaGGX(aL) + SmithGLambdaGGX(aV));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
vec3 SampleDiffuse(vec3 point, vec3 hitNorm) {

	// Get 2 random numbers to select our sample with
	float r1 = randFloatSampleUniform(point.xy);
	float r2 = randFloatSampleUniform(point.yz);

	// Cosine weighted hemisphere sample from RNG
	vec3 bitangent = getPerpendicularVector(hitNorm);
	vec3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(abs(r1));
	float phi = 2.0f * M_PI * r2;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(abs(1 - r1));
}

float SpecularSampleWeight(float alpha, float alphaSquared, float NdotS, float NdotSSquared) {
	return 2.0f / (sqrt(((alphaSquared * (1.0f - NdotSSquared)) + NdotSSquared) / NdotSSquared) + 1.0f);
}

// GGX microfacet sample
vec3 SampleSpecularHalfVec(vec3 point, float roughness, vec3 hitNorm) {
	// Get our uniform random numbers
	float rand1 = randFloatSampleUniform(vec2(point.x, point.y));
	float rand2 = randFloatSampleUniform(vec2(point.y, point.z));
	vec2 randVal = vec2(rand1, rand2); // randomOnHemisphere(hitNorm, point);

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

// Inspired by the "Crash Course in BRDF Implementation" by Jakub Boksansky
vec3 SampleSpecularMicrofacet(HitRecord hit, vec3 V, float alpha, float alphaSquared, vec3 specularF0, out vec3 weight) {
	
	// Sample a microfacet normal (H)
	vec3 H;
	if (alpha == 0.0f) {
		// Fast path for zero roughness (perfect reflection)
		vec3 Ltemp = reflect(-V, hit.normal);
		H = normalize(-V + Ltemp);
	}
	else {
		// For non-zero roughness
		H = SampleSpecularHalfVec(hit.p, hit.mat.roughness, hit.normal);
	}

	// Reflect view direction to obtain light vector
	vec3 L = reflect(-V, H);

	// Clamp dot products here to small value to prevent numerical instability. Assume that rays incident from below the hemisphere have been filtered
	vec3 N = hit.normal;
	float HdotL = max(0.00001, min(1.0f, dot(H, L)));
	float NdotL = max(0.00001, min(1.0f, dot(N, L)));
	float NdotV = max(0.00001, min(1.0f, dot(N, V)));
	float NdotH = max(0.00001, min(1.0f, dot(N, H)));
	vec3 F = FresnelSchlickNew(specularF0, shadowedF90(specularF0), HdotL);// schlickFresnel(hit.mat.specular, HdotL);

	// Calculate weight of the sample specific for selected sampling method 
	// (this is microfacet BRDF divided by PDF of sampling method - notice how most terms cancel out)
	weight = F * SpecularSampleWeight(alpha, alphaSquared, NdotL, NdotL * NdotL);

	return L;
}

vec3 EvalDiffuse(BrdfData data) {
	float oneOverPi = 1.0 / M_PI;
	return data.diffuseReflectance * (oneOverPi * data.NdotL);
}

vec3 EvalSpecular(BrdfData data) {
	float D = ggxNormalDistributionNew(max(0.00001f, data.alphaSquared), data.NdotH);
	float G = Smith_G2_Height_Correlated(data.alpha, data.NdotL, data.NdotV);// ggxSchlickMaskingTerm(data.NdotL, data.NdotV, data.roughness);
	float denom = 4.0 * max(data.NdotL, 0.001) * max(data.NdotV, 0.001);

	return ((data.F * G * D) / max(denom, 0.001)) * data.NdotL;
}

float GetLightFalloff(HitRecord hit, Light light) {
	vec3 dist = light.position - hit.p;
	float distSquared = dot(dist, dist);
	float falloff = 1 / ((0.01 * 0.01) + distSquared); // The 0.01 is to avoid infs when the light source is close to the shading point
	return falloff;
}

void GetAllBRDFValues(HitRecord hit, vec3 V, vec3 L, vec3 H, out float NdotL, out float NdotH, out float LdotH, out float NdotV, out float D, out float G, out vec3 F) {
	// Compute (N dot L)
	vec3 N = hit.normal;
	NdotL = saturate(dot(N, L));

	// Compute half vectors and additional dot products for GGX
	//vec3 H = normalize(V + L);
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

BrdfData GetAllBRDFValues(vec3 N, vec3 L, vec3 V, HitRecord hit) {
	BrdfData data;
	
	data.V = V;
	data.N = N;
	data.H = normalize(L + V);
	data.L = L;

	// Find vectors
	data.NdotL = saturate(dot(N, L));
	data.NdotV = saturate(dot(N, V));
	data.LdotH = saturate(dot(L, data.H));
	data.NdotH = saturate(dot(N, data.H));
	data.VdotH = saturate(dot(V, data.H));

	// Find material properties
	data.specularF0 = specularF0(hit.mat.albedo, hit.mat.metalness);
	data.diffuseReflectance = hit.mat.albedo * (1.0f - hit.mat.metalness);
	data.roughness = hit.mat.roughness;
	data.alpha = hit.mat.roughness * hit.mat.roughness;
	data.alphaSquared = data.alpha * data.alpha;

	data.F = FresnelSchlickNew(data.specularF0, shadowedF90(data.specularF0), data.LdotH);

	return data;
}

vec3 SampleDirect(HitRecord hit, vec3 V, Light light, float shadowMult) {
	// Query the scene to find info about the randomly selected light
	vec3 lightColor = light.color;
	vec3 L = getLightData(light, hit.p);

	float NdotL;
	float NdotH;
	float LdotH;
	float NdotV;
	float D;
	float G;
	vec3 F;
	vec3 H = length(V + L) > 0.0 ? normalize(V + L) : V + L;
	GetAllBRDFValues(hit, V, L, H, NdotL, NdotH, LdotH, NdotV, D, G, F);
	float falloff = GetLightFalloff(hit, light);
	float lightIntensity = light.intensity * falloff;

	// Evaluate the Cook-Torrance Microfacet BRDF model
	// Cancel out NdotL here & the next eq. to avoid catastrophic numerical precision issues.
	vec3 ggxTerm = D * G * F / (4 * max(0.001, NdotV) /* * NdotL */);

	// Compute our final color (combining diffuse lobe plus specular GGX lobe)
	vec3 lightTerm = shadowMult * light.color * lightIntensity;
	return lightTerm * ( /* NdotL * */ ggxTerm + NdotL * hit.mat.albedo / M_PI);
}

vec3 SampleDirectNew(HitRecord hit, vec3 V, vec3 L) {
	vec3 specular;
	vec3 diffuse;
	BrdfData data;

	data = GetAllBRDFValues(hit.normal, L, V, hit);
	specular = EvalSpecular(data);
	diffuse = EvalDiffuse(data);

	// Compute our final color (combining diffuse lobe plus specular GGX lobe)
	return (vec3(1.0f, 1.0f, 1.0f) - data.F) * diffuse + specular;
}

bool SampleIndirectNew(HitRecord hit, vec3 normal, vec3 V, Material material, int brdfType, out vec3 direction, out vec3 sampleWeight) {

	// Incident ray is somehow below the surface
	if (dot(normal, V) <= 0.0f) {
		return false;
	}
	
	vec3 newRayDir = vec3(0.0);
	
	if (brdfType == DIFFUSE_BRDF) {
		newRayDir = SampleDiffuse(hit.p, normal);
		BrdfData data = GetAllBRDFValues(normal, newRayDir, V, hit);
	
		sampleWeight = data.diffuseReflectance;
	
		// Sample a half-vector of specular BRDF.
		vec3 H = SampleSpecularHalfVec(hit.p, material.roughness, normal);
	
		// Clamp HdotL to small value to prevent numerical instability. Assume that rays incident from below the hemisphere have been filtered
		float VdotH = max(0.00001, min(1.0, dot(V, H)));
		sampleWeight *= (vec3(1.0f, 1.0f, 1.0f) - FresnelSchlickNew(data.specularF0, shadowedF90(data.specularF0), VdotH));
	}
	else {
		BrdfData data = GetAllBRDFValues(normal, vec3(0.0f, 0.0f, 1.0f) /* unused L vector */, V, hit);
		newRayDir = SampleSpecularMicrofacet(hit, V, data.alpha, data.alphaSquared, data.specularF0, sampleWeight);
	}

	
	if (luminance(sampleWeight) == 0.0) {
		return false;
	}
	
	direction = normalize(newRayDir);
	if (dot(normal, direction) <= 0.0f) {
		return false;
	}
	
	return true;
}

float GetBrdfProbability(Material material, vec3 V, vec3 normal) {
	float specF0 = luminance(specularF0(material.albedo, material.metalness));
	float diffuseReflectance = luminance((material.albedo * (1.0 - material.metalness)));
	float Fresnel = saturate(luminance(FresnelSchlickNew(vec3(specF0), shadowedF90(vec3(specF0)), max(0.0, dot(V, normal)))));

	float specular = Fresnel;
	float diffuse = diffuseReflectance * (1.0 - Fresnel);
	float p = (specular / max(0.0001, (specular + diffuse)));
	return clamp(p, 0.1f, 0.9f);
}

vec3 SampleNextRay(HitRecord rec, Ray inRay, inout float pdf, out bool isDiffuse) {
	float diffuseProb = probabilityToSampleDiffuse(rec.mat.albedo, rec.mat.specular);
	float rand = randFloatSampleUniform(rec.p.xy);
	float NdotL;
	float NdotH;
	float LdotH;
	float NdotV;
	float D;
	float G;
	vec3 F;
	vec3 L;
	//vec3 newDir;
	
	isDiffuse = rand < diffuseProb;
	if (isDiffuse) {
		L = SampleDiffuse(rec.p, rec.normal);
		vec3 halfVec = normalize(inRay.direction + L);
		GetAllBRDFValues(rec, inRay.direction, L, halfVec, NdotL, NdotH, LdotH, NdotV, D, G, F);
		pdf = (NdotL / M_PI);
	}
	else {
		vec3 halfVec = SampleSpecularHalfVec(rec.p, rec.mat.roughness, rec.normal);
		L = reflect(inRay.direction, halfVec);
		GetAllBRDFValues(rec, -inRay.direction, L, halfVec, NdotL, NdotH, LdotH, NdotV, D, G, F);
		pdf = D * NdotH / (4 * LdotH);
	}
	
	return L;
}

float DiffusePDF(HitRecord hit, vec3 L) {
	return max(dot(hit.normal, L), 0.0) / M_PI;
}

float SpecularPDF(HitRecord hit, vec3 V, vec3 L) {
	float rough = hit.mat.roughness;
	vec3 H = SampleSpecularHalfVec(hit.p, hit.mat.roughness, hit.normal);// normalize(V + L);
	vec3 N = hit.normal;

	float LdotH = saturate(dot(L, H));
	float NdotH = saturate(dot(N, H));
	float D = ggxNormalDistribution(NdotH, rough);

	return D * NdotH / (4 * LdotH);
}

vec3 DiffuseBRDF(HitRecord hit) {
	return hit.mat.albedo / M_PI;
}

vec3 SpecularBRDF(HitRecord hit, vec3 V, vec3 L) {
	vec3 H = /*normalize(V + L);*/ SampleSpecularHalfVec(hit.p, hit.mat.roughness, hit.normal);
	vec3 N = hit.normal;

	// Compute additional dot products for GGX
	float NdotL = saturate(dot(N, L));
	float NdotH = saturate(dot(N, H));
	float LdotH = saturate(dot(L, H));
	float NdotV = saturate(dot(N, V));

	// Evaluate terms for our GGX BRDF model
	float rough = hit.mat.roughness;
	vec3 spec = hit.mat.specular;
	float D = ggxNormalDistribution(NdotH, rough);
	float G = ggxSchlickMaskingTerm(NdotL, NdotV, rough);
	vec3 F = schlickFresnel(spec, LdotH);
	float denom = 4.0 * max(NdotV, 0.001) * max(NdotL, 0.001);
	return (D * G * F) / max(denom, 0.001);
}

vec3 BRDF(HitRecord hit, vec3 inDir, vec3 outDir, bool isDiffuse) {
	float NdotL;
	float NdotH;
	float LdotH;
	float NdotV;
	float D;
	float G;
	vec3 F;
	vec3 H;

	if (isDiffuse) {
		H = normalize(inDir + outDir);
	}
	else {
		H = SampleSpecularHalfVec(hit.p, hit.mat.roughness, hit.normal);
	}
	GetAllBRDFValues(hit, -inDir, outDir, H, NdotL, NdotH, LdotH, NdotV, D, G, F);
	
	vec3 ggxTerm = D * G * F / (4 * NdotV /* * NdotL */);
	vec3 diffuseTerm = NdotL * hit.mat.albedo / M_PI;
	if (isDiffuse) {
		return diffuseTerm;
	}
	else {
		return ggxTerm;
	}
}