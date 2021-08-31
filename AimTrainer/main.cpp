// Std. Includes
#include <string>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

#include <cstdlib>

#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

// classes developed during lab lectures to manage shaders, to load models, for FPS camera, and for physical simulation
#include <utils/shader_v1.h>
#include <utils/model_v1.h>
#include <utils/camera.h>
#include <utils/physics_v1.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// Dimensioni della finestra dell'applicazione
GLuint screenWidth = 1920, screenHeight = 1080;

// the rendering steps used in the application
enum render_passes{ SHADOWMAP, RENDER};

// callback functions for keyboard, mouse and camera events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void apply_camera_movements();

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;

// a vector for all the shader subroutines names used and swapped in the application
vector<std::string> shaders;
// the name of the subroutines are searched in the shaders, and placed in the shaders vector (to allow shaders swapping)
void SetupShader(int shader_program);

// function to be called in each rendering step, we pass the models and the shaders 
void RenderObjects(Shader &shader, Model &cubeModel, Model &sphereModel, GLint render_pass, GLuint depthMap);

// activate texture ad index i (each texture needs color and normal map at index i+1)
void ActivateTexture(GLint index, GLfloat repeat, GLint textureLocation, GLint nMapLocation, GLint repeatLocation, GLint proceduralLocation);
// load image from disk and create an OpenGL texture
GLint LoadTexture(const char* path);
// load 6 images for the cubemap
GLint LoadTextureCube(string path);

// function used to reset rigid bodies and parametes during room swap
void reset(GLint next_room_index);

// 2D text renderer function
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color);

// score management functions
void increaseScore();
void resetScore();


//////////// VARIABLES INITIALIZATION /////////////

// texture uint for different cubemaps
GLuint textureCube_room1;
GLuint textureCube_room2;
GLuint textureCube_room3;
// offset used to load multiple environment maps
GLint loadedCubes=0;

// vector for the textures IDs
vector<GLint> textureID;

// we initialize an array of booleans for each keybord key
bool keys[1024];

// number of walls (useful to avoid certain controls on collisions)
GLint walls_number = 5;
// number of buttons (useful to avoid certain controls on collisions)
GLint buttons_number = 5;

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;
// we will use these value to "pass" the cursor position to the keyboard callback, in order to determine the bullet trajectory
double cursorX,cursorY;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

// view matrices (global because we need to use them in the keyboard callback)
glm::mat4 view; 

// we create a camera. We pass the initial position as a parameter to the constructor.
Camera camera(glm::vec3(0.0f, 0.0f, 9.0f), GL_TRUE);

// Directional light
glm::vec3 lightDir0 = glm::vec3(1.0f, 1.0f, 1.0f);

// Point light
glm::vec3 pointLightPosition = glm::vec3(0.0f, -100.0f, 0.0f);
glm::vec3 pointLightColor = glm::vec3(0.0, 0.0, 0.0);
GLint pointLightLocation;
GLint pointLightColorLocation;

// weight for the diffusive component
GLfloat Kd = 3.0f;
// roughness index for GGX shader
GLfloat alpha = 0.4f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// dimension of the targets (global because we need it also in the keyboard callback)
glm::vec3 sphere_size = glm::vec3(0.2f, 0.2f, 0.2f);

// repeat needed as a texture parameter
GLfloat repeat = 1.0f;

// active target array
GLboolean active_targets [49] = {false};
GLboolean active = true;

// boolean to check if the taget is hit
GLboolean hit=true;

// parametest used to generate the targets rigid bodies (global because they are needed in various functions)
GLint num_side = 7;
// total number of the targets
GLint total_targets = num_side*num_side;
// position of the target
glm::vec3 target_pos;
// dimension of the target
glm::vec3 target_size = glm::vec3(0.13f,0.2f,0.2f);
// target rigidbody
btRigidBody* target;

//Selected room index
enum room{FIRST, SECOND, THIRD};
GLint active_room = FIRST;

// score variable
GLint score = 0;

// Model and Normal transformation matrices for the objects in the scene: we set to identity
glm::mat4 objModelMatrix = glm::mat4(1.0f);
glm::mat3 objNormalMatrix = glm::mat3(1.0f);
glm::mat4 planeModelMatrix = glm::mat4(1.0f);
glm::mat3 planeNormalMatrix = glm::mat3(1.0f);
glm::mat4 rwallModelMatrix = glm::mat4(1.0f);
glm::mat3 rwallNormalMatrix = glm::mat3(1.0f);
glm::mat4 lwallModelMatrix = glm::mat4(1.0f);
glm::mat3 lwallNormalMatrix = glm::mat3(1.0f);
glm::mat4 bwallModelMatrix = glm::mat4(1.0f);
glm::mat3 bwallNormalMatrix = glm::mat3(1.0f);

glm::mat4 button_concrete_ModelMatrix = glm::mat4(1.0f);
glm::mat3 button_concrete_NormalMatrix = glm::mat3(1.0f);
glm::mat4 button_metallic_ModelMatrix = glm::mat4(1.0f);
glm::mat3 button_metallic_NormalMatrix = glm::mat3(1.0f);
glm::mat4 button_abstract_ModelMatrix = glm::mat4(1.0f);
glm::mat3 button_abstract_NormalMatrix = glm::mat3(1.0f);

glm::mat4 button_lower_sensitivity_ModelMatrix = glm::mat4(1.0f);
glm::mat3 button_lower_sensitivity_NormalMatrix = glm::mat3(1.0f);
glm::mat4 button_higher_sensitivity_ModelMatrix = glm::mat4(1.0f);
glm::mat3 button_higher_sensitivity_NormalMatrix = glm::mat3(1.0f);


// dimensions and position of the static plane, we will use the cube mesh to simulate the plane, because we need some "height" in the mesh
// in order to make it work with the physics simulation
glm::vec3 plane_pos = glm::vec3(0.0f, -1.1f, 0.0f);
glm::vec3 plane_size = glm::vec3(25.0f, 0.1f, 25.0f);
glm::vec3 plane_rot = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 rwall_pos = glm::vec3(6.2f, 3.0f, 0.0f);
glm::vec3 rwall_size = glm::vec3(4.0f, 2.0f, 4.0f);
glm::vec3 rwall_rot_axis = glm::vec3(0.0f, 0.0f, 1.0f);

