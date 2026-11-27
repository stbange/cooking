in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_tex;

out vec4 frag_color;

void main() {
    frag_color = texture(u_tex, v_uv) * v_color;
}
