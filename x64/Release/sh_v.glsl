#version 330

layout(location = 0) in vec2 pos;		// Position
layout(location = 1) in vec2 tc;		// Texture coordinates

smooth out vec2 fragTC;		// Interpolated texture coordinate

uniform mat4 xform;			// Transformation matrix

void main() {
	// Transform vertex position
	gl_Position = xform * vec4(pos, 0.0, 1.0);

	// Interpolate texture coordinates
	fragTC = tc;
}
