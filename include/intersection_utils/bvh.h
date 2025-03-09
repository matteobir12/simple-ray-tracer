#pragma once

#include <limits>
#include <vector>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <functional>

#include <glm/glm.hpp>

#include "common/types.h"

namespace IntersectionUtils {
/**
* Will have to conform to std430 alignment
* 
* @var first_child A pointer to the first child in the BVH. The children
*                  are expected to be afterwards
*/
struct BVHNode {
  glm::vec3 min_bounds;
  glm::vec3 max_bounds;
  std::uint32_t first_child;
  std::uint32_t first_prim_index;
  std::uint32_t prim_count;
  bool IsLeaf() const { return prim_count > 0; }
};

template <class Prim>
class BVH {
 public:
  /** Constructs a BVH for the given primatives
   * Will also rearrange primatives
   * 
   * probably needs different input type. Will depend on how scene is defined
   */
  BVH(
      std::vector<Prim> primatives,
      const std::function<glm::vec3(const Prim&)>& center_fn,
      const std::function<std::pair<glm::vec3, glm::vec3>(const Prim&)>& bounds_fn)
    : primatives_(std::move(primatives))
  {
    std::vector<std::uint32_t> primatives_idxs;
    primatives_idxs.resize(primatives_.size());
    std::transform(primatives_idxs.cbegin(), primatives_idxs.cend(), primatives_idxs.begin(), [](auto idx) {
      return idx;
    });

    bvh_.resize((2 * primatives_.size()) - 1); // might need to change if switching to BVH4 or BVH8

    std::vector<glm::vec3> centers;
    centers.reserve(primatives_.size());
    for (const auto& primative : primatives_)
      centers.emplace_back(center_fn(primative));

    BVHNode& root = bvh_[0];
    root.first_child = 0;
    root.first_prim_index = 0;
    root.prim_count = primatives_.size();
    UpdateNodeBounds(primatives_idxs, bounds_fn, &root);
    Subdivide(&primatives_idxs, centers, bounds_fn, &root);

    // reorder primatives_
  }

  const std::vector<BVHNode>& GetBVH() const { return bvh_; }
  const std::vector<Prim>& GetPrims() const { primatives_; } 
 private:
  void UpdateNodeBounds(
      const std::vector<std::uint32_t>& primatives_idxs,
      const std::function<std::pair<glm::vec3, glm::vec3>(const Prim&)>& bounds_fn,
      BVHNode* const node_ptr) {
    BVHNode& node = *node_ptr;
    node.min_bounds = glm::vec3(std::numeric_limits<float>::max());
    node.max_bounds = glm::vec3(std::numeric_limits<float>::lowest());
    const std::uint32_t first = node.first_prim_index;
    for (std::uint32_t i = 0; i < node.prim_count; i++) {
      const auto leaf_prim_idx = primatives_idxs[first + i];
      const auto& leaf_prim = primatives_[leaf_prim_idx];
      const auto [prim_min, prim_max] = bounds_fn(leaf_prim);

      node.min_bounds = glm::min(node.min_bounds, prim_min);
      node.max_bounds = glm::max(node.max_bounds, prim_max);
    }
  }

  void Subdivide(
      std::vector<std::uint32_t>* const primatives_idxs_ptr,
      const std::vector<glm::vec3>& centers,
      const std::function<std::pair<glm::vec3, glm::vec3>(const Prim&)>& bounds_fn,
      BVHNode* const node_ptr) {
    auto& primatives_idxs = *primatives_idxs_ptr;
    auto& node = *node_ptr;
    // I'm not certain this condition is logically sound.
    // Should probably implement something like if prim_count == prev prim_count return
    if (node.prim_count <= 2)
      return;

    const glm::vec3 extent = node.max_bounds - node.min_bounds;

    // stand in for axis to divide, find better partitioning algo.
    int axis = 0;
    if (extent.y > extent.x)
      axis = 1;

    if (extent.z > extent[axis])
      axis = 2;

    const float split_pos = node.min_bounds[axis] + extent[axis] * 0.5f;

    std::uint32_t i = node.first_prim_index;
    std::uint32_t j = i + node.prim_count - 1;
    while (i <= j) {
      if (centers[primatives_idxs[i]][axis] < split_pos)
        i++;
      else
        std::swap(primatives_idxs[i], primatives_idxs[j--]);
    }
    
    const std::uint32_t left_count = i - node.first_prim_index;
    if (left_count == 0 || left_count == node.prim_count)
      return;

    const std::uint32_t left_child_idx = next_free_node_idx_;
    const std::uint32_t right_child_idx = next_free_node_idx_ + 1;
    node.first_child = left_child_idx;
    bvh_[left_child_idx].first_prim_index = node.first_prim_index;
    bvh_[left_child_idx].prim_count = left_count;
    bvh_[right_child_idx].first_prim_index = i;
    bvh_[right_child_idx].prim_count = node.prim_count - left_count;

    node.prim_count = 0;
    next_free_node_idx_ += 2;

    UpdateNodeBounds(primatives_idxs, bounds_fn, &bvh_[left_child_idx]);
    UpdateNodeBounds(primatives_idxs, bounds_fn, &bvh_[right_child_idx]);
    Subdivide(&primatives_idxs, centers, bounds_fn, &bvh_[left_child_idx]);
    Subdivide(&primatives_idxs, centers, bounds_fn, &bvh_[right_child_idx]);
  }

  std::vector<Prim> primatives_;
  std::vector<BVHNode> bvh_;
  std::uint32_t next_free_node_idx_;
};

}
