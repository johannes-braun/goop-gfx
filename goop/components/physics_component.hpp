#pragma once

#include <rnu/ecs/component.hpp>
#include <rnu/ecs/system.hpp>
#include <rnu/math/math.hpp>

#include "transform_component.hpp"
#include "collider_component.hpp"

namespace goop
{
  struct physics_component : rnu::component<physics_component>
  {
    rnu::vec3 acceleration{};
    rnu::vec3 velocity{};
    bool on_floor = false;
  };

  class physics_system : public rnu::system
  {
  public:
    physics_system()
    {
      add_component_type<transform_component>();
      add_component_type<physics_component>();
      add_component_type<collider_component>(rnu::component_flag::optional);
    }

    void update(duration_type delta, rnu::component_base** components) const override
    {
      auto* transform = static_cast<transform_component*>(components[0]);
      auto* physics = static_cast<physics_component*>(components[1]);
      auto* collider = static_cast<collider_component*>(components[2]);

      float accel_factor = 1.0f;
      physics->on_floor = false;

      auto const to_world_rot = rnu::mat3(rnu::transpose(rnu::inverse(transform->create_matrix())));
      rnu::vec3 offset{ 0,0,0 };

      for (auto const& coll : collider->collisions)
      {
        auto const n = normalize(to_world_rot * coll.normal);
        if (n.y > 0.8)
        {
          physics->velocity += (n * std::max(0.f, -dot(n, physics->velocity)));
          physics->acceleration += (n * std::max(0.f, -dot(n, physics->acceleration)));
          accel_factor = 0.8;
          physics->on_floor = true;
        }
        offset += coll.other_point - coll.point;
      }

      auto const rel = physics->velocity * float(delta.count());
      transform->position += to_world_rot * offset + rel;
      physics->velocity += accel_factor * physics->acceleration * delta.count();
    }

  private:

  };
}