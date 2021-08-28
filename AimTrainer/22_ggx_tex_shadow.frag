
#version 410 core

const float PI = 3.14159265359;

// output shader variable
out vec4 colorFrag;

// light incidence directions (calculated in vertex shader, interpolated by rasterization)
in vec3 lightDir;
// the transformed normal has been calculated per-vertex in the vertex shader
in vec3 vNormal;
// vector from fragment to camera (in view coordinate)
in vec3 vViewPosition;
// position of fragments in view coord
in vec4 mvPosition;

//TBN matrix used for normal maps
in mat3 TBN;

// interpolated texture coordinates
in vec2 interp_UV;

// for the correct rendering of the shadows, we need to calculate the vertex coordinates also in "light coordinates" (= using light as a camera)
in vec4 posLightSpace;

//point light direction 
in vec3 pointLightDir;

//point light color
uniform vec3 pointLightColor;

// texture repetitions
uniform float repeat;

// texture sampler
uniform sampler2D tex;
// texture sampler for the depth map
uniform sampler2D shadowMap;
// normal map sampler
uniform sampler2D normalMap;
// height map sampler
uniform sampler2D depthMap;

uniform float alpha; // rugosity - 0 : smooth, 1: rough
uniform float F0; // fresnel reflectance at normal incidence
uniform float Kd; // weight of diffuse reflection

// the execution time of the application is passed to the shader using an uniform variable
uniform float timer;

//activates the normal maps
uniform bool normalMapping;

//activates the procedural texture
uniform bool procedural;

//value needed to activet target specific behaviour
uniform bool isTarget;

////////////////////////////////////////////////////////////////////

// the "type" of the Subroutine
subroutine float shadow_map();

// Subroutine Uniform (it is conceptually similar to a C pointer function)
subroutine uniform shadow_map Shadow_Calculation;

////////////////////////////////////////////////////////////////////

//////////////////////////////////////////
// it applies Percentage-Closer Filtering to smooth the shadow edged. Moreover, the rendering of the areas behind the far plane of the light frustum is corrected
subroutine(shadow_map)
float Shadow_PCF_Final() // this name is the one which is detected by the SetupShaders() function in the main application, and the one used to swap subroutines
{
    // given the fragment position in light coordinates, we apply the perspective divide. Usually, perspective divide is applied in an automatic way to the coordinates saved in the gl_Position variable. In this case, the vertex position in light coordinates has been saved in a separate variable, so we need to do it manually
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    // after the perspective divide the values are in the range [-1,1]: we must convert them in [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // we get the depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // we calculate an adaptive bias to apply to the currentDepth value, to avoid the shadow acne effect.
    // the bias value is in the range [0.005,0.05]: the final value is calculated considering the angle between the normal and the direction of light
    vec3 normal = normalize(vNormal);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // Version 3: we apply Percentage Close Filtering (PCF) to smooth shadow edges
    float shadow = 0.0;
    // we determine the texel dimension
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    // we sample the depth map considering the 3x3 neighbourhood of the current fragment, and we apply the same test of Version 2 to each sample
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            // we sample the depth map
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            // if the depth (with bias) of the current fragment is greater than the depth in the shadow map, then the fragment is in shadow. We add the result to the shadow variable
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    // we average the shadow result on the kernel size of the PCF
    shadow /= 9.0;

    // To avoid that the areas behind the far plane of the light frustum are considered in shadow, we set their shadow value to 0 (= in light)
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

//////////////////////////////////////////
// Schlick-GGX method for geometry obstruction (used by GGX model)
float G1(float angle, float alpha)
{
    // in case of Image Based Lighting, the k factor is different:
    // usually it is set as k=(alpha*alpha)/2
    float r = (alpha + 1.0);
    float k = (r*r) / 8.0;

    float num   = angle;
    float denom = angle * (1.0 - k) + k;

    return num / denom;
}

//function used to create displacement using parallax mapping
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    float height =  texture(depthMap, texCoords).r;    
    vec2 p = viewDir.xy / viewDir.z * (height * 10);//height_scale);
    return texCoords - p;    
} 

vec3 GGX(vec4 surfaceColor, vec3 Ntexture) // this name is the one which is detected by the SetupShaders() function in the main application, and the one used to swap subroutines
{
    // normalization of the per-fragment normal
    vec3 N = Ntexture;
    // normalization of the per-fragment light incidence direction
    vec3 L = normalize(pointLightDir.xyz);

    // cosine angle between direction of light and normal
    float NdotL = max(dot(N, L), 0.0);

    // diffusive (Lambert) reflection component
    vec3 lambert = (Kd*surfaceColor.rgb)/PI;

    // we initialize the specular component
    vec3 specular = vec3(0.0);

    // if the cosine of the angle between direction of light and normal is positive, then I can calculate the specular component
    if(NdotL > 0.0)
    {
        // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
        vec3 V = normalize( vViewPosition );

        // half vector
        vec3 H = normalize(L + V);

        // we implement the components seen in the slides for a PBR BRDF
        // we calculate the cosines and parameters to be used in the different components
        float NdotH = max(dot(N, H), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float alpha_Squared = alpha * alpha;
        float NdotH_Squared = NdotH * NdotH;

        // Geometric factor G2 
        // Smith’s method (uses Schlick-GGX method for both geometry obstruction and shadowing )
        float G2 = G1(NdotV, alpha)*G1(NdotL, alpha);

        // Rugosity D
        // GGX Distribution
        float D = alpha_Squared;
        float denom = (NdotH_Squared*(alpha_Squared-1.0)+1.0);
        D /= PI*denom*denom;

        // Fresnel reflectance F (approx Schlick)
        vec3 F = vec3(pow(1.0 - VdotH, 5.0));
        F *= (1.0 - F0);
        F += F0;

        // we put everything together for the specular component
        specular = (F * G2 * D) / (4.0 * NdotV * NdotL);
    }

    // the rendering equation is:
    // integral of: BRDF * Li * (cosine angle between N and L)
    // BRDF in our case is: the sum of Lambert and GGX
    // Li is considered as equal to 1: light is white, and we have not applied attenuation. With colored lights, and with attenuation, the code must be modified and the Li factor must be multiplied to finalColor
    return (lambert + specular)*NdotL;
}



/*












*/
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                        -0.577350269189626,  // -1.0 + 2.0 * C.x
                        0.024390243902439); // 1.0 / 41.0
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod289(i); // Avoid truncation effects in permutation
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
        + i.x + vec3(0.0, i1.x, 1.0 ));

    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

