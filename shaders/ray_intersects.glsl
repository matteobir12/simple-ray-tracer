#version 450

// describes the start and end of each BVH in the BVHNodeBuffer
struct BVH {
  uint first_index;
  uint count;
};

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
  // Sampler2D texture; // when textures are supported
  uint use_texture = 0; // bool
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
bool IntersectsTriangle(vec3 ray_origin, vec3 ray_dir, Triangle tri) {
  vec3 edge_1 = tri.v1 - tri.v0;
  vec3 edge_2 = tri.v2 - tri.v0;
  vec3 h = cross(ray_dir, edge_2);
  float a = dot(edge_1, h);
  if (a > -0.0001f && a < 0.0001f)
    return false; // ray parallel to triangle

  float f = 1 / a;
  vec3 s = ray_origin - tri.v0;
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

bool Intersects(vec3 ray_origin, vec3 ray_dir) {
  int stack[64];
  int stack_idx = 0;
  stack[stack_idx++] = 0;

  while (stack_idx > 0) {
    int node_idx = stack[--stack_idx];
    BVHNode node = nodes[node_idx];

    if (IntersectsBox(ray_origin, ray_dir, node.min_bounds, node.max_bounds)) {
      if (node.triangle_index >= 0) { // leaf node
        Triangle tri = triangles[node.triangle_index];
        if (IntersectsTriangle(ray_origin, ray_dir, tri)) {
          // currently just finds the first triangle
          //No gurentee this is the only or closest
          // also will need to return triangle mtl data
          return true;
        }
      } else {
        // internal node
        // if we add the child that is closer to the ray origin first we may
        // be able to return on first triangle found, but will still need to
        // think on this
        stack[stack_idx++] = node.left_child;
        stack[stack_idx++] = node.right_child;
      }
    }
  }

  // hit nothing
  return false;
}