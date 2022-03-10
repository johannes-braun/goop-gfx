#pragma once

#include <rnu/math/math.hpp>
#include <variant>
#include <vector>
#include <chrono>
#include "smooth.hpp"

namespace goop
{
  template<typename T>
  struct interpolate_value
  {
    T operator()(T const& prev, T const& next, float t) const {
      return prev + (next - prev) * t;
    }
  };

  template<typename T>
  struct interpolate_value<rnu::quat_t<T>>
  {
    using quat_type = rnu::quat_t<T>;

    quat_type operator()(quat_type const& prev, quat_type const& next, float t) const {
      return rnu::slerp(prev, next, t);
    }
  };

  template<typename T>
  auto const interp(T a, T b, float t)
  {
    return interpolate_value<T>{}(a, b, t);
  }

  struct transform_node
  {
    struct loc_rot_scale
    {
      rnu::vec3 location;
      rnu::quat rotation;
      rnu::vec3 scale;
    };
    using transform = std::variant<rnu::mat4, loc_rot_scale>;

    loc_rot_scale& lrs();
    loc_rot_scale const& lrs() const;

    void set_location(rnu::vec3 location);
    void set_rotation(rnu::quat rotation);
    void set_scale(rnu::vec3 scale);
    void set_matrix(rnu::mat4 matrix);

    rnu::mat4 matrix() const;

    ptrdiff_t children_offset;
    size_t num_children;
    ptrdiff_t parent;
    transform transformation;
  };

  enum class animation_target
  {
    location,
    rotation,
    scale
  };

  class animation
  {
  public:
    using checkpoints = std::variant<
      std::vector<rnu::vec3>,
      std::vector<rnu::quat>
    >;

    animation(animation_target target, size_t node_index, checkpoints checkpoints, std::vector<float> timestamps);

    float duration() const;

    void transform(float time, float mix_factor, std::vector<transform_node>& nodes) const;

  private:
    size_t _node_index;
    animation_target _target;
    checkpoints _checkpoints;
    std::vector<float> _timestamps;
  };

  class joint_animation
  {
  public:
    struct anim_info
    {
      std::vector<animation> animation;
      bool one_shot;
    };

    void set(std::vector<animation> anim, bool one_shot = true);

    void start(double offset = 0.0f);

    void update(std::chrono::duration<double> d, std::vector<transform_node>& nodes);

  private:
    anim_info _anim;
    size_t _current;
    double _time = 0;
    float _longest_duration = 0.0f;
    goop::smooth<double> _ramp_up;
  };

  class transform_tree
  {
  public:
    transform_tree(std::vector<transform_node> nodes);

    void animate(joint_animation& animation, std::chrono::duration<double> delta);
    rnu::mat4 const& global_transform(size_t node) const;

  private:
    void recompute_globals();

    std::vector<transform_node> _nodes;
    std::vector<rnu::mat4> _global_matrices;
  };

  class skin
  {
  public:
    skin(size_t root, std::vector<std::uint32_t> joint_nodes, std::vector<rnu::mat4> joint_matrices);

    std::uint32_t joint_node(size_t index) const;
    rnu::mat4 const& joint_matrix(size_t index) const;
    size_t size() const;

    std::vector<rnu::mat4>& apply_global_transforms(transform_tree const& tree);

  private:
    size_t _root_node;
    std::vector<std::uint32_t> _joint_nodes;
    std::vector<rnu::mat4> _joint_matrices;
    std::vector<rnu::mat4> _global_joint_matrices;
  };
}