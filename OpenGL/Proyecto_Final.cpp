#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <learnopengl/CollisionMesh.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void moveCameraSafely(Camera_Movement direction);

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

// Ciclo dia/noche
// Empieza en noche para conservar la apariencia original.
bool dayMode = false;
bool nKeyWasPressed = false;
float dayFactor = 0.0f; // 0.0 = noche, 1.0 = dia

// Colisión exacta contra los triángulos de parking.obj.
CollisionMesh parkingCollision;

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
    Shader nightSkyShader("shaders/night_sky.vs", "shaders/night_sky.fs");
    Shader floorShader("shaders/floor.vs", "shaders/floor.fs");

    // load models
    // -----------
    //Model ourModel(FileSystem::getPath("resources/objects/backpack/backpack.obj"));
    Model parkingModel("model/parking/parking.obj");
    Model lightModel("model/ceilinglight/ceilinglight.obj");
    //Model streetModel("model/street/street.obj");
    Model nightSkyModel("model/night_sky/night_sky.obj");

    // ================================================================
    // COLISIONES DE LA INFRAESTRUCTURA
    //
    // Se carga el mismo OBJ que se dibuja. Por eso las paredes, columnas,
    // pisos, rampas, barandas y demás piezas sólidas bloquean la cámara.
    // ================================================================
    if (!parkingCollision.loadFromObj("model/parking/parking.obj"))
    {
        std::cerr
            << "No se pudo cargar parking.obj para las colisiones.\n";
    }

    // El piso exterior generado por código también es sólido.
    parkingCollision.setGroundPlane(-5.30f);

    // El techo está formado por varias piezas y espacios decorativos.
    // Esta caja sólida impide atravesarlo tanto desde abajo como desde arriba.
    parkingCollision.addSolidBox(
        glm::vec3(0.0f, 31.86f, 0.0f),
        glm::vec3(93.90f, 11.20f, 69.50f)
    );

    //C:/Users/roma9/source/repos/Proyecto_Final_OpenGL/OpenGL/model/parking/parking.obj
    //Model ourModel("model/backpack/backpack.obj");

    camera.MovementSpeed = 30; //Optional. Modify the speed of the camera

    // ------------------------------------------------------------------
    // Piso generado por código: solo 2 triángulos y sin textura externa.
    // Se coloca debajo de los modelos para ocultar las estrellas inferiores.
    // ------------------------------------------------------------------
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
        reinterpret_cast<void*>(0)
    );
    glBindVertexArray(0);

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

        // Transicion suave entre ambos modos.
        const float targetDayFactor = dayMode ? 1.0f : 0.0f;
        const float transitionStep = 0.70f * deltaTime;

        if (dayFactor < targetDayFactor)
        {
            dayFactor += transitionStep;
            if (dayFactor > targetDayFactor)
                dayFactor = targetDayFactor;
        }
        else if (dayFactor > targetDayFactor)
        {
            dayFactor -= transitionStep;
            if (dayFactor < targetDayFactor)
                dayFactor = targetDayFactor;
        }

        // ============================================
        // 3. LIMPIAR PANTALLA
        // ============================================
        const glm::vec3 nightClear(0.005f, 0.008f, 0.020f);
        const glm::vec3 dayClear(0.45f, 0.70f, 0.95f);
        const glm::vec3 clearColor = glm::mix(nightClear, dayClear, dayFactor);

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ============================================
        // 4. MATRICES DE CÁMARA
        // ============================================
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            static_cast<float>(SCR_WIDTH) /
            static_cast<float>(SCR_HEIGHT),
            0.1f,
            5000.0f
        );

        glm::mat4 view = camera.GetViewMatrix();

        // ============================================
        // RENDERIZAR EL CIELO NOCTURNO
        // ============================================
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        nightSkyShader.use();

        // Conserva la rotación de la cámara,
        // pero elimina su desplazamiento.
        glm::mat4 skyView =
            glm::mat4(glm::mat3(camera.GetViewMatrix()));

        nightSkyShader.setMat4("projection", projection);
        nightSkyShader.setMat4("view", skyView);
        nightSkyShader.setFloat("dayFactor", dayFactor);

        glm::mat4 nightSky = glm::mat4(1.0f);

        // Prueba inicialmente con 20, no con 300.
        nightSky = glm::scale(
            nightSky,
            glm::vec3(20.0f)
        );

        nightSkyShader.setMat4("model", nightSky);
        nightSkyModel.Draw(nightSkyShader);

        // Restaurar estados de OpenGL
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);

        // ============================================
        // 6. RENDERIZAR PISO PROCEDURAL
        // ============================================
        floorShader.use();
        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);
        const glm::vec3 nightFloorColor(0.040f, 0.045f, 0.050f);
        const glm::vec3 dayFloorColor(0.180f, 0.190f, 0.200f);
        floorShader.setVec3(
            "floorColor",
            glm::mix(nightFloorColor, dayFloorColor, dayFactor)
        );

        glm::mat4 ground = glm::mat4(1.0f);
        ground = glm::translate(ground, glm::vec3(0.0f, -5.30f, 0.0f));
        ground = glm::scale(ground, glm::vec3(1000.0f, 1.0f, 1000.0f));
        floorShader.setMat4("model", ground);

        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // ============================================
        // 7. ACTIVAR SHADER PRINCIPAL
        // ============================================
        ourShader.use();

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setFloat("dayFactor", dayFactor);

        // ============================================
        // 8. CONFIGURAR LAS LUCES DE LAS LÁMPARAS
        // ============================================
        int lightIndex = 0;

        for (int i = 0; i < lampCount; i++)
        {
            for (int j = 0; j < lightsPerLamp; j++)
            {
                glm::vec3 currentLightPosition =
                    lampPositions[i] + lightOffsets[j];

                std::string base =
                    "pointLights[" +
                    std::to_string(lightIndex) +
                    "]";

                ourShader.setVec3(
                    base + ".position",
                    currentLightPosition
                );

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

                ourShader.setFloat(
                    base + ".constant",
                    1.0f
                );

                ourShader.setFloat(
                    base + ".linear",
                    0.14f
                );

                ourShader.setFloat(
                    base + ".quadratic",
                    0.07f
                );

                lightIndex++;
            }
        }

        // ============================================
        // 9. RENDERIZAR EL ESTACIONAMIENTO
        // ============================================
        glm::mat4 parking = glm::mat4(1.0f);

        parking = glm::translate(
            parking,
            glm::vec3(0.0f, 0.0f, 0.0f)
        );

        parking = glm::scale(
            parking,
            glm::vec3(1.0f)
        );

        ourShader.setMat4("model", parking);
        parkingModel.Draw(ourShader);

        // ============================================
        // 11. RENDERIZAR LAS LÁMPARAS
        // ============================================
        for (int i = 0; i < lampCount; i++)
        {
            glm::mat4 lamp = glm::mat4(1.0f);

            lamp = glm::translate(
                lamp,
                lampPositions[i]
            );

            lamp = glm::scale(
                lamp,
                glm::vec3(1.0f)
            );

            ourShader.setMat4("model", lamp);
            lightModel.Draw(ourShader);
        }

        // ============================================
        // 12. MOSTRAR EL FOTOGRAMA
        // ============================================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Liberar los buffers creados para el piso.
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// Mueve la cámara y después corrige su posición contra toda la malla
// del parqueadero. La resolución permite deslizarse junto a las paredes.
void moveCameraSafely(Camera_Movement direction)
{
    const glm::vec3 startPosition = camera.Position;

    camera.ProcessKeyboard(direction, deltaTime);

    const glm::vec3 desiredPosition = camera.Position;
    camera.Position = parkingCollision.moveCamera(
        startPosition,
        desiredPosition
    );
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // La tecla N alterna una sola vez por pulsacion.
    const bool nKeyIsPressed =
        glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;

    if (nKeyIsPressed && !nKeyWasPressed)
    {
        dayMode = !dayMode;

        if (dayMode)
        {
            glfwSetWindowTitle(
                window,
                "Smart Parking - DIA (N cambia modo)"
            );
            std::cout << "Modo DIA activado" << std::endl;
        }
        else
        {
            glfwSetWindowTitle(
                window,
                "Smart Parking - NOCHE (N cambia modo)"
            );
            std::cout << "Modo NOCHE activado" << std::endl;
        }
    }

    nKeyWasPressed = nKeyIsPressed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveCameraSafely(FORWARD);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveCameraSafely(BACKWARD);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveCameraSafely(LEFT);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveCameraSafely(RIGHT);
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