glm::vec3 lwall_pos = glm::vec3(-6.2f, 3.0f, 0.0f);
glm::vec3 lwall_size = glm::vec3(4.0f, 2.0f, 4.0f);
glm::vec3 lwall_rot_axis = glm::vec3(0.0f, 0.0f, 1.0f);
    
glm::vec3 bwall_pos = glm::vec3(0.0f, 3.0f, -3.0f);
glm::vec3 bwall_size = glm::vec3(4.0f, 1.0f, 4.2f);
glm::vec3 bwall_rot_axis = glm::vec3(1.0f, 0.0f, 0.0f);

// invisible wall (usefull only in third room)
glm::vec3 fwall_pos = glm::vec3(0.0f, 3.0f, 5.0f);
glm::vec3 fwall_size = glm::vec3(4.0f, 1.0f, 4.2f);

// buttons 
glm::vec3 button_concrete_room_pos = glm::vec3(-7.4f, -0.5f, 4.5f);
glm::vec3 button_concrete_room_size = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 button_concrete_room_rot = glm::vec3(1.0f, 0.0f, 0.0f);

glm::vec3 button_metallic_room_pos = glm::vec3(-6.2f, -0.5f, 4.5f);
glm::vec3 button_metallic_room_size = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 button_metallic_room_rot = glm::vec3(1.0f, 0.0f, 0.0f);

glm::vec3 button_abstract_room_pos = glm::vec3(-5.0f,-0.5f, 4.5f);
glm::vec3 button_abstract_room_size = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 button_abstract_room_rot = glm::vec3(1.0f, 0.0f, 0.0f);

glm::vec3 button_lower_sensitivity_pos = glm::vec3(5.7f, 0.5f, 4.0f);
glm::vec3 button_higher_sensitivity_pos = glm::vec3(6.9f, 0.5f, 4.0f);
glm::vec3 button_sensitivity_size = glm::vec3(0.25f, 0.25f, 0.25f);

GLint objDiffuseLocation;

// instance of the physics class
Physics bulletSimulation;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
// VAO, VBO declaration for HUD
GLuint VAO, VBO;


