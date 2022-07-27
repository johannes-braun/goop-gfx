#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv[3];
layout(location = 5) in vec4 color;
layout(location = 6) in uvec4 joint_arr;
layout(location = 7) in vec4 weight_arr;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(std430, binding = 0) restrict readonly buffer Matrices
{
  mat4 view;
  mat4 proj;
};

layout(std430, binding = 3) restrict readonly buffer Transform
{
  mat4 transform;
  int animated;
};

layout(std430, binding = 1) restrict readonly buffer Joints
{
  mat4 joints[];
};

layout(location = 0) out vec3 pass_position;
layout(location = 1) out vec3 pass_normal;
layout(location = 2) out vec2 pass_uv[3];
layout(location = 5) out vec4 pass_color;
layout(location = 6) out vec3 pass_view_position;

void main()
{
  mat4 skinMat = (animated != 0) ? 
    (weight_arr.x * joints[int(joint_arr.x)] +
    weight_arr.y * joints[int(joint_arr.y)] +
    weight_arr.z * joints[int(joint_arr.z)] +
    weight_arr.w * joints[int(joint_arr.w)]) : mat4(1.0);

  vec3 pos = position;
  vec4 hom_position = transform * skinMat * vec4(pos, 1);
  pass_view_position = (view * hom_position).xyz;
  gl_Position = proj * view * hom_position;

  pass_position = hom_position.xyz / hom_position.w;
  pass_normal = vec3(inverse(transpose(mat4(mat3(transform * skinMat)))) * vec4(normal, 0));
  pass_uv[0] = uv[0];
  pass_uv[1] = uv[1];
  pass_uv[2] = uv[2];
  pass_color = color;
}