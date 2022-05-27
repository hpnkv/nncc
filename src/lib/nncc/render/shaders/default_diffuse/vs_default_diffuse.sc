$input a_position, a_normal, a_texcoord0
$output v_pos, v_view, v_normal, v_texcoord0

#include "common.sh"

void main() {
	//vec3 pos = a_position;
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
	v_pos = mul(u_modelView, vec4(a_position, 1.0) ).xyz;
	v_normal = mul(u_modelView, vec4(a_normal, 0.0) ).xyz;
	v_texcoord0 = a_texcoord0;
}