////////////////// MAIN FUNCTION ///////////////////////
int main()
{
    // Initialization of OpenGL context using GLFW
    glfwInit();
    // We set OpenGL specifications required for this application
    // In this case: 4.1 Core
    // If not supported by your graphics HW, the context will not be created and the application will close
    // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
    // in relation also to the values indicated in these GLFW commands
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // we create the application's window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "AimTrainer", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window,mouse_button_callback);

    // we disable the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);
    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // seed for randomizer
    srand(time(0));

    // HUD enable function
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shader Programs creation
    // Shader Program for the creation of the shadow map
    Shader shadow_shader("shadowmap.vert", "shadowmap.frag");
    // Shader Program for the objects used in the application
    Shader object_shader("ggx_tex_shadow.vert", "ggx_tex_shadow.frag");
    // Shader Porgram for the crosshair
    Shader crosshair_shader("crosshair.vert","crosshair.frag");
    // Shader Program for the environment map
    Shader skybox_shader("skybox.vert", "skybox.frag");
    // Shader Program for the 2D text
    Shader text_shader("text.vert", "text.frag");
    glm::mat4 text_projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));
    text_shader.Use();
    glUniformMatrix4fv(glGetUniformLocation(text_shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(text_projection));
    glUniform1d(glGetUniformLocation(text_shader.Program, "text"), 23);

    // Setup object shader
    SetupShader(object_shader.Program);

    // we load the textures and store them in a vector
    // first room textures
    textureID.push_back(LoadTexture("../textures/Stylized_Stone/Stylized_Stone_Floor_002_basecolor.jpg"));
    textureID.push_back(LoadTexture("../textures/Stylized_Stone/Stylized_Stone_Floor_002_normal.jpg"));
    textureID.push_back(LoadTexture("../textures/Concrete_Wall/Concrete_Wall_008_basecolor.jpg"));
    textureID.push_back(LoadTexture("../textures/Concrete_Wall/Concrete_Wall_008_normal.jpg"));
    textureID.push_back(LoadTexture("../textures/Sapphire/Sapphire_001_COLOR.jpg")); 
    textureID.push_back(LoadTexture("../textures/Sapphire/Sapphire_001_NORM.jpg"));
    // second room textures
    textureID.push_back(LoadTexture("../textures/Metal_Plate_012/Metal_Plate_012_basecolor.jpg"));
    textureID.push_back(LoadTexture("../textures/Metal_Plate_012/Metal_Plate_012_normal.jpg"));
    textureID.push_back(LoadTexture("../textures/Metallic_Material/Metal_Mesh_006_basecolor.jpg"));
    textureID.push_back(LoadTexture("../textures/Metallic_Material/Metal_Mesh_006_normal.jpg"));
    textureID.push_back(LoadTexture("../textures/Sphere_Color/sphere_off.jpg")); 
    textureID.push_back(LoadTexture("../textures/Sphere_Color/sphere_on.jpg"));
    // third room textures
    textureID.push_back(LoadTexture("../textures/Rubber_Floor/Rubber_Floor_001_basecolor.jpg"));
    textureID.push_back(LoadTexture("../textures/Rubber_Floor/Rubber_Floor_001_normal.jpg"));
    textureID.push_back(LoadTexture("../textures/Rubber_Floor/Rubber_Floor_001_basecolor.jpg"));
    textureID.push_back(LoadTexture("../textures/Rubber_Floor/Rubber_Floor_001_normal.jpg"));
    textureID.push_back(LoadTexture("../textures/Basketball/basketball_basecolor.jpg")); 
    textureID.push_back(LoadTexture("../textures/Basketball/basketball_normal.jpg"));

    // we load the cube map (we pass the path to the folder containing the 6 views)
    textureCube_room1 = LoadTextureCube("../textures/cube/skybox/");
    textureCube_room2 = LoadTextureCube("../textures/cube/red/");
    textureCube_room3 = LoadTextureCube("../textures/cube/lightblue/");

    // we load the models
    Model cubeModel("../models/cube.obj");
    Model sphereModel("../models/sphere.obj");

    // we create a static rigid body for plane and walls
    glm::vec3 rwall_RBrot = glm::vec3(0.0f, 0.0f, glm::radians(90.0f));
    glm::vec3 lwall_RBrot = glm::vec3(0.0f, 0.0f, glm::radians(90.0f));
    glm::vec3 bwall_RBrot = glm::vec3(0.0f, glm::radians(90.0f), 0.0f);
    glm::vec3 fwall_RBrot = glm::vec3(0.0f, glm::radians(90.0f), 0.0f);
    btRigidBody* plane = bulletSimulation.createRigidBody(BOX,plane_pos,plane_size,plane_rot,0.0f,0.3f,1.0f);
    btRigidBody* rwall = bulletSimulation.createRigidBody(BOX,rwall_pos,rwall_size,rwall_RBrot,0.0f,0.3f,1.0f);
    btRigidBody* lwall = bulletSimulation.createRigidBody(BOX,lwall_pos,lwall_size,lwall_RBrot,0.0f,0.3f,1.0f);
    btRigidBody* bwall = bulletSimulation.createRigidBody(BOX,bwall_pos,bwall_size,bwall_RBrot,0.0f,0.3f,1.0f);
    btRigidBody* fwall = bulletSimulation.createRigidBody(BOX,fwall_pos,fwall_size,fwall_RBrot,0.0f,0.3f,1.0f);
    
    // rigid bodies for buttons
    glm::vec3 button_collision_size = glm::vec3(0.3f,0.3f,0.3f);
    btRigidBody* button_concrete_room = bulletSimulation.createRigidBody(SPHERE,button_concrete_room_pos,button_collision_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);
    btRigidBody* button_metallic_room = bulletSimulation.createRigidBody(SPHERE,button_metallic_room_pos,button_collision_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);
    btRigidBody* button_abstract_room = bulletSimulation.createRigidBody(SPHERE,button_abstract_room_pos,button_collision_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);
    btRigidBody* button_lower_sensitivity = bulletSimulation.createRigidBody(SPHERE,button_lower_sensitivity_pos,button_sensitivity_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);
    btRigidBody* button_higer_sensitivity = bulletSimulation.createRigidBody(SPHERE,button_higher_sensitivity_pos,button_sensitivity_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);

    //crosshair parameters
    glm::vec3 cross_pos;
    glm::vec3 cross_size = glm::vec3(0.005f, 0.0005f, 0.0f);
    glm::mat4 crossModelMatrix = glm::mat4(1.0f);
    glm::mat3 crossNormalMatrix = glm::mat3(1.0f);

    // we create 49 rigid bodies for the targets.
    num_side = 7;
    // total number of the targets
    total_targets = num_side * num_side;
    GLint i,j;
    // dimension of the target
    target_size = glm::vec3(0.13f,0.2f,0.2f);

    // targets rigidbodies
    for(i = 0; i < num_side; i++ )
    {
        for(j = 0; j < num_side; j++ )
        {
            // position of each target in the grid
            target_pos = glm::vec3((num_side - j*0.5f)-5.5f, (num_side - i*0.5f)-4.5f, 0.7f);
            target = bulletSimulation.createRigidBody(SPHERE,target_pos,target_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);
        }
    }

    // we set the maximum delta time for the update of the physical simulation
    GLfloat maxSecPerFrame = 1.0f / 60.0f;


    /////////////////// CREATION OF BUFFER FOR THE  DEPTH MAP /////////////////////////////////////////

    const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    GLuint depthMapFBO;
    // we create a Frame Buffer Object: the first rendering step will render to this buffer, and not to the real frame buffer
    glGenFramebuffers(1, &depthMapFBO);
    // we create a texture for the depth map
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    // in the texture, we will save only the depth data of the fragments. Thus, we specify that we need to render only depth in the first rendering step
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // we set to clamp the uv coordinates outside [0,1] to the color of the border
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // outside the area covered by the light frustum, everything is rendered in shadow (because we set GL_CLAMP_TO_BORDER)
    // thus, we set the texture border to white, so to render correctly everything not involved by the shadow map
    GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // we bind the depth map FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    // we set that we are not calculating nor saving color data
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

    //HUD initialization
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, "../fonts/arial.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, 48); 

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
  
    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
    
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture, 
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();
        // we apply FPS camera movements
        apply_camera_movements();
      
        /////////////////// STEP 1 - SHADOW MAP: RENDERING OF SCENE FROM LIGHT POINT OF VIEW ////////////////////////////////////////////////
        // we set view and projection matrix for the rendering using light as a camera
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        GLfloat near_plane = -10.0f, far_plane = 20.0f;
        GLfloat frustumSize = 15.0f;
        // we have a directional light, the projection is orthographic.
        lightProjection = glm::ortho(-frustumSize, frustumSize, -frustumSize, frustumSize, near_plane, far_plane);
        // the light is directional, so technically it has no position. We need a view matrix, so we consider a position on the the direction vector of the light
        lightView = glm::lookAt(lightDir0, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // transformation matrix for the light
        lightSpaceMatrix = lightProjection * lightView;
        // We "install" the  Shader Program for the shadow mapping creation
        shadow_shader.Use();
        // we pass the transformation matrix as uniform
        glUniformMatrix4fv(glGetUniformLocation(shadow_shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        // we set the viewport for the first rendering step = dimensions of the depth texture
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        // we activate the FBO for the depth map rendering
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
      
        // we render the scene, using the shadow shader
        RenderObjects(shadow_shader, cubeModel, sphereModel, SHADOWMAP, depthMap);

        /////////////////// STEP 2 - SCENE RENDERING FROM CAMERA ////////////////////////////////////////////////

        // View matrix (=camera): position, view direction, camera "up" vector
        view = camera.GetViewMatrix();

        // we activate back the standard Frame Buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // we set the viewport for the final rendering step
        glViewport(0, 0, width, height);

        // we update the physics simulation. We must pass the deltatime to be used for the update of the physical state of the scene.
        bulletSimulation.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame),10);

        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the selected Shader Program as part of the current rendering process
        object_shader.Use();
        // We search inside the Shader Program the name of a subroutine, and we get the numerical index
        GLuint index = glGetSubroutineIndex(object_shader.Program, GL_FRAGMENT_SHADER, shaders[current_subroutine].c_str());
        // we activate the subroutine using the index
        glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &index);

        // we pass projection, view and light space matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        // we determine the position in the Shader Program of the uniform variables
        pointLightLocation = glGetUniformLocation(object_shader.Program, "pointLightPosition");
        pointLightColorLocation = glGetUniformLocation(object_shader.Program, "pointLightColor");
        GLint lightDirLocation = glGetUniformLocation(object_shader.Program, "lightVector");
        GLint kdLocation = glGetUniformLocation(object_shader.Program, "Kd");
        GLint alphaLocation = glGetUniformLocation(object_shader.Program, "alpha");
        GLint f0Location = glGetUniformLocation(object_shader.Program, "F0");
        GLint timerLocation = glGetUniformLocation(object_shader.Program, "timer");

        // we assign the value to the uniform variable
        glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDir0));
        glUniform1f(kdLocation, Kd);
        glUniform1f(alphaLocation, alpha);
        glUniform1f(f0Location, F0);
        glUniform3fv(pointLightLocation, 1, glm::value_ptr(pointLightPosition));
        glUniform3fv(pointLightColorLocation, 1, glm::value_ptr(pointLightColor));
        glUniform1f(timerLocation, currentFrame);

        // we render the scene
        RenderObjects(object_shader, cubeModel, sphereModel, RENDER, depthMap);

        // Crosshair rendering
        crosshair_shader.Use();

        // we pass projection and view matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
        // horizontal part of the Crosshair 
        crossModelMatrix = glm::mat4(1.0f);
        crossNormalMatrix = glm::mat3(1.0f);
        crossModelMatrix = glm::translate(crossModelMatrix, (camera.Front*0.5f));
        crossModelMatrix = crossModelMatrix*glm::inverse(camera.GetViewMatrix());
        crossModelMatrix = glm::scale(crossModelMatrix, cross_size);
        crossNormalMatrix = glm::inverseTranspose(glm::mat3(view*crossModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(crossModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(crosshair_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(crossNormalMatrix));

        // we render the horizontal part of the crosshair
        cubeModel.Draw();
        crossModelMatrix = glm::mat4(1.0f);

        // vertical part of the Crosshair
        crossModelMatrix = glm::mat4(1.0f);
        crossNormalMatrix = glm::mat3(1.0f);
        crossModelMatrix = glm::translate(crossModelMatrix, (camera.Front*0.5f));
        crossModelMatrix = crossModelMatrix*glm::inverse(camera.GetViewMatrix());
        crossModelMatrix = glm::rotate(crossModelMatrix,glm::radians(90.0f),glm::vec3(0.0f,0.0f,1.0f));
        crossModelMatrix = glm::scale(crossModelMatrix, cross_size);
        crossNormalMatrix = glm::inverseTranspose(glm::mat3(view*crossModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(crossModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(crosshair_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(crossNormalMatrix));

        // we render the vertical part of the crosshair
        cubeModel.Draw();
        crossModelMatrix = glm::mat4(1.0f);
      
        /////////////////// SKYBOX ////////////////////////////////////////////////
        // we use the cube to attach the 6 textures of the environment map.
        // we render it after all the other objects, in order to avoid the depth tests as much as possible.
        glDepthFunc(GL_LEQUAL);
        // we activate the skybox shader
        skybox_shader.Use();
        // we activate the cube map corresponding to the current active room
        glActiveTexture(GL_TEXTURE28+active_room); //they are stored subsequently 
        switch (active_room)
        {
        case FIRST:
            glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube_room1);
            break;
        case SECOND:
            glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube_room2);
            break;
        case THIRD:
            glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube_room3);
            break;
        }
        // we pass projection and view matrices to the Shader Program of the skybox
        glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        // to have the background fixed during camera movements, we have to remove the translations from the view matrix
        // thus, we consider only the top-left submatrix, and we create a new 4x4 matrix
        view = glm::mat4(glm::mat3(view)); // Remove any translation component of the view matrix
        glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint textureLocation = glGetUniformLocation(skybox_shader.Program, "tCube");
        // we assign the value to the uniform variable
        glUniform1i(textureLocation, 28+active_room);

        glm::mat4 skyboxModelMatrix = glm::mat4(1.0f);
        skyboxModelMatrix = glm::rotate(skyboxModelMatrix,glm::radians(195.0f),glm::vec3(0.0f,1.0f,0.0f));
        glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(skyboxModelMatrix));

        // we render the cube with the environment map
        cubeModel.Draw();
        // we set again the depth test to the default operation for the next frame
        glDepthFunc(GL_LESS);

        //HUD Text Rendering 
        float dT = deltaTime;
        RenderText(text_shader, "FPS " + std::to_string((int)(1/dT)), 10.0f, 1045.0f, 0.6f, glm::vec3(0.0, 1.0f, 0.0f));
        RenderText(text_shader, "Score " + std::to_string(score), 900.0f, 1045.0f, 0.7f, glm::vec3(0.0, 1.0f, 0.0f));
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << camera.getCameraSensitivity();
        std::string sens = stream.str();
        RenderText(text_shader, "Sens " + sens, 10.0f, 1012.0f, 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));
        RenderText(text_shader, "ROOM " + std::to_string(active_room+1), 10.0f, 24.0f, 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));

        // we swap back and front buffer
        glfwSwapBuffers(window);
    } // end rendering loop

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    object_shader.Delete();
    shadow_shader.Delete();
    crosshair_shader.Delete();
    skybox_shader.Delete();
    text_shader.Delete();
    // we delete the data of the physical simulation
    bulletSimulation.Clear();
    // we close and delete the created context
    glfwTerminate();
    return 0;
} //end main


