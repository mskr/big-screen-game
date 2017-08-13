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
#define TEMPORARY 2048U

/* Max health (should match GridCell::MAX_HEALTH) */
#define MAX_HEALTH 100U

/* Build state and health for the whole grid (bilinear interpolation enabled) */
uniform sampler2D curr_grid_state;
uniform sampler2D last_grid_state;

/* Coordinates of this fragment on the grid (in texture space) */
in vec2 cellCoords;

/* Determining "infectedness" of a fragment
by spatial and temporal interpolation of the infected state number
and then normalizing
(the 32 bit uint in the texture are read as unsigned normalized (unorm),
i.e. 2^32 == 1.0 ===> 2^-23 == 2^9 == 512U == INFECTED) */
const float INFECTEDNESS_NORMALIZATION_FACTOR = pow(2.0, -23);

uniform float automatonTimeDelta;

/*Directional Lights Parts*/
uniform vec3 viewPos;

struct DirLight{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform DirLight dirLight;
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

/*Influence and Source Lights*/
struct PointLight{
    vec3 position;  
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight outerInfLight;
#define NR_SOURCE_LIGHTS 4  
uniform PointLight sourceLights[NR_SOURCE_LIGHTS];
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir); 
/*Material attributes*/
struct Material {
    float alpha;
    vec3 ambient;
    float bumpMultiplier;
    sampler2D bumpTex;
    vec3 diffuse;
    sampler2D diffuseTex;
    float refraction;
    vec3 specular;
    float specularExponent;
};

uniform Material material;

/*alternative Shader modes */
uniform int isDepthPass;
uniform int isDebugMode;

/*Output*/
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
    if((st & INFECTED) > 0U) {
        float infectednessInterpolatedSpatialTemporal = mix(
            lastCellBuildStateInterpolatedSpatial,
            currCellBuildStateInterpolatedSpatial,
            automatonTimeDelta);
        float infectedness = infectednessInterpolatedSpatialTemporal 
            / INFECTEDNESS_NORMALIZATION_FACTOR;
        if(infectedness < 0.6) discard;
        color = vec4(1,1,1, infectedness);
    }
    else {
        if((st & TEMPORARY) > 0U){
            float tmpAlpha = 0.5f;
            if((st & INVALID) > 0U){
                color = vec4(1,0,0,tmpAlpha);
            }else{
                color = vec4(material.diffuse, tmpAlpha);
            }
        }else{
			vec3 norm = normalize(vNormal);
			vec3 viewDir = normalize(viewPos-vPosition);
			vec3 result = CalcDirLight(dirLight, norm, viewDir);
			result += CalcPointLight(outerInfLight, norm, vPosition, viewDir);
			for(int i = 0; i < NR_SOURCE_LIGHTS; i++){
		        result += CalcPointLight(sourceLights[i], norm, vPosition, viewDir); 
			}
            color = vec4(result,max(material.alpha,0.9f));            
        }
    }
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir){
	vec3 lightDir = normalize(-light.direction);
	//diffuse
	float diff = max(dot(normal, lightDir),0.0);
	//specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.specularExponent);
	//combine
	vec3 ambient = light.ambient * material.ambient;
	vec3 diffuse = light.diffuse * diff * material.diffuse;
	vec3 specular = light.specular * spec * material.specular;
	return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse 
    float diff = max(dot(normal, lightDir), 0.0);
    // specular 
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.specularExponent);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
	//combine
	vec3 ambient = light.ambient * material.ambient;
	vec3 diffuse = light.diffuse * diff * material.diffuse;
	vec3 specular = light.specular * spec * material.specular;
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 