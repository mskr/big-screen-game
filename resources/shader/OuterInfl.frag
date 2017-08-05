#version 330 core

uniform sampler2D pathTexture;
uniform float time;
//uniform vec2 influencesPos[5];

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;
in vec4 gl_FragCoord;

layout(location = 0) out vec4 color;

vec2 pos[5];

float visibilityCalc(vec2 curPos){
	float result = 0.;
    for(int i = 0; i< 5; i++){
        result += exp(-20.*length(pos[i]-curPos));
    }
    //result = exp(-20.*length(pos[4]-curPos));
    return -log(result)/20.;
}

void calcPos(float t){
    for(int i = 0; i< 5; i++){
        float b = float(i)*t;
        pos[i] += vec2(.3*sin(b),0.);
    }
}

vec4 drawSlice( vec2 uv )
{
    float t = time/2.;
    calcPos(t);
	uv = vec2(.01,0.);
    float pot = visibilityCalc(uv);
//    return vec3(smoothstep(0.03,0.01,pot));
//    return vec4(smoothstep(0.03,0.01,pot),smoothstep(0.03,0.01,pot),smoothstep(0.03,0.01,pot),1-pot);
	return vec4(smoothstep(0.03,0.,pot));
}

void main() {
	// draw spots
	vec2 uv = vTexCoords;
	//uv = vec2(.01,0.);
    //vec4 spots = drawSlice( uv );
    
    // accumulate
    //color.rgba = spots;
	color.rgba = vec4(1.,0.,0.,1.);
}
