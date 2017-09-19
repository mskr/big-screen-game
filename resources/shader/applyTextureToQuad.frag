#version 430

in vec2 pixel;
uniform sampler2D texture;

const float near = 0.1;
const float far = 100.0;

float lin(float depth) {
	float z = depth * 2.0 - 1.0; // Back to NDC 
	return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
    float depth = texture(texture, pixel).r;
	gl_FragColor = vec4(depth);
}