//////////////// FUNCTIONS DEFINITION /////////////////////

// Ray-Shpere Intersection function. Return true if the ray hits the sphere
bool hit_sphere(const glm::vec3& center, float radius, const glm::vec3& origin){
    glm::vec3 oc = origin - center;
    float a = dot(camera.Front, camera.Front);
    float b = 2.0 * dot(oc, camera.Front);
    float c = dot(oc,oc) - radius*radius;
    float discriminant = b*b - 4*a*c;
    return (discriminant>0);
}

// Camera movement function. If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements()
{
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;

    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

// Callback function for mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // we move the camera view following the mouse cursor
    // we calculate the offset of the mouse cursor from the position in the last frame
    // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)

    if(firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // we save the current cursor position in 2 global variables, in order to use the values in the keyboard callback function
    cursorX = xpos;
    cursorY = ypos;

    // offset of mouse cursor position
    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    // the new position will be the previous one for the next frame
    lastX = xpos;
    lastY = ypos;

    // we pass the offset to the Camera class instance in order to update the rendering
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Callback function for mouse buttons 
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){

        btVector3 temp(0.0f,0.0f,0.0f);
        btScalar radius;
        int num_cobjs = bulletSimulation.dynamicsWorld->getNumCollisionObjects();

        // we cycle through all collision objects to compute collision detection and collision response
        for(int i=walls_number; i<walls_number+buttons_number; i++){

            btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];
            btCollisionShape* shape = obj->getCollisionShape();
            // get target center and radius
            shape->getBoundingSphere(temp, radius); 
            glm::vec3 center (obj->getWorldTransform().getOrigin().getX(),obj->getWorldTransform().getOrigin().getY(),obj->getWorldTransform().getOrigin().getZ());

            // target collission detection and collision response
            if (((i-walls_number)<3) && hit_sphere(center, radius, camera.Position)){
                active_room = i-walls_number;
                reset(active_room);
                if(active_room == THIRD){
                    resetScore();
                    score--;
                }
            } 

            //collision detection and response for sens buttons
            if((i-walls_number)==3 && hit_sphere(center, radius, camera.Position)){
                camera.DecreaseCameraSensitivity();
            }
            if((i-walls_number)==4 && hit_sphere(center, radius, camera.Position)){
                camera.IncreaseCameraSensitivity();
            }
        }

        for(int i=walls_number+buttons_number; i<num_cobjs; i++){
            btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];
            btCollisionShape* shape = obj->getCollisionShape();
            shape->getBoundingSphere(temp, radius);
            glm::vec3 center (obj->getWorldTransform().getOrigin().getX(),obj->getWorldTransform().getOrigin().getY(),obj->getWorldTransform().getOrigin().getZ());
            if (hit_sphere(center, radius, camera.Position)){
                //cout<< "Colpita sfera " << i <<endl;
                hit=true;
                if(active_targets[i-(walls_number+buttons_number)]){ // if i hit the active target
                    active_targets[i-(walls_number+buttons_number)] = false;
                    active = true;
                    score++;
                }
            }
        }
    }
        
}

