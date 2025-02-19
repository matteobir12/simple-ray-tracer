#version 450

struct BVHNode {
  vec3 min_bounds;
  vec3 max_bounds;
  int left_child;
  int right_child;
  int triangle_index;
};

struct Triangle {
  vec3 v0;
  vec3 v1;
  vec3 v2;
};

layout(std430, binding = 0) buffer BVHBuffer {
  BVHNode nodes[];
};

layout(std430, binding = 1) buffer TriangleBuffer {
  Triangle triangles[];
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
          return true;
        }
      } else { // internal node
        stack[stack_idx++] = node.left_child;
        stack[stack_idx++] = node.right_child;
      }
    }
  }
  return false;
}