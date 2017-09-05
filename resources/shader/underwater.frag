#version 330 core


// Speed of water shader
#define speed 10

// the amount of shearing (shifting of a single column or row)
// 1.0 = entire screen height offset (to both sides, meaning it's 2.0 in total)
#define xDistMag 0.05
#define yDistMag 0.05

// cycle multiplier for a given screen height
// 2*PI = you see a complete sine wave from top..bottom
#define xSineCycles 6.28 /10
#define ySineCycles 6.28 /10

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
#define NR_OUT_INF_LIGHTS 5  
uniform PointLight outerInfLights[NR_OUT_INF_LIGHTS];
#define MAX_NR_SOURCE_LIGHTS 10  
uniform PointLight sourceLights[MAX_NR_SOURCE_LIGHTS];
uniform int numSourceLights;
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


uniform sampler2D shadowMap;
uniform float time;

uniform int isDebugMode;

uniform sampler2D causticTex;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;
in vec4 vPosLightSpace;

out vec4 color;

const float DEPTH_BIAS = 0.00001;

float visibility(vec3 thisFragment) {
	float v = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; x++)
		for(int y = -1; y <= 1; y++)
			if(texture(shadowMap, thisFragment.xy + vec2(x,y) * texelSize).r > thisFragment.z) v += 1.0;
	return v/9.0;
}

void main() {
	if(isDebugMode == 1) {
		color = vec4(1);
		return;
	}
	vec3 thisFragment = vPosLightSpace.xyz / vPosLightSpace.w * 0.5 + 0.5;



	// underwater effect

	vec2 waterTexCoords = vTexCoords;
	waterTexCoords /= 2;
	
    // the value for the sine has 2 inputs:
    // 1. the time, so that it animates.
    // 2. the y-row, so that ALL scanlines do not distort equally.
	float localtime = speed * time;
    float xAngle = time + waterTexCoords.y * ySineCycles;
    float yAngle = time + waterTexCoords.x * xSineCycles;

    vec2 distortOffset = 
        vec2(sin(xAngle), sin(yAngle)) * // amount of shearing
        vec2(xDistMag,yDistMag); // magnitude adjustment

	waterTexCoords += distortOffset; 	


	//color = texture(materal.diffuseTex, waterTexCoords);
	//color = texture(material.diffuseTex, vTexCoords);


	vec3 norm = normalize(vNormal);
	vec3 viewDir = normalize(viewPos-vPosition);
	vec3 result = CalcDirLight(dirLight, norm, viewDir);
	for(int i = 0; i < NR_OUT_INF_LIGHTS; i++){
		result += CalcPointLight(outerInfLights[i], norm, vPosition, viewDir);
	}
	for(int i = 0; i < numSourceLights; i++){
		result += CalcPointLight(sourceLights[i], norm, vPosition, viewDir); 
	}
    color = vec4(result,max(material.alpha,0.9f));

    // blue fog for the underwater effect
    color += pow(distance(vPosition,viewPos)*0.8f,4) * 0.001f * vec4(0.0f,0.15f,0.25f,0.0f);

    float fac1 = 10.0f;
    float fac2 = 0.5f;
    float causticx = fac2 * sin(time + vPosition.x * fac1) - 0.4;
    float causticy = fac2 * sin(time + vPosition.y * fac1) - 0.4;
    
    float caustic = max(0,causticx+causticy);
    //color += caustic * vec4(0.7f);
    color = texture2D(causticTex,vTexCoords);

    float tmpcol = max(1.0f,pow(distance(vPosition,viewPos)*0.8f,4) * 0.01f);
    //color = (color / tmpcol) + vec4(0.0f,0.2f,0.3f,0.0f) * tmpcol * 0.07f + vec4(0.0f,0.1f,0.25f,0.0f);// * vec4(0.0f,0.15f,0.25f,0.0f);


	// color.rgb = vec3(0.0,0.1,0.3) + color.rgb * vec3(0.5,0.6,0.1);
	// color *= visibility(thisFragment);
//	if(texture(shadowMap, thisFragment.xy).r < (thisFragment.z - DEPTH_BIAS))
//		color *= 0.5;

}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir){
	vec3 lightDir = normalize(-light.direction);
	//diffuse
	float diff = max(dot(normal, lightDir),0.0);
	//specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.specularExponent);
	//combine
	vec3 ambient = light.ambient * material.diffuse;
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
	vec3 ambient = light.ambient * material.diffuse;
	vec3 diffuse = light.diffuse * diff * material.diffuse;
	vec3 specular = light.specular * spec * material.specular;
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 