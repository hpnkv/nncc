$input v_pos, v_view, v_normal, v_texcoord0

#include "common.sh"

SAMPLER2D(diffuseTX, 0);

uniform vec4 diffuseCol;  // the diffuse colour

#define MAX_LIGHTS 4

// Input lighting values
//uniform vec4 lightPos[MAX_LIGHTS];
//uniform vec4 lightColour[MAX_LIGHTS];
//uniform vec4 viewPos;


void main() {
//    vec4 light = vec4(0);
//
//    for (int l = 0; l < MAX_LIGHTS; l++) {
//        vec3 lp =  mul(u_modelView, lightPos[l] ).xyz;
//        vec3 lightDir = normalize(lp - v_pos);
//        float ndotl = max(dot(v_normal.xyz, lightDir), 0.0);
//        float spec = pow(ndotl, 30.0); // TODO calculate proper specular term, although doesn't look terrible...
//        light += lightColour[l] * (ndotl+spec);
//    }

//    gl_FragColor = vec4(light.rgb * diffuseCol.rgb * texture2D( diffuseTX, v_texcoord0 ).rgb,1);
//    gl_FragColor = texture2D( diffuseTX, v_texcoord0 );
    gl_FragColor = vec4(diffuseCol.rgb * texture2D( diffuseTX, v_texcoord0 ).rgb, 1);
}
