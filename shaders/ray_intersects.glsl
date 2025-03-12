#version 450

// Look into these sizes
layout(local_size_x = 8, local_size_y = 8) in;

// describes the start and len of each BVH in the BVHNodeBuffer
// as well as the coordinate frame the bvh is in (world frame to model frame)
// I'm pretty sure this is also inverse traditional model matrix
struct BVH {
  uint first_index;
  uint count;
  mat4 frame;
};

uniform uint bvh_count;
layout(std430, binding = 0) buffer BVHsBuffer {
  BVH bvhs[];
};

// we're currently going with 2 children, so at first_child and first_child + 1
struct BVHNode {
  vec3 min_bounds;
  vec3 max_bounds;
  uint first_child;
  uint first_prim_index;
  uint prim_count;
};

// massive buffer of all BVHs in the scene
// the current idea is one BVH per model
// it's also possible that the first BVH becomes a special case, scene BVH
layout(std430, binding = 1) buffer BVHNodeBuffer {
  BVHNode nodes[];
};

// TODO: texture support
// needs to match asset_utils/types.h Material
struct Material {
  vec3 diffuse;
  vec3 specular;
  float specular_ex;
  uint use_texture; // bool
};

// buffer of all materials in the scene
layout(std430, binding = 2) buffer MaterialBuffer {
  Material materials[];
};

// data for each triangle in buffer
// each member points to another buffer
struct Triangle {
  uint v0_idx;
  uint v1_idx;
  uint v2_idx;
  uint material_idx;
};

// massive buffer of all triangles in scene
// there is a max size of sizeof(uint) ~ 4b
layout(std430, binding = 3) buffer TriangleBuffer {
  Triangle triangles[];
};

// data for each vertex in buffer
struct VertexData {
  vec3 vertex;
  vec2 texture;
};

// massive buffer of all verticies in scene
// there is a max size of sizeof(uint) ~ 4b
layout(std430, binding = 4) buffer VertexBuffer {
  VertexData vertices[];
};

struct Ray {
  vec3 origin;
  float _pad0; // ?? Idk I've seen this around
  vec3 direction;
  float intersection_distance;
};

layout(std430, binding = 5) buffer RayBuffer {
  Ray rays[];
};

// tmp testing out buffer
layout(std430, binding = 6) buffer HitBuffer {
  uint hits[];
};

bool IntersectsBox(vec3 ray_origin, vec3 ray_dir, vec3 min_bounds, vec3 max_bounds) {
  vec3 inv_dir = 1.0 / ray_dir;
  vec3 t0 = (min_bounds - ray_origin) * inv_dir;
  vec3 t1 = (max_bounds - ray_origin) * inv_dir;
  vec3 t_min = min(t0, t1);
  vec3 t_max = max(t0, t1);
  float t_near = max(max(t_min.x, t_min.y), t_min.z);
  float t_far = min(min(t_max.x, t_max.y), t_max.z);
  return t_near <= t_far;
}

// could make branchless, but doubt it will make a large diff
bool IntersectsTriangle(
    vec3 ray_origin,
    vec3 ray_dir,
    vec3 v0,
    vec3 v1,
    vec3 v2) {
  vec3 edge_1 = v1 - v0;
  vec3 edge_2 = v2 - v0;
  vec3 h = cross(ray_dir, edge_2);
  float a = dot(edge_1, h);
  if (a > -0.0001f && a < 0.0001f)
    return false; // ray parallel to triangle

  float f = 1 / a;
  vec3 s = ray_origin - v0;
  float u = f * dot(s, h);
  if (u < 0 || u > 1)
    return false;

  vec3 q = cross(s, edge_1);
  float v = f * dot(ray_dir, q);
  if (v < 0 || u + v > 1)
    return false;

  float t = f * dot( edge_2, q );
  return t > 0.0001f;
}

// Returns index of triangle hit, or sentinal if miss (uint(-1))
uint Intersects(uint bvh_start_index, vec3 ray_origin, vec3 ray_dir) {
  uint stack[64];
  int stack_idx = 0;
  stack[stack_idx++] = bvh_start_index;

  while (stack_idx > 0) {
    uint node_idx = stack[--stack_idx];
    BVHNode node = nodes[node_idx];

    if (IntersectsBox(ray_origin, ray_dir, node.min_bounds, node.max_bounds)) {
      if (node.prim_count > 0) {
        // leaf node
        for (uint i = 0; i < node.prim_count; ++i) {
          Triangle tri = triangles[node.first_prim_index + i];

          vec3 v0 = vertices[tri.v0_idx].vertex;
          vec3 v1 = vertices[tri.v1_idx].vertex;
          vec3 v2 = vertices[tri.v2_idx].vertex;

          if (IntersectsTriangle(ray_origin, ray_dir, v0, v1, v2)) {
            return node.first_prim_index + i;
          }
        }
      } else {
        // internal node
        uint left_child  = node.first_child;
        uint right_child = node.first_child + 1;
        // if we add the child that is closer to the ray origin first we may
        // be able to return on first triangle found, but I will still need to
        // think on this
        stack[stack_idx++] = left_child;
        stack[stack_idx++] = right_child;
      }
    }
  }

  // hit nothing
  return uint(-1);
}

void main() {
  // 2D dispatch
  uvec2 gid = gl_GlobalInvocationID.xy;

  // validate this indexing
  uint width = gl_NumWorkGroups.x * gl_WorkGroupSize.x;
  uint index = gid.y * width + gid.x;

  Ray ray = rays[index];

  // will need some sort of closest hit
  // for (every bounce)
  // for (every model in scene) or replace with a bvh traverse
  for (uint i = 0; i < bvh_count; i++) {
    // transform ray into model's space
    vec4 trans_origin = bvhs[i].frame * vec4(ray.origin, 1.);
    vec4 trans_direction = bvhs[i].frame * vec4(ray.direction, 0.);
    uint hit = Intersects(bvhs[i].first_index, trans_origin.xyz, trans_direction.xyz);
    // calc new ray from bounce or exit
    hits[index] = hit != uint(-1)? 1u : 0u;
  }
}
