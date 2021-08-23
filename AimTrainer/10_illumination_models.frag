/*

10_illumination_models.frag: Fragment shader for the Lambert, Phong, Blinn-Phong and GGX illumination models

N.B. 1)  "09_illumination_models.vert" must be used as vertex shader

N.B. 2)  the different illumination models are implemented using Shaders Subroutines

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 410 core

// output shader variable
out vec4 colorFrag;


// main
void main(void)
{
    // we call the pointer function Illumination_Model():
    // the subroutine selected in the main application will be called and executed 
  
    colorFrag = vec4(1.0f,1.0f,1.0f, 1.0);
}
