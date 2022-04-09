#pragma once

#include <rnu/ecs/component.hpp>
#include <rnu/ecs/system.hpp>
#include <rnu/math/math.hpp>
#include <optional>
#include <variant>
#include "transform_component.hpp"

namespace goop
{
  struct collision
  {
    rnu::vec3 point;
    rnu::vec3 other_point;
    rnu::vec3 normal;
    rnu::entity_handle other;
  };

  struct sphere_collider
  {
    rnu::vec3 center_offset{};
    float radius{};
  };

  struct plane_collider
  {
  };

  rnu::vec3 unhom(rnu::vec4 x) { return rnu::vec3(x) / x.w; }

  struct collider_state
  {
    rnu::entity_handle entity;
    rnu::mat4 matrix;
    rnu::mat4 inverse_matrix;
  };

  std::optional<std::pair<collision, collision>> collide(
    collider_state const& state_lhs, sphere_collider const& lhs,
    collider_state const& state_rhs, plane_collider const& rhs)
  {
    auto const inv_tf_rhs = state_rhs.matrix;
    auto const rhs_to_lhs = state_lhs.inverse_matrix * inv_tf_rhs;

    auto const inv_tf_lhs = state_lhs.matrix;
    auto const lhs_to_rhs = state_rhs.inverse_matrix * inv_tf_lhs;

    auto const plane_to_point = unhom(lhs_to_rhs * rnu::vec4(lhs.center_offset, 1));
    auto const point_on_plane = rnu::vec3(plane_to_point.x, 0, plane_to_point.z);
    auto const pop_in_lhs = unhom(rhs_to_lhs * rnu::vec4(point_on_plane, 1));
    auto const point_on_sphere = lhs.center_offset + lhs.radius * normalize(pop_in_lhs - lhs.center_offset);
    auto const pos_in_rhs = unhom(lhs_to_rhs * rnu::vec4(point_on_sphere, 1));

    if (pos_in_rhs.y < 0)
    {
      auto const neg_lhs_normal_unnorm = rnu::vec3(rhs_to_lhs * rnu::vec4(0, 1, 0, 0));

      collision lhs_coll;
      lhs_coll.normal = normalize(neg_lhs_normal_unnorm);
      lhs_coll.point = (point_on_sphere);
      lhs_coll.other_point = (pop_in_lhs);
      lhs_coll.other = state_rhs.entity;
      collision rhs_coll;
      rhs_coll.normal = -rnu::vec3(0, 1, 0);
      rhs_coll.point = (point_on_plane);
      rhs_coll.other_point = (pos_in_rhs);
      rhs_coll.other = state_lhs.entity;

      return std::pair(lhs_coll, rhs_coll);
    }
    return std::nullopt;
  }

  std::optional<std::pair<collision, collision>> collide(
    collider_state const& state_lhs, plane_collider const& lhs,
    collider_state const& state_rhs, sphere_collider const& rhs)
  {
    return collide(state_lhs, rhs, state_rhs, lhs);
  }

  std::optional<std::pair<collision, collision>> collide(
    collider_state const& state_lhs, plane_collider const& lhs,
    collider_state const& state_rhs, plane_collider const& rhs)
  {
    return std::nullopt;
  }

  std::optional<std::pair<collision, collision>> collide(
    collider_state const& state_lhs, sphere_collider const& lhs,
    collider_state const& state_rhs, sphere_collider const& rhs)
  {
    auto const inv_tf_rhs = state_rhs.matrix;
    auto const rhs_to_lhs = state_lhs.inverse_matrix * inv_tf_rhs;
    auto const inv_tf_lhs = state_lhs.matrix;
    auto const lhs_to_rhs = state_rhs.inverse_matrix * inv_tf_lhs;

    auto const rhs_center_in_lhs_coords = unhom(rhs_to_lhs * rnu::vec4(rhs.center_offset, 1));
    auto const lhs_center_in_rhs_coords = unhom(lhs_to_rhs * rnu::vec4(lhs.center_offset, 1));

    auto const nearest_on_rhs = rhs.center_offset + normalize(lhs_center_in_rhs_coords - rhs.center_offset) * rhs.radius;
    auto const nearest_on_rhs_in_lhs = unhom(rhs_to_lhs * rnu::vec4(nearest_on_rhs, 1));

    auto const nearest_on_lhs = lhs.center_offset + normalize(rhs_center_in_lhs_coords - lhs.center_offset) * lhs.radius;
    auto const nearest_on_lhs_in_rhs = unhom(lhs_to_rhs * rnu::vec4(nearest_on_lhs, 1));
    
    auto const neg_lhs_normal_unnorm = nearest_on_rhs_in_lhs - lhs.center_offset;
    auto const neg_rhs_normal_unnorm = nearest_on_lhs_in_rhs - rhs.center_offset;

    if (rnu::norm(neg_lhs_normal_unnorm) < lhs.radius)
    {
      // collide
      collision lhs_coll;
      lhs_coll.normal = -normalize(neg_lhs_normal_unnorm);
      lhs_coll.point = (nearest_on_lhs);
      lhs_coll.other_point = (nearest_on_rhs_in_lhs);
      lhs_coll.other = state_rhs.entity;
      collision rhs_coll;
      rhs_coll.normal = -normalize(neg_rhs_normal_unnorm);
      rhs_coll.point = (nearest_on_rhs);
      rhs_coll.other_point = (nearest_on_lhs_in_rhs);
      rhs_coll.other = state_lhs.entity;

      return std::pair(lhs_coll, rhs_coll);
    }
    return std::nullopt;
  }

  struct collider_component : rnu::component<collider_component>
  {
    using collider = std::variant<sphere_collider, plane_collider>;

    // sphere
    collider shape;
    bool movable = false;

    std::vector<collision> collisions{};
  };

  struct collision_collector_system : rnu::system
  {
  public:
    collision_collector_system()
    {
      add_component_type<collider_component>();
      add_component_type<transform_component>();
    }

    void update(duration_type delta, rnu::component_base** components) const override
    {
      auto* collider = static_cast<collider_component*>(components[0]);
      auto* transform = static_cast<transform_component*>(components[1]);

      collider->collisions.clear();
      auto const mat = transform->create_matrix();
      _found_colliders.push_back(collider_entity_info{
        collider_state{
          .entity = collider->entity,
          .matrix = mat,
          .inverse_matrix = inverse(mat)
        }
        , collider});
    }

    void post_update() override
    {
      for (size_t i = 0; i < _found_colliders.size(); ++i)
        for (size_t j = i + 1; j < _found_colliders.size(); ++j)
          check_collision(_found_colliders[i], _found_colliders[j]);
      _found_colliders.clear();
    }

  private:
    struct collider_entity_info
    {
      collider_state state;
      collider_component* collider;
    };

    void check_collision(collider_entity_info const& src, collider_entity_info const& other)
    {
      auto const& collider_lhs = src.collider->shape;
      auto const& collider_rhs = other.collider->shape;

      auto const result = std::visit([&](auto const& lhs, auto const& rhs) {
        return collide(src.state, lhs, other.state, rhs);
        }, collider_lhs, collider_rhs);

      if (result)
      {
        src.collider->collisions.push_back(result.value().first);
        other.collider->collisions.push_back(result.value().second);
      }
    }

    mutable std::vector<collider_entity_info> _found_colliders;
  };
}