#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Smart Parking", NULL, NULL);
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
    Shader ourShader("shaders/shader_project_mloading.vs", "shaders/shader_project_mloading.fs");

    // load models
    // -----------
    //Model ourModel(FileSystem::getPath("resources/objects/backpack/backpack.obj"));
    Model parkingModel("model/parking/parking.obj");
    Model lightModel("model/ceilinglight/ceilinglight.obj");
    //C:/Users/roma9/source/repos/Proyecto_Final_OpenGL/OpenGL/model/parking/parking.obj
    //Model ourModel("model/backpack/backpack.obj");

    camera.MovementSpeed = 30; //Optional. Modify the speed of the camera

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    glm::vec3 lampPositions[] = {

        // PRIMER PISO (Y = 4.5834)
        glm::vec3(-15.2176f,  4.5834f, 8.7f),
        glm::vec3(-15.2176f,  4.5834f, -4.7f),
        glm::vec3(-5.2176f,  4.5834f, 8.7f),
        glm::vec3(-5.2176f,  4.5834f, -4.7f),
        glm::vec3(18.2180f,  4.5834f, 8.7f),
        glm::vec3(18.2180f,  4.5834f, -4.7f),
        glm::vec3(8.2180f,  4.5834f, 8.7f),
        glm::vec3(8.2180f,  4.5834f, -4.7f),

        glm::vec3(23.7700f,  4.5834f, 20.0f),
        glm::vec3(0.8734f,  4.5834f, 20.0f),
        glm::vec3(-22.0232f,  4.5834f, 20.0f),
        glm::vec3(36.809f,  4.5834f, 20.0f),
        glm::vec3(-36.809f,  4.5834f, 20.0f),
        glm::vec3(23.7700f,  4.5834f, -23.0f),
        glm::vec3(0.8734f,  4.5834f, -23.0f),
        glm::vec3(-22.0232f,  4.5834f, -23.0f),
        glm::vec3(36.809f,  4.5834f, -23.0f),
        glm::vec3(-36.809f,  4.5834f, -23.0f),

        // SEGUNDO PISO (Y = 12.5810)
        glm::vec3(-15.2176f, 12.5810f, 8.7f),
        glm::vec3(-15.2176f, 12.5810f, -4.7f),
        glm::vec3(-5.2176f, 12.5810f, 8.7f),
        glm::vec3(-5.2176f, 12.5810f, -4.7f),
        glm::vec3(18.2180f, 12.5810f, 8.7f),
        glm::vec3(18.2180f, 12.5810f, -4.7f),
        glm::vec3(8.2180f, 12.5810f, 8.7f),
        glm::vec3(8.2180f, 12.5810f, -4.7f),

        glm::vec3(23.7700f, 12.5810f, 20.0f),
        glm::vec3(0.8734f, 12.5810f, 20.0f),
        glm::vec3(-22.0232f, 12.5810f, 20.0f),
        glm::vec3(36.809f, 12.5810f, 20.0f),
        glm::vec3(-36.809f, 12.5810f, 20.0f),
        glm::vec3(23.7700f, 12.5810f, -23.0f),
        glm::vec3(0.8734f, 12.5810f, -23.0f),
        glm::vec3(-22.0232f, 12.5810f, -23.0f),
        glm::vec3(36.809f, 12.5810f, -23.0f),
        glm::vec3(-36.809f, 12.5810f, -23.0f),

        // TERCER PISO (Y = 20.5786)
        glm::vec3(-15.2176f, 20.5786f, 8.7f),
        glm::vec3(-15.2176f, 20.5786f, -4.7f),
        glm::vec3(-5.2176f, 20.5786f, 8.7f),
        glm::vec3(-5.2176f, 20.5786f, -4.7f),
        glm::vec3(18.2180f, 20.5786f, 8.7f),
        glm::vec3(18.2180f, 20.5786f, -4.7f),
        glm::vec3(8.2180f, 20.5786f, 8.7f),
        glm::vec3(8.2180f, 20.5786f, -4.7f),

        glm::vec3(23.7700f, 20.5786f, 20.0f),
        glm::vec3(0.8734f, 20.5786f, 20.0f),
        glm::vec3(-22.0232f, 20.5786f, 20.0f),
        glm::vec3(36.809f, 20.5786f, 20.0f),
        glm::vec3(-36.809f, 20.5786f, 20.0f),
        glm::vec3(23.7700f, 20.5786f, -23.0f),
        glm::vec3(0.8734f, 20.5786f, -23.0f),
        glm::vec3(-22.0232f, 20.5786f, -23.0f),
        glm::vec3(36.809f, 20.5786f, -23.0f),
        glm::vec3(-36.809f, 20.5786f, -23.0f),
    };

    glm::vec3 lightOffsets[] = {
    glm::vec3(-1.7f, -2.00f, 0.0f),
    //glm::vec3(0.0f, -2.00f, 0.0f),
    glm::vec3(1.7f, -2.00f, 0.0f)
    };


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
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
        glClearColor(0.12f, 0.15f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        //ourShader.setVec3("lightPos",glm::vec3(0.0f, 15.0f, 5.0f));
        //ourShader.setVec3("lightColor",glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec3("viewPos",camera.Position);
        //ourShader.setVec3("objectColor",glm::vec3(0.45f, 0.45f, 0.50f));

        const int lampCount =sizeof(lampPositions) / sizeof(lampPositions[0]);

        const int lightsPerLamp =sizeof(lightOffsets) / sizeof(lightOffsets[0]);

        int lightIndex = 0;

        for (int i = 0; i < lampCount; i++)
        {
            for (int j = 0; j < lightsPerLamp; j++)
            {
                glm::vec3 currentLightPosition =
                    lampPositions[i] + lightOffsets[j];

                std::string base =
                    "pointLights[" + std::to_string(lightIndex) + "]";

                ourShader.setVec3(base + ".position",currentLightPosition);

                ourShader.setVec3(base + ".ambient",glm::vec3(0.025f, 0.025f, 0.020f));

                ourShader.setVec3(base + ".diffuse",glm::vec3(0.65f, 0.62f, 0.48f));

                ourShader.setVec3(base + ".specular",glm::vec3(0.80f, 0.78f, 0.65f));

                ourShader.setFloat(base + ".constant", 1.0f);
                ourShader.setFloat(base + ".linear", 0.14f);
                ourShader.setFloat(base + ".quadratic", 0.07f);

                lightIndex++;
            }
        }


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model
        /*glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        parkingModel.Draw(ourShader);*/

        // renderizar el estacionamiento
        glm::mat4 parking = glm::mat4(1.0f);
        parking = glm::translate(parking,glm::vec3(0.0f, 0.0f, 0.0f));
        parking = glm::scale(parking,glm::vec3(1.0f, 1.0f, 1.0f));

        ourShader.setMat4("model", parking);
        parkingModel.Draw(ourShader);


        for (int i = 0; i < lampCount; i++)
        {
            glm::mat4 lamp = glm::mat4(1.0f);

            lamp = glm::translate(lamp, lampPositions[i]);
            lamp = glm::scale(lamp, glm::vec3(1.0f));

            ourShader.setMat4("model", lamp);
            lightModel.Draw(ourShader);
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
void processInput(GLFWwindow* window)
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