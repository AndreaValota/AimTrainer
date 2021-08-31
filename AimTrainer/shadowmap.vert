/*
shadowmap.vert: vertex shader for the creation of the shadow map
It applies to the vertex the projection and view transformations of the light (used as a camera)
*/

#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// projection and view matrix of the light
uniform mat4 lightSpaceMatrix;

// model matrix
uniform mat4 modelMatrix;

void main()
{
	// we apply the transformation to express the vertex from the point of view of the light
    gl_Position = lightSpaceMatrix * modelMatrix * vec4(position, 1.0f);
}
