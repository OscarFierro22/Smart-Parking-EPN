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
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 8.0f, 95.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Control del ciclo día/noche
bool dayMode = true;
float dayFactor = 1.0f;

// Evita que N cambie muchas veces mientras se mantiene presionada
bool nKeyWasDown = false;

// Dimensiones reales del framebuffer
int framebufferWidth = static_cast<int>(SCR_WIDTH);
int framebufferHeight = static_cast<int>(SCR_HEIGHT);

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

    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // 
    // -------------------------
    Shader ourShader(
        "shaders/shader_project_mloading.vs",
        "shaders/shader_project_mloading.fs"
    );

    Shader skyShader(
        "shaders/procedural_sky.vs",
        "shaders/procedural_sky.fs"
    );

    Shader floorShader(
        "shaders/floor.vs",
        "shaders/floor.fs"
    );


    // load models
    // -----------
    //Model ourModel(FileSystem::getPath("resources/objects/backpack/backpack.obj"));
    Model parkingModel("model/parking/parking.obj");
    Model lightModel("model/ceilinglight/ceilinglight.obj");
    Model streetModel("model/street/street.obj");

    //C:/Users/roma9/source/repos/Proyecto_Final_OpenGL/OpenGL/model/parking/parking.obj
    //Model ourModel("model/backpack/backpack.obj");

    camera.MovementSpeed = 30.0f; //Optional. Modify the speed of the camera
    // =====================================================
// GEOMETRÍA DEL PISO PROCEDURAL
// =====================================================

// Plano de 2 x 2 unidades.
// Después será escalado hasta 1000 x 1000.
    const float floorVertices[] = {
        -1.0f, 0.0f, -1.0f,
         1.0f, 0.0f,  1.0f,
         1.0f, 0.0f, -1.0f,

        -1.0f, 0.0f, -1.0f,
        -1.0f, 0.0f,  1.0f,
         1.0f, 0.0f,  1.0f
    };

    unsigned int floorVAO = 0;
    unsigned int floorVBO = 0;

    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(floorVertices),
        floorVertices,
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        nullptr
    );

    glBindVertexArray(0);


    // =====================================================
    // VAO DEL CIELO PROCEDURAL
    // =====================================================

    // El shader crea los tres vértices usando gl_VertexID.
    // No requiere VBO.
    unsigned int skyVAO = 0;
    glGenVertexArrays(1, &skyVAO);

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


    const int lampCount = sizeof(lampPositions) / sizeof(lampPositions[0]);
    const int lightsPerLamp = sizeof(lightOffsets) / sizeof(lightOffsets[0]);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // ============================================
        // 1. TIEMPO POR FOTOGRAMA
        // ============================================
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ============================================
        // 2. ENTRADA DE TECLADO Y CÁMARA
        // ============================================
        processInput(window);
        // =====================================================
