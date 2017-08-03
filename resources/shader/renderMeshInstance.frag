#version 330 core

/* Build state bits */
#define EMPTY 0U
#define INSIDE_ROOM 1U
#define CORNER 2U
#define WALL 4U
#define TOP 8U
#define BOTTOM 16U
#define RIGHT 32U
#define LEFT 64U
#define INVALID 128U
#define SOURCE 256U
#define INFECTED 512U
#define OUTER_INFLUENCE 1024U

/* Max health, respectively sand pile height */
#define MAX_HEALTH 100U

/* Build state and health for the whole grid (bilinear interpolation enabled) */
uniform sampler2D curr_grid_state;
uniform sampler2D last_grid_state;

/* Coordinates of this fragment on the grid (in texture space) */
in vec2 cellCoords;

/* Build state and health of this grid cell */
flat in uint st;
flat in uint hp;

/* Interpolated vertex attributes */
in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;

uniform float automatonTimeDelta;

struct Material {
    float alpha;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
//    float shininess;
};

uniform Material material;

uniform int isDepthPass;
uniform int isDebugMode;

out vec4 color;

void main() {
    // In depth pass do not need to do anything because depth is implicitly written
    if(isDepthPass == 1) return;

    // Debug mode draws white wireframe
    if(isDebugMode == 1) {
        color = vec4(1);
        return;
    }

    // Lighting
    // vec3 lightDir = normalize(vPosition - vec3(-10.0f, -10.0f, -10.0f));
    // float NdotL = clamp(dot(lightDir, normalize(vNormal)), 0.0f, 1.0f);
    
    float healthNormalized = float(hp) / float(MAX_HEALTH);
    vec2 lastCellState = texture(last_grid_state, cellCoords).rg;
    vec2 currCellState = texture(curr_grid_state, cellCoords).rg;
    float lastBuildState = lastCellState.r;
    float lastHealth = lastCellState.g;
    float currBuildState = currCellState.r;
    float currHealth = currCellState.g;

    // if current mesh instance is INFECTED, apply some effect
    if((st & INFECTED) > EMPTY) {
        color = vec4(1,0,0,.5);
    }
    else { // else visualize normals
        color = vec4(vNormal, 1) * healthNormalized;

        //TODO do phong lighting correctly
        //vec3 col = material.ambient + material.diffuse * NdotL + material.specular * R;
        //color = vec4(col,1) * healthNormalized;

    }
}