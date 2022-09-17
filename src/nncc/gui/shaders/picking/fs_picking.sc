$input v_pos, v_view, v_normal, v_texcoord0

#include <../examples/common/common.sh>
#include <bgfx_shader.sh>

uniform vec4 u_id;

void main() {
	gl_FragColor.rgb = u_id.rgb; // This is dumb, should use u8 texture
	gl_FragColor.w = 1.0;
}