// Random float number generation function
float randomNumber(float Min, float Max)
{
    return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
}

// Render Objects function, used to implement different render passes
void RenderObjects(Shader &shader, Model &cubeModel, Model &sphereModel, GLint render_pass, GLuint depthMap ){
    
    // For the second rendering step -> we pass the shadow map to the shaders
    if (render_pass==RENDER)
    {
        glActiveTexture(GL_TEXTURE22);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        GLint shadowLocation = glGetUniformLocation(shader.Program, "shadowMap");
        glUniform1i(shadowLocation, 22);
    }
    // we pass the needed uniforms
    GLint textureLocation = glGetUniformLocation(shader.Program, "tex");
    GLint proceduralLocation = glGetUniformLocation(shader.Program, "procedural");
    GLint nMapLocation = glGetUniformLocation(shader.Program, "normalMap");
    GLint dMapLocation = glGetUniformLocation(shader.Program, "depthMap");
    GLint repeatLocation = glGetUniformLocation(shader.Program, "repeat");

    // STATIC PLANE
    // we activate the texture of the plane
    ActivateTexture(0+active_room*6,100.0,textureLocation,nMapLocation,repeatLocation,proceduralLocation);

    // The plane is static, so its Collision Shape is not subject to forces, and it does not move. Thus, we do not need to use dynamicsWorld to acquire the rototraslations, but we can just use directly glm to manage the matrices
    // we reset to identity at each frame
    planeModelMatrix = glm::mat4(1.0f);
    planeNormalMatrix = glm::mat3(1.0f);
    planeModelMatrix = glm::translate(planeModelMatrix, plane_pos);
    planeModelMatrix = glm::scale(planeModelMatrix, plane_size);
    planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));
    glUniform1i(glGetUniformLocation(shader.Program, "normalMapping"), GL_TRUE);

    // we render the plane
    cubeModel.Draw();
    planeModelMatrix = glm::mat4(1.0f);

    // we activate textures of the walls
    ActivateTexture(2+active_room*6,10.0f,textureLocation,nMapLocation,repeatLocation,proceduralLocation);

    // right wall
    rwallModelMatrix = glm::mat4(1.0f);
    rwallNormalMatrix = glm::mat3(1.0f);
    rwallModelMatrix = glm::translate(rwallModelMatrix, rwall_pos);
    rwallModelMatrix = glm::rotate(rwallModelMatrix,glm::radians(90.0f),rwall_rot_axis);
    rwallModelMatrix = glm::scale(rwallModelMatrix, rwall_size);
    rwallNormalMatrix = glm::inverseTranspose(glm::mat3(view*rwallModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(rwallModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(rwallNormalMatrix));

    // we render the right wall
    cubeModel.Draw();
    rwallModelMatrix = glm::mat4(1.0f);

    // left wall
    lwallModelMatrix = glm::mat4(1.0f);
    lwallNormalMatrix = glm::mat3(1.0f);
    lwallModelMatrix = glm::translate(lwallModelMatrix, lwall_pos);
    lwallModelMatrix = glm::rotate(lwallModelMatrix,glm::radians(-90.0f),lwall_rot_axis);
    lwallModelMatrix = glm::scale(lwallModelMatrix, lwall_size);
    lwallNormalMatrix = glm::inverseTranspose(glm::mat3(view*lwallModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(lwallModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(lwallNormalMatrix));

    // we render the left wall
    cubeModel.Draw();
    lwallModelMatrix = glm::mat4(1.0f);

    // back wall
    bwallModelMatrix = glm::mat4(1.0f);
    bwallNormalMatrix = glm::mat3(1.0f);
    bwallModelMatrix = glm::translate(bwallModelMatrix, bwall_pos);
    bwallModelMatrix = glm::rotate(bwallModelMatrix,glm::radians(90.0f),glm::vec3(0.0f,0.0f,1.0f));
    bwallModelMatrix = glm::rotate(bwallModelMatrix,glm::radians(90.0f),bwall_rot_axis);
    bwallModelMatrix = glm::scale(bwallModelMatrix, bwall_size);
    bwallNormalMatrix = glm::inverseTranspose(glm::mat3(view*bwallModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bwallModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bwallNormalMatrix));  

    // we render the back wall
    cubeModel.Draw();
    bwallModelMatrix = glm::mat4(1.0f);

    // we activate the texture for button to room1
    ActivateTexture(2,2.0f,textureLocation,nMapLocation,repeatLocation,proceduralLocation);

    // button to room1
    button_concrete_ModelMatrix = glm::mat4(1.0f);
    button_concrete_NormalMatrix = glm::mat3(1.0f);
    button_concrete_ModelMatrix = glm::translate(button_concrete_ModelMatrix, button_concrete_room_pos);
    button_concrete_ModelMatrix = glm::scale(button_concrete_ModelMatrix, button_concrete_room_size);
    button_concrete_NormalMatrix = glm::inverseTranspose(glm::mat3(view*button_concrete_ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(button_concrete_ModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(button_concrete_NormalMatrix));
      
    // we render the button to room1
    sphereModel.Draw();
    button_concrete_ModelMatrix = glm::mat4(1.0f);

    // we activate the texture for the button to room2
    ActivateTexture(8,2.0f,textureLocation,nMapLocation,repeatLocation,proceduralLocation);

    // button to room2
    button_metallic_ModelMatrix = glm::mat4(1.0f);
    button_metallic_NormalMatrix = glm::mat3(1.0f);
    button_metallic_ModelMatrix = glm::translate(button_metallic_ModelMatrix, button_metallic_room_pos);
    button_metallic_ModelMatrix = glm::scale(button_metallic_ModelMatrix, button_metallic_room_size);
    button_metallic_NormalMatrix = glm::inverseTranspose(glm::mat3(view*button_metallic_ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(button_metallic_ModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(button_metallic_NormalMatrix));  
      
    // we render the button to room2
    sphereModel.Draw();
    button_metallic_ModelMatrix = glm::mat4(1.0f);

    // we activate the texture for button to room3
    ActivateTexture(14,2.0f,textureLocation,nMapLocation,repeatLocation,proceduralLocation);

    // button to room3
    button_abstract_ModelMatrix = glm::mat4(1.0f);
    button_abstract_NormalMatrix = glm::mat3(1.0f);
    button_abstract_ModelMatrix = glm::translate(button_abstract_ModelMatrix, button_abstract_room_pos);
    button_abstract_ModelMatrix = glm::scale(button_abstract_ModelMatrix, button_abstract_room_size);
    button_abstract_NormalMatrix = glm::inverseTranspose(glm::mat3(view*button_abstract_ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(button_abstract_ModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(button_abstract_NormalMatrix));  

    // we render the button to room 3
    sphereModel.Draw();
    button_abstract_ModelMatrix = glm::mat4(1.0f);
    glUniform1i(glGetUniformLocation(shader.Program, "normalMapping"), GL_FALSE);

    // we activate the texture for sens buttons
    ActivateTexture(11,5.0f,textureLocation,nMapLocation,repeatLocation,proceduralLocation);

    // button used to decrease the camera sensitivity
    button_lower_sensitivity_ModelMatrix = glm::mat4(1.0f);
    button_lower_sensitivity_NormalMatrix = glm::mat3(1.0f);
    button_lower_sensitivity_ModelMatrix = glm::translate(button_lower_sensitivity_ModelMatrix, button_lower_sensitivity_pos);
    button_lower_sensitivity_ModelMatrix = glm::scale(button_lower_sensitivity_ModelMatrix, button_sensitivity_size);
    button_lower_sensitivity_NormalMatrix = glm::inverseTranspose(glm::mat3(view*button_lower_sensitivity_ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(button_lower_sensitivity_ModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(button_lower_sensitivity_NormalMatrix));
      
    // we render the button 
    sphereModel.Draw();
    button_abstract_ModelMatrix = glm::mat4(1.0f);

    // button used to increase the camera sensitivity
    button_higher_sensitivity_ModelMatrix = glm::mat4(1.0f);
    button_higher_sensitivity_NormalMatrix = glm::mat3(1.0f);
    button_higher_sensitivity_ModelMatrix = glm::translate(button_higher_sensitivity_ModelMatrix, button_higher_sensitivity_pos);
    button_higher_sensitivity_ModelMatrix = glm::scale(button_higher_sensitivity_ModelMatrix, button_sensitivity_size);
    button_higher_sensitivity_NormalMatrix = glm::inverseTranspose(glm::mat3(view*button_higher_sensitivity_ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(button_higher_sensitivity_ModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(button_higher_sensitivity_NormalMatrix));
     
    // we render the button
    sphereModel.Draw();
    button_abstract_ModelMatrix = glm::mat4(1.0f);
      
    
    /////////////// TARGETS RENDERING ////////////////
    // Customized target rendering for each room

    // we activate the texture of the targets
    ActivateTexture(4 + active_room * 6, 2.0f, textureLocation, nMapLocation, repeatLocation, proceduralLocation);

    // array of 16 floats = "native" matrix of OpenGL. We need it as an intermediate data structure to "convert" the Bullet matrix to a GLM matrix
    GLfloat matrix[16];
    btTransform transform;

    // we need two variables to manage the rendering of targets
    glm::vec3 obj_size;
    Model* objectModel;

    // we ask Bullet to provide the total number of Rigid Bodies in the scene
    int num_cobjs = bulletSimulation.dynamicsWorld->getNumCollisionObjects();

    // active sphere randomizer
    if(active){
        active_targets[(rand()%49)] = true;
        active = false;
    }

    // different rendering options for different active room
    switch (active_room)
    {
    case FIRST:

        alpha = 0.4f;
        F0 = 0.9f;

        // we cycle among all the Rigid Bodies (starting from walls_number+buttons_numbers to avoid the plane, the walls and the buttons)
        for (int i = walls_number + buttons_number; i < num_cobjs; i++)
        {
            // we render only the active target
            if (active_targets[i-(walls_number+buttons_number)])
            {
                // we point objectModel to the sphere
                objectModel = &sphereModel;
                obj_size = sphere_size;
          
                // we take the Collision Object from the list
                btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];

                // we upcast it in order to use the methods of the main class RigidBody
                btRigidBody* body = btRigidBody::upcast(obj);

                // we take the transformation matrix of the rigid boby, as calculated by the physics engine
                body->getMotionState()->getWorldTransform(transform);

                // we convert the Bullet matrix (transform) to an array of floats
                transform.getOpenGLMatrix(matrix);

                // we reset to identity at each frame
                objModelMatrix = glm::mat4(1.0f);
                objNormalMatrix = glm::mat3(1.0f);

                // we create the GLM transformation matrix
                // 1) we convert the array of floats to a GLM mat4 (using make_mat4 method)
                // 2) Bullet matrix provides rotations and translations: it does not consider scale (usually the Collision Shape is generated using directly the scaled dimensions). If, like in our case, we have applied a scale to the original model, we need to multiply the scale to the rototranslation matrix created in 1). If we are working on an imported and not scaled model, we do not need to do this
                objModelMatrix = glm::make_mat4(matrix) * glm::scale(objModelMatrix, obj_size);
                // we create the normal matrix
                objNormalMatrix = glm::inverseTranspose(glm::mat3(view*objModelMatrix));
                glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objModelMatrix));
                glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objNormalMatrix));
                glUniform1i(glGetUniformLocation(shader.Program, "normalMapping"), GL_TRUE);
                glUniform1i(glGetUniformLocation(shader.Program, "isTarget"), GL_TRUE);

                // we render the model
                objectModel->Draw();
                // we "reset" the matrix
                objModelMatrix = glm::mat4(1.0f);
                glUniform1i(glGetUniformLocation(shader.Program, "normalMapping"), GL_FALSE);
                glUniform1i(glGetUniformLocation(shader.Program, "isTarget"), GL_FALSE);
            }
        }

        break;

    case SECOND:

        alpha = 0.15f;
        F0 = 0.5f;

        // we cycle among all rigid bodies avoiding planes, walls and buttons. In the second room all spheres needs to be rendered  
        for (int i = walls_number + buttons_number; i<num_cobjs; i++)
        {
            // we activate texture for turned off sphere
            ActivateTexture(4 + active_room * 6, 2.0f, textureLocation, nMapLocation, repeatLocation, proceduralLocation);

            // we point objectModel to the sphere
            objectModel = &sphereModel;
            obj_size = sphere_size;
          
            // we take the Collision Object from the list
            btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];

            // we upcast it in order to use the methods of the main class RigidBody
            btRigidBody* body = btRigidBody::upcast(obj);

            // we take the transformation matrix of the rigid boby, as calculated by the physics engine
            body->getMotionState()->getWorldTransform(transform);

            // we convert the Bullet matrix (transform) to an array of floats
            transform.getOpenGLMatrix(matrix);

            // we reset to identity at each frame
            objModelMatrix = glm::mat4(1.0f);
            objNormalMatrix = glm::mat3(1.0f);

            // we create the GLM transformation matrix
            // 1) we convert the array of floats to a GLM mat4 (using make_mat4 method)
            // 2) Bullet matrix provides rotations and translations: it does not consider scale (usually the Collision Shape is generated using directly the scaled dimensions). If, like in our case, we have applied a scale to the original model, we need to multiply the scale to the rototranslation matrix created in 1). If we are working on an imported and not scaled model, we do not need to do this
            objModelMatrix = glm::make_mat4(matrix) * glm::scale(objModelMatrix, obj_size);
            // we create the normal matrix
            objNormalMatrix = glm::inverseTranspose(glm::mat3(view*objModelMatrix));
            glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objModelMatrix));
            glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objNormalMatrix));
            glUniform1i(glGetUniformLocation(shader.Program, "normalMapping"), GL_FALSE);
            glUniform1i(glGetUniformLocation(shader.Program, "isTarget"), GL_TRUE);

            // if the sphere is turned on we need to active another texture and change the point light position
            if (active_targets[i-(walls_number+buttons_number)]){
                ActivateTexture(5 + active_room * 6, 2.0f, textureLocation, nMapLocation, repeatLocation, proceduralLocation);
                pointLightPosition = glm::vec3(body->getCenterOfMassPosition().getX(), body->getCenterOfMassPosition().getY(), body->getCenterOfMassPosition().getZ());              
            }

            // we render the model
            objectModel->Draw();

            // we "reset" the matrix
            objModelMatrix = glm::mat4(1.0f);
            glUniform1i(glGetUniformLocation(shader.Program, "isTarget"), GL_FALSE); 
        }

        break;

    case THIRD:

        alpha = 0.2f;
        F0 = 0.9f;

        // we activate the texture of the target
        ActivateTexture(4 + active_room * 6, 1.0f, textureLocation, nMapLocation, repeatLocation, proceduralLocation);

        if(hit){

            reset(active_room);
            // we randomize the sphere starting point in the grid
            target_pos = glm::vec3((num_side - (rand()%7)*0.5f)-5.5f, (num_side - (rand()%7)*0.5f)-2.5f, 0.7f);
            target = bulletSimulation.createRigidBody(SPHERE,target_pos,glm::vec3(0.15f,0.2f,0.2f),glm::vec3(0.0f,0.0f,0.0f),1.0f,0.5f,0.9f);

            // we apply the impulse and shoot the bullet in the scene
            GLfloat shootInitialSpeed = 7.0f;
            glm::vec3 shoot = glm::normalize(glm::vec3(randomNumber(-1.0f, 1.0f),randomNumber(-1.0f, 1.0f),randomNumber(-1.0f, 1.0f)));
            btVector3 impulse = btVector3(shoot.x,shoot.y,shoot.z) * shootInitialSpeed;
            target->applyCentralImpulse(impulse);
            hit=false;
        }

        // we cycle among all the Rigid Bodies (starting from walls_number + buttons_number to avoid the plane, the walls and the buttons)
        for (int i = walls_number + buttons_number; i<bulletSimulation.dynamicsWorld->getCollisionObjectArray().size(); i++)
        {

            // we point objectModel to the cube
            objectModel = &sphereModel;
            obj_size = sphere_size;

            // we take the Collision Object from the list
            btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];

            // we upcast it in order to use the methods of the main class RigidBody
            btRigidBody* body = btRigidBody::upcast(obj);

            // we take the transformation matrix of the rigid boby, as calculated by the physics engine
            body->getMotionState()->getWorldTransform(transform);

            // we convert the Bullet matrix (transform) to an array of floats
            transform.getOpenGLMatrix(matrix);

            // we reset to identity at each frame
            objModelMatrix = glm::mat4(1.0f);
            objNormalMatrix = glm::mat3(1.0f);

            // we create the GLM transformation matrix
            objModelMatrix = glm::make_mat4(matrix) * glm::scale(objModelMatrix, obj_size);
            // we create the normal matrix
            objNormalMatrix = glm::inverseTranspose(glm::mat3(view*objModelMatrix));
            glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objModelMatrix));
            glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objNormalMatrix));
            glUniform1i(glGetUniformLocation(shader.Program, "normalMapping"), GL_TRUE);
            glUniform1i(glGetUniformLocation(shader.Program, "isTarget"), GL_TRUE);

            // we render the model
            objectModel->Draw();
            // we "reset" the matrix
            objModelMatrix = glm::mat4(1.0f);
            glUniform1i(glGetUniformLocation(shader.Program, "normalMapping"), GL_FALSE);
            glUniform1i(glGetUniformLocation(shader.Program, "isTarget"), GL_FALSE);
        }
        
        break;
    
    default:
        break;
    }
      
}