// TRANSICIÓN SUAVE ENTRE DÍA Y NOCHE
// =====================================================

        const float targetDayFactor = dayMode ? 1.0f : 0.0f;
        const float transitionSpeed = 0.75f * deltaTime;

        if (dayFactor < targetDayFactor)
        {
            dayFactor += transitionSpeed;

            if (dayFactor > targetDayFactor)
                dayFactor = targetDayFactor;
        }
        else if (dayFactor > targetDayFactor)
        {
            dayFactor -= transitionSpeed;

            if (dayFactor < targetDayFactor)
                dayFactor = targetDayFactor;
        }

        // ============================================
        // 3. LIMPIAR PANTALLA
        // ============================================
        const glm::vec3 nightClear(0.002f, 0.004f, 0.012f);
        const glm::vec3 dayClear(0.45f, 0.70f, 0.95f);
        const glm::vec3 clearColor = glm::mix(nightClear, dayClear, dayFactor);

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ============================================
        // 4. MATRICES DE CÁMARA
        // ============================================
        const float aspect =
            framebufferHeight > 0
            ? static_cast<float>(framebufferWidth) /
            static_cast<float>(framebufferHeight)
            : 1.0f;

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            aspect,
            0.03f,
            5000.0f
        );

        glm::mat4 view = camera.GetViewMatrix();

        // =====================================================
        // 5. RENDERIZAR EL CIELO PROCEDURAL
        // =====================================================
        // El cielo se dibuja primero, sin prueba de profundidad.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        skyShader.use();
        skyShader.setMat4("inverseProjection", glm::inverse(projection));

        // Eliminar la traslación hace que el cielo acompañe la rotación
        // de la cámara, pero permanezca siempre a una distancia infinita.
        const glm::mat4 rotationOnlyView = glm::mat4(glm::mat3(view));
        skyShader.setMat4("inverseView", glm::inverse(rotationOnlyView));
        skyShader.setFloat("dayFactor", dayFactor);
        skyShader.setFloat("timeSeconds", currentFrame);

        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        // =====================================================
        // 6. RENDERIZAR EL PISO PROCEDURAL
        // =====================================================
        floorShader.use();
        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);
        floorShader.setVec3(
            "floorColor",
            glm::mix(
                glm::vec3(0.035f, 0.040f, 0.047f),
                glm::vec3(0.17f, 0.18f, 0.19f),
                dayFactor
            )
        );

        glm::mat4 ground(1.0f);
        ground = glm::translate(ground, glm::vec3(0.0f, -5.30f, 0.0f));
        ground = glm::scale(ground, glm::vec3(1000.0f, 1.0f, 1000.0f));
        floorShader.setMat4("model", ground);

        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // =====================================================
        // 7. VOLVER A ACTIVAR EL SHADER DE LOS MODELOS
        // =====================================================
        // Este use() es indispensable. floorShader era el shader activo.
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("viewPos", camera.Position);

        // Es compatible con la versión final del shader. Si el uniform no
        // existe todavía, OpenGL ignora la asignación con ubicación -1.
        ourShader.setFloat("dayFactor", dayFactor);

        // =====================================================
        // 8. CONFIGURAR LAS LUCES DE LAS LÁMPARAS
        // =====================================================
        int lightIndex = 0;

        for (int i = 0; i < lampCount; ++i)
        {
            for (int j = 0; j < lightsPerLamp; ++j)
            {
                const glm::vec3 currentLightPosition =
                    lampPositions[i] + lightOffsets[j];

                const std::string base =
                    "pointLights[" + std::to_string(lightIndex) + "]";

                ourShader.setVec3(base + ".position", currentLightPosition);
                ourShader.setVec3(
                    base + ".ambient",
                    glm::vec3(0.01f, 0.01f, 0.015f)
                );
                ourShader.setVec3(
                    base + ".diffuse",
                    glm::vec3(1.0f, 0.85f, 0.55f)
                );
                ourShader.setVec3(
                    base + ".specular",
                    glm::vec3(1.0f, 0.90f, 0.65f)
                );
                ourShader.setFloat(base + ".constant", 1.0f);
                ourShader.setFloat(base + ".linear", 0.14f);
                ourShader.setFloat(base + ".quadratic", 0.07f);

                ++lightIndex;
            }
        }

        // =====================================================
        // 9. RENDERIZAR EL ESTACIONAMIENTO
        // =====================================================
        glm::mat4 parking(1.0f);
        ourShader.setMat4("model", parking);
        parkingModel.Draw(ourShader);

        // =====================================================
        // 10. RENDERIZAR LAS CALLES
        // =====================================================
        glm::mat4 streetMatrix(1.0f);
        streetMatrix = glm::translate(
            streetMatrix,
            glm::vec3(0.0f, 0.0f, 67.9f)
        );
        ourShader.setMat4("model", streetMatrix);
        streetModel.Draw(ourShader);

        // =====================================================
        // 11. RENDERIZAR LAS LÁMPARAS
        // =====================================================
        for (int i = 0; i < lampCount; ++i)
        {
            glm::mat4 lamp(1.0f);
            lamp = glm::translate(lamp, lampPositions[i]);
            ourShader.setMat4("model", lamp);
            lightModel.Draw(ourShader);
        }

        // ============================================
        // 11. MOSTRAR EL FOTOGRAMA
        // ============================================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);

    glDeleteVertexArrays(1, &skyVAO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const bool nKeyIsDown =
        glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;

    if (nKeyIsDown && !nKeyWasDown)
    {
        dayMode = !dayMode;

        std::cout
            << (dayMode
                ? "Modo dia activado\n"
                : "Modo noche activado\n");
    }

    nKeyWasDown = nKeyIsDown;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(
    GLFWwindow* window,
    int width,
    int height
)
{
    framebufferWidth = width;
    framebufferHeight = height;

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