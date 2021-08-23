// Std. Includes
#include <string>

// Loader estensioni OpenGL
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
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

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;
// a vector for all the shader subroutines names used and swapped in the application
vector<std::string> shaders;

// the name of the subroutines are searched in the shaders, and placed in the shaders vector (to allow shaders swapping)
void SetupShader(int shader_program);

// print on console the name of current shader subroutine
void PrintCurrentShader(int subroutine);

// in this application, we have isolated the models rendering using a function, which will be called in each rendering step
void RenderObjects(Shader &shader, Model &cubeModel, Model &sphereModel, GLint render_pass, GLuint depthMap);

// load image from disk and create an OpenGL texture
GLint LoadTexture(const char* path);

// vector for the textures IDs
vector<GLint> textureID;

// we initialize an array of booleans for each keybord key
bool keys[1024];

//number of walls (useful to avoid certain controls on collisions)
GLint walls_number = 4;

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

// we create a camera. We pass the initial position as a parameter to the constructor. In this case, we use a "floating" camera (we pass false as last parameter)
Camera camera(glm::vec3(0.0f, 0.0f, 9.0f), GL_FALSE);

// Directional light
glm::vec3 lightDir0 = glm::vec3(1.0f, 1.0f, 1.0f);

// weight for the diffusive component
GLfloat Kd = 3.0f;
// roughness index for GGX shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// color of the falling objects
GLfloat diffuseColor[] = {1.0f,0.0f,0.0f};
// color of the plane
GLfloat planeMaterial[] = {0.0f,0.5f,0.0f};
// dimension of the targets (global because we need it also in the keyboard callback)
glm::vec3 sphere_size = glm::vec3(0.2f, 0.2f, 0.2f);

//repeat needed as a texture parameter
GLfloat repeat = 1.0f;

//active target array
GLboolean active_targets [49] = {false};
GLboolean active = true;


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


// dimensions and position of the static plane
// we will use the cube mesh to simulate the plane, because we need some "height" in the mesh
// in order to make it work with the physics simulation
glm::vec3 plane_pos = glm::vec3(0.0f, -1.1f, 0.0f);
glm::vec3 plane_size = glm::vec3(200.0f, 0.1f, 200.0f);
glm::vec3 plane_rot = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 rwall_pos = glm::vec3(4.5f, 3.0f, 0.0f);
glm::vec3 rwall_size = glm::vec3(4.0f, 0.4f, 4.0f);
glm::vec3 rwall_rot = glm::vec3(0.0f, 0.0f, 1.0f);

glm::vec3 lwall_pos = glm::vec3(-4.5f, 3.0f, 0.0f);
glm::vec3 lwall_size = glm::vec3(4.0f, 0.4f, 4.0f);
glm::vec3 lwall_rot = glm::vec3(0.0f, 0.0f, 1.0f);
    
glm::vec3 bwall_pos = glm::vec3(0.0f, 3.0f, -3.6f);
glm::vec3 bwall_size = glm::vec3(4.2f, 0.4f, 4.0f);
glm::vec3 bwall_rot = glm::vec3(1.0f, 0.0f, 0.0f);

GLint objDiffuseLocation;

// instance of the physics class
Physics bulletSimulation;

