#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv[3];
layout(location = 5) in vec4 color;

layout(binding = 0) uniform sampler2D image_texture;
layout(location = 0) out vec4 out_color;

layout(std430, binding=2) restrict readonly buffer Mat
{
  vec4 mat_color;
  uint has_texture;
};

void main()
{
  vec2 ux = vec2(uv[0].x, uv[0].y);

  float light = max(0, dot(normalize(normal), normalize(vec3(1,1,1))));

  vec4 tex_color = (has_texture != 0) ? texture(image_texture, ux) : mat_color;
  
  if(tex_color.a < 0.6)
    discard;
  vec3 col = max(color.rgb, tex_color.rgb);
  col = col * vec3(0.5f, 0.7f, 1.0f) + col * light;
  out_color = vec4(col, 1);
}