vec4 noise() {
    vec2 u_resolution = vec2(0.2,0.2);
    vec2 st = interp_UV*2/u_resolution.xy;
    st.x *= u_resolution.x/u_resolution.y;
    vec3 color = vec3(0.0);
    vec2 pos = vec2(st*3.);
    vec4 result;

    float DF = 0.0;

    // Add a random position
    float a = 0.0;
    vec2 vel = vec2(timer*.1);
    DF += snoise(pos+vel)*.25+.25;

    // Add a random position
    a = snoise(pos*vec2(cos(timer*0.15),sin(timer*0.1))*0.1)*3.1415;
    vel = vec2(cos(a),sin(a));
    DF += snoise(pos+vel)*.25+.25;

    color = vec3(smoothstep(.7,.75,fract(DF)))*vec3(256/256f,110/256f,15/256f);
    if (smoothstep(.7,.75,fract(DF))==1.0){
        result = vec4(color,1.0);
    }else{
        result = vec4(1.0-color,1.0)*vec4(69/256f,96/256f,165/256f,1.0f);
    }
    return result;
}

/*









*/

///////////// MAIN ////////////////////////////////////////////////
void main()
{   
    vec4 surfaceColor;
    vec3 N;
    vec2 texCoords;
    // we repeat the UVs and we sample the textures
    vec2 repeated_Uv = mod(interp_UV*repeat, 1.0);

    if(procedural){
        surfaceColor = noise();
    }
    else{
        surfaceColor = texture(tex, repeated_Uv); 
    }

    if(normalMapping && !procedural){
        N = texture(normalMap, repeated_Uv).rgb;
        N = N * 2.0 - 1.0;
        N = normalize(TBN * N);
    }else{
        // normalization of the per-fragment normal
        N = normalize(vNormal);
    }
    

    /*vec3 N;
    if(normalMapping){
        N = texture(normalMap, repeated_Uv).rgb;
        N = N * 2.0 - 1.0;
        N = normalize(TBN * N);
    }
    else{
        // normalization of the per-fragment normal
        N = normalize(vNormal);
    }
    */

    // normalization of the per-fragment light incidence direction
    vec3 L = normalize(lightDir.xyz);

    // cosine angle between direction of light and normal
    float NdotL = max(dot(N, L), 0.0);

    // diffusive (Lambert) reflection component
    vec3 lambert = (Kd*surfaceColor.rgb)/PI;

    // we initialize the specular component
    vec3 specular = vec3(0.0);

    // initialization of shadow value
    float shadow = 0.0;

    // if the cosine of the angle between direction of light and normal is positive, then I can calculate the specular component
    if(NdotL > 0.0)
    {
        // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
        vec3 V = normalize( vViewPosition );

        // half vector
        vec3 H = normalize(L + V);

        // we implement the components seen in the slides for a PBR BRDF
        // we calculate the cosines and parameters to be used in the different components
        float NdotH = max(dot(N, H), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float alpha_Squared = alpha * alpha;
        float NdotH_Squared = NdotH * NdotH;

        // Geometric factor G2
        float G2 = G1(NdotV, alpha)*G1(NdotL, alpha);

        // Rugosity D
        // GGX Distribution
        float D = alpha_Squared;
        float denom = (NdotH_Squared*(alpha_Squared-1.0)+1.0);
        D /= PI*denom*denom;

        // Fresnel reflectance F (approx Schlick)
        vec3 F = vec3(pow(1.0 - VdotH, 5.0));
        F *= (1.0 - F0);
        F += F0;

        // we put everything together for the specular component
        specular = (F * G2 * D) / (4.0 * NdotV * NdotL);

        // we calculate the shadow value for the fragment
        shadow = Shadow_Calculation();
    }

     // the rendering equation is:
    //integral of: BRDF * Li * (cosine angle between N and L)
    // BRDF in our case is: the sum of Lambert and GGX
    // Li is considered as equal to 1: light is white, and we have not applied attenuation. With colored lights, and with attenuation, the code must be modified and the Li factor must be multiplied to finalColor
    //We weight using the shadow value
    // N.B. ) shadow value = 1 -> fragment is in shadow
       //        shadow value = 0 -> fragment is in light
    // Therefore, we use (1-shadow) as weight to apply to the illumination model
    vec3 finalColor = (1.0 - (shadow*0.7))*(lambert + specular)*NdotL;


    if(NdotL==0.0){
        finalColor = 0.1*(lambert);
    }

    

    if(isTarget){
        finalColor = (lambert + specular)*NdotL;
        //finalColor += vec3(0.1f,0.1f,0.3f);
    }

    finalColor+=(GGX(surfaceColor,N)*0.8)*pointLightColor;
    colorFrag = vec4(finalColor, 1.0);
}