// Function LoadTexture we load the texture images from disk and create OpenGL textures
GLint LoadTexture(const char* path)
{
    GLuint textureImage;
    int w, h, channels;
    unsigned char* image;
    image = stbi_load(path, &w, &h, &channels, STBI_rgb);

    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;

    glGenTextures(1, &textureImage);
    glBindTexture(GL_TEXTURE_2D, textureImage);
    // 3 channels = RGB ; 4 channel = RGBA
    if (channels==3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (channels==4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    // we set how to consider UVs outside [0,1] range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);

    // we set the binding to 0 once we have finished
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureImage;
}


// SetupShader Function //?????????????????
void SetupShader(int program)
{
    int maxSub,maxSubU,countActiveSU;
    GLchar name[256];
    int len, numCompS;

    // global parameters about the Subroutines parameters of the system
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSub);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubU);
    //std::cout << "Max Subroutines:" << maxSub << " - Max Subroutine Uniforms:" << maxSubU << std::endl;

    // get the number of Subroutine uniforms (only for the Fragment shader, due to the nature of the exercise)
    // it is possible to add similar calls also for the Vertex shader
    glGetProgramStageiv(program, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &countActiveSU);

    // print info for every Subroutine uniform
    for (int i = 0; i < countActiveSU; i++) {

        // get the name of the Subroutine uniform (in this example, we have only one)
        glGetActiveSubroutineUniformName(program, GL_FRAGMENT_SHADER, i, 256, &len, name);
        // print index and name of the Subroutine uniform
        //std::cout << "Subroutine Uniform: " << i << " - name: " << name << std::endl;

        // get the number of subroutines
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_NUM_COMPATIBLE_SUBROUTINES, &numCompS);

        // get the indices of the active subroutines info and write into the array s
        int *s =  new int[numCompS];
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_COMPATIBLE_SUBROUTINES, s);
        //std::cout << "Compatible Subroutines:" << std::endl;

        // for each index, get the name of the subroutines, print info, and save the name in the shaders vector
        for (int j=0; j < numCompS; ++j) {
            glGetActiveSubroutineName(program, GL_FRAGMENT_SHADER, s[j], 256, &len, name);
            shaders.push_back(name);
        }
        //std::cout << std::endl;

        delete[] s;
    }
}


