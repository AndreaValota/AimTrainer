/*
skybox.vert: vertex shader for the visualization of the cube map as environment map
*/

#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// texture coordinates for the environment map sampling (we use 3 coordinates because we are sampling in 3 dimensions)
out vec3 interp_UVW;

// view matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

void main()
{
		// but we use the vertex position as 3D texture coordinates, in order to have a 1:1 mapping from the cube map and the cube used as "the world"
		interp_UVW = position;

		// we apply the transformations to the vertex
    vec4 pos = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
		// we want to set the Z coordinate of the projected vertex at the maximum depth so we set Z equal to W (because in the projection divide, after clipping, all the components will be divided by W).
		gl_Position = pos.xyww;
}
