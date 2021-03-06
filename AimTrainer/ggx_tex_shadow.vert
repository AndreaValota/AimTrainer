/*
ggx_tex_shadow.vert: vertex shader for GGX illumination model, with shadow rendering using shadow map
The shader considers only a directional light for the creation of the shadow map. It also considers the presence of a single point light.
*/

#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// vertex normal in world coordinate
layout (location = 1) in vec3 normal;
// UV coordinates
layout (location = 2) in vec2 UV;
// tangent vector
layout (location = 3) in vec3 tangent;
// bitangent vector
layout (location = 4) in vec3 bitangent;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat3 normalMatrix;

// transformation (projection and view) matrix for the light
uniform mat4 lightSpaceMatrix;

// direction of incoming light is passed as an uniform
uniform vec3 lightVector;

//position of pointlight
uniform vec3 pointLightPosition;

// direction of incoming light in view coordinates
out vec3 lightDir;

//light incidence direction in view coordinates
out vec3 pointLightDir;

// normals in view coordinates
out vec3 vNormal;

//TBN matrix used for normal maps
out mat3 TBN;

// in the fragment shader, we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
out vec3 vViewPosition;

// position needed in the fragment shader to compute the displacements for parallax mapping
out vec4 mvPosition;

// the output variable for UV coordinates
out vec2 interp_UV;

// for the correct rendering of the shadows, we need to calculate the vertex coordinates also in "light coordinates" (= using light as a camera)
out vec4 posLightSpace;


void main(){

  // vertex position in world coordinates
  vec4 mPosition = modelMatrix * vec4( position, 1.0 );
  // vertex position in camera coordinates
  vec4 mvPosition = viewMatrix * mPosition;

  // view direction, negated to have vector from the vertex to the camera
  vViewPosition = -mvPosition.xyz;

  // transformations are applied to the normal
  vNormal = normalize( normalMatrix * normal );

  //computing the TBN matrix
  vec3 T = normalize( normalMatrix * tangent);
  vec3 B = normalize( normalMatrix * bitangent);
  vec3 N = normalize( normalMatrix * normal);
  TBN = mat3(T,B,N);

  // light incidence directions in view coordinate
  lightDir = vec3(viewMatrix  * vec4(lightVector, 0.0));

  //light incidence of point light
  vec4 lightPos = viewMatrix * vec4(pointLightPosition, 1.0);
  pointLightDir = lightPos.xyz - mvPosition.xyz;
  
  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;

  // I assign the values to a variable with "out" qualifier so to use the per-fragment interpolated values in the Fragment shader
  interp_UV = UV;

  // vertex position in "light coordinates"
  posLightSpace = lightSpaceMatrix * mPosition;

}
