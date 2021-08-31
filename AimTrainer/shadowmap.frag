/*
shadowmap.frag: fragment shader for the creation of the shadow map
It does nothing: the shadow rendering step writes in a dedicated FBO the depth information of each fragment from the point of view of the light, and it does not calculate the color information
*/

#version 410 core

void main()
{}
