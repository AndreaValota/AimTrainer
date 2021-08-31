/*
crosshair.vert: Vertex shader for crosshair
we position the crosshair based on the matrices given as uniforms
*/

#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;


void main(){
  // we apply the Model-View-Projection transformation
  gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4( position, 1.0 );
}
