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

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

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

// view and projection matrices (global because we need to use them in the keyboard callback)
glm::mat4 view, projection;

// we create a camera. We pass the initial position as a parameter to the constructor. In this case, we use a "floating" camera (we pass false as last parameter)
Camera camera(glm::vec3(0.0f, 0.0f, 9.0f), GL_FALSE);

// Uniforms to be passed to shaders
// point light position
glm::vec3 lightPos0 = glm::vec3(5.0f, 10.0f, 10.0f);

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
// color of the bullets
GLfloat shootColor[] = {1.0f,1.0f,0.0f};
// dimension of the bullets (global because we need it also in the keyboard callback)
glm::vec3 sphere_size = glm::vec3(0.2f, 0.2f, 0.2f);
glm::vec3 projectile_size = glm::vec3(0.1f,0.1f,0.1f);

//active target array
GLboolean active_targets [49] = {false};
GLboolean active = true;

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
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_lecture05b", nullptr, nullptr);
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
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // the Shader Program for the objects used in the application
    Shader object_shader = Shader("09_illumination_models.vert", "10_illumination_models.frag");

    // we load the model(s) (code of Model class is in include/utils/model_v2.h)
    Model cubeModel("../models/cube.obj");
    Model sphereModel("../models/sphere.obj");

    // dimensions and position of the static plane
    // we will use the cube mesh to simulate the plane, because we need some "height" in the mesh
    // in order to make it work with the physics simulation
    glm::vec3 plane_pos = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 plane_size = glm::vec3(200.0f, 0.1f, 200.0f);
    glm::vec3 plane_rot = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 rwall_pos = glm::vec3(4.0f, 3.0f, 0.0f);
    glm::vec3 rwall_size = glm::vec3(4.0f, 0.1f, 4.0f);
    glm::vec3 rwall_rot = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::vec3 lwall_pos = glm::vec3(-4.0f, 3.0f, 0.0f);
    glm::vec3 lwall_size = glm::vec3(4.0f, 0.1f, 4.0f);
    glm::vec3 lwall_rot = glm::vec3(0.0f, 0.0f, 1.0f);
    
    glm::vec3 bwall_pos = glm::vec3(0.0f, 3.0f, -4.0f);
    glm::vec3 bwall_size = glm::vec3(4.0f, 0.1f, 4.0f);
    glm::vec3 bwall_rot = glm::vec3(1.0f, 0.0f, 0.0f);

    // we create a rigid body for the plane. In this case, it is static, so we pass mass = 0;
    // in this way, the plane will not fall following the gravity force.
    btRigidBody* plane = bulletSimulation.createRigidBody(BOX,plane_pos,plane_size,plane_rot,0.0f,0.3f,0.3f);
    btRigidBody* rwall = bulletSimulation.createRigidBody(BOX,rwall_pos,rwall_size,rwall_rot,0.0f,0.3f,0.3f);
    btRigidBody* lwall = bulletSimulation.createRigidBody(BOX,lwall_pos,lwall_size,lwall_rot,0.0f,0.3f,0.3f);
    btRigidBody* bwall = bulletSimulation.createRigidBody(BOX,bwall_pos,bwall_size,bwall_rot,0.0f,0.3f,0.3f);

    // we create 25 rigid bodies for the cubes of the scene. In this case, we use BoxShape, with the same dimensions of the cubes, as collision shape of Bullet. For more complex cases, a Bounding Box of the model may have to be calculated, and its dimensions to be passed to the physics library
    GLint num_side = 7;
    // total number of the cubes
    GLint total_targets = num_side*num_side;
    GLint i,j;
    // position of the cube
    glm::vec3 target_pos;
    // dimension of the cube
    glm::vec3 target_size = glm::vec3(0.2f, 0.5f, 0.2f);
    // we set a small initial rotation for the cubes
    glm::vec3 target_rot = glm::vec3(0.1f, 0.0f, 0.1f);
    // rigid body
    btRigidBody* target;


    for(i = 0; i < num_side; i++ )
    {
        for(j = 0; j < num_side; j++ )
        {
            // position of each cube in the grid (we add 3 to x to have a bigger displacement)
            target_pos = glm::vec3((num_side - j*0.5f)-5.5f, (num_side - i*0.5f)-4.5f, 0.7f);
            // we create a rigid body (in this case, a dynamic body, with mass = 2)
            target = bulletSimulation.createRigidBody(SPHERE,target_pos,target_size,target_rot,0.0f,0.3f,0.3f);

        }
    }

  // we set the maximum delta time for the update of the physical simulation
  GLfloat maxSecPerFrame = 1.0f / 60.0f;

  // Projection matrix: FOV angle, aspect ratio, near and far planes
  projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

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
      // View matrix (=camera): position, view direction, camera "up" vector
      // in this example, it has been defined as a global variable (we need it in the keyboard callback function)
      view = camera.GetViewMatrix();

      // we "clear" the frame and z buffer
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // we set the rendering mode
      if (wireframe)
          // Draw in wireframe
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      // we update the physics simulation. We must pass the deltatime to be used for the update of the physical state of the scene. The default value for Bullet is 60 Hz, for lesser deltatime the library interpolates and does not calculate the simulation. In this example, we use deltatime from the last rendering: if it is < 1\60 sec, than we use it, otherwise we use the deltatime we have set above
      // we also set the max number of substeps to consider for the simulation (=10)
      bulletSimulation.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame),10);

      /////////////////// OBJECTS ////////////////////////////////////////////////
      // We "install" the selected Shader Program as part of the current rendering process
      object_shader.Use();
      // We search inside the Shader Program the name of a subroutine, and we get the numerical index
      GLuint index = glGetSubroutineIndex(object_shader.Program, GL_FRAGMENT_SHADER, "GGX");
      // we activate the subroutine using the index
      glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &index);

      // we pass projection and view matrices to the Shader Program
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

      // we determine the position in the Shader Program of the uniform variables
      GLint objDiffuseLocation = glGetUniformLocation(object_shader.Program, "diffuseColor");
      GLint pointLightLocation = glGetUniformLocation(object_shader.Program, "pointLightPosition");
      GLint kdLocation = glGetUniformLocation(object_shader.Program, "Kd");
      GLint alphaLocation = glGetUniformLocation(object_shader.Program, "alpha");
      GLint f0Location = glGetUniformLocation(object_shader.Program, "F0");

      // we assign the value to the uniform variable
      glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPos0));
      glUniform1f(kdLocation, Kd);
      glUniform1f(alphaLocation, alpha);
      glUniform1f(f0Location, F0);

      /////
      // STATIC PLANE
      // we use a specific color for the plane
      glUniform3fv(objDiffuseLocation, 1, planeMaterial);

      // The plane is static, so its Collision Shape is not subject to forces, and it does not move. Thus, we do not need to use dynamicsWorld to acquire the rototraslations, but we can just use directly glm to manage the matrices
      // if, for some reason, the plane becomes a dynamic rigid body, the following code must be modified
      // we reset to identity at each frame
      planeModelMatrix = glm::mat4(1.0f);
      planeNormalMatrix = glm::mat3(1.0f);
      planeModelMatrix = glm::translate(planeModelMatrix, plane_pos);
      planeModelMatrix = glm::scale(planeModelMatrix, plane_size);
      planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));

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
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(rwallModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(rwallNormalMatrix));

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
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(lwallModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(lwallNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      lwallModelMatrix = glm::mat4(1.0f);

      //back wall
      bwallModelMatrix = glm::mat4(1.0f);
      bwallNormalMatrix = glm::mat3(1.0f);
      bwallModelMatrix = glm::translate(bwallModelMatrix, bwall_pos);
      bwallModelMatrix = glm::rotate(bwallModelMatrix,glm::radians(90.0f),bwall_rot);
      bwallModelMatrix = glm::scale(bwallModelMatrix, rwall_size);
      bwallNormalMatrix = glm::inverseTranspose(glm::mat3(view*bwallModelMatrix));
      glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bwallModelMatrix));
      glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bwallNormalMatrix));

      // we render the plane
      cubeModel.Draw();
      bwallModelMatrix = glm::mat4(1.0f);

      /////
      // DYNAMIC OBJECTS (FALLING CUBES + BULLETS)
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
        //cout << rand()%49;
      }

      // we cycle among all the Rigid Bodies (starting from 1 to avoid the plane)
      for (i=walls_number; i<num_cobjs;i++)
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
          glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objModelMatrix));
          glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objNormalMatrix));

          // we render the model
          // N.B.) if the number of models is relatively low, this approach (we render the same mesh several time from the same buffers) can work. If we must render hundreds or more of copies of the same mesh,
          // there are more advanced techniques to manage Instanced Rendering (see https://learnopengl.com/#!Advanced-OpenGL/Instancing for examples).
          objectModel->Draw();
          // we "reset" the matrix
          objModelMatrix = glm::mat4(1.0f);
          }
      }

      // Faccio lo swap tra back e front buffer
      glfwSwapBuffers(window);
  }

  // when I exit from the graphics loop, it is because the application is closing
  // we delete the Shader Programs
  object_shader.Delete();
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

    ///////
    /// BULLET MANAGEMENT (SPACE KEY)
    // if space is pressed, we "shoot" a bullet in the scene
    // the initial trajectory of the bullet is given by a vector from the position of the camera to the mouse cursor position, which must be converted from Viewport Coordinates back to World Coordinate

    btVector3 impulse;
    // we need a initial rotation, even if useless for a sphere
    glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec4 shoot;
    // initial velocity of the bullet
    GLfloat shootInitialSpeed = 15.0f;
    // rigid body of the bullet
    btRigidBody* sphere;
    // matrix for the inverse matrix of view and projection
    glm::mat4 unproject;

    // if space is pressed
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        /*// we create a Rigid Body with mass = 1
        sphere = bulletSimulation.createRigidBody(SPHERE,camera.Position,projectile_size,rot,0.1f,0.1f,0.3f);

        // we must retro-project the coordinates of the mouse pointer, in order to have a point in world coordinate to be used to determine a vector from the camera (= direction and orientation of the bullet)
        // we convert the cursor position (taken from the mouse callback) from Viewport Coordinates to Normalized Device Coordinate (= [-1,1] in both coordinates)
        shoot.x = 0.0f;//camera.Position.x; //((screenWidth/2)/screenWidth) * 2.0f - 1.0f;
        shoot.y = 0.0f;//camera.Position.y;//-((screenHeight/2)/screenHeight) * 2.0f + 1.0f; // Viewport Y coordinates are from top-left corner to the bottom
        // we need a 3D point, so we set a minimum value to the depth with respect to camera position
        shoot.z = 1.0f;
        // w = 1.0 because we are using homogeneous coordinates
        shoot.w = 1.0f;

        // we determine the inverse matrix for the projection and view transformations
        unproject = glm::inverse(projection * view);

        // we convert the position of the cursor from NDC to world coordinates, and we multiply the vector by the initial speed
        shoot = glm::normalize(unproject * shoot) * shootInitialSpeed;

        // we apply the impulse and shoot the bullet in the scene
        // N.B.) the graphical aspect of the bullet is treated in the rendering loop
        impulse = btVector3(shoot.x, shoot.y, shoot.z);
        sphere->applyCentralImpulse(impulse);
        */
        
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
