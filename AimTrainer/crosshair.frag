/*
crosshair.frag: Fragment shader for the crosshair
The color of the crosshair is set to white 
*/

#version 410 core

// output shader variable
out vec4 colorFrag;


// main
void main(void)
{
    // we set the crosshair color to white
    colorFrag = vec4( 1.0f, 1.0f, 1.0f, 1.0);
}