////////////////// MAIN function ///////////////////////
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
    //glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    //seed for randomizer
    srand(time(0));

    // we create the Shader Program for the creation of the shadow map
    Shader shadow_shader("19_shadowmap.vert", "20_shadowmap.frag");
    // the Shader Program for the objects used in the application
    Shader object_shader = Shader("21_ggx_tex_shadow.vert", "22_ggx_tex_shadow.frag");

    Shader crosshair_shader = Shader("09_illumination_models.vert","10_illumination_models.frag");

    // we parse the Shader Program to search for the number and names of the subroutines.
    // the names are placed in the shaders vector
    SetupShader(object_shader.Program);
    // we print on console the name of the first subroutine used
    PrintCurrentShader(current_subroutine);

    // we load the images and store them in a vector
    textureID.push_back(LoadTexture("../textures/UV_Grid_Sm.png"));
    textureID.push_back(LoadTexture("../textures/SoilCracked.png"));
    
    // we load the model(s) (code of Model class is in include/utils/model_v2.h)
    Model cubeModel("../models/cube.obj");
    Model sphereModel("../models/sphere.obj");

    

    // we create a rigid body for the plane. In this case, it is static, so we pass mass = 0;
    // in this way, the plane will not fall following the gravity force.
    btRigidBody* plane = bulletSimulation.createRigidBody(BOX,plane_pos,plane_size,plane_rot,0.0f,0.3f,0.3f);
    btRigidBody* rwall = bulletSimulation.createRigidBody(BOX,rwall_pos,rwall_size,rwall_rot,0.0f,0.3f,0.3f);
    btRigidBody* lwall = bulletSimulation.createRigidBody(BOX,lwall_pos,lwall_size,lwall_rot,0.0f,0.3f,0.3f);
    btRigidBody* bwall = bulletSimulation.createRigidBody(BOX,bwall_pos,bwall_size,bwall_rot,0.0f,0.3f,0.3f);

    //crosshair parameters
    glm::vec3 cross_pos;
    glm::vec3 cross_size = glm::vec3(0.005f, 0.0005f, 0.0f);
    glm::mat4 crossModelMatrix = glm::mat4(1.0f);
    glm::mat3 crossNormalMatrix = glm::mat3(1.0f);

    // we create 25 rigid bodies for the cubes of the scene. In this case, we use BoxShape, with the same dimensions of the cubes, as collision shape of Bullet. For more complex cases, a Bounding Box of the model may have to be calculated, and its dimensions to be passed to the physics library
    GLint num_side = 7;
    // total number of the cubes
    GLint total_targets = num_side*num_side;
    GLint i,j;
    // position of the cube
    glm::vec3 target_pos;
    // dimension of the cube
    glm::vec3 target_size = glm::vec3(0.13f,0.2f,0.2f);
    // rigid body
    btRigidBody* target;


    for(i = 0; i < num_side; i++ )
    {
        for(j = 0; j < num_side; j++ )
        {
            // position of each cube in the grid (we add 3 to x to have a bigger displacement)
            target_pos = glm::vec3((num_side - j*0.5f)-5.5f, (num_side - i*0.5f)-4.5f, 0.7f);
            // we create a rigid body (in this case, a dynamic body, with mass = 2)
            target = bulletSimulation.createRigidBody(SPHERE,target_pos,target_size,glm::vec3(0.0f,0.0f,0.0f),0.0f,0.3f,0.3f);

        }
    }

  // we set the maximum delta time for the update of the physical simulation
  GLfloat maxSecPerFrame = 1.0f / 60.0f;

  /////////////////// CREATION OF BUFFER FOR THE  DEPTH MAP /////////////////////////////////////////
    // buffer dimension: too large -> performance may slow down if we have many lights; too small -> strong aliasing
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
    //*************
    GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // we bind the depth map FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    // we set that we are not calculating nor saving color data
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ///////////////////////////////////////////////////////////////////

  // Projection matrix: FOV angle, aspect ratio, near and far planes
  glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

 

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
    // for a directional light, the projection is orthographic. For point lights, we should use a perspective projection
    lightProjection = glm::ortho(-frustumSize, frustumSize, -frustumSize, frustumSize, near_plane, far_plane);
    // the light is directional, so technically it has no position. We need a view matrix, so we consider a position on the the direction vector of the light
    lightView = glm::lookAt(lightDir0, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // transformation matrix for the light
    lightSpaceMatrix = lightProjection * lightView;
    /// We "install" the  Shader Program for the shadow mapping creation
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
    // in this example, it has been defined as a global variable (we need it in the keyboard callback function)
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

    // we update the physics simulation. We must pass the deltatime to be used for the update of the physical state of the scene. The default value for Bullet is 60 Hz, for lesser deltatime the library interpolates and does not calculate the simulation. In this example, we use deltatime from the last rendering: if it is < 1\60 sec, than we use it, otherwise we use the deltatime we have set above
    // we also set the max number of substeps to consider for the simulation (=10)
    bulletSimulation.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame),10);

      /////////////////// OBJECTS ////////////////////////////////////////////////
      // We "install" the selected Shader Program as part of the current rendering process
      object_shader.Use();
      // We search inside the Shader Program the name of a subroutine, and we get the numerical index
      GLuint index = glGetSubroutineIndex(object_shader.Program, GL_FRAGMENT_SHADER, shaders[current_subroutine].c_str());
      // we activate the subroutine using the index
      glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &index);

      // we pass projection and view matrices to the Shader Program
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

      // we determine the position in the Shader Program of the uniform variables
      //objDiffuseLocation = glGetUniformLocation(object_shader.Program, "diffuseColor");
      //GLint pointLightLocation = glGetUniformLocation(object_shader.Program, "pointLightPosition");
      GLint lightDirLocation = glGetUniformLocation(object_shader.Program, "lightVector");
      GLint kdLocation = glGetUniformLocation(object_shader.Program, "Kd");
      GLint alphaLocation = glGetUniformLocation(object_shader.Program, "alpha");
      GLint f0Location = glGetUniformLocation(object_shader.Program, "F0");

      // we assign the value to the uniform variable
      glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDir0));
      glUniform1f(kdLocation, Kd);
      glUniform1f(alphaLocation, alpha);
      glUniform1f(f0Location, F0);

    // we render the scene
    RenderObjects(object_shader, cubeModel, sphereModel, RENDER, depthMap);

      crosshair_shader.Use();

      // we pass projection and view matrices to the Shader Program
      glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
      //horizontal part of the Crosshair 
      crossModelMatrix = glm::mat4(1.0f);
      crossNormalMatrix = glm::mat3(1.0f);
      crossModelMatrix = glm::translate(crossModelMatrix, (camera.Front*0.5f));
      crossModelMatrix = crossModelMatrix*glm::inverse(camera.GetViewMatrix());
      crossModelMatrix = glm::scale(crossModelMatrix, cross_size);
      crossNormalMatrix = glm::inverseTranspose(glm::mat3(view*crossModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(crossModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(crosshair_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(crossNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      crossModelMatrix = glm::mat4(1.0f);

      //vertical part of the Crosshair
      crossModelMatrix = glm::mat4(1.0f);
      crossNormalMatrix = glm::mat3(1.0f);
      crossModelMatrix = glm::translate(crossModelMatrix, (camera.Front*0.5f));
      crossModelMatrix = crossModelMatrix*glm::inverse(camera.GetViewMatrix());
      crossModelMatrix = glm::rotate(crossModelMatrix,glm::radians(90.0f),glm::vec3(0.0f,0.0f,1.0f));
      crossModelMatrix = glm::scale(crossModelMatrix, cross_size);
      crossNormalMatrix = glm::inverseTranspose(glm::mat3(view*crossModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(crosshair_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(crossModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(crosshair_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(crossNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      crossModelMatrix = glm::mat4(1.0f);
      

      // Faccio lo swap tra back e front buffer
      glfwSwapBuffers(window);
  }

  // when I exit from the graphics loop, it is because the application is closing
  // we delete the Shader Programs
  object_shader.Delete();
  shadow_shader.Delete();
  crosshair_shader.Delete();
  // we delete the data of the physical simulation
  bulletSimulation.Clear();
  // we close and delete the created context
  glfwTerminate();
  return 0;
}

//ray casting 
bool hit_sphere(const glm::vec3& center, float radius, const glm::vec3& origin){
    glm::vec3 oc = origin - center;
    float a = dot(camera.Front, camera.Front);
    float b = 2.0 * dot(oc, camera.Front);
    float c = dot(oc,oc) - radius*radius;
    float discriminant = b*b - 4*a*c;
    return (discriminant>0);
}

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
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

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_UP && action == GLFW_PRESS)
        camera.IncreaseCameraSensitivity();

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        camera.DecreaseCameraSensitivity();

    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

//////////////////////////////////////////
// callback for mouse events
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

//callback for mouse click 
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        btVector3 temp(0.0f,0.0f,0.0f);
        btScalar radius;
        int num_cobjs = bulletSimulation.dynamicsWorld->getNumCollisionObjects();
        for(int i=walls_number; i<(walls_number+7*7); i++){
            btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];
            btCollisionShape* shape = obj->getCollisionShape();
            shape->getBoundingSphere(temp, radius);
            glm::vec3 center (obj->getWorldTransform().getOrigin().getX(),obj->getWorldTransform().getOrigin().getY(),obj->getWorldTransform().getOrigin().getZ());
            if (hit_sphere(center, radius, camera.Position)){
                cout<< "Colpita sfera " << i <<endl;
                if(active_targets[i-walls_number]){
                    active_targets[i-walls_number]=false;
                    active=true;
                }
            }
        }
    }
        
}

void RenderObjects(Shader &shader, Model &cubeModel, Model &sphereModel, GLint render_pass, GLuint depthMap ){
    
    // For the second rendering step -> we pass the shadow map to the shaders
    if (render_pass==RENDER)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        GLint shadowLocation = glGetUniformLocation(shader.Program, "shadowMap");
        glUniform1i(shadowLocation, 2);
    }
    // we pass the needed uniforms
    GLint textureLocation = glGetUniformLocation(shader.Program, "tex");
    GLint repeatLocation = glGetUniformLocation(shader.Program, "repeat");

    // PLANE
    // we activate the texture of the plane
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureID[1]);
    glUniform1i(textureLocation, 1);
    glUniform1f(repeatLocation, 80.0);
    
    
    /////
      // STATIC PLANE
      // we use a specific color for the plane
      //glUniform3fv(objDiffuseLocation, 1, planeMaterial);

      // The plane is static, so its Collision Shape is not subject to forces, and it does not move. Thus, we do not need to use dynamicsWorld to acquire the rototraslations, but we can just use directly glm to manage the matrices
      // if, for some reason, the plane becomes a dynamic rigid body, the following code must be modified
      // we reset to identity at each frame
      planeModelMatrix = glm::mat4(1.0f);
      planeNormalMatrix = glm::mat3(1.0f);
      planeModelMatrix = glm::translate(planeModelMatrix, plane_pos);
      planeModelMatrix = glm::scale(planeModelMatrix, plane_size);
      planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      planeModelMatrix = glm::mat4(1.0f);

      //right wall
      rwallModelMatrix = glm::mat4(1.0f);
      rwallNormalMatrix = glm::mat3(1.0f);
      rwallModelMatrix = glm::translate(rwallModelMatrix, rwall_pos);
      rwallModelMatrix = glm::rotate(rwallModelMatrix,glm::radians(90.0f),rwall_rot);
      rwallModelMatrix = glm::scale(rwallModelMatrix, rwall_size);
      rwallNormalMatrix = glm::inverseTranspose(glm::mat3(view*rwallModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(rwallModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(rwallNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      rwallModelMatrix = glm::mat4(1.0f);

      //left wall
      lwallModelMatrix = glm::mat4(1.0f);
      lwallNormalMatrix = glm::mat3(1.0f);
      lwallModelMatrix = glm::translate(lwallModelMatrix, lwall_pos);
      lwallModelMatrix = glm::rotate(lwallModelMatrix,glm::radians(-90.0f),lwall_rot);
      lwallModelMatrix = glm::scale(lwallModelMatrix, lwall_size);
      lwallNormalMatrix = glm::inverseTranspose(glm::mat3(view*lwallModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(lwallModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(lwallNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      lwallModelMatrix = glm::mat4(1.0f);

      //back wall
      bwallModelMatrix = glm::mat4(1.0f);
      bwallNormalMatrix = glm::mat3(1.0f);
      bwallModelMatrix = glm::translate(bwallModelMatrix, bwall_pos);
      bwallModelMatrix = glm::rotate(bwallModelMatrix,glm::radians(90.0f),bwall_rot);
      bwallModelMatrix = glm::scale(bwallModelMatrix, bwall_size);
      bwallNormalMatrix = glm::inverseTranspose(glm::mat3(view*bwallModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bwallModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bwallNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      bwallModelMatrix = glm::mat4(1.0f);

      /////
      // DYNAMIC OBJECTS (TARGETS)

      // PLANE
      // we activate the texture of the plane
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textureID[0]);
      glUniform1i(textureLocation, 0);
      glUniform1f(repeatLocation, 1.0f);

      // array of 16 floats = "native" matrix of OpenGL. We need it as an intermediate data structure to "convert" the Bullet matrix to a GLM matrix
      GLfloat matrix[16];
      btTransform transform;

      // we need two variables to manage the rendering of both cubes and bullets
      glm::vec3 obj_size;
      Model* objectModel;

      // we ask Bullet to provide the total number of Rigid Bodies in the scene
      int num_cobjs = bulletSimulation.dynamicsWorld->getNumCollisionObjects();

      //randomizer
      if(active){
        active_targets[(rand()%49)]=true;
        active=false;
      }

      // we cycle among all the Rigid Bodies (starting from 1 to avoid the plane)
      for (int i=walls_number; i<num_cobjs;i++)
      {
          // the first 25 objects are the falling cubes
          if (active_targets[i-walls_number])
          {
              // we point objectModel to the cube
              objectModel = &sphereModel;
              obj_size = sphere_size;
              // we pass red color to the shader
              glUniform3fv(objDiffuseLocation, 1, diffuseColor);
          
          

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

          // we render the model
          // N.B.) if the number of models is relatively low, this approach (we render the same mesh several time from the same buffers) can work. If we must render hundreds or more of copies of the same mesh,
          // there are more advanced techniques to manage Instanced Rendering (see https://learnopengl.com/#!Advanced-OpenGL/Instancing for examples).
          objectModel->Draw();
          // we "reset" the matrix
          objModelMatrix = glm::mat4(1.0f);
          }
      }
}

//////////////////////////////////////////
// we load the image from disk and we create an OpenGL texture
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

///////////////////////////////////////////
// The function parses the content of the Shader Program, searches for the Subroutine type names,
// the subroutines implemented for each type, print the names of the subroutines on the terminal, and add the names of
// the subroutines to the shaders vector, which is used for the shaders swapping
void SetupShader(int program)
{
    int maxSub,maxSubU,countActiveSU;
    GLchar name[256];
    int len, numCompS;

    // global parameters about the Subroutines parameters of the system
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSub);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubU);
    std::cout << "Max Subroutines:" << maxSub << " - Max Subroutine Uniforms:" << maxSubU << std::endl;

    // get the number of Subroutine uniforms (only for the Fragment shader, due to the nature of the exercise)
    // it is possible to add similar calls also for the Vertex shader
    glGetProgramStageiv(program, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &countActiveSU);

    // print info for every Subroutine uniform
    for (int i = 0; i < countActiveSU; i++) {

        // get the name of the Subroutine uniform (in this example, we have only one)
        glGetActiveSubroutineUniformName(program, GL_FRAGMENT_SHADER, i, 256, &len, name);
        // print index and name of the Subroutine uniform
        std::cout << "Subroutine Uniform: " << i << " - name: " << name << std::endl;

        // get the number of subroutines
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_NUM_COMPATIBLE_SUBROUTINES, &numCompS);

        // get the indices of the active subroutines info and write into the array s
        int *s =  new int[numCompS];
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_COMPATIBLE_SUBROUTINES, s);
        std::cout << "Compatible Subroutines:" << std::endl;

        // for each index, get the name of the subroutines, print info, and save the name in the shaders vector
        for (int j=0; j < numCompS; ++j) {
            glGetActiveSubroutineName(program, GL_FRAGMENT_SHADER, s[j], 256, &len, name);
            std::cout << "\t" << s[j] << " - " << name << "\n";
            shaders.push_back(name);
        }
        std::cout << std::endl;

        delete[] s;
    }
}

/////////////////////////////////////////
// we print on console the name of the currently used shader subroutine
void PrintCurrentShader(int subroutine)
{
    std::cout << "Current shader subroutine: " << shaders[subroutine]  << std::endl;
}