// Function used to activate texture at index i (each texture needs color and normal map at subsequent indexes)
void ActivateTexture(GLint index, GLfloat repeat, GLint textureLocation, GLint nMapLocation, GLint repeatLocation, GLint proceduralLocation){

    // base texture
    glActiveTexture(33984+index);
    glBindTexture(GL_TEXTURE_2D, textureID[index]);

    // normal map
    glActiveTexture(33984+index+1);
    glBindTexture(GL_TEXTURE_2D, textureID[index+1]);

    // special case for procedural texture
    glUniform1i(proceduralLocation, GL_FALSE);
    if(index==14){
        glUniform1i(proceduralLocation, GL_TRUE);
        repeat = 5.0f;
    }

    glUniform1i(textureLocation, index);
    glUniform1i(nMapLocation, index+1);
    glUniform1f(repeatLocation, repeat);
}



// Function to load one side of the cubemap, passing the name of the file and the side of the corresponding OpenGL cubemap
void LoadTextureCubeSide(string path, string side_image, GLuint side_name)
{
    int w, h;
    unsigned char* image;
    string fullname;

    // full name and path of the side of the cubemap
    fullname = path + side_image;
    // we load the image file
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    // we set the image file as one of the side of the cubemap (passed as a parameter)
    glTexImage2D(side_name, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
}


// Function to load the 6 images from disk and create an OpenGL cube map
GLint LoadTextureCube(string path)
{
    GLuint textureImage;

    // we create and activate the OpenGL cubemap texture
    glGenTextures(1, &textureImage);
    glActiveTexture(GL_TEXTURE28+loadedCubes);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureImage);

    // we load and set the 6 images corresponding to the 6 views of the cubemap
    // we use as convention that the names of the 6 images are "posx, negx, posy, negy, posz, negz", placed at the path passed as parameter
    // we load the images individually and we assign them to the correct sides of the cube map
    LoadTextureCubeSide(path, std::string("right.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    LoadTextureCubeSide(path, std::string("left.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    LoadTextureCubeSide(path, std::string("top.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    LoadTextureCubeSide(path, std::string("bottom.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    LoadTextureCubeSide(path, std::string("back.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    LoadTextureCubeSide(path, std::string("front.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // we set how to consider the texture coordinates outside [0,1] range
    // in this case we have a cube map, so
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // we set the binding to 0 once we have finished
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // we need to load 3 different cubemaps
    loadedCubes = loadedCubes+1;

    return textureImage;
}


// function used to reset rigid bodies and parametes at room swap
void reset(GLint next_room_index){

    increaseScore();

    for(int i=bulletSimulation.dynamicsWorld->getCollisionObjectArray().size()-1; i>=walls_number+buttons_number; i--){

        btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];
        // we upcast it in order to use the methods of the main class RigidBody
        btRigidBody* body = btRigidBody::upcast(obj);
        bulletSimulation.dynamicsWorld->removeCollisionObject(body);
    }

    if(next_room_index == FIRST || next_room_index == SECOND){

        resetScore();

        for(GLint i = 0; i < num_side; i++ )
        {
            for(GLint j = 0; j < num_side; j++ )
            {
                // position of each target in the grid
                target_pos = glm::vec3((num_side - j*0.5f)-5.5f, (num_side - i*0.5f)-4.5f, 0.7f);
                // we create a rigid body
                target = bulletSimulation.createRigidBody(SPHERE,target_pos,target_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);
            }
        }
    }

    if(next_room_index == FIRST || next_room_index == THIRD){
        hit = true;
        pointLightColor=glm::vec3(0.0f,0.0f,0.0f); // we turn off the point light in first and third room
    }else{
        pointLightColor=glm::vec3(0.5f,0.0f,0.0f);
    }
}


// function used to render text in HUD
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state
    shader.Use();
    glUniform3f(glGetUniformLocation(shader.Program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    GLint temp;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &temp);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        //cout<<ch.TextureID<<endl;
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


// function used to reset the score
void resetScore(){
    score = 0;
}

// function used to increase the score
void increaseScore(){
    score++;
}