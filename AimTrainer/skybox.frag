/*
skybox.frag: fragment shader for the visualization of the cube map as environment map
*/

#version 410 core

// output shader variable
out vec4 colorFrag;

// interpolated texture coordinates
in vec3 interp_UVW;

// texture sampler for the cube map
uniform samplerCube tCube;

void main()
{
	// we sample the cube map
    colorFrag = texture(tCube, interp_UVW);
}
