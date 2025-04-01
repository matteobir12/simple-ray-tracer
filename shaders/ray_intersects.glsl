#define FIRST_BIND_POINT 3

// Look into these sizes
// #ifdef COMPUTE_TEST
// layout(local_size_x = 8, local_size_y = 8) in;
// #endif

uniform uint bvh_count;
layout(std430, binding = FIRST_BIND_POINT + 0) buffer BVHsBuffer {
  BVH bvhs[];
};

// massive buffer of all BVHs in the scene
// the current idea is one BVH per model
// it's also possible that the first BVH becomes a special case, scene BVH
layout(std430, binding = FIRST_BIND_POINT + 1) buffer BVHNodeBuffer {
  BVHNode nodes[];
};

// buffer of all materials in the scene
layout(std430, binding = FIRST_BIND_POINT + 2) buffer MaterialBuffer {
  MaterialFromOBJ materials[];
};

// massive buffer of all triangles in scene
// there is a max size of sizeof(uint) ~ 4b
layout(std430, binding = FIRST_BIND_POINT + 3) buffer TriangleBuffer {
  Triangle triangles[];
};

// massive buffer of all verticies in scene
// there is a max size of sizeof(uint) ~ 4b
layout(std430, binding = FIRST_BIND_POINT + 4) buffer VertexBuffer {
  VertexData vertices[];
};

// #ifdef COMPUTE_TEST
// layout(std430, binding = FIRST_BIND_POINT + 5) buffer RayBuffer {
//   Ray rays[];
// };

// // tmp testing out buffer
// layout(std430, binding = FIRST_BIND_POINT + 6) buffer HitBuffer {
//   uint hits[];
// };
// #endif

// if we return the distance here we can short circut if we've found closer triangles
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
    vec3 v2,
    inout float intersection_distance,
    inout vec3 tri_norm) {
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

  float t = f * dot(edge_2, q);

  if (t > 0.00001 /* eps */ && t < intersection_distance) {
    tri_norm = normalize(cross(edge_1, edge_2));
    intersection_distance = t;
    return true;
  }

  return false;
}

// Returns index of triangle hit, or sentinal if miss (uint(-1))
uint Intersects(uint bvh_start_index, vec3 ray_origin, vec3 ray_dir, inout float intersection_distance, inout vec3 tri_norm) {
  uint stack[64];
  int stack_idx = 0;
  stack[stack_idx++] = bvh_start_index;
  uint out_tri_indx = -1;

  while (stack_idx > 0) {
    uint node_idx = stack[--stack_idx];
    BVHNode node = nodes[node_idx];

    if (IntersectsBox(ray_origin, ray_dir, node.min_bounds, node.max_bounds)) {
      if (node.prim_count > 0) {
        // leaf node
        //return (-1);
        for (uint i = 0; i < node.prim_count; ++i) {
          Triangle tri = triangles[node.first_child_or_prim_index + i];

          vec3 v0 = vertices[tri.v0_idx].vertex;
          vec3 v1 = vertices[tri.v1_idx].vertex;
          vec3 v2 = vertices[tri.v2_idx].vertex;

          if (IntersectsTriangle(ray_origin, ray_dir, v0, v1, v2, intersection_distance, tri_norm)) {
            out_tri_indx = node.first_child_or_prim_index + i;
          }
        }
      } else {
        // internal node
        stack[stack_idx++] = node.first_child_or_prim_index;
        stack[stack_idx++] = node.first_child_or_prim_index + 1;
      }
    }
  }

  return out_tri_indx;
}

// void main() {
//   // 2D dispatch
//   uvec2 gid = gl_GlobalInvocationID.xy;

//   // validate this indexing
//   uint width = gl_NumWorkGroups.x * gl_WorkGroupSize.x;
//   uint index = gid.y * width + gid.x;

//   Ray ray = rays[index];

//   hits[index] = uint(-1); // Not necessary if we can guarentee input is uint(-1)
//   // will need some sort of closest hit
//   // for (every bounce)
//   // for (every model in scene) or replace with a bvh traverse
//   for (uint i = 0; i < bvh_count; i++) {
//     // transform ray into model's space
//     vec4 trans_origin = bvhs[i].frame * vec4(ray.origin, 1.);
//     vec4 trans_direction = bvhs[i].frame * vec4(ray.direction, 0.);
//     uint hit = Intersects(bvhs[i].first_index, trans_origin.xyz, trans_direction.xyz, ray.intersection_distance);
//     // rays[index].intersection_distance = ray.intersection_distance; // only necessary if we need the distance back in the buffer
//     if (hit != uint(-1)) {
//       hits[index] = hit;
//       // hits[index] = 0;
//     }
//     // calc new ray from bounce or exit
//   }
// }
