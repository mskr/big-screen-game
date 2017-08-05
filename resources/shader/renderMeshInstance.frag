#version 330 core

/* Interpolated vertex attributes */
in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;

/* Build state and health of this grid cell (not interpolated) */
flat in uint st;
flat in uint hp;

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

/* Normalization factor for infected state of a fragment
after spatial and temporal interpolation */
const float INFECTEDNESS_NORMALIZATION_FACTOR = INFECTED;

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
    
    // Normalized health can be a nice weight for colors/ some damage overlay
    float healthNormalized = float(hp) / float(MAX_HEALTH);

    // Lookup cell state of this fragment with bilinear interpolation enabled
    vec2 lastCellStateInterpolatedSpatial = texture(last_grid_state, cellCoords).rg;
    vec2 currCellStateInterpolatedSpatial = texture(curr_grid_state, cellCoords).rg;
    float lastCellBuildStateInterpolatedSpatial = lastCellStateInterpolatedSpatial.r;
    float lastCellHealthInterpolatedSpatial = lastCellStateInterpolatedSpatial.g;
    float currCellBuildStateInterpolatedSpatial = currCellStateInterpolatedSpatial.r;
    float currCellHealthInterpolatedSpatial = currCellStateInterpolatedSpatial.g;

    // When a mesh instance is rendered on a cell, it can be:
    // 1) a room segment (st has WALL, CORNER or INSIDE_ROOM bit set)
    // 2) an infected room segment (st has INFECTED bit set)
    // 3) an infection source, most likely in a wall (st has SOURCE bit set)
    // 4) an invalid room, that is too small or too big (st has INVALID bit set)
    // ...
    if(st == INFECTED) {
        float infectednessInterpolatedSpatialTemporal = mix(
            lastCellBuildStateInterpolatedSpatial,
            currCellBuildStateInterpolatedSpatial,
            automatonTimeDelta);
        float infectedness = infectednessInterpolatedSpatialTemporal / INFECTEDNESS_NORMALIZATION_FACTOR;
        color = vec4(1,0,0, 0.2+infectedness);
    }
    else {
        //color = vec4(vNormal, material.alpha) * healthNormalized;
        color = vec4(material.diffuse,material.alpha);
        //TODO do phong lighting correctly
        //vec3 col = material.ambient + material.diffuse * NdotL + material.specular * R;
        //color = vec4(col,1) * healthNormalized;

    }
}