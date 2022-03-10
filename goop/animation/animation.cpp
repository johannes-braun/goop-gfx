#include "animation.hpp"

namespace goop
{
  animation::animation(animation_target target, size_t node_index, checkpoints checkpoints, std::vector<float> timestamps)
    : _node_index(node_index), _target(target), _checkpoints(std::move(checkpoints)), _timestamps(std::move(timestamps))
  {
  }
  float animation::duration() const { return _timestamps.empty() ? 0 : _timestamps.back(); }
  void animation::transform(float time, float mix_factor, std::vector<transform_node>& nodes) const {
    if (_timestamps.empty())
      return;

    auto const clamped_time = std::clamp(time, _timestamps.front(), _timestamps.back());

    auto const ts_iter = std::prev(std::upper_bound(begin(_timestamps), end(_timestamps), clamped_time));
    auto const current_ts_index = std::distance(begin(_timestamps), ts_iter);
    auto const next_ts_index = (current_ts_index + 1) % _timestamps.size();

    auto const current_ts = _timestamps[current_ts_index];
    auto const next_ts = _timestamps[next_ts_index];
    auto const i = (next_ts_index == current_ts_index) ? 0.0 : (time - current_ts) / (next_ts - current_ts);

    auto& n = nodes[_node_index];

    switch (_target)
    {
    case animation_target::location:
    {
      auto const& locations = std::get<0>(_checkpoints);
      auto const calc = interp(locations[current_ts_index], locations[next_ts_index], i);
      auto const attenuated = interp(n.lrs().location, calc, mix_factor);

      n.set_location(attenuated);
      break;
    }
    case animation_target::rotation:
    {
      auto const& rotations = std::get<1>(_checkpoints);
      auto const rot = interp(rotations[current_ts_index], rotations[next_ts_index], i);
      auto const attenuated = interp(n.lrs().rotation, rot, mix_factor);

      n.set_rotation(attenuated);
      break;
    }
    case animation_target::scale:
    {
      auto const& scales = std::get<0>(_checkpoints);
      auto const sca = interp(scales[current_ts_index], scales[next_ts_index], i);
      auto const attenuated = interp(n.lrs().scale, sca, mix_factor);

      n.set_scale(attenuated);
      break;
    }
    }
  }

  skin::skin(size_t root, std::vector<std::uint32_t> joint_nodes, std::vector<rnu::mat4> joint_matrices)
    : _root_node(root), _joint_nodes(std::move(joint_nodes)), _joint_matrices(std::move(joint_matrices)) {
  }
  std::uint32_t skin::joint_node(size_t index) const { return _joint_nodes[index]; }
  rnu::mat4 const& skin::joint_matrix(size_t index) const { return _joint_matrices[index]; }
  size_t skin::size() const { return _joint_nodes.size(); }
  std::vector<rnu::mat4>& skin::apply_global_transforms(transform_tree const& tree)
  {
    _global_joint_matrices.resize(size(), rnu::mat4(1.0f));

    for (int i = 0; i < size(); ++i)
    {
      auto const& g = tree.global_transform(joint_node(i));
      _global_joint_matrices[i] = g * joint_matrix(i);
    }
    return _global_joint_matrices;
  }

  transform_tree::transform_tree(std::vector<transform_node> nodes)
    : _nodes(std::move(nodes)), _global_matrices(_nodes.size(), rnu::mat4(1.0f)) {
    recompute_globals();
  }

  void transform_tree::animate(joint_animation& animation, std::chrono::duration<double> delta)
  {
    animation.update(delta, _nodes);
    recompute_globals();
  }

  rnu::mat4 const& transform_tree::global_transform(size_t node) const
  {
    return _global_matrices[node];
  }

  void transform_tree::recompute_globals()
  {
    for (int i = 0; i < _nodes.size(); ++i)
    {
      _global_matrices[i] = _nodes[i].matrix();
    }

    for (int i = 0; i < _nodes.size(); ++i)
    {
      if (_nodes[i].parent != -1)
      {
        _global_matrices[i] = _global_matrices[_nodes[i].parent] * _global_matrices[i];
      }
    }
  }


  void joint_animation::set(std::vector<animation> anim, bool one_shot)
  {
    for (auto const& a : anim)
      _longest_duration = std::max(_longest_duration, a.duration());
    _anim.animation = std::move(anim);
    _anim.one_shot = one_shot;
  }

  void joint_animation::start(double offset)
  {
    _time = offset;
    _ramp_up = 0;
    _ramp_up.to(1.0);
    _current = 0;
  }

  void joint_animation::update(std::chrono::duration<double> d, std::vector<transform_node>& nodes)
  {
    _time += d.count();
    _ramp_up.update(d.count());

    bool animation_finished = false;
    if (_longest_duration <= _time)
      animation_finished = true;

    if (animation_finished)
    {
      if (!_anim.one_shot)
        _time = std::fmodf(_time, _longest_duration);
    }

    for (auto& a : _anim.animation)
      a.transform(_time, float(_ramp_up.value()), nodes);
  }


  transform_node::loc_rot_scale& transform_node::lrs() {
    return std::get<loc_rot_scale>(transformation);
  }

  transform_node::loc_rot_scale const& transform_node::lrs() const {
    return std::get<loc_rot_scale>(transformation);
  }

  void transform_node::set_location(rnu::vec3 location) {
    std::get<loc_rot_scale>(transformation).location = location;
  }

  void transform_node::set_rotation(rnu::quat rotation) {
    std::get<loc_rot_scale>(transformation).rotation = rotation;
  }

  void transform_node::set_scale(rnu::vec3 scale) {
    std::get<loc_rot_scale>(transformation).scale = scale;
  }

  void transform_node::set_matrix(rnu::mat4 matrix) {
    std::get<rnu::mat4>(transformation) = matrix;
  }

  rnu::mat4 transform_node::matrix() const {
    static constexpr struct
    {
      rnu::mat4 operator()(rnu::mat4 const& mat) const
      {
        return mat;
      }

      rnu::mat4 operator()(loc_rot_scale const& lrs) const
      {
        return rnu::translation(lrs.location) * rnu::rotation(lrs.rotation) * rnu::scale(lrs.scale);
      }
    } visitor{};
    return std::visit(visitor, transformation);
  }


}