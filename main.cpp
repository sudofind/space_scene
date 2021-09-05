#include "filesystem.h"
#include "shader.h"
#include "camera.h"
#include "mesh.h"
#include "model.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>




const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;
bool shadows = true;
bool shadowsKeyPressed = false;

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


void renderScene(Shader &shader, Model &suit_model, Model &planet_model, Model &rock_model,
                    float &rock_x_position, float deltaTime, float &rock_x_multiplier, float &rock_y_multiplier,
                    glm::vec3 lightPos, Camera camera, bool activate_viewer)
{
    shader.use();
    shader.setVec3("light.position", lightPos);
    shader.setVec3("viewPos", camera.Position);

    // light properties
    shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
    shader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
    shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    // material properties
    shader.setFloat("material.shininess", 64.0f);

    // view/projection transformations
    if (activate_viewer) {
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
    }

    // world transformation
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);


    glm::mat4 suit_model_matrix = glm::mat4(1.0f);
    suit_model_matrix = glm::translate(suit_model_matrix, glm::vec3(0.0f, 3.4f, 0.0f)); // translate it down so it's at the center of the scene
    suit_model_matrix = glm::scale(suit_model_matrix, glm::vec3(0.5f, 0.5f, 0.5f));	// it's a bit too big for our scene, so scale it down
    shader.setMat4("model", suit_model_matrix);
    suit_model.Draw(shader);

    glm::mat4 planet_model_matrix = glm::mat4(1.0f);
    planet_model_matrix = glm::translate(planet_model_matrix, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
    planet_model_matrix = glm::scale(planet_model_matrix, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
    shader.setMat4("model", planet_model_matrix);
    planet_model.Draw(shader);



    // Calculating new position for the rock
    rock_x_position += deltaTime * rock_x_multiplier;
    if (glm::abs(rock_x_position) > 4) {
        rock_x_position =  4;
        if (rock_x_multiplier < 0)
            rock_x_position *= -1;
        rock_x_multiplier *= -1;
        // rock_y_multiplier *= -1;
    }


    auto rock_y_position = -1 * rock_y_multiplier * glm::sqrt(49 - rock_x_position*rock_x_position);
            
    glm::mat4 rock_model_matrix = glm::mat4(1.0f);
    rock_model_matrix = glm::translate(rock_model_matrix, glm::vec3(rock_x_position, rock_y_position, 0.0f)); // translate it down so it's at the center of the scene
    rock_model_matrix = glm::scale(rock_model_matrix, glm::vec3(0.3f, 0.3f, 0.3f));	// it's a bit too big for our scene, so scale it down
    shader.setMat4("model", rock_model_matrix);
    rock_model.Draw(shader);
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);


// camera
Camera camera(glm::vec3(0.0f, 7.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int view_mode = 1;
bool game_continues = true;

glm::vec3 lightPos(-1.0f, 10.0f, -2.0f);


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SpaceScene", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    // build and compile shaders
    // -------------------------
    
    Shader simpleDepthShader("shadow_vertex.glsl", "shadow_fragment.glsl");
    Shader debugDepthQuad("depth_vertex.glsl", "depth_fragment.glsl");
    Shader shader("true_shadow_vertex.glsl", "true_shadow_fragment.glsl");
    Shader shader3d("3dlight_vertex.glsl", "3dlight_fragment.glsl");
    Shader simpleDepthShader3d("3dlight_depth_vertex.glsl", "3dlight_depth_fragment.glsl", "3dlight_depth_geometry.glsl");

    // load models
    // -----------
    Model planet_model(FileSystem::getPath("resources/objects/planet/planet.obj"));
    Model rock_model(FileSystem::getPath("resources/objects/rock/rock.obj"));
    Model suit_model(FileSystem::getPath("resources/objects/cyborg/cyborg.obj"));

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------

    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth cubemap texture

    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);



    unsigned int depthMapFBO2;
    glGenFramebuffers(1, &depthMapFBO2);
    // create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    debugDepthQuad.use();
    debugDepthQuad.setInt("depthMap", 0);
    shader3d.use();
    shader3d.setInt("diffuseTexture", 0);
    shader3d.setInt("depthMap", 1);

    auto prev_time = glfwGetTime();

    auto rock_x_position = 7.0f;
    auto rock_y_position = 0.0f;
    float rock_x_multiplier = -1;
    float rock_y_multiplier = -1;

    while (!glfwWindowShouldClose(window) and game_continues)
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        if (view_mode == 2) {
        // 1. render depth of scene to texture (from light's perspective)
        // --------------------------------------------------------------
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 25.0f;
        lightProjection = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        // render scene from light's point of view
        simpleDepthShader.use();
        simpleDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO2);
            glClear(GL_DEPTH_BUFFER_BIT);
            renderScene(simpleDepthShader, suit_model, planet_model, rock_model,
                    rock_x_position, deltaTime, rock_x_multiplier, rock_y_multiplier,
                    lightPos, camera, false);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // reset viewport
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render Depth map to quad for visual debugging
        // ---------------------------------------------
        debugDepthQuad.use();
        debugDepthQuad.setFloat("near_plane", near_plane);
        debugDepthQuad.setFloat("far_plane", far_plane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        renderQuad();
        } else if (view_mode == 1) {
                
            float near_plane = 1.0f;
            float far_plane  = 25.0f;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
            std::vector<glm::mat4> shadowTransforms;
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

            // 1. render scene to depth cubemap
            // --------------------------------
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
                glClear(GL_DEPTH_BUFFER_BIT);
                simpleDepthShader3d.use();
                for (unsigned int i = 0; i < 6; ++i)
                    simpleDepthShader3d.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
                simpleDepthShader3d.setFloat("far_plane", far_plane);
                simpleDepthShader3d.setVec3("lightPos", lightPos);
                renderScene(simpleDepthShader3d, suit_model, planet_model, rock_model,
                    rock_x_position, deltaTime, rock_x_multiplier, rock_y_multiplier,
                    lightPos, camera, true);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 2. render scene as normal 
            // -------------------------
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shader3d.use();
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();
            shader3d.setMat4("projection", projection);
            shader3d.setMat4("view", view);
            // set lighting uniforms
            shader3d.setVec3("lightPos", lightPos);
            shader3d.setVec3("viewPos", camera.Position);
            shader3d.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
            shader3d.setFloat("far_plane", far_plane);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
            renderScene(shader3d, suit_model, planet_model, rock_model,
                        rock_x_position, deltaTime, rock_x_multiplier, rock_y_multiplier,
                        lightPos, camera, true);

        }
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}




// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        view_mode = 1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        view_mode = 2;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        view_mode = 3;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        view_mode = 4;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        game_continues = false;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
