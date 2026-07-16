#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include "Vehicle/Vehicle.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

namespace fs = std::filesystem;

#if defined(_WIN32)
extern "C"
{
    // Solicita la GPU dedicada de alto rendimiento en equipos NVIDIA/AMD.
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, Vehicle& vehicle);
void processGamepadInput(Vehicle& vehicle);
glm::mat4 getVehicleCameraView(const Vehicle& vehicle);
glm::vec3 getActiveCameraPosition(const Vehicle& vehicle);
void updateThirdPersonCameraReturn(float deltaTime);
void resetThirdPersonCameraOffset();
glm::vec3 getSimulationPosition();

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int framebufferWidth = static_cast<int>(SCR_WIDTH);
int framebufferHeight = static_cast<int>(SCR_HEIGHT);

Camera camera(glm::vec3(0.0f, 8.0f, 95.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// =========================================================
// VEHÍCULO Y CÁMARAS INTEGRADAS
// =========================================================
enum class CameraMode
{
    FREE = 1,
    THIRD_PERSON = 2,
    DRIVER = 3
};

CameraMode cameraMode = CameraMode::THIRD_PERSON;

float thirdPersonYawOffset = 0.0f;
float thirdPersonPitchOffset = 0.0f;
float lastThirdPersonMouseTime = 0.0f;

constexpr float THIRD_PERSON_RETURN_DELAY = 0.8f;
constexpr float THIRD_PERSON_RETURN_SPEED = 3.5f;
constexpr float THIRD_PERSON_MOUSE_SENSITIVITY = 0.18f;

bool leftBumperPressed = false;
bool rightBumperPressed = false;
bool key1Pressed = false;
bool key2Pressed = false;
bool key3Pressed = false;

bool cameraPositionKeyPressed = false;
bool vehiclePositionKeyPressed = false;

constexpr glm::vec3 VEHICLE_COLLISION_HALF_SIZE(1.25f, 0.85f, 2.30f);
Vehicle* controlledVehicle = nullptr;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool dayMode = false;
float dayFactor = 0.0f;
bool phoneVisible = true;
bool keyWasDown[GLFW_KEY_LAST + 1]{};

float noticeTimer = 0.0f;
std::string noticeTitle;
std::string noticeSubtitle;

constexpr int MAX_RENDERED_POINT_LIGHTS = 24;
constexpr float PI_VALUE = 3.14159265359f;

enum class SlotState
{
    Available,
    Occupied,
    Reserved,
    Disabled
};

enum class GuidanceTarget
{
    None,
    ParkingSlot,
    Exit
};

struct ParkingSlot
{
    std::string id;
    std::string floorId;
    int floorIndex;
    char row;
    int number;
    glm::vec3 position;
    float rotationYDegrees;
    SlotState state;
    int graphNode;
    int carType;
};

struct Edge
{
    int to;
    float weight;
};

struct ParkingGraph
{
    std::vector<glm::vec3> nodes;
    std::vector<std::vector<Edge>> adjacency;
    int entranceNode = -1;
    int exitNode = -1;
};

struct DijkstraResult
{
    std::vector<float> distance;
    std::vector<int> previous;
};

struct UIVertex
{
    glm::vec2 position;
    glm::vec4 color;
};

struct CarPreset
{
    std::string path;
    float scale;
    float rotationOffsetDegrees;
    glm::vec3 positionOffset;
    glm::vec4 fallbackColor;
};

struct CollisionBox
{
    glm::vec3 center;
    glm::vec3 halfSize;
    std::string name;
};

struct ScenePointLight
{
    glm::vec3 position;
    glm::vec3 color;
    float linear;
    float quadratic;
};

std::vector<ParkingSlot> parkingSlots;
ParkingGraph parkingGraph;
std::vector<int> activeRoute;
std::vector<int> occupiedHistory;
int reservedSlotIndex = -1;
int mainVehicleParkedSlotIndex = -1;
int nextSpawnSearchIndex = 0;
int selectedCarType = 0;
int selectedPhoneFloor = 0;
int selectedPhoneSlotOrdinal = 2;
GuidanceTarget guidanceTarget = GuidanceTarget::None;
bool noticeSuccessStyle = false;
std::mt19937 randomEngine{ std::random_device{}() };
double lastCommandCheckTime = 0.0;
double lastStateExportTime = 0.0;
std::string lastProcessedCommandId;
std::array<std::unique_ptr<Model>, 5> carModels;
std::array<bool, 5> carModelLoaded{ false, false, false, false, false };
std::vector<CollisionBox> staticCollisionBoxes;
constexpr glm::vec3 CAMERA_COLLISION_HALF_SIZE(0.65f, 1.15f, 0.65f);

const std::array<CarPreset, 5> carPresets = {
    CarPreset{ "", 1.0f, 0.0f, glm::vec3(0.0f), glm::vec4(0.10f, 0.48f, 0.96f, 1.0f) }, // Sedan azul
    CarPreset{ "", 1.0f, 0.0f, glm::vec3(0.0f), glm::vec4(0.95f, 0.16f, 0.12f, 1.0f) }, // Hatchback rojo
    CarPreset{ "", 1.0f, 0.0f, glm::vec3(0.0f), glm::vec4(0.96f, 0.70f, 0.08f, 1.0f) }, // SUV amarillo
    CarPreset{ "", 1.0f, 0.0f, glm::vec3(0.0f), glm::vec4(0.16f, 0.78f, 0.38f, 1.0f) }, // Pickup verde
    CarPreset{ "", 1.0f, 0.0f, glm::vec3(0.0f), glm::vec4(0.72f, 0.20f, 0.92f, 1.0f) }  // Deportivo morado
};

std::vector<glm::vec3> allLampLightPositions;

bool pressedOnce(GLFWwindow* window, int key);
std::vector<ParkingSlot> createParkingSlots();
ParkingGraph createParkingGraph(std::vector<ParkingSlot>& slots);
std::vector<CollisionBox> createStaticCollisionBoxes();
bool collidesWithStaticObstacle(const glm::vec3& center, const glm::vec3& halfSize);
glm::vec3 resolveCollisionMovement(
    const glm::vec3& start,
    const glm::vec3& desired,
    const glm::vec3& halfSize
);
glm::vec3 resolveVehicleMovement(
    const glm::vec3& start,
    const glm::vec3& desired,
    float vehicleRotationDegrees,
    bool& blocked
);
void moveCameraSafely(Camera_Movement direction);
int floorSlotCount(int floorIndex);
int slotGridOrdinal(const ParkingSlot& slot);
void addUndirectedEdge(ParkingGraph& graph, int a, int b);
void addDirectedEdge(ParkingGraph& graph, int from, int to);
DijkstraResult runDijkstra(const ParkingGraph& graph, int startNode);
std::vector<int> reconstructPath(const DijkstraResult& result, int destinationNode);
void reserveBestAvailableSlot();
void reserveSpecificSlot(const std::string& slotId);
void routeToExit();
void cancelCurrentReservation();
void releaseSlotById(const std::string& slotId);
int findSlotIndexById(const std::string& slotId);
int findNearestGraphNode(const glm::vec3& position);
int getSelectedPhoneSlotIndex();
void movePhoneSelection(int direction);
void simulateRandomOccupancy();
void spawnCarInNextAvailableSlot();
void removeLastSpawnedCar();
void completeReservedParking();
void updateAutomaticArrival();
void processExternalCommands();
std::string extractJsonString(const std::string& json, const std::string& key);
void exportParkingStateJson();
int floorFromWorldHeight(float y);
float calculateRemainingRouteDistance(const glm::vec3& vehiclePosition);
std::string currentNavigationInstruction(const glm::vec3& vehiclePosition);
void parkMainVehicleInReservedSlot();
void updateWindowTitle(GLFWwindow* window);
std::string slotStateName(SlotState state);
glm::vec4 slotStateColor(SlotState state);
void showNotice(
    const std::string& title,
    const std::string& subtitle,
    float duration = 3.0f,
    bool successStyle = false
);

void addRect(std::vector<UIVertex>& vertices, float x, float y, float width, float height, const glm::vec4& color);
void addText(std::vector<UIVertex>& vertices, const std::string& text, float x, float y, float scale, const glm::vec4& color);
const std::array<std::uint8_t, 7>& glyphForCharacter(char character);
void renderPhoneUI(Shader& uiShader, unsigned int uiVAO, unsigned int uiVBO);

void drawCube(
    Shader& shader,
    unsigned int cubeVAO,
    const glm::vec3& position,
    const glm::vec3& scale,
    float rotationYDegrees,
    const glm::vec4& color
)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationYDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, scale);

    shader.setMat4("model", model);
    shader.setVec4("objectColor", color);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawOrientedBox(
    Shader& shader,
    unsigned int cubeVAO,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& forwardInput,
    const glm::vec4& color
)
{
    if (glm::length(forwardInput) < 0.0001f)
        return;

    const glm::vec3 forward = glm::normalize(forwardInput);
    glm::vec3 referenceUp(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(forward, referenceUp)) > 0.98f)
        referenceUp = glm::vec3(1.0f, 0.0f, 0.0f);

    const glm::vec3 right = glm::normalize(glm::cross(referenceUp, forward));
    const glm::vec3 up = glm::normalize(glm::cross(forward, right));

    glm::mat4 orientation(1.0f);
    orientation[0] = glm::vec4(right, 0.0f);
    orientation[1] = glm::vec4(up, 0.0f);
    orientation[2] = glm::vec4(forward, 0.0f);

    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model *= orientation;
    model = glm::scale(model, scale);

    shader.setMat4("model", model);
    shader.setVec4("objectColor", color);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawArrow(
    Shader& shader,
    unsigned int cubeVAO,
    const glm::vec3& center,
    const glm::vec3& directionInput,
    const glm::vec4& color,
    float size,
    bool glow
)
{
    if (glm::length(directionInput) < 0.0001f)
        return;

    const glm::vec3 direction = glm::normalize(directionInput);
    glm::vec3 referenceUp(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(direction, referenceUp)) > 0.98f)
        referenceUp = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(referenceUp, direction));

    auto drawBetween = [&](const glm::vec3& startPoint,
                           const glm::vec3& endPoint,
                           float width,
                           float height,
                           const glm::vec4& drawColor)
    {
        const glm::vec3 segment = endPoint - startPoint;
        const float length = glm::length(segment);
        if (length < 0.0001f)
            return;
        drawOrientedBox(
            shader,
            cubeVAO,
            (startPoint + endPoint) * 0.5f,
            glm::vec3(width, height, length),
            segment,
            drawColor
        );
    };

    const glm::vec3 shaftStart = center - direction * (0.95f * size);
    const glm::vec3 shaftEnd = center + direction * (0.35f * size);
    const glm::vec3 tip = center + direction * (1.20f * size);
    const glm::vec3 headBase = center + direction * (0.18f * size);
    const glm::vec3 leftBase = headBase + right * (0.70f * size);
    const glm::vec3 rightBase = headBase - right * (0.70f * size);

    if (glow)
    {
        const glm::vec4 halo(color.r, color.g, color.b, 0.22f);
        drawBetween(shaftStart, shaftEnd, 0.42f * size, 0.10f, halo);
        drawBetween(leftBase, tip, 0.38f * size, 0.10f, halo);
        drawBetween(rightBase, tip, 0.38f * size, 0.10f, halo);
    }

    drawBetween(shaftStart, shaftEnd, 0.22f * size, 0.075f, color);
    drawBetween(leftBase, tip, 0.20f * size, 0.075f, color);
    drawBetween(rightBase, tip, 0.20f * size, 0.075f, color);
}

void drawFallbackCar(Shader& primitiveShader, unsigned int cubeVAO, const ParkingSlot& slot);
void drawParkingIndicators(Shader& primitiveShader, unsigned int cubeVAO, float currentTime);
void drawTrafficArrows(Shader& primitiveShader, unsigned int cubeVAO);
void drawRampSafetyWalls(Shader& primitiveShader, unsigned int cubeVAO);
void drawRestrictedCorridorBarriers(
    Shader& primitiveShader,
    unsigned int cubeVAO,
    float currentTime
);
void drawGuidanceRoute(
    Shader& primitiveShader,
    unsigned int routeVAO,
    unsigned int routeVBO,
    unsigned int cubeVAO,
    float currentTime
);

int main()
{
    if (!glfwInit())
    {
        std::cerr << "No se pudo inicializar GLFW.\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH,
        SCR_HEIGHT,
        "Smart Parking - PROYECTO FINAL",
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::cerr << "No se pudo crear la ventana GLFW.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "No se pudo inicializar GLAD.\n";
        glfwTerminate();
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader modelShader(
        "shaders/shader_project_mloading.vs",
        "shaders/shader_project_mloading.fs"
    );
    Shader nightSkyShader(
        "shaders/night_sky.vs",
        "shaders/night_sky.fs"
    );
    Shader floorShader(
        "shaders/floor.vs",
        "shaders/floor.fs"
    );
    Shader primitiveShader(
        "shaders/primitive.vs",
        "shaders/primitive.fs"
    );
    Shader uiShader(
        "shaders/ui.vs",
        "shaders/ui.fs"
    );

    Model parkingModel("model/parking/parking.obj");
    Model lightModel("model/ceilinglight/ceilinglight.obj");
    Model streetModel("model/street/street.obj");
    Model nightSkyModel("model/night_sky/night_sky.obj");

    // =====================================================
    // VEHÍCULO CONTROLABLE
    // =====================================================
    Vehicle vehicle(
        "model/Vehicle/Body.obj",
        "model/Vehicle/Wheel_FL.obj",
        "model/Vehicle/Wheel_FR.obj",
        "model/Vehicle/Wheel_BL.obj",
        "model/Vehicle/Wheel_BR.obj"
    );

    // El vehículo inicia sobre el pavimento exterior, antes de la rampa
    // principal. La altura coincide con el extremo inferior real de la rampa
    // más la separación de suspensión usada en Vehicle::updateRamp().
    constexpr float exteriorGroundY = -3.62221f;
    constexpr float vehicleSuspensionHeight = 0.10f;
    vehicle.setPosition(glm::vec3(12.20f, exteriorGroundY + vehicleSuspensionHeight, 52.50f));
    vehicle.setRotation(35.0f);
    // Escala medida con respecto al ancho real de las casillas.
    vehicle.setScale(1.25f);
    vehicle.setForwardOffset(145.0f);
    // Mantiene el punto mas bajo de las ruedas a la misma altura que la
    // calibracion original de escala 2.0, aunque el Mazda ahora use 1.25.
    vehicle.setModelPivotOffset(glm::vec3(0.0f, 0.3640f, 0.0f));
    controlledVehicle = &vehicle;

    // Los cinco autos estacionados se construyen solo con cubos; no cargan OBJ.

    camera.MovementSpeed = 30.0f;

    parkingSlots = createParkingSlots();
    parkingGraph = createParkingGraph(parkingSlots);
    staticCollisionBoxes = createStaticCollisionBoxes();

    const float floorVertices[] = {
        -1.0f, 0.0f, -1.0f,
         1.0f, 0.0f,  1.0f,
         1.0f, 0.0f, -1.0f,

        -1.0f, 0.0f, -1.0f,
        -1.0f, 0.0f,  1.0f,
         1.0f, 0.0f,  1.0f
    };

    const float cubeVertices[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
         0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
    };

    unsigned int floorVAO = 0;
    unsigned int floorVBO = 0;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    unsigned int routeVAO = 0;
    unsigned int routeVBO = 0;
    glGenVertexArrays(1, &routeVAO);
    glGenBuffers(1, &routeVBO);
    glBindVertexArray(routeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, routeVBO);
    glBufferData(GL_ARRAY_BUFFER, 1, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    unsigned int uiVAO = 0;
    unsigned int uiVBO = 0;
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, 1, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(UIVertex),
        reinterpret_cast<void*>(offsetof(UIVertex, position))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(UIVertex),
        reinterpret_cast<void*>(offsetof(UIVertex, color))
    );
    glBindVertexArray(0);

    const std::array<glm::vec3, 54> lampPositions = {
        glm::vec3(-15.2176f,  4.5834f,  8.7f), glm::vec3(-15.2176f,  4.5834f, -4.7f),
        glm::vec3( -5.2176f,  4.5834f,  8.7f), glm::vec3( -5.2176f,  4.5834f, -4.7f),
        glm::vec3( 18.2180f,  4.5834f,  8.7f), glm::vec3( 18.2180f,  4.5834f, -4.7f),
        glm::vec3(  8.2180f,  4.5834f,  8.7f), glm::vec3(  8.2180f,  4.5834f, -4.7f),
        glm::vec3( 23.7700f,  4.5834f, 20.0f), glm::vec3(  0.8734f,  4.5834f, 20.0f),
        glm::vec3(-22.0232f,  4.5834f, 20.0f), glm::vec3( 36.8090f,  4.5834f, 20.0f),
        glm::vec3(-36.8090f,  4.5834f, 20.0f), glm::vec3( 23.7700f,  4.5834f,-23.0f),
        glm::vec3(  0.8734f,  4.5834f,-23.0f), glm::vec3(-22.0232f,  4.5834f,-23.0f),
        glm::vec3( 36.8090f,  4.5834f,-23.0f), glm::vec3(-36.8090f,  4.5834f,-23.0f),

        glm::vec3(-15.2176f, 12.5810f,  8.7f), glm::vec3(-15.2176f, 12.5810f, -4.7f),
        glm::vec3( -5.2176f, 12.5810f,  8.7f), glm::vec3( -5.2176f, 12.5810f, -4.7f),
        glm::vec3( 18.2180f, 12.5810f,  8.7f), glm::vec3( 18.2180f, 12.5810f, -4.7f),
        glm::vec3(  8.2180f, 12.5810f,  8.7f), glm::vec3(  8.2180f, 12.5810f, -4.7f),
        glm::vec3( 23.7700f, 12.5810f, 20.0f), glm::vec3(  0.8734f, 12.5810f, 20.0f),
        glm::vec3(-22.0232f, 12.5810f, 20.0f), glm::vec3( 36.8090f, 12.5810f, 20.0f),
        glm::vec3(-36.8090f, 12.5810f, 20.0f), glm::vec3( 23.7700f, 12.5810f,-23.0f),
        glm::vec3(  0.8734f, 12.5810f,-23.0f), glm::vec3(-22.0232f, 12.5810f,-23.0f),
        glm::vec3( 36.8090f, 12.5810f,-23.0f), glm::vec3(-36.8090f, 12.5810f,-23.0f),

        glm::vec3(-15.2176f, 20.5786f,  8.7f), glm::vec3(-15.2176f, 20.5786f, -4.7f),
        glm::vec3( -5.2176f, 20.5786f,  8.7f), glm::vec3( -5.2176f, 20.5786f, -4.7f),
        glm::vec3( 18.2180f, 20.5786f,  8.7f), glm::vec3( 18.2180f, 20.5786f, -4.7f),
        glm::vec3(  8.2180f, 20.5786f,  8.7f), glm::vec3(  8.2180f, 20.5786f, -4.7f),
        glm::vec3( 23.7700f, 20.5786f, 20.0f), glm::vec3(  0.8734f, 20.5786f, 20.0f),
        glm::vec3(-22.0232f, 20.5786f, 20.0f), glm::vec3( 36.8090f, 20.5786f, 20.0f),
        glm::vec3(-36.8090f, 20.5786f, 20.0f), glm::vec3( 23.7700f, 20.5786f,-23.0f),
        glm::vec3(  0.8734f, 20.5786f,-23.0f), glm::vec3(-22.0232f, 20.5786f,-23.0f),
        glm::vec3( 36.8090f, 20.5786f,-23.0f), glm::vec3(-36.8090f, 20.5786f,-23.0f)
    };

    // El OBJ ceilinglight tiene los dos tubos fluorescentes alrededor de
    // Y=-0.446 respecto al origen del modelo. Un emisor puntual centrado unos
    // centimetros debajo de los tubos hace que la luz nazca exactamente desde
    // cada foco, en lugar de quedar flotando dos metros por debajo.
    constexpr glm::vec3 ceilingEmitterOffset(0.0f, -0.58f, 0.0f);

    allLampLightPositions.reserve(lampPositions.size());
    for (const glm::vec3& lampPosition : lampPositions)
    {
        allLampLightPositions.push_back(lampPosition + ceilingEmitterOffset);
    }

    exportParkingStateJson();
    updateWindowTitle(window);
    showNotice(
        "SMART PARKING",
        "G SIMULAR  J/L ELEGIR  K RUTA  E SALIDA",
        6.0f
    );

    while (!glfwWindowShouldClose(window))
    {
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, vehicle);

        // Mientras el Mazda está estacionado permanece inmóvil. El botón
        // Buscar salida (o la tecla E) libera la plaza antes de permitir
        // nuevamente los controles del vehículo.
        if (mainVehicleParkedSlotIndex < 0)
        {
            vehicle.processInput(window, deltaTime);
            processGamepadInput(vehicle);
        }
        else
        {
            vehicle.stop();
        }

        const glm::vec3 previousVehiclePosition = vehicle.getPosition();
        vehicle.update(deltaTime);

        // Colisión continua contra paredes, separadores, barreras y todos
        // los límites exteriores del edificio. El movimiento se resuelve por
        // ejes y con subpasos para impedir atravesar objetos a alta velocidad.
        bool vehicleBlocked = false;
        const glm::vec3 resolvedVehiclePosition = resolveVehicleMovement(
            previousVehiclePosition,
            vehicle.getPosition(),
            vehicle.getRotation(),
            vehicleBlocked
        );
        vehicle.setCurrentPosition(resolvedVehiclePosition);
        if (vehicleBlocked)
        {
            vehicle.stop();
        }

        updateThirdPersonCameraReturn(deltaTime);
        processExternalCommands();

        const float targetDayFactor = dayMode ? 1.0f : 0.0f;
        const float transitionSpeed = 0.75f * deltaTime;
        if (dayFactor < targetDayFactor)
            dayFactor = std::min(dayFactor + transitionSpeed, targetDayFactor);
        else if (dayFactor > targetDayFactor)
            dayFactor = std::max(dayFactor - transitionSpeed, targetDayFactor);

        if (noticeTimer > 0.0f)
            noticeTimer = std::max(0.0f, noticeTimer - deltaTime);

        updateAutomaticArrival();

        // Actualiza la app por Wi-Fi varias veces por segundo con la posición
        // exacta del Mazda, la ruta y los estados de las plazas.
        if (static_cast<double>(currentFrame) - lastStateExportTime >= 0.15)
        {
            lastStateExportTime = static_cast<double>(currentFrame);
            exportParkingStateJson();
        }

        updateWindowTitle(window);

        const glm::vec3 nightClear(0.002f, 0.004f, 0.012f);
        const glm::vec3 dayClear(0.45f, 0.70f, 0.95f);
        const glm::vec3 clearColor = glm::mix(nightClear, dayClear, dayFactor);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = framebufferHeight > 0
            ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight)
            : 1.0f;

        const glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            aspect,
            0.1f,
            5000.0f
        );
        const glm::mat4 view = getVehicleCameraView(vehicle);

        // Cielo nocturno OBJ original con textura estrellada.
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        nightSkyShader.use();
        const glm::mat4 skyView = glm::mat4(glm::mat3(view));
        nightSkyShader.setMat4("projection", projection);
        nightSkyShader.setMat4("view", skyView);
        nightSkyShader.setFloat("dayFactor", dayFactor);
        glm::mat4 nightSky(1.0f);
        nightSky = glm::scale(nightSky, glm::vec3(20.0f));
        nightSkyShader.setMat4("model", nightSky);
        nightSkyModel.Draw(nightSkyShader);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);

        // Piso procedural.
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

        // Shader de modelos. Solo se envian las 24 luces mas cercanas a la camara.
        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        const glm::vec3 activeCameraPosition = getActiveCameraPosition(vehicle);
        modelShader.setVec3("viewPos", activeCameraPosition);
        modelShader.setFloat("dayFactor", dayFactor);

        // Las luces se seleccionan en dos grupos para impedir que los muchos
        // indicadores de plazas desplacen a todos los focos del techo. Se
        // reservan 18 emisores para techo y 6 para estados de parqueadero.
        const float ceilingPower = glm::mix(2.35f, 0.38f, dayFactor);
        std::vector<ScenePointLight> ceilingLights;
        ceilingLights.reserve(allLampLightPositions.size());
        for (const glm::vec3& position : allLampLightPositions)
        {
            ceilingLights.push_back({
                position,
                glm::vec3(1.00f, 0.92f, 0.76f) * ceilingPower,
                0.085f,
                0.032f
            });
        }

        std::vector<ScenePointLight> slotLights;
        slotLights.reserve(parkingSlots.size());
        for (const ParkingSlot& slot : parkingSlots)
        {
            const float aisleOffsetZ = slot.row == 'A' ? -4.15f : 4.15f;
            const glm::vec4 stateColor = slotStateColor(slot.state);
            slotLights.push_back({
                slot.position + glm::vec3(0.0f, 2.75f, aisleOffsetZ),
                glm::vec3(stateColor) * 1.35f,
                0.20f,
                0.14f
            });
        }

        auto nearestFromGroup = [&](const std::vector<ScenePointLight>& source,
                                    std::size_t maximumCount)
        {
            std::vector<std::pair<float, ScenePointLight>> sorted;
            sorted.reserve(source.size());
            for (const ScenePointLight& light : source)
            {
                const glm::vec3 difference = light.position - activeCameraPosition;
                sorted.emplace_back(glm::dot(difference, difference), light);
            }

            const std::size_t count = std::min(maximumCount, sorted.size());
            if (count > 0)
            {
                std::partial_sort(
                    sorted.begin(),
                    sorted.begin() + count,
                    sorted.end(),
                    [](const auto& left, const auto& right)
                    {
                        return left.first < right.first;
                    }
                );
            }
            sorted.resize(count);
            return sorted;
        };

        constexpr std::size_t ceilingLightQuota = 18;
        constexpr std::size_t slotLightQuota =
            static_cast<std::size_t>(MAX_RENDERED_POINT_LIGHTS) - ceilingLightQuota;

        const auto nearestCeilingLights =
            nearestFromGroup(ceilingLights, ceilingLightQuota);
        const auto nearestSlotLights =
            nearestFromGroup(slotLights, slotLightQuota);

        std::vector<ScenePointLight> activeLights;
        activeLights.reserve(MAX_RENDERED_POINT_LIGHTS);
        for (const auto& entry : nearestCeilingLights)
            activeLights.push_back(entry.second);
        for (const auto& entry : nearestSlotLights)
            activeLights.push_back(entry.second);

        const int activeLightCount = static_cast<int>(activeLights.size());
        modelShader.setInt("activePointLights", activeLightCount);

        for (int i = 0; i < activeLightCount; ++i)
        {
            const ScenePointLight& light = activeLights[static_cast<std::size_t>(i)];
            const std::string base = "pointLights[" + std::to_string(i) + "]";
            modelShader.setVec3(base + ".position", light.position);
            modelShader.setVec3(base + ".ambient", light.color * 0.040f);
            modelShader.setVec3(base + ".diffuse", light.color);
            modelShader.setVec3(
                base + ".specular",
                glm::min(light.color * 1.10f, glm::vec3(3.0f))
            );
            modelShader.setFloat(base + ".constant", 1.0f);
            modelShader.setFloat(base + ".linear", light.linear);
            modelShader.setFloat(base + ".quadratic", light.quadratic);
        }

        glm::mat4 modelMatrix(1.0f);
        modelShader.setFloat("emissiveStrength", 1.0f);
        modelShader.setMat4("model", modelMatrix);
        parkingModel.Draw(modelShader);

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 67.9f));
        modelShader.setMat4("model", modelMatrix);
        streetModel.Draw(modelShader);

        // Los tubos del modelo usan material emisivo. De noche brillan y
        // durante el dia permanecen visibles con intensidad reducida.
        modelShader.setFloat(
            "emissiveStrength",
            glm::mix(4.5f, 0.70f, dayFactor)
        );
        for (const glm::vec3& lampPosition : lampPositions)
        {
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, lampPosition);
            modelShader.setMat4("model", modelMatrix);
            lightModel.Draw(modelShader);
        }
        modelShader.setFloat("emissiveStrength", 1.0f);

        // Carros ocupando espacios.
        for (const ParkingSlot& slot : parkingSlots)
        {
            if (slot.state != SlotState::Occupied)
                continue;

            const int carType = std::clamp(slot.carType, 0, static_cast<int>(carPresets.size()) - 1);
            if (carModelLoaded[carType] && carModels[carType])
            {
                const CarPreset& preset = carPresets[carType];
                glm::mat4 carMatrix(1.0f);
                carMatrix = glm::translate(carMatrix, slot.position + preset.positionOffset);
                carMatrix = glm::rotate(
                    carMatrix,
                    glm::radians(slot.rotationYDegrees + preset.rotationOffsetDegrees),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
                carMatrix = glm::scale(carMatrix, glm::vec3(preset.scale));
                modelShader.setMat4("model", carMatrix);
                carModels[carType]->Draw(modelShader);
            }
        }

        // Vehículo controlable: usa el mismo shader de modelos y las mismas luces.
        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        modelShader.setVec3("viewPos", activeCameraPosition);
        modelShader.setFloat("dayFactor", dayFactor);
        glDisable(GL_CULL_FACE);
        vehicle.draw(modelShader);
        glEnable(GL_CULL_FACE);

        primitiveShader.use();
        primitiveShader.setMat4("projection", projection);
        primitiveShader.setMat4("view", view);

        for (std::size_t slotIndex = 0; slotIndex < parkingSlots.size(); ++slotIndex)
        {
            const ParkingSlot& slot = parkingSlots[slotIndex];
            if (slot.state == SlotState::Occupied &&
                static_cast<int>(slotIndex) != mainVehicleParkedSlotIndex)
            {
                drawFallbackCar(primitiveShader, cubeVAO, slot);
            }
        }

        drawRampSafetyWalls(primitiveShader, cubeVAO);
        drawRestrictedCorridorBarriers(primitiveShader, cubeVAO, currentFrame);
        drawTrafficArrows(primitiveShader, cubeVAO);
        drawParkingIndicators(primitiveShader, cubeVAO, currentFrame);
        drawGuidanceRoute(primitiveShader, routeVAO, routeVBO, cubeVAO, currentFrame);

        if (phoneVisible || noticeTimer > 0.0f)
            renderPhoneUI(uiShader, uiVAO, uiVBO);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &routeVAO);
    glDeleteBuffers(1, &routeVBO);
    glDeleteVertexArrays(1, &uiVAO);
    glDeleteBuffers(1, &uiVBO);

    glfwTerminate();
    return 0;
}

bool pressedOnce(GLFWwindow* window, int key)
{
    const bool isDown = glfwGetKey(window, key) == GLFW_PRESS;
    const bool pressed = isDown && !keyWasDown[key];
    keyWasDown[key] = isDown;
    return pressed;
}

void processInput(GLFWwindow* window, Vehicle& vehicle)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (pressedOnce(window, GLFW_KEY_N))
    {
        // Se conserva el mismo OBJ del cielo estrellado. El shader cambia su
        // apariencia entre noche y dia sin sustituir el modelo del escenario.
        dayMode = !dayMode;
        showNotice(
            dayMode ? "MODO DIA" : "MODO NOCHE",
            dayMode
                ? "LUZ SOLAR ACTIVA - FOCOS DE TECHO EN BAJA INTENSIDAD"
                : "FOCOS DE TECHO ENCENDIDOS",
            2.5f
        );
        updateWindowTitle(window);
    }

    // G genera una nueva ocupacion aleatoria solo en espacios reales.
    if (pressedOnce(window, GLFW_KEY_G))
    {
        simulateRandomOccupancy();
        updateWindowTitle(window);
    }

    if (pressedOnce(window, GLFW_KEY_C))
    {
        spawnCarInNextAvailableSlot();
        updateWindowTitle(window);
    }

    if (pressedOnce(window, GLFW_KEY_X))
    {
        removeLastSpawnedCar();
        updateWindowTitle(window);
    }

    if (pressedOnce(window, GLFW_KEY_V))
    {
        selectedCarType = (selectedCarType + 1) % static_cast<int>(carPresets.size());
        static const std::array<std::string, 5> carTypeNames = {
            "SEDAN", "HATCHBACK", "SUV", "PICKUP", "DEPORTIVO"
        };
        showNotice(
            "TIPO DE CARRO " + std::to_string(selectedCarType + 1),
            carTypeNames[selectedCarType] + " CONSTRUIDO CON CUBOS",
            2.5f
        );
    }

    if (pressedOnce(window, GLFW_KEY_Q))
    {
        reserveBestAvailableSlot();
        updateWindowTitle(window);
    }

    if (pressedOnce(window, GLFW_KEY_F))
    {
        selectedPhoneFloor = (selectedPhoneFloor + 1) % 4;
        selectedPhoneSlotOrdinal = 0;
        movePhoneSelection(0);
        const std::array<std::string, 4> floorNames = {
            "PLANTA BAJA", "PISO 1", "PISO 2", "PISO 3"
        };
        showNotice("APP: " + floorNames[selectedPhoneFloor], "J/L SELECCIONA UN ESPACIO", 2.0f);
    }

    if (pressedOnce(window, GLFW_KEY_J))
        movePhoneSelection(-1);

    if (pressedOnce(window, GLFW_KEY_L))
        movePhoneSelection(1);

    if (pressedOnce(window, GLFW_KEY_K))
    {
        const int selectedIndex = getSelectedPhoneSlotIndex();
        if (selectedIndex >= 0)
            reserveSpecificSlot(parkingSlots[selectedIndex].id);
        updateWindowTitle(window);
    }

    if (pressedOnce(window, GLFW_KEY_E))
    {
        routeToExit();
        updateWindowTitle(window);
    }

    if (pressedOnce(window, GLFW_KEY_M))
    {
        completeReservedParking();
        updateWindowTitle(window);
    }

    if (pressedOnce(window, GLFW_KEY_T))
        phoneVisible = !phoneVisible;

    // =====================================================
    // CÁMARA LIBRE: flechas. WASD queda reservado al vehículo.
    // =====================================================
    if (cameraMode == CameraMode::FREE)
    {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            moveCameraSafely(FORWARD);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            moveCameraSafely(BACKWARD);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            moveCameraSafely(LEFT);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            moveCameraSafely(RIGHT);
    }

    // Cambio de cámara con 1, 2 y 3.
    const bool key1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    const bool key2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    const bool key3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;

    if (key1 && !key1Pressed)
    {
        cameraMode = CameraMode::FREE;
        resetThirdPersonCameraOffset();

        const glm::vec3 vehiclePosition = vehicle.getPosition();
        const glm::vec3 forward = vehicle.getForwardVector();
        camera.Position =
            vehiclePosition - forward * 8.0f + glm::vec3(0.0f, 4.0f, 0.0f);

        firstMouse = true;
        std::cout << "Camara libre activada.\n";
    }

    if (key2 && !key2Pressed)
    {
        cameraMode = CameraMode::THIRD_PERSON;
        resetThirdPersonCameraOffset();
        std::cout << "Camara tercera persona activada.\n";
    }

    if (key3 && !key3Pressed)
    {
        cameraMode = CameraMode::DRIVER;
        resetThirdPersonCameraOffset();
        std::cout << "Camara conductor activada.\n";
    }

    key1Pressed = key1;
    key2Pressed = key2;
    key3Pressed = key3;

    const bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pPressed && !cameraPositionKeyPressed)
    {
        const glm::vec3 activePosition = getActiveCameraPosition(vehicle);
        std::cout
            << "\n========== CAMERA POSITION ==========\n"
            << "X: " << activePosition.x << "\n"
            << "Y: " << activePosition.y << "\n"
            << "Z: " << activePosition.z << "\n"
            << "=====================================\n";
    }
    cameraPositionKeyPressed = pPressed;

    const bool oPressed = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
    if (oPressed && !vehiclePositionKeyPressed)
    {
        const glm::vec3 position = vehicle.getPosition();
        std::cout
            << "\n========== VEHICLE POSITION =========\n"
            << "X: " << position.x << "\n"
            << "Y: " << position.y << "\n"
            << "Z: " << position.z << "\n"
            << "=====================================\n";
    }
    vehiclePositionKeyPressed = oPressed;
}


std::vector<ParkingSlot> createParkingSlots()
{
    // Centros obtenidos de las lineas blancas del OBJ.
    const std::array<float, 22> standardCenters = {
        -40.7881f, -37.0486f, -33.3092f, -29.5697f, -25.8303f, -22.0368f,
        -18.2433f, -14.5039f, -10.7644f,  -7.0250f,  -3.2855f,   0.8156f,
          4.9014f,   8.6408f,  12.3957f,  16.1351f,  19.8746f,  24.1426f,
         28.4106f,  32.1501f,  35.8895f,  39.6290f
    };

    const std::array<float, 4> floorSurfaceY = {
        -2.7509f, 5.2563f, 13.2589f, 21.2597f
    };
    const std::array<std::string, 4> floorIds = {
        "PB", "P1", "P2", "P3"
    };

    constexpr float northRowZ = 28.9350f;
    constexpr float southRowZ = -28.6050f;

    std::vector<ParkingSlot> slots;
    slots.reserve(169);

    auto addSlot = [&](int floor, char row, int number, float x)
    {
        std::ostringstream id;
        id << floorIds[floor] << '-' << row
           << std::setw(2) << std::setfill('0') << number;

        const SlotState initialState =
            (row == 'A' && number <= 2)
            ? SlotState::Disabled
            : SlotState::Available;

        const float z = row == 'A' ? northRowZ : southRowZ;
        const float rotation = row == 'A' ? 0.0f : 180.0f;
        slots.push_back({
            id.str(),
            floorIds[floor],
            floor,
            row,
            number,
            glm::vec3(x, floorSurfaceY[floor] + 0.28f, z),
            rotation,
            initialState,
            -1,
            0
        });
    };

    for (int floor = 0; floor < 4; ++floor)
    {
        // En PB la entrada ocupa PB-A12 hasta PB-A18. El OBJ no contiene
        // lineas de estacionamiento en ese sector, por eso nunca se generan
        // carros dentro del acceso principal.
        for (int number = 1; number <= 22; ++number)
        {
            const bool entranceGap = floor == 0 && number >= 12 && number <= 18;
            if (!entranceGap)
                addSlot(floor, 'A', number, standardCenters[number - 1]);
        }

        for (int number = 1; number <= 22; ++number)
            addSlot(floor, 'B', number, standardCenters[number - 1]);
    }

    return slots;
}



ParkingGraph createParkingGraph(std::vector<ParkingSlot>& slots)
{
    const std::array<float, 22> xCenters = {
        -40.7881f, -37.0486f, -33.3092f, -29.5697f, -25.8303f, -22.0368f,
        -18.2433f, -14.5039f, -10.7644f,  -7.0250f,  -3.2855f,   0.8156f,
          4.9014f,   8.6408f,  12.3957f,  16.1351f,  19.8746f,  24.1426f,
         28.4106f,  32.1501f,  35.8895f,  39.6290f
    };
    const std::array<float, 4> floorSurfaceY = {
        -2.7509f, 5.2563f, 13.2589f, 21.2597f
    };

    constexpr float northLaneZ = 20.0f;
    constexpr float southLaneZ = -20.0f;
    constexpr float leftRampX = -37.12f;
    constexpr float rightRampX = 37.10f;
    constexpr float leftCorridorX = -10.68f;  // entre muro 1 y muro 2
    constexpr float rightCorridorX = 12.45f;  // entre muro 2 y muro 3

    ParkingGraph graph;

    auto addNode = [&graph](const glm::vec3& position)
    {
        const int index = static_cast<int>(graph.nodes.size());
        graph.nodes.push_back(position);
        graph.adjacency.emplace_back();
        return index;
    };

    auto nearestColumnForX = [&xCenters](float x)
    {
        int nearest = 0;
        float nearestDistance = std::numeric_limits<float>::infinity();
        for (int column = 0; column < 22; ++column)
        {
            const float distance = std::abs(xCenters[column] - x);
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = column;
            }
        }
        return nearest;
    };

    // Acceso frontal. La puerta principal real está entre X=1.55 y X=23.35.
    // Usamos el mismo centro para la ruta, el círculo de misión y la detección
    // de llegada, evitando que la salida aparezca desplazada hacia la derecha.
    constexpr float mainGateCenterX = 12.45f;
    constexpr float mainGateRouteZ = 47.0f;

    graph.entranceNode = addNode(
        glm::vec3(mainGateCenterX, floorSurfaceY[0] + 0.42f, mainGateRouteZ)
    );
    graph.exitNode = addNode(
        glm::vec3(mainGateCenterX, floorSurfaceY[0] + 0.42f, mainGateRouteZ)
    );

    std::array<std::array<int, 22>, 4> northLane{};
    std::array<std::array<int, 22>, 4> southLane{};
    std::array<std::array<int, 3>, 4> centralCorridorLeft{};
    std::array<std::array<int, 3>, 4> centralCorridorRight{};
    std::array<int, 4> leftRampSouth{};
    std::array<int, 4> leftRampNorth{};
    std::array<int, 4> rightRampSouth{};
    std::array<int, 4> rightRampNorth{};

    for (int floor = 0; floor < 4; ++floor)
    {
        const float routeY = floorSurfaceY[floor] + 0.42f;

        // Dos calles longitudinales, una frente a cada fila de parqueaderos.
        for (int column = 0; column < 22; ++column)
        {
            northLane[floor][column] = addNode(glm::vec3(xCenters[column], routeY, northLaneZ));
            southLane[floor][column] = addNode(glm::vec3(xCenters[column], routeY, southLaneZ));

            if (column > 0)
            {
                addUndirectedEdge(graph, northLane[floor][column - 1], northLane[floor][column]);
                addUndirectedEdge(graph, southLane[floor][column - 1], southLane[floor][column]);
            }
        }

        // Solo estos dos pasillos atraviesan la zona de muros. Los corredores
        // exteriores junto a las rampas quedan cerrados por barreras.
        const std::array<float, 3> corridorZ = { southLaneZ, 0.0f, northLaneZ };
        for (int point = 0; point < 3; ++point)
        {
            centralCorridorLeft[floor][point] = addNode(
                glm::vec3(leftCorridorX, routeY, corridorZ[point])
            );
            centralCorridorRight[floor][point] = addNode(
                glm::vec3(rightCorridorX, routeY, corridorZ[point])
            );

            if (point > 0)
            {
                addUndirectedEdge(
                    graph,
                    centralCorridorLeft[floor][point - 1],
                    centralCorridorLeft[floor][point]
                );
                addUndirectedEdge(
                    graph,
                    centralCorridorRight[floor][point - 1],
                    centralCorridorRight[floor][point]
                );
            }
        }

        const int leftColumn = nearestColumnForX(leftCorridorX);
        const int rightColumn = nearestColumnForX(rightCorridorX);
        addUndirectedEdge(graph, southLane[floor][leftColumn], centralCorridorLeft[floor][0]);
        addUndirectedEdge(graph, northLane[floor][leftColumn], centralCorridorLeft[floor][2]);
        addUndirectedEdge(graph, southLane[floor][rightColumn], centralCorridorRight[floor][0]);
        addUndirectedEdge(graph, northLane[floor][rightColumn], centralCorridorRight[floor][2]);

        // Centros reales de las rampas del OBJ.
        leftRampSouth[floor] = addNode(glm::vec3(leftRampX, routeY, southLaneZ));
        leftRampNorth[floor] = addNode(glm::vec3(leftRampX, routeY, northLaneZ));
        rightRampSouth[floor] = addNode(glm::vec3(rightRampX, routeY, southLaneZ));
        rightRampNorth[floor] = addNode(glm::vec3(rightRampX, routeY, northLaneZ));

        const int leftRampColumn = nearestColumnForX(leftRampX);
        const int rightRampColumn = nearestColumnForX(rightRampX);
        addUndirectedEdge(graph, southLane[floor][leftRampColumn], leftRampSouth[floor]);
        addUndirectedEdge(graph, northLane[floor][leftRampColumn], leftRampNorth[floor]);
        addUndirectedEdge(graph, southLane[floor][rightRampColumn], rightRampSouth[floor]);
        addUndirectedEdge(graph, northLane[floor][rightRampColumn], rightRampNorth[floor]);
    }

    addUndirectedEdge(
        graph,
        graph.entranceNode,
        northLane[0][nearestColumnForX(mainGateCenterX)]
    );
    addUndirectedEdge(
        graph,
        graph.exitNode,
        northLane[0][nearestColumnForX(mainGateCenterX)]
    );

    // La geometria izquierda sube desde Z positivo hacia Z negativo.
    // La geometria derecha baja desde Z positivo hacia Z negativo.
    for (int floor = 0; floor < 3; ++floor)
    {
        addDirectedEdge(graph, leftRampNorth[floor], leftRampSouth[floor + 1]);
        addDirectedEdge(graph, rightRampNorth[floor + 1], rightRampSouth[floor]);
    }

    for (ParkingSlot& slot : slots)
    {
        glm::vec3 nodePosition = slot.position;
        nodePosition.y = floorSurfaceY[slot.floorIndex] + 0.42f;
        slot.graphNode = addNode(nodePosition);

        const int column = std::clamp(slot.number - 1, 0, 21);
        const int laneNode = slot.row == 'A'
            ? northLane[slot.floorIndex][column]
            : southLane[slot.floorIndex][column];
        addUndirectedEdge(graph, laneNode, slot.graphNode);
    }

    return graph;
}



std::vector<CollisionBox> createStaticCollisionBoxes()
{
    std::vector<CollisionBox> boxes;

    auto addBox = [&](const glm::vec3& center, const glm::vec3& fullSize, const std::string& name)
    {
        boxes.push_back({ center, fullSize * 0.5f, name });
    };

    // Tres muros originales del OBJ. Los espacios ENTRE estos muros siguen libres.
    addBox(glm::vec3(-22.4734f, 9.0770f, 1.3645f), glm::vec3(0.50f, 24.20f, 20.90f), "MURO_CENTRAL_IZQUIERDO");
    addBox(glm::vec3(0.7847f, 9.0770f, 1.3645f), glm::vec3(0.50f, 24.20f, 20.90f), "MURO_CENTRAL_MEDIO");
    addBox(glm::vec3(24.1110f, 9.0770f, 1.3645f), glm::vec3(0.50f, 24.20f, 20.90f), "MURO_CENTRAL_DERECHO");

    const std::array<float, 4> floorSurfaceY = {
        -2.7509f, 5.2563f, 13.2589f, 21.2597f
    };

    for (int floor = 0; floor < 4; ++floor)
    {
        const float baseY = floorSurfaceY[floor];

        // Separadores colocados en el borde interior real de cada rampa.
        addBox(glm::vec3(-30.72f, baseY + 1.65f, 0.0f), glm::vec3(0.70f, 3.30f, 25.0f), "MURO_RAMPA_IZQUIERDA");
        addBox(glm::vec3(30.72f, baseY + 1.65f, 0.0f), glm::vec3(0.70f, 3.30f, 25.0f), "MURO_RAMPA_DERECHA");

        // Barreras en los corredores exteriores: rampa-muro. No se bloquean
        // los dos pasillos centrales situados entre muro y muro.
        addBox(glm::vec3(-26.92f, baseY + 1.35f, 0.0f), glm::vec3(7.10f, 2.70f, 1.20f), "BARRERA_EXTERIOR_IZQUIERDA");
        addBox(glm::vec3(27.72f, baseY + 1.35f, 0.0f), glm::vec3(6.30f, 2.70f, 1.20f), "BARRERA_EXTERIOR_DERECHA");

        // ================================================================
        // LÍMITES FÍSICOS DEL EDIFICIO
        // ================================================================
        // Las barandas y bordes del OBJ son visibles, pero no poseen una
        // colisión automática. Estas cajas invisibles cierran cada nivel y
        // dejan abierta únicamente la puerta principal de la planta baja.
        constexpr float perimeterHalfHeight = 1.90f;
        constexpr float perimeterThickness = 1.20f;
        constexpr float westX = -45.05f;
        constexpr float eastX = 45.05f;
        constexpr float southZ = -33.55f;
        constexpr float northZ = 33.55f;
        constexpr float buildingWidth = eastX - westX;
        constexpr float buildingDepth = northZ - southZ;

        const float perimeterCenterY = baseY + perimeterHalfHeight;

        // Costados y parte posterior: cerrados en los cuatro niveles.
        addBox(
            glm::vec3(westX, perimeterCenterY, 0.0f),
            glm::vec3(
                perimeterThickness,
                perimeterHalfHeight * 2.0f,
                buildingDepth + perimeterThickness * 2.0f
            ),
            "LIMITE_OESTE_PISO_" + std::to_string(floor)
        );
        addBox(
            glm::vec3(eastX, perimeterCenterY, 0.0f),
            glm::vec3(
                perimeterThickness,
                perimeterHalfHeight * 2.0f,
                buildingDepth + perimeterThickness * 2.0f
            ),
            "LIMITE_ESTE_PISO_" + std::to_string(floor)
        );
        addBox(
            glm::vec3(0.0f, perimeterCenterY, southZ),
            glm::vec3(
                buildingWidth + perimeterThickness * 2.0f,
                perimeterHalfHeight * 2.0f,
                perimeterThickness
            ),
            "LIMITE_SUR_PISO_" + std::to_string(floor)
        );

        if (floor == 0)
        {
            // La puerta principal queda abierta entre estos dos valores.
            constexpr float entranceLeftX = 1.55f;
            constexpr float entranceRightX = 23.35f;

            const float leftSegmentWidth = entranceLeftX - westX;
            const float rightSegmentWidth = eastX - entranceRightX;

            addBox(
                glm::vec3(
                    westX + leftSegmentWidth * 0.5f,
                    perimeterCenterY,
                    northZ
                ),
                glm::vec3(
                    leftSegmentWidth,
                    perimeterHalfHeight * 2.0f,
                    perimeterThickness
                ),
                "LIMITE_NORTE_PB_IZQUIERDO"
            );
            addBox(
                glm::vec3(
                    entranceRightX + rightSegmentWidth * 0.5f,
                    perimeterCenterY,
                    northZ
                ),
                glm::vec3(
                    rightSegmentWidth,
                    perimeterHalfHeight * 2.0f,
                    perimeterThickness
                ),
                "LIMITE_NORTE_PB_DERECHO"
            );

            // Canal que conduce a la puerta. Impide salir cortando por los
            // costados, pero deja libre el extremo frontal para entrar/salir.
            constexpr float entranceEndZ = 49.25f;
            const float entranceDepth = entranceEndZ - northZ;
            const float entranceCenterZ = northZ + entranceDepth * 0.5f;

            addBox(
                glm::vec3(entranceLeftX, perimeterCenterY, entranceCenterZ),
                glm::vec3(
                    perimeterThickness,
                    perimeterHalfHeight * 2.0f,
                    entranceDepth + perimeterThickness
                ),
                "CANAL_ENTRADA_IZQUIERDO"
            );
            addBox(
                glm::vec3(entranceRightX, perimeterCenterY, entranceCenterZ),
                glm::vec3(
                    perimeterThickness,
                    perimeterHalfHeight * 2.0f,
                    entranceDepth + perimeterThickness
                ),
                "CANAL_ENTRADA_DERECHO"
            );
        }
        else
        {
            // Los niveles superiores no tienen una salida exterior.
            addBox(
                glm::vec3(0.0f, perimeterCenterY, northZ),
                glm::vec3(
                    buildingWidth + perimeterThickness * 2.0f,
                    perimeterHalfHeight * 2.0f,
                    perimeterThickness
                ),
                "LIMITE_NORTE_PISO_" + std::to_string(floor)
            );
        }
    }

    // Pilares estructurales: posiciones medidas directamente desde Pillars_0
    // en parking.obj. Su altura completa evita atravesarlos en pisos superiores.
    auto addPillar = [&](float x, float y, float z, float height, const std::string& name)
    {
        boxes.push_back({
            glm::vec3(x, y, z),
            glm::vec3(1.10f, height * 0.5f, 1.10f),
            name
        });
    };

    addPillar(-44.0381f, 11.6681f, -32.5211f, 33.7317f, "PILAR_01");
    addPillar(-44.0381f,  9.3239f,  -9.6180f, 29.0433f, "PILAR_02");
    addPillar(-44.0381f,  9.3240f,  12.5664f, 29.0433f, "PILAR_03");
    addPillar(-44.0381f, 11.8276f,  31.9347f, 34.0506f, "PILAR_04");
    addPillar(-22.4734f,  9.3239f, -32.5211f, 29.0433f, "PILAR_05");
    addPillar(-22.4734f, 14.4509f,  -9.6180f, 39.2972f, "PILAR_06");
    addPillar(-22.4734f, 14.4509f,  12.5664f, 39.2972f, "PILAR_07");
    addPillar(-22.4734f,  9.3240f,  31.9347f, 29.0433f, "PILAR_08");
    addPillar(  0.8495f,  9.3239f, -32.5211f, 29.0433f, "PILAR_09");
    addPillar(  0.8495f, 14.4509f,  -9.6180f, 39.2972f, "PILAR_10");
    addPillar(  0.8495f, 14.4509f,  12.5664f, 39.2972f, "PILAR_11");
    addPillar(  0.8495f,  9.3240f,  31.9347f, 29.0433f, "PILAR_12");
    addPillar( 24.0039f,  9.3239f, -32.5211f, 29.0433f, "PILAR_13");
    addPillar( 24.0039f, 14.4509f,  -9.6180f, 39.2972f, "PILAR_14");
    addPillar( 24.0039f, 14.4509f,  12.5664f, 39.2972f, "PILAR_15");
    addPillar( 24.0039f,  9.3240f,  31.9347f, 29.0433f, "PILAR_16");
    addPillar( 44.0580f, 11.6681f, -32.5211f, 33.7317f, "PILAR_17");
    addPillar( 44.0580f,  9.3239f,  -9.6180f, 29.0433f, "PILAR_18");
    addPillar( 44.0580f,  9.3240f,  12.5664f, 29.0433f, "PILAR_19");
    addPillar( 44.0580f, 11.8276f,  31.9347f, 34.0506f, "PILAR_20");

    return boxes;
}

bool collidesWithStaticObstacle(const glm::vec3& center, const glm::vec3& halfSize)
{
    for (const CollisionBox& box : staticCollisionBoxes)
    {
        const bool overlapX =
            center.x + halfSize.x > box.center.x - box.halfSize.x &&
            center.x - halfSize.x < box.center.x + box.halfSize.x;
        const bool overlapY =
            center.y + halfSize.y > box.center.y - box.halfSize.y &&
            center.y - halfSize.y < box.center.y + box.halfSize.y;
        const bool overlapZ =
            center.z + halfSize.z > box.center.z - box.halfSize.z &&
            center.z - halfSize.z < box.center.z + box.halfSize.z;

        if (overlapX && overlapY && overlapZ)
            return true;
    }

    return false;
}

glm::vec3 resolveCollisionMovement(
    const glm::vec3& start,
    const glm::vec3& desired,
    const glm::vec3& halfSize
)
{
    glm::vec3 resolved = start;

    // Resolver por ejes permite deslizarse junto al muro en lugar de quedar pegado.
    glm::vec3 candidate = resolved;
    candidate.x = desired.x;
    if (!collidesWithStaticObstacle(candidate, halfSize))
        resolved.x = candidate.x;

    candidate = resolved;
    candidate.z = desired.z;
    if (!collidesWithStaticObstacle(candidate, halfSize))
        resolved.z = candidate.z;

    candidate = resolved;
    candidate.y = desired.y;
    if (!collidesWithStaticObstacle(candidate, halfSize))
        resolved.y = candidate.y;

    return resolved;
}

glm::vec3 resolveVehicleMovement(
    const glm::vec3& start,
    const glm::vec3& desired,
    float vehicleRotationDegrees,
    bool& blocked
)
{
    blocked = false;

    // La caja AABB se amplía según el ángulo actual del vehículo. De esta
    // manera las esquinas del carro tampoco pueden introducirse en un muro.
    const float yawRadians = glm::radians(vehicleRotationDegrees);
    const float cosine = std::abs(std::cos(yawRadians));
    const float sine = std::abs(std::sin(yawRadians));
    const glm::vec3 rotatedHalfSize(
        cosine * VEHICLE_COLLISION_HALF_SIZE.x +
            sine * VEHICLE_COLLISION_HALF_SIZE.z,
        VEHICLE_COLLISION_HALF_SIZE.y,
        sine * VEHICLE_COLLISION_HALF_SIZE.x +
            cosine * VEHICLE_COLLISION_HALF_SIZE.z
    );

    const glm::vec3 totalDelta = desired - start;
    const float horizontalDistance = glm::length(
        glm::vec2(totalDelta.x, totalDelta.z)
    );

    // Se divide un movimiento largo en pasos pequeños. Esto evita que el
    // carro atraviese una pared entre dos fotogramas a velocidad alta.
    const int stepCount = std::max(
        1,
        static_cast<int>(std::ceil(horizontalDistance / 0.20f))
    );

    glm::vec3 resolved = start;

    for (int step = 1; step <= stepCount; ++step)
    {
        const float t = static_cast<float>(step) /
            static_cast<float>(stepCount);
        const glm::vec3 target = start + totalDelta * t;

        // Resolver X y Z por separado permite deslizarse a lo largo de una
        // pared en vez de quedar completamente bloqueado.
        glm::vec3 candidate = resolved;
        candidate.x = target.x;
        candidate.y = target.y;
        glm::vec3 collisionCenter = candidate + glm::vec3(
            0.0f,
            VEHICLE_COLLISION_HALF_SIZE.y,
            0.0f
        );

        if (!collidesWithStaticObstacle(collisionCenter, rotatedHalfSize))
        {
            resolved.x = candidate.x;
        }
        else
        {
            blocked = true;
        }

        candidate = resolved;
        candidate.z = target.z;
        candidate.y = target.y;
        collisionCenter = candidate + glm::vec3(
            0.0f,
            VEHICLE_COLLISION_HALF_SIZE.y,
            0.0f
        );

        if (!collidesWithStaticObstacle(collisionCenter, rotatedHalfSize))
        {
            resolved.z = candidate.z;
        }
        else
        {
            blocked = true;
        }

        // La altura la administra la física de pisos/rampas del vehículo.
        resolved.y = target.y;
    }

    return resolved;
}

void moveCameraSafely(Camera_Movement direction)
{
    const glm::vec3 start = camera.Position;
    camera.ProcessKeyboard(direction, deltaTime);
    const glm::vec3 desired = camera.Position;
    camera.Position = resolveCollisionMovement(start, desired, CAMERA_COLLISION_HALF_SIZE);
}

int floorSlotCount(int floorIndex)
{
    int count = 0;
    for (const ParkingSlot& slot : parkingSlots)
    {
        if (slot.floorIndex == floorIndex)
            ++count;
    }
    return count;
}

int slotGridOrdinal(const ParkingSlot& slot)
{
    return (slot.row == 'A' ? 0 : 22) + std::clamp(slot.number - 1, 0, 21);
}

void addUndirectedEdge(ParkingGraph& graph, int a, int b)
{
    if (a < 0 || b < 0 || a >= static_cast<int>(graph.nodes.size()) || b >= static_cast<int>(graph.nodes.size()))
        return;

    const float weight = glm::length(graph.nodes[a] - graph.nodes[b]);
    graph.adjacency[a].push_back({ b, weight });
    graph.adjacency[b].push_back({ a, weight });
}

void addDirectedEdge(ParkingGraph& graph, int from, int to)
{
    if (from < 0 || to < 0 ||
        from >= static_cast<int>(graph.nodes.size()) ||
        to >= static_cast<int>(graph.nodes.size()))
    {
        return;
    }

    const float weight = glm::length(graph.nodes[from] - graph.nodes[to]);
    graph.adjacency[from].push_back({ to, weight });
}


DijkstraResult runDijkstra(const ParkingGraph& graph, int startNode)
{
    const float infinity = std::numeric_limits<float>::infinity();
    DijkstraResult result;
    result.distance.assign(graph.nodes.size(), infinity);
    result.previous.assign(graph.nodes.size(), -1);

    using QueueItem = std::pair<float, int>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> queue;

    result.distance[startNode] = 0.0f;
    queue.push({ 0.0f, startNode });

    while (!queue.empty())
    {
        const auto [currentDistance, node] = queue.top();
        queue.pop();

        if (currentDistance > result.distance[node])
            continue;

        for (const Edge& edge : graph.adjacency[node])
        {
            const float candidate = currentDistance + edge.weight;
            if (candidate < result.distance[edge.to])
            {
                result.distance[edge.to] = candidate;
                result.previous[edge.to] = node;
                queue.push({ candidate, edge.to });
            }
        }
    }

    return result;
}

std::vector<int> reconstructPath(const DijkstraResult& result, int destinationNode)
{
    std::vector<int> path;
    for (int node = destinationNode; node != -1; node = result.previous[node])
        path.push_back(node);

    std::reverse(path.begin(), path.end());
    return path;
}

int findNearestGraphNode(const glm::vec3& position)
{
    if (parkingGraph.nodes.empty())
        return -1;

    int nearestNode = 0;
    float nearestDistanceSquared = std::numeric_limits<float>::infinity();

    for (std::size_t i = 0; i < parkingGraph.nodes.size(); ++i)
    {
        const glm::vec3 difference = parkingGraph.nodes[i] - position;
        // La altura tiene mas peso para evitar elegir un nodo de otro piso.
        const float distanceSquared =
            difference.x * difference.x +
            difference.z * difference.z +
            4.0f * difference.y * difference.y;

        if (distanceSquared < nearestDistanceSquared)
        {
            nearestDistanceSquared = distanceSquared;
            nearestNode = static_cast<int>(i);
        }
    }

    return nearestNode;
}

int getSelectedPhoneSlotIndex()
{
    const int ordinal = std::clamp(selectedPhoneSlotOrdinal, 0, 43);
    const char row = ordinal < 22 ? 'A' : 'B';
    const int number = (ordinal % 22) + 1;

    for (std::size_t i = 0; i < parkingSlots.size(); ++i)
    {
        const ParkingSlot& slot = parkingSlots[i];
        if (slot.floorIndex == selectedPhoneFloor &&
            slot.row == row &&
            slot.number == number)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}


void movePhoneSelection(int direction)
{
    const int step = direction < 0 ? -1 : 1;
    int candidate = selectedPhoneSlotOrdinal;

    for (int attempt = 0; attempt < 44; ++attempt)
    {
        if (direction != 0 || attempt > 0)
            candidate = (candidate + step + 44) % 44;

        selectedPhoneSlotOrdinal = candidate;
        const int selectedIndex = getSelectedPhoneSlotIndex();
        if (selectedIndex < 0)
            continue; // Hueco fisico de la entrada en PB.

        const ParkingSlot& slot = parkingSlots[selectedIndex];
        if (slot.state != SlotState::Available)
            continue;

        showNotice(
            "SELECCION: " + slot.id,
            "ESPACIO VERDE DISPONIBLE",
            1.4f
        );
        return;
    }

    showNotice("SIN ESPACIOS VERDES", "GENERA OTRA SIMULACION CON G", 2.5f);
}


void reserveBestAvailableSlot()
{
    cancelCurrentReservation();

    const int startNode = findNearestGraphNode(getSimulationPosition());
    if (startNode < 0)
        return;

    const DijkstraResult dijkstra = runDijkstra(parkingGraph, startNode);
    float bestDistance = std::numeric_limits<float>::infinity();
    int bestSlot = -1;

    for (std::size_t i = 0; i < parkingSlots.size(); ++i)
    {
        if (parkingSlots[i].state != SlotState::Available)
            continue;

        const int node = parkingSlots[i].graphNode;
        if (node >= 0 && dijkstra.distance[node] < bestDistance)
        {
            bestDistance = dijkstra.distance[node];
            bestSlot = static_cast<int>(i);
        }
    }

    if (bestSlot < 0)
    {
        showNotice("PARQUEADERO LLENO", "NO EXISTEN ESPACIOS VERDES", 4.0f);
        exportParkingStateJson();
        return;
    }

    reservedSlotIndex = bestSlot;
    guidanceTarget = GuidanceTarget::ParkingSlot;
    parkingSlots[bestSlot].state = SlotState::Reserved;
    activeRoute = reconstructPath(dijkstra, parkingSlots[bestSlot].graphNode);
    selectedPhoneFloor = parkingSlots[bestSlot].floorIndex;
    selectedPhoneSlotOrdinal = slotGridOrdinal(parkingSlots[bestSlot]);

    std::ostringstream distanceText;
    distanceText
        << parkingSlots[bestSlot].floorId
        << " - RUTA DIJKSTRA "
        << static_cast<int>(std::round(bestDistance))
        << " METROS";

    showNotice(
        "ESPACIO " + parkingSlots[bestSlot].id + " RESERVADO",
        distanceText.str(),
        4.0f
    );
    exportParkingStateJson();
}

int findSlotIndexById(const std::string& slotId)
{
    for (std::size_t i = 0; i < parkingSlots.size(); ++i)
    {
        if (parkingSlots[i].id == slotId)
            return static_cast<int>(i);
    }

    return -1;
}

void cancelCurrentReservation()
{
    if (reservedSlotIndex >= 0 &&
        reservedSlotIndex < static_cast<int>(parkingSlots.size()) &&
        parkingSlots[reservedSlotIndex].state == SlotState::Reserved)
    {
        parkingSlots[reservedSlotIndex].state = SlotState::Available;
    }

    reservedSlotIndex = -1;
    activeRoute.clear();
    guidanceTarget = GuidanceTarget::None;
}

void reserveSpecificSlot(const std::string& slotId)
{
    const int slotIndex = findSlotIndexById(slotId);
    if (slotIndex < 0)
    {
        showNotice("ESPACIO NO ENCONTRADO", slotId, 3.0f);
        exportParkingStateJson();
        return;
    }

    if (parkingSlots[slotIndex].state != SlotState::Available)
    {
        showNotice(
            "NO SE PUEDE ELEGIR " + slotId,
            "SOLO PUEDES ELEGIR UNA CASILLA VERDE",
            3.5f
        );
        exportParkingStateJson();
        return;
    }

    cancelCurrentReservation();

    const int startNode = findNearestGraphNode(getSimulationPosition());
    if (startNode < 0)
        return;

    const DijkstraResult dijkstra = runDijkstra(parkingGraph, startNode);
    const int destinationNode = parkingSlots[slotIndex].graphNode;

    if (destinationNode < 0 ||
        destinationNode >= static_cast<int>(dijkstra.distance.size()) ||
        !std::isfinite(dijkstra.distance[destinationNode]))
    {
        showNotice("RUTA NO DISPONIBLE", "DIJKSTRA NO ENCONTRO UN CAMINO", 3.5f);
        exportParkingStateJson();
        return;
    }

    reservedSlotIndex = slotIndex;
    guidanceTarget = GuidanceTarget::ParkingSlot;
    parkingSlots[slotIndex].state = SlotState::Reserved;
    activeRoute = reconstructPath(dijkstra, destinationNode);
    selectedPhoneFloor = parkingSlots[slotIndex].floorIndex;
    selectedPhoneSlotOrdinal = slotGridOrdinal(parkingSlots[slotIndex]);

    std::ostringstream distanceText;
    distanceText
        << parkingSlots[slotIndex].floorId
        << " - RUTA "
        << static_cast<int>(std::round(dijkstra.distance[destinationNode]))
        << " METROS";

    showNotice(
        "DESTINO " + parkingSlots[slotIndex].id,
        distanceText.str(),
        4.0f
    );
    exportParkingStateJson();
}

void routeToExit()
{
    cancelCurrentReservation();

    // Al pedir salida, el Mazda abandona su casilla y esa plaza vuelve a verde.
    if (mainVehicleParkedSlotIndex >= 0 &&
        mainVehicleParkedSlotIndex < static_cast<int>(parkingSlots.size()))
    {
        ParkingSlot& parkedSlot = parkingSlots[mainVehicleParkedSlotIndex];
        if (parkedSlot.state == SlotState::Occupied)
            parkedSlot.state = SlotState::Available;
        occupiedHistory.erase(
            std::remove(occupiedHistory.begin(), occupiedHistory.end(), mainVehicleParkedSlotIndex),
            occupiedHistory.end()
        );
        mainVehicleParkedSlotIndex = -1;
    }

    const int startNode = findNearestGraphNode(getSimulationPosition());
    if (startNode < 0 || parkingGraph.exitNode < 0)
        return;

    const DijkstraResult dijkstra = runDijkstra(parkingGraph, startNode);
    if (parkingGraph.exitNode >= static_cast<int>(dijkstra.distance.size()) ||
        !std::isfinite(dijkstra.distance[parkingGraph.exitNode]))
    {
        showNotice(
            "SALIDA NO ALCANZABLE",
            "REVISA LA RAMPA DERECHA DE BAJADA",
            4.0f
        );
        exportParkingStateJson();
        return;
    }

    activeRoute = reconstructPath(dijkstra, parkingGraph.exitNode);
    guidanceTarget = GuidanceTarget::Exit;

    std::ostringstream subtitle;
    subtitle
        << "USA LA RAMPA DERECHA - "
        << static_cast<int>(std::round(dijkstra.distance[parkingGraph.exitNode]))
        << " METROS";

    showNotice("RUTA A LA SALIDA", subtitle.str(), 4.0f);
    exportParkingStateJson();
}

void releaseSlotById(const std::string& slotId)
{
    const int slotIndex = findSlotIndexById(slotId);
    if (slotIndex < 0)
    {
        showNotice("ESPACIO NO ENCONTRADO", slotId, 2.5f);
        exportParkingStateJson();
        return;
    }

    ParkingSlot& slot = parkingSlots[slotIndex];

    if (slot.state == SlotState::Disabled)
    {
        showNotice("ESPACIO ACCESIBLE", "NO SE MODIFICA DESDE LA APP", 2.5f);
        exportParkingStateJson();
        return;
    }

    if (slotIndex == reservedSlotIndex)
        cancelCurrentReservation();

    slot.state = SlotState::Available;
    if (slotIndex == mainVehicleParkedSlotIndex)
        mainVehicleParkedSlotIndex = -1;
    occupiedHistory.erase(
        std::remove(occupiedHistory.begin(), occupiedHistory.end(), slotIndex),
        occupiedHistory.end()
    );

    showNotice("ESPACIO " + slot.id + " LIBERADO", "ESTADO VERDE", 2.5f);
    exportParkingStateJson();
}

void simulateRandomOccupancy()
{
    cancelCurrentReservation();
    occupiedHistory.clear();
    if (mainVehicleParkedSlotIndex >= 0)
        occupiedHistory.push_back(mainVehicleParkedSlotIndex);

    std::uniform_int_distribution<int> occupiedPerFloor(18, 32);
    std::uniform_int_distribution<int> carTypeDistribution(
        0,
        static_cast<int>(carPresets.size()) - 1
    );

    int totalOccupied = mainVehicleParkedSlotIndex >= 0 ? 1 : 0;

    for (int floor = 0; floor < 4; ++floor)
    {
        std::vector<int> candidates;
        for (std::size_t i = 0; i < parkingSlots.size(); ++i)
        {
            ParkingSlot& slot = parkingSlots[i];
            if (slot.floorIndex != floor)
                continue;

            if (slot.state == SlotState::Disabled)
                continue;

            if (static_cast<int>(i) == mainVehicleParkedSlotIndex)
            {
                slot.state = SlotState::Occupied;
                continue;
            }

            slot.state = SlotState::Available;
            candidates.push_back(static_cast<int>(i));
        }

        std::shuffle(candidates.begin(), candidates.end(), randomEngine);
        const int count = std::min<int>(occupiedPerFloor(randomEngine), candidates.size());

        for (int i = 0; i < count; ++i)
        {
            ParkingSlot& slot = parkingSlots[candidates[i]];
            slot.state = SlotState::Occupied;
            slot.carType = carTypeDistribution(randomEngine);
            occupiedHistory.push_back(candidates[i]);
            ++totalOccupied;
        }
    }

    selectedPhoneSlotOrdinal = 0;
    for (int ordinal = 0; ordinal < 44; ++ordinal)
    {
        selectedPhoneSlotOrdinal = ordinal;
        const int selectedIndex = getSelectedPhoneSlotIndex();
        if (selectedIndex >= 0 && parkingSlots[selectedIndex].state == SlotState::Available)
            break;
    }

    showNotice(
        "SIMULACION ACTUALIZADA",
        std::to_string(totalOccupied) + " CARROS EN POSICIONES ALEATORIAS",
        4.0f
    );
    exportParkingStateJson();
}

void spawnCarInNextAvailableSlot()
{
    if (parkingSlots.empty())
        return;

    for (std::size_t offset = 0; offset < parkingSlots.size(); ++offset)
    {
        const int index = (nextSpawnSearchIndex + static_cast<int>(offset)) % static_cast<int>(parkingSlots.size());
        if (parkingSlots[index].state == SlotState::Available)
        {
            parkingSlots[index].state = SlotState::Occupied;
            parkingSlots[index].carType = selectedCarType;
            occupiedHistory.push_back(index);
            nextSpawnSearchIndex = (index + 1) % static_cast<int>(parkingSlots.size());

            showNotice(
                "CARRO AGREGADO EN " + parkingSlots[index].id,
                "EL FOCO CAMBIO DE VERDE A ROJO",
                2.8f
            );
            exportParkingStateJson();
            return;
        }
    }

    showNotice("SIN ESPACIOS LIBRES", "NO SE PUEDE AGREGAR OTRO CARRO", 3.5f);
}

void removeLastSpawnedCar()
{
    while (!occupiedHistory.empty())
    {
        const int index = occupiedHistory.back();
        occupiedHistory.pop_back();

        if (index >= 0 && index < static_cast<int>(parkingSlots.size()) &&
            parkingSlots[index].state == SlotState::Occupied)
        {
            parkingSlots[index].state = SlotState::Available;
            showNotice(
                "CARRO RETIRADO DE " + parkingSlots[index].id,
                "EL FOCO CAMBIO DE ROJO A VERDE",
                2.8f
            );
            exportParkingStateJson();
            return;
        }
    }

    showNotice("NINGUN CARRO AGREGADO", "USA G PARA GENERAR UNA SIMULACION", 2.5f);
}

void parkMainVehicleInReservedSlot()
{
    if (controlledVehicle == nullptr ||
        reservedSlotIndex < 0 ||
        reservedSlotIndex >= static_cast<int>(parkingSlots.size()))
    {
        return;
    }

    const ParkingSlot& slot = parkingSlots[reservedSlotIndex];
    constexpr std::array<float, 4> vehicleFloorY = {
        -1.42000f, 6.47726f, 14.55185f, 22.64915f
    };

    controlledVehicle->stop();
    controlledVehicle->setCurrentPosition(glm::vec3(
        slot.position.x,
        vehicleFloorY[slot.floorIndex],
        slot.position.z
    ));

    // El OBJ del Mazda mira 145 grados respecto a su eje matematico.
    // La fila A entra hacia +Z; la fila B entra hacia -Z.
    const float parkingRotation = slot.row == 'A' ? -145.0f : 35.0f;
    controlledVehicle->setCurrentRotation(parkingRotation);
}

void completeReservedParking()
{
    if (reservedSlotIndex < 0 || reservedSlotIndex >= static_cast<int>(parkingSlots.size()))
    {
        showNotice("NO HAY DESTINO", "ELIGE UNA CASILLA VERDE Y PRESIONA K", 2.5f);
        return;
    }

    parkMainVehicleInReservedSlot();

    ParkingSlot& slot = parkingSlots[reservedSlotIndex];
    slot.state = SlotState::Occupied;
    slot.carType = selectedCarType;
    mainVehicleParkedSlotIndex = reservedSlotIndex;
    occupiedHistory.push_back(reservedSlotIndex);

    const std::string completedId = slot.id;
    reservedSlotIndex = -1;
    activeRoute.clear();
    guidanceTarget = GuidanceTarget::None;

    showNotice(
        "MISION SUPERADA",
        "MAZDA ESTACIONADO AUTOMATICAMENTE EN " + completedId,
        6.0f,
        true
    );
    exportParkingStateJson();
}

void updateAutomaticArrival()
{
    if (guidanceTarget == GuidanceTarget::ParkingSlot)
    {
        if (reservedSlotIndex < 0 || reservedSlotIndex >= static_cast<int>(parkingSlots.size()))
            return;

        const glm::vec3 targetPosition =
            parkingSlots[reservedSlotIndex].position + glm::vec3(0.0f, 1.2f, 0.0f);

        if (glm::length(getSimulationPosition() - targetPosition) <= 6.5f)
            completeReservedParking();
        return;
    }

    if (guidanceTarget == GuidanceTarget::Exit && parkingGraph.exitNode >= 0)
    {
        const glm::vec3 exitPosition = parkingGraph.nodes[parkingGraph.exitNode];
        if (glm::length(getSimulationPosition() - exitPosition) <= 7.0f)
        {
            activeRoute.clear();
            guidanceTarget = GuidanceTarget::None;
            showNotice(
                "MISION SUPERADA",
                "HAS LLEGADO A LA SALIDA",
                6.0f,
                true
            );
            exportParkingStateJson();
        }
    }
}

std::string extractJsonString(const std::string& json, const std::string& key)
{
    const std::string token = "\"" + key + "\"";
    const std::size_t keyPosition = json.find(token);
    if (keyPosition == std::string::npos)
        return {};

    const std::size_t colonPosition = json.find(':', keyPosition + token.size());
    if (colonPosition == std::string::npos)
        return {};

    const std::size_t valueStart = json.find('"', colonPosition + 1);
    if (valueStart == std::string::npos)
        return {};

    const std::size_t valueEnd = json.find('"', valueStart + 1);
    if (valueEnd == std::string::npos)
        return {};

    return json.substr(valueStart + 1, valueEnd - valueStart - 1);
}

void processExternalCommands()
{
    const double now = glfwGetTime();
    if (now - lastCommandCheckTime < 0.20)
        return;

    lastCommandCheckTime = now;

    const fs::path commandPath = fs::path("sync") / "parking_command.json";
    if (!fs::exists(commandPath))
        return;

    std::ifstream input(commandPath);
    if (!input)
        return;

    const std::string json(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );

    const std::string commandId = extractJsonString(json, "command_id");
    const std::string action = extractJsonString(json, "action");
    const std::string slotId = extractJsonString(json, "slot_id");

    if (commandId.empty() || action.empty() || commandId == lastProcessedCommandId)
        return;

    lastProcessedCommandId = commandId;

    if (action == "RESERVE")
    {
        reserveSpecificSlot(slotId);
    }
    else if (action == "RESERVE_BEST")
    {
        reserveBestAvailableSlot();
    }
    else if (action == "CANCEL_RESERVATION")
    {
        cancelCurrentReservation();
        showNotice("RUTA CANCELADA", "SE ELIMINO LA GUIA ACTIVA", 2.8f);
        exportParkingStateJson();
    }
    else if (action == "SIMULATE_RANDOM")
    {
        simulateRandomOccupancy();
    }
    else if (action == "ROUTE_EXIT")
    {
        routeToExit();
    }
    else if (action == "RELEASE")
    {
        releaseSlotById(slotId);
    }
    else if (action == "OCCUPY")
    {
        const int slotIndex = findSlotIndexById(slotId);
        if (slotIndex >= 0 && parkingSlots[slotIndex].state != SlotState::Disabled)
        {
            if (slotIndex == reservedSlotIndex)
                cancelCurrentReservation();

            parkingSlots[slotIndex].state = SlotState::Occupied;
            parkingSlots[slotIndex].carType = selectedCarType;
            occupiedHistory.push_back(slotIndex);
            selectedPhoneFloor = parkingSlots[slotIndex].floorIndex;
            showNotice("CARRO EN " + slotId, "COMANDO RECIBIDO DESDE LA APP", 2.8f);
        }
        else
        {
            showNotice("NO SE PUEDE OCUPAR", slotId, 2.8f);
        }
        exportParkingStateJson();
    }
    else
    {
        showNotice("COMANDO DESCONOCIDO", action, 2.5f);
        exportParkingStateJson();
    }

    updateWindowTitle(glfwGetCurrentContext());
}


std::string slotStateName(SlotState state)
{
    switch (state)
    {
    case SlotState::Available: return "AVAILABLE";
    case SlotState::Occupied:  return "OCCUPIED";
    case SlotState::Reserved:  return "RESERVED";
    case SlotState::Disabled:  return "DISABLED";
    }

    return "UNKNOWN";
}

glm::vec4 slotStateColor(SlotState state)
{
    switch (state)
    {
    case SlotState::Available: return glm::vec4(0.12f, 0.95f, 0.28f, 1.0f);
    case SlotState::Occupied:  return glm::vec4(0.98f, 0.12f, 0.10f, 1.0f);
    case SlotState::Reserved:  return glm::vec4(1.00f, 0.78f, 0.08f, 1.0f);
    case SlotState::Disabled:  return glm::vec4(0.10f, 0.55f, 1.00f, 1.0f);
    }

    return glm::vec4(1.0f);
}

int floorFromWorldHeight(float y)
{
    constexpr std::array<float, 4> vehicleFloorY = {
        -1.42000f, 6.47726f, 14.55185f, 22.64915f
    };

    int closestFloor = 0;
    float closestDistance = std::abs(y - vehicleFloorY[0]);
    for (int floor = 1; floor < 4; ++floor)
    {
        const float distance = std::abs(y - vehicleFloorY[floor]);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestFloor = floor;
        }
    }
    return closestFloor;
}

float calculateRemainingRouteDistance(const glm::vec3& vehiclePosition)
{
    if (activeRoute.empty())
        return 0.0f;

    std::size_t nearestPosition = 0;
    float nearestDistance = std::numeric_limits<float>::infinity();
    for (std::size_t routePosition = 0; routePosition < activeRoute.size(); ++routePosition)
    {
        const int nodeIndex = activeRoute[routePosition];
        if (nodeIndex < 0 || nodeIndex >= static_cast<int>(parkingGraph.nodes.size()))
            continue;
        const float distance = glm::length(parkingGraph.nodes[nodeIndex] - vehiclePosition);
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestPosition = routePosition;
        }
    }

    float total = nearestDistance;
    for (std::size_t routePosition = nearestPosition + 1; routePosition < activeRoute.size(); ++routePosition)
    {
        const int previousNode = activeRoute[routePosition - 1];
        const int currentNode = activeRoute[routePosition];
        if (previousNode < 0 || currentNode < 0 ||
            previousNode >= static_cast<int>(parkingGraph.nodes.size()) ||
            currentNode >= static_cast<int>(parkingGraph.nodes.size()))
        {
            continue;
        }
        total += glm::length(parkingGraph.nodes[currentNode] - parkingGraph.nodes[previousNode]);
    }
    return total;
}

std::string currentNavigationInstruction(const glm::vec3& vehiclePosition)
{
    if (activeRoute.empty())
        return mainVehicleParkedSlotIndex >= 0
            ? "Vehiculo estacionado"
            : "Sin ruta activa";

    std::size_t nearestPosition = 0;
    float nearestDistance = std::numeric_limits<float>::infinity();
    for (std::size_t routePosition = 0; routePosition < activeRoute.size(); ++routePosition)
    {
        const int nodeIndex = activeRoute[routePosition];
        if (nodeIndex < 0 || nodeIndex >= static_cast<int>(parkingGraph.nodes.size()))
            continue;
        const float distance = glm::length(parkingGraph.nodes[nodeIndex] - vehiclePosition);
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestPosition = routePosition;
        }
    }

    const std::size_t targetPosition = std::min(nearestPosition + 1, activeRoute.size() - 1);
    const int nextNode = activeRoute[targetPosition];
    const glm::vec3 target = parkingGraph.nodes[nextNode];
    const int currentFloor = floorFromWorldHeight(vehiclePosition.y);
    const int nextFloor = floorFromWorldHeight(target.y);
    if (nextFloor > currentFloor)
        return "Suba por la rampa izquierda";
    if (nextFloor < currentFloor)
        return "Baje por la rampa derecha";

    if (guidanceTarget == GuidanceTarget::Exit && nextFloor == 0 && target.z > 32.0f)
        return "Continue hacia la salida principal";

    const glm::vec3 direction = target - vehiclePosition;
    if (std::abs(direction.x) > std::abs(direction.z))
        return direction.x >= 0.0f ? "Gire a la derecha" : "Gire a la izquierda";
    return "Continue recto";
}

void exportParkingStateJson()
{
    try
    {
        fs::create_directories("sync");
        std::ofstream output("sync/parking_state.json", std::ios::trunc);
        if (!output)
            return;

        int available = 0;
        int occupied = 0;
        int reserved = 0;
        int disabled = 0;
        std::array<int, 4> floorAvailable{};
        std::array<int, 4> floorOccupied{};
        std::array<int, 4> floorReserved{};
        std::array<int, 4> floorDisabled{};
        std::array<int, 4> floorTotal{};

        for (const ParkingSlot& slot : parkingSlots)
        {
            ++floorTotal[slot.floorIndex];
            switch (slot.state)
            {
            case SlotState::Available: ++available; ++floorAvailable[slot.floorIndex]; break;
            case SlotState::Occupied:  ++occupied;  ++floorOccupied[slot.floorIndex]; break;
            case SlotState::Reserved:  ++reserved;  ++floorReserved[slot.floorIndex]; break;
            case SlotState::Disabled:  ++disabled;  ++floorDisabled[slot.floorIndex]; break;
            }
        }

        const std::array<std::string, 4> floorIds = { "PB", "P1", "P2", "P3" };
        const std::array<std::string, 4> floorNames = {
            "Planta Baja", "Piso 1", "Piso 2", "Piso 3"
        };

        const std::string reservedId =
            reservedSlotIndex >= 0 && reservedSlotIndex < static_cast<int>(parkingSlots.size())
            ? parkingSlots[reservedSlotIndex].id : "";
        const std::string parkedSlotId =
            mainVehicleParkedSlotIndex >= 0 && mainVehicleParkedSlotIndex < static_cast<int>(parkingSlots.size())
            ? parkingSlots[mainVehicleParkedSlotIndex].id : "";
        const std::string guidanceType =
            guidanceTarget == GuidanceTarget::ParkingSlot ? "PARKING" :
            guidanceTarget == GuidanceTarget::Exit ? "EXIT" : "NONE";
        const std::string routeTargetId =
            guidanceTarget == GuidanceTarget::ParkingSlot ? reservedId :
            guidanceTarget == GuidanceTarget::Exit ? "SALIDA" : "";
        const int selectedIndex = getSelectedPhoneSlotIndex();
        const std::string selectedSlotId = selectedIndex >= 0 ? parkingSlots[selectedIndex].id : "";

        const glm::vec3 vehiclePosition = controlledVehicle != nullptr
            ? controlledVehicle->getPosition() : glm::vec3(0.0f);
        const float physicalHeading = controlledVehicle != nullptr
            ? std::fmod(controlledVehicle->getRotation() + 145.0f + 360.0f, 360.0f)
            : 0.0f;
        const float speedKmh = controlledVehicle != nullptr
            ? std::abs(controlledVehicle->getSpeed()) * 3.6f : 0.0f;
        int vehicleFloor = floorFromWorldHeight(vehiclePosition.y);
        const bool onStreet = vehiclePosition.z >= 46.18f && vehiclePosition.y < -2.0f;
        if (onStreet)
            vehicleFloor = -1;
        const std::string vehicleFloorName = onStreet
            ? "Calle"
            : floorNames[std::clamp(vehicleFloor, 0, 3)];
        const float remainingDistance = calculateRemainingRouteDistance(vehiclePosition);
        const std::string instruction = currentNavigationInstruction(vehiclePosition);

        output << std::fixed << std::setprecision(3);
        output << "{\n";
        output << "  \"version\": 6,\n";
        output << "  \"simulation_time_seconds\": " << glfwGetTime() << ",\n";
        output << "  \"total_slots\": " << parkingSlots.size() << ",\n";
        output << "  \"available\": " << available << ",\n";
        output << "  \"occupied\": " << occupied << ",\n";
        output << "  \"reserved\": " << reserved << ",\n";
        output << "  \"disabled\": " << disabled << ",\n";
        output << "  \"guidance_active\": " << (!activeRoute.empty() ? "true" : "false") << ",\n";
        output << "  \"reserved_slot_id\": \"" << reservedId << "\",\n";
        output << "  \"guidance_type\": \"" << guidanceType << "\",\n";
        output << "  \"route_target_id\": \"" << routeTargetId << "\",\n";
        output << "  \"selected_slot_id\": \"" << selectedSlotId << "\",\n";
        output << "  \"last_command_id\": \"" << lastProcessedCommandId << "\",\n";

        output << "  \"vehicle\": {\n";
        output << "    \"x\": " << vehiclePosition.x << ",\n";
        output << "    \"y\": " << vehiclePosition.y << ",\n";
        output << "    \"z\": " << vehiclePosition.z << ",\n";
        output << "    \"floor\": " << vehicleFloor << ",\n";
        output << "    \"floor_name\": \"" << vehicleFloorName << "\",\n";
        output << "    \"heading_degrees\": " << physicalHeading << ",\n";
        output << "    \"speed_kmh\": " << speedKmh << ",\n";
        output << "    \"parked_slot_id\": \"" << parkedSlotId << "\"\n";
        output << "  },\n";

        output << "  \"navigation\": {\n";
        output << "    \"active\": " << (!activeRoute.empty() ? "true" : "false") << ",\n";
        output << "    \"type\": \"" << guidanceType << "\",\n";
        output << "    \"destination\": \"" << routeTargetId << "\",\n";
        output << "    \"destination_floor\": "
               << (reservedSlotIndex >= 0 ? parkingSlots[reservedSlotIndex].floorIndex : 0) << ",\n";
        output << "    \"distance_remaining\": " << remainingDistance << ",\n";
        output << "    \"instruction\": \"" << instruction << "\",\n";
        output << "    \"route\": [\n";
        bool firstRoutePoint = true;
        for (const int nodeIndex : activeRoute)
        {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int>(parkingGraph.nodes.size()))
                continue;
            const glm::vec3& point = parkingGraph.nodes[nodeIndex];
            if (!firstRoutePoint)
                output << ",\n";
            firstRoutePoint = false;
            output << "      {\"x\":" << point.x
                   << ",\"y\":" << point.y
                   << ",\"z\":" << point.z
                   << ",\"floor\":" << floorFromWorldHeight(point.y) << "}";
        }
        if (!firstRoutePoint)
            output << '\n';
        output << "    ]\n";
        output << "  },\n";

        output << "  \"floors\": [\n";
        for (int floor = 0; floor < 4; ++floor)
        {
            output << "    {\"id\":\"" << floorIds[floor]
                   << "\",\"name\":\"" << floorNames[floor]
                   << "\",\"level\":" << floor
                   << ",\"total\":" << floorTotal[floor]
                   << ",\"available\":" << floorAvailable[floor]
                   << ",\"occupied\":" << floorOccupied[floor]
                   << ",\"reserved\":" << floorReserved[floor]
                   << ",\"disabled\":" << floorDisabled[floor] << "}";
            if (floor < 3)
                output << ',';
            output << '\n';
        }
        output << "  ],\n";

        output << "  \"slots\": [\n";
        for (std::size_t i = 0; i < parkingSlots.size(); ++i)
        {
            const ParkingSlot& slot = parkingSlots[i];
            output << "    {\"id\":\"" << slot.id
                   << "\",\"floor\":\"" << slot.floorId
                   << "\",\"floor_index\":" << slot.floorIndex
                   << ",\"row\":\"" << slot.row
                   << "\",\"number\":" << slot.number
                   << ",\"state\":\"" << slotStateName(slot.state)
                   << "\",\"x\":" << slot.position.x
                   << ",\"y\":" << slot.position.y
                   << ",\"z\":" << slot.position.z
                   << ",\"car_type\":" << slot.carType << "}";
            if (i + 1 < parkingSlots.size())
                output << ',';
            output << '\n';
        }
        output << "  ]\n";
        output << "}\n";
    }
    catch (const std::exception& exception)
    {
        std::cerr << "No se pudo escribir parking_state.json: "
                  << exception.what() << '\n';
    }
}


void updateWindowTitle(GLFWwindow* window)
{
    int available = 0;
    int occupied = 0;
    int reserved = 0;

    for (const ParkingSlot& slot : parkingSlots)
    {
        if (slot.state == SlotState::Available) ++available;
        if (slot.state == SlotState::Occupied) ++occupied;
        if (slot.state == SlotState::Reserved) ++reserved;
    }

    std::ostringstream title;
    title
        << "Smart Parking | " << (dayMode ? "DIA" : "NOCHE")
        << " | Disponibles: " << available
        << " | Ocupados: " << occupied
        << " | Reservados: " << reserved;

    glfwSetWindowTitle(window, title.str().c_str());
}

void showNotice(
    const std::string& title,
    const std::string& subtitle,
    float duration,
    bool successStyle
)
{
    noticeTitle = title;
    noticeSubtitle = subtitle;
    noticeTimer = duration;
    noticeSuccessStyle = successStyle;
    std::cout << title << " - " << subtitle << '\n';
}

void drawFallbackCar(Shader& primitiveShader, unsigned int cubeVAO, const ParkingSlot& slot)
{
    const int carType = std::clamp(slot.carType, 0, static_cast<int>(carPresets.size()) - 1);
    const glm::vec4 bodyColor = carPresets[carType].fallbackColor;
    const float rotation = slot.rotationYDegrees;
    const float radians = glm::radians(rotation);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    auto worldPoint = [&](const glm::vec3& local)
    {
        return slot.position + glm::vec3(
            local.x * cosine + local.z * sine,
            local.y,
            -local.x * sine + local.z * cosine
        );
    };

    auto part = [&](const glm::vec3& local, const glm::vec3& size, const glm::vec4& color)
    {
        drawCube(primitiveShader, cubeVAO, worldPoint(local), size, rotation, color);
    };

    const glm::vec4 glassColor(0.08f, 0.16f, 0.22f, 1.0f);
    const glm::vec4 darkBody(bodyColor.r * 0.72f, bodyColor.g * 0.72f, bodyColor.b * 0.72f, 1.0f);
    const glm::vec4 wheelColor(0.022f, 0.022f, 0.026f, 1.0f);

    // Cinco siluetas diferentes construidas únicamente con cubos.
    switch (carType)
    {
    case 0: // Sedan
        part(glm::vec3(0.0f, 0.68f, 0.0f), glm::vec3(2.18f, 0.62f, 4.25f), bodyColor);
        part(glm::vec3(0.0f, 1.40f, -0.18f), glm::vec3(1.72f, 0.78f, 2.25f), darkBody);
        part(glm::vec3(0.0f, 1.52f, -0.18f), glm::vec3(1.58f, 0.45f, 1.70f), glassColor);
        break;
    case 1: // Hatchback
        part(glm::vec3(0.0f, 0.72f, 0.05f), glm::vec3(2.15f, 0.68f, 3.65f), bodyColor);
        part(glm::vec3(0.0f, 1.48f, -0.35f), glm::vec3(1.78f, 0.92f, 2.20f), darkBody);
        part(glm::vec3(0.0f, 1.58f, -0.25f), glm::vec3(1.58f, 0.52f, 1.62f), glassColor);
        break;
    case 2: // SUV
        part(glm::vec3(0.0f, 0.82f, 0.0f), glm::vec3(2.30f, 0.82f, 4.18f), bodyColor);
        part(glm::vec3(0.0f, 1.72f, -0.15f), glm::vec3(1.92f, 1.08f, 2.58f), darkBody);
        part(glm::vec3(0.0f, 1.84f, -0.15f), glm::vec3(1.70f, 0.58f, 1.94f), glassColor);
        break;
    case 3: // Pickup
        part(glm::vec3(0.0f, 0.78f, 0.0f), glm::vec3(2.25f, 0.72f, 4.35f), bodyColor);
        part(glm::vec3(0.0f, 1.60f, -0.92f), glm::vec3(1.88f, 0.95f, 1.60f), darkBody);
        part(glm::vec3(0.0f, 1.68f, -0.90f), glm::vec3(1.64f, 0.50f, 1.10f), glassColor);
        part(glm::vec3(0.0f, 1.15f, 1.10f), glm::vec3(1.95f, 0.18f, 1.55f), glm::vec4(0.11f, 0.11f, 0.12f, 1.0f));
        break;
    default: // Deportivo
        part(glm::vec3(0.0f, 0.58f, 0.0f), glm::vec3(2.22f, 0.50f, 4.30f), bodyColor);
        part(glm::vec3(0.0f, 1.18f, -0.25f), glm::vec3(1.78f, 0.58f, 1.92f), darkBody);
        part(glm::vec3(0.0f, 1.24f, -0.25f), glm::vec3(1.55f, 0.35f, 1.35f), glassColor);
        part(glm::vec3(0.0f, 0.92f, 1.86f), glm::vec3(1.75f, 0.12f, 0.32f), darkBody);
        break;
    }

    const float wheelY = carType == 4 ? 0.24f : 0.30f;
    const float wheelX = carType == 2 ? 1.20f : 1.13f;
    const float wheelZ = carType == 1 ? 1.20f : 1.42f;
    const glm::vec3 wheelSize = carType == 2
        ? glm::vec3(0.38f, 0.52f, 0.72f)
        : glm::vec3(0.34f, 0.44f, 0.66f);

    for (const glm::vec3& wheelOffset : {
        glm::vec3(-wheelX, wheelY,  wheelZ),
        glm::vec3( wheelX, wheelY,  wheelZ),
        glm::vec3(-wheelX, wheelY, -wheelZ),
        glm::vec3( wheelX, wheelY, -wheelZ) })
    {
        part(wheelOffset, wheelSize, wheelColor);
    }

    // Faros y luces traseras ayudan a distinguir la orientación.
    part(glm::vec3(-0.70f, 0.75f,  2.12f), glm::vec3(0.30f, 0.22f, 0.10f), glm::vec4(1.0f, 0.95f, 0.62f, 1.0f));
    part(glm::vec3( 0.70f, 0.75f,  2.12f), glm::vec3(0.30f, 0.22f, 0.10f), glm::vec4(1.0f, 0.95f, 0.62f, 1.0f));
    part(glm::vec3(-0.70f, 0.72f, -2.12f), glm::vec3(0.30f, 0.22f, 0.10f), glm::vec4(1.0f, 0.08f, 0.05f, 1.0f));
    part(glm::vec3( 0.70f, 0.72f, -2.12f), glm::vec3(0.30f, 0.22f, 0.10f), glm::vec4(1.0f, 0.08f, 0.05f, 1.0f));
}

void drawParkingIndicators(Shader& primitiveShader, unsigned int cubeVAO, float currentTime)
{
    // Núcleo visible de cada semáforo.
    for (const ParkingSlot& slot : parkingSlots)
    {
        const float aisleOffsetZ = slot.row == 'A' ? -4.15f : 4.15f;
        const glm::vec3 lampPosition =
            slot.position + glm::vec3(0.0f, 2.75f, aisleOffsetZ);

        glm::vec4 statusColor = slotStateColor(slot.state);
        if (slot.state == SlotState::Reserved)
        {
            const float pulse = 0.72f + 0.28f * std::sin(currentTime * 5.0f);
            statusColor *= glm::vec4(pulse, pulse, pulse, 1.0f);
            statusColor.a = 1.0f;
        }

        drawCube(
            primitiveShader,
            cubeVAO,
            lampPosition,
            glm::vec3(0.42f, 0.32f, 0.42f),
            0.0f,
            statusColor
        );
    }

    // Halo aditivo: el cubo parece emitir el color correspondiente.
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (const ParkingSlot& slot : parkingSlots)
    {
        const float aisleOffsetZ = slot.row == 'A' ? -4.15f : 4.15f;
        const glm::vec3 lampPosition =
            slot.position + glm::vec3(0.0f, 2.75f, aisleOffsetZ);
        glm::vec4 glow = slotStateColor(slot.state);
        const float pulse = slot.state == SlotState::Reserved
            ? 0.16f + 0.08f * (0.5f + 0.5f * std::sin(currentTime * 5.0f))
            : 0.14f;
        glow.a = pulse;
        drawCube(
            primitiveShader,
            cubeVAO,
            lampPosition,
            glm::vec3(0.86f, 0.68f, 0.86f),
            0.0f,
            glow
        );
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}
void drawRampSafetyWalls(Shader& primitiveShader, unsigned int cubeVAO)
{
    const std::array<float, 4> floorSurfaceY = {
        -2.7509f, 5.2563f, 13.2589f, 21.2597f
    };

    const glm::vec4 concreteColor(0.20f, 0.22f, 0.25f, 1.0f);
    const glm::vec4 warningColor(0.95f, 0.72f, 0.08f, 1.0f);

    for (int floor = 0; floor < 4; ++floor)
    {
        const float baseY = floorSurfaceY[floor];

        // Borde interior real de las rampas. Los extremos quedan abiertos
        // para entrar o salir por Z +/-20, pero el muro central es solido.
        for (float x : { -30.72f, 30.72f })
        {
            drawCube(
                primitiveShader,
                cubeVAO,
                glm::vec3(x, baseY + 1.65f, 0.0f),
                glm::vec3(0.70f, 3.30f, 25.0f),
                0.0f,
                concreteColor
            );

            for (float z = -9.0f; z <= 9.0f; z += 6.0f)
            {
                const float faceOffset = x < 0.0f ? 0.38f : -0.38f;
                drawCube(
                    primitiveShader,
                    cubeVAO,
                    glm::vec3(x + faceOffset, baseY + 1.65f, z),
                    glm::vec3(0.08f, 0.62f, 2.2f),
                    0.0f,
                    warningColor
                );
            }
        }
    }
}


void drawRestrictedCorridorBarriers(
    Shader& primitiveShader,
    unsigned int cubeVAO,
    float currentTime
)
{
    const std::array<float, 4> floorSurfaceY = {
        -2.7509f, 5.2563f, 13.2589f, 21.2597f
    };

    const bool redPhase = std::fmod(currentTime, 1.2f) < 0.6f;
    const glm::vec4 activeColor = redPhase
        ? glm::vec4(1.0f, 0.08f, 0.04f, 1.0f)
        : glm::vec4(1.0f, 0.72f, 0.03f, 1.0f);
    const glm::vec4 inactiveColor = redPhase
        ? glm::vec4(1.0f, 0.72f, 0.03f, 1.0f)
        : glm::vec4(1.0f, 0.08f, 0.04f, 1.0f);
    const glm::vec4 darkMetal(0.05f, 0.06f, 0.08f, 1.0f);

    struct BarrierLayout
    {
        float centerX;
        float width;
    };

    // Solo se cierran los corredores rampa-muro. Los dos corredores entre
    // los tres muros centrales permanecen completamente transitables.
    const std::array<BarrierLayout, 2> layouts = {
        BarrierLayout{ -26.92f, 7.10f },
        BarrierLayout{  27.72f, 6.30f }
    };

    for (int floor = 0; floor < 4; ++floor)
    {
        const float baseY = floorSurfaceY[floor];
        for (const BarrierLayout& layout : layouts)
        {
            const float leftX = layout.centerX - layout.width * 0.5f;
            const float rightX = layout.centerX + layout.width * 0.5f;

            for (float postX : { leftX, rightX })
            {
                drawCube(
                    primitiveShader,
                    cubeVAO,
                    glm::vec3(postX, baseY + 1.15f, 0.0f),
                    glm::vec3(0.55f, 2.30f, 0.55f),
                    0.0f,
                    darkMetal
                );
                drawCube(
                    primitiveShader,
                    cubeVAO,
                    glm::vec3(postX, baseY + 2.45f, 0.0f),
                    glm::vec3(0.78f, 0.45f, 0.78f),
                    0.0f,
                    activeColor
                );
            }

            constexpr int stripeCount = 8;
            const float stripeWidth = layout.width / static_cast<float>(stripeCount);
            for (int stripe = 0; stripe < stripeCount; ++stripe)
            {
                const float stripeX = leftX + stripeWidth * (static_cast<float>(stripe) + 0.5f);
                drawCube(
                    primitiveShader,
                    cubeVAO,
                    glm::vec3(stripeX, baseY + 1.25f, 0.0f),
                    glm::vec3(stripeWidth + 0.02f, 0.34f, 0.42f),
                    0.0f,
                    stripe % 2 == 0 ? activeColor : inactiveColor
                );
            }
        }
    }
}

void drawTrafficArrows(Shader& primitiveShader, unsigned int cubeVAO)
{
    const std::array<float, 4> floorSurfaceY = {
        -2.7509f, 5.2563f, 13.2589f, 21.2597f
    };
    const glm::vec4 whiteArrow(0.94f, 0.95f, 0.97f, 0.92f);

    glDisable(GL_CULL_FACE);

    for (int floor = 0; floor < 4; ++floor)
    {
        const float y = floorSurfaceY[floor] + 0.13f;

        // Calles junto a las filas A y B: circulacion bidireccional.
        for (float x : { -33.0f, -16.0f, 1.0f, 18.0f, 35.0f })
        {
            drawArrow(
                primitiveShader,
                cubeVAO,
                glm::vec3(x, y, 18.8f),
                glm::vec3(1.0f, 0.0f, 0.0f),
                whiteArrow,
                1.05f,
                false
            );
            drawArrow(
                primitiveShader,
                cubeVAO,
                glm::vec3(x, y, 21.2f),
                glm::vec3(-1.0f, 0.0f, 0.0f),
                whiteArrow,
                1.05f,
                false
            );
            drawArrow(
                primitiveShader,
                cubeVAO,
                glm::vec3(x, y, -21.2f),
                glm::vec3(1.0f, 0.0f, 0.0f),
                whiteArrow,
                1.05f,
                false
            );
            drawArrow(
                primitiveShader,
                cubeVAO,
                glm::vec3(x, y, -18.8f),
                glm::vec3(-1.0f, 0.0f, 0.0f),
                whiteArrow,
                1.05f,
                false
            );
        }

        // Unicamente los corredores ENTRE muros son bidireccionales.
        for (float corridorX : { -10.68f, 12.45f })
        {
            for (float z : { -12.0f, 7.0f })
            {
                drawArrow(
                    primitiveShader,
                    cubeVAO,
                    glm::vec3(corridorX - 1.0f, y, z),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    whiteArrow,
                    1.0f,
                    false
                );
                drawArrow(
                    primitiveShader,
                    cubeVAO,
                    glm::vec3(corridorX + 1.0f, y, z + 5.0f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    whiteArrow,
                    1.0f,
                    false
                );
            }
        }
    }

    // Rampa izquierda: subida real desde Z positivo a Z negativo.
    // Rampa derecha: bajada real desde Z positivo a Z negativo.
    for (int floor = 0; floor < 3; ++floor)
    {
        const glm::vec3 leftStart(-37.12f, floorSurfaceY[floor] + 0.18f, 20.0f);
        const glm::vec3 leftEnd(-37.12f, floorSurfaceY[floor + 1] + 0.18f, -20.0f);
        const glm::vec3 rightStart(37.10f, floorSurfaceY[floor + 1] + 0.18f, 20.0f);
        const glm::vec3 rightEnd(37.10f, floorSurfaceY[floor] + 0.18f, -20.0f);

        for (int marker = 1; marker <= 5; ++marker)
        {
            const float factor = static_cast<float>(marker) / 6.0f;
            drawArrow(
                primitiveShader,
                cubeVAO,
                glm::mix(leftStart, leftEnd, factor),
                leftEnd - leftStart,
                whiteArrow,
                1.05f,
                false
            );
            drawArrow(
                primitiveShader,
                cubeVAO,
                glm::mix(rightStart, rightEnd, factor),
                rightEnd - rightStart,
                whiteArrow,
                1.05f,
                false
            );
        }
    }

    glEnable(GL_CULL_FACE);
}


void drawGuidanceRoute(
    Shader& primitiveShader,
    unsigned int routeVAO,
    unsigned int routeVBO,
    unsigned int cubeVAO,
    float currentTime
)
{
    if (activeRoute.size() < 2)
        return;

    std::vector<glm::vec3> routePositions;
    routePositions.reserve(activeRoute.size());
    for (const int node : activeRoute)
    {
        glm::vec3 position = parkingGraph.nodes[node];
        position.y += 0.23f;
        routePositions.push_back(position);
    }

    glDisable(GL_CULL_FACE);

    // Linea base luminosa que une todos los nodos de Dijkstra.
    primitiveShader.setMat4("model", glm::mat4(1.0f));
    const float pulse = 0.76f + 0.24f * std::sin(currentTime * 4.0f);
    const glm::vec4 routeColor = guidanceTarget == GuidanceTarget::Exit
        ? glm::vec4(0.18f, 1.0f, 0.42f, pulse)
        : glm::vec4(0.10f, 0.90f, 1.0f, pulse);
    primitiveShader.setVec4("objectColor", routeColor);

    glBindVertexArray(routeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, routeVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(routePositions.size() * sizeof(glm::vec3)),
        routePositions.data(),
        GL_DYNAMIC_DRAW
    );
    glLineWidth(4.0f);
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(routePositions.size()));
    glLineWidth(1.0f);
    glBindVertexArray(0);

    // Flechas brillantes orientadas segun cada segmento, incluso en rampas.
    for (std::size_t i = 1; i < routePositions.size(); ++i)
    {
        const glm::vec3 startPoint = routePositions[i - 1];
        const glm::vec3 endPoint = routePositions[i];
        const glm::vec3 segment = endPoint - startPoint;
        const float segmentLength = glm::length(segment);
        if (segmentLength < 0.01f)
            continue;

        const int arrowCount = std::max(1, static_cast<int>(std::floor(segmentLength / 5.5f)));
        for (int arrow = 1; arrow <= arrowCount; ++arrow)
        {
            const float factor = static_cast<float>(arrow) / static_cast<float>(arrowCount + 1);
            const float animatedOffset = 0.04f * std::sin(currentTime * 5.0f + arrow + static_cast<float>(i));
            glm::vec3 center = glm::mix(startPoint, endPoint, std::clamp(factor + animatedOffset, 0.08f, 0.92f));
            center.y += 0.06f;
            drawArrow(
                primitiveShader,
                cubeVAO,
                center,
                segment,
                routeColor,
                1.25f,
                true
            );
        }
    }

    // Circulo pulsante tipo mision de GTA alrededor del destino.
    glm::vec3 target(0.0f);
    bool hasTarget = false;
    glm::vec4 targetColor(1.0f, 0.78f, 0.05f, 0.95f);

    if (guidanceTarget == GuidanceTarget::ParkingSlot &&
        reservedSlotIndex >= 0 &&
        reservedSlotIndex < static_cast<int>(parkingSlots.size()))
    {
        target = parkingSlots[reservedSlotIndex].position + glm::vec3(0.0f, 0.26f, 0.0f);
        hasTarget = true;
    }
    else if (guidanceTarget == GuidanceTarget::Exit && parkingGraph.exitNode >= 0)
    {
        target = parkingGraph.nodes[parkingGraph.exitNode] + glm::vec3(0.0f, 0.18f, 0.0f);
        targetColor = glm::vec4(0.18f, 1.0f, 0.42f, 0.95f);
        hasTarget = true;
    }

    if (hasTarget)
    {
        const float radius = 3.4f + 0.35f * std::sin(currentTime * 4.5f);
        constexpr int markerCount = 40;
        for (int marker = 0; marker < markerCount; ++marker)
        {
            const float angle = 2.0f * PI_VALUE * static_cast<float>(marker) / static_cast<float>(markerCount);
            const glm::vec3 markerPosition = target + glm::vec3(
                std::cos(angle) * radius,
                0.0f,
                std::sin(angle) * radius
            );
            drawCube(
                primitiveShader,
                cubeVAO,
                markerPosition,
                glm::vec3(0.32f, 0.10f, 0.65f),
                -glm::degrees(angle),
                targetColor
            );
        }

        // Haz vertical tenue que ayuda a localizar el destino entre pisos.
        drawCube(
            primitiveShader,
            cubeVAO,
            target + glm::vec3(0.0f, 2.2f, 0.0f),
            glm::vec3(0.18f, 4.4f, 0.18f),
            0.0f,
            glm::vec4(targetColor.r, targetColor.g, targetColor.b, 0.35f)
        );
    }

    glEnable(GL_CULL_FACE);
}

void addRect(
    std::vector<UIVertex>& vertices,
    float x,
    float y,
    float width,
    float height,
    const glm::vec4& color
)
{
    const UIVertex topLeft{ glm::vec2(x, y), color };
    const UIVertex topRight{ glm::vec2(x + width, y), color };
    const UIVertex bottomLeft{ glm::vec2(x, y + height), color };
    const UIVertex bottomRight{ glm::vec2(x + width, y + height), color };

    vertices.push_back(topLeft);
    vertices.push_back(bottomLeft);
    vertices.push_back(bottomRight);
    vertices.push_back(topLeft);
    vertices.push_back(bottomRight);
    vertices.push_back(topRight);
}

const std::array<std::uint8_t, 7>& glyphForCharacter(char character)
{
    static const std::array<std::uint8_t, 7> blank{ 0,0,0,0,0,0,0 };
    static const std::unordered_map<char, std::array<std::uint8_t, 7>> glyphs = {
        {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
        {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
        {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
        {'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
        {'I',{31,4,4,4,4,4,31}},      {'J',{7,2,2,2,18,18,12}},
        {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
        {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
        {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
        {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
        {'S',{15,16,16,14,1,1,30}},   {'T',{31,4,4,4,4,4,4}},
        {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
        {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
        {'Y',{17,17,10,4,4,4,4}},     {'Z',{31,1,2,4,8,16,31}},
        {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
        {'2',{14,17,1,2,4,8,31}},     {'3',{30,1,1,14,1,1,30}},
        {'4',{2,6,10,18,31,2,2}},     {'5',{31,16,16,30,1,1,30}},
        {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
        {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
        {':',{0,4,4,0,4,4,0}},        {'-',{0,0,0,31,0,0,0}},
        {'.',{0,0,0,0,0,6,6}},        {'/',{1,2,2,4,8,8,16}},
        {' ',{0,0,0,0,0,0,0}}
    };

    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    const auto iterator = glyphs.find(upper);
    return iterator != glyphs.end() ? iterator->second : blank;
}

void addText(
    std::vector<UIVertex>& vertices,
    const std::string& text,
    float x,
    float y,
    float scale,
    const glm::vec4& color
)
{
    float cursorX = x;
    float cursorY = y;

    for (const char character : text)
    {
        if (character == '\n')
        {
            cursorX = x;
            cursorY += 9.0f * scale;
            continue;
        }

        const auto& glyph = glyphForCharacter(character);
        for (int row = 0; row < 7; ++row)
        {
            for (int column = 0; column < 5; ++column)
            {
                const std::uint8_t mask = static_cast<std::uint8_t>(1u << (4 - column));
                if ((glyph[row] & mask) != 0)
                {
                    addRect(
                        vertices,
                        cursorX + static_cast<float>(column) * scale,
                        cursorY + static_cast<float>(row) * scale,
                        scale,
                        scale,
                        color
                    );
                }
            }
        }

        cursorX += 6.0f * scale;
    }
}

void renderPhoneUI(Shader& uiShader, unsigned int uiVAO, unsigned int uiVBO)
{
    std::vector<UIVertex> vertices;
    vertices.reserve(18000);

    if (phoneVisible)
    {
        const float phoneWidth = 370.0f;
        const float phoneHeight = 520.0f;
        const float phoneX = std::max(
            10.0f,
            static_cast<float>(framebufferWidth) - phoneWidth - 22.0f
        );
        const float phoneY = std::max(
            10.0f,
            static_cast<float>(framebufferHeight) - phoneHeight - 22.0f
        );

        addRect(
            vertices,
            phoneX,
            phoneY,
            phoneWidth,
            phoneHeight,
            glm::vec4(0.012f, 0.014f, 0.020f, 0.96f)
        );
        addRect(
            vertices,
            phoneX + 6.0f,
            phoneY + 6.0f,
            phoneWidth - 12.0f,
            phoneHeight - 12.0f,
            glm::vec4(0.028f, 0.045f, 0.070f, 0.97f)
        );
        addRect(
            vertices,
            phoneX + 138.0f,
            phoneY + 13.0f,
            94.0f,
            7.0f,
            glm::vec4(0.005f, 0.006f, 0.008f, 1.0f)
        );

        addText(
            vertices,
            "SMART PARKING",
            phoneX + 48.0f,
            phoneY + 34.0f,
            3.0f,
            glm::vec4(0.82f, 0.94f, 1.0f, 1.0f)
        );
        addText(
            vertices,
            "SIMULADOR DE ESTACIONAMIENTO",
            phoneX + 26.0f,
            phoneY + 68.0f,
            1.45f,
            glm::vec4(0.35f, 0.80f, 1.0f, 1.0f)
        );

        const std::array<std::string, 4> floorNames = {
            "PLANTA BAJA", "PISO 1", "PISO 2", "PISO 3"
        };

        int floorAvailable = 0;
        int floorOccupied = 0;
        int floorReserved = 0;
        int floorDisabled = 0;
        for (const ParkingSlot& slot : parkingSlots)
        {
            if (slot.floorIndex != selectedPhoneFloor)
                continue;
            if (slot.state == SlotState::Available) ++floorAvailable;
            if (slot.state == SlotState::Occupied) ++floorOccupied;
            if (slot.state == SlotState::Reserved) ++floorReserved;
            if (slot.state == SlotState::Disabled) ++floorDisabled;
        }

        addText(
            vertices,
            "F: " + floorNames[selectedPhoneFloor],
            phoneX + 24.0f,
            phoneY + 102.0f,
            1.9f,
            glm::vec4(0.88f, 0.94f, 1.0f, 1.0f)
        );

        std::ostringstream counts;
        counts
            << "VERDES " << floorAvailable
            << "  ROJOS " << floorOccupied
            << "  AMARILLOS " << floorReserved
            << "  AZULES " << floorDisabled;
        addText(
            vertices,
            counts.str(),
            phoneX + 24.0f,
            phoneY + 132.0f,
            1.05f,
            glm::vec4(0.74f, 0.82f, 0.90f, 1.0f)
        );

        const int selectedIndex = getSelectedPhoneSlotIndex();
        const ParkingSlot* selectedSlot =
            selectedIndex >= 0 ? &parkingSlots[selectedIndex] : nullptr;

        if (selectedSlot != nullptr)
        {
            const glm::vec4 selectedColor = slotStateColor(selectedSlot->state);
            addRect(
                vertices,
                phoneX + 22.0f,
                phoneY + 158.0f,
                phoneWidth - 44.0f,
                42.0f,
                glm::vec4(selectedColor.r, selectedColor.g, selectedColor.b, 0.18f)
            );
            addText(
                vertices,
                "DESTINO: " + selectedSlot->id,
                phoneX + 34.0f,
                phoneY + 170.0f,
                1.75f,
                glm::vec4(selectedColor.r, selectedColor.g, selectedColor.b, 1.0f)
            );
        }

        const float gridX = phoneX + 34.0f;
        const float gridY = phoneY + 220.0f;
        const float cellWidth = 13.7f;
        const float cellHeight = 34.0f;

        for (const ParkingSlot& slot : parkingSlots)
        {
            if (slot.floorIndex != selectedPhoneFloor)
                continue;

            const int rowIndex = slot.row == 'A' ? 0 : 1;
            const int column = slot.number - 1;
            const int ordinal = slotGridOrdinal(slot);
            const float x = gridX + static_cast<float>(column) * cellWidth;
            const float y = gridY + static_cast<float>(rowIndex) * 46.0f;
            const glm::vec4 color = slotStateColor(slot.state);
            const bool selected = ordinal == selectedPhoneSlotOrdinal;

            if (selected)
            {
                addRect(
                    vertices,
                    x - 2.0f,
                    y - 2.0f,
                    15.0f,
                    cellHeight + 4.0f,
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.95f)
                );
            }

            addRect(
                vertices,
                x,
                y,
                11.0f,
                cellHeight,
                glm::vec4(0.01f, 0.015f, 0.025f, 0.98f)
            );
            addRect(
                vertices,
                x + 2.0f,
                y + 2.0f,
                7.0f,
                cellHeight - 4.0f,
                glm::vec4(color.r, color.g, color.b, 0.96f)
            );
        }

        addText(
            vertices,
            "A",
            phoneX + 17.0f,
            gridY + 10.0f,
            1.4f,
            glm::vec4(0.86f, 0.92f, 1.0f, 1.0f)
        );
        addText(
            vertices,
            "B",
            phoneX + 17.0f,
            gridY + 56.0f,
            1.4f,
            glm::vec4(0.86f, 0.92f, 1.0f, 1.0f)
        );
        addText(
            vertices,
            "01          11          22",
            gridX,
            phoneY + 318.0f,
            0.82f,
            glm::vec4(0.62f, 0.72f, 0.82f, 1.0f)
        );

        addText(
            vertices,
            "G: NUEVA SIMULACION RANDOM",
            phoneX + 24.0f,
            phoneY + 350.0f,
            1.45f,
            glm::vec4(1.0f, 0.82f, 0.18f, 1.0f)
        );
        addText(
            vertices,
            "J/L: ELEGIR ESPACIO VERDE",
            phoneX + 24.0f,
            phoneY + 378.0f,
            1.35f,
            glm::vec4(0.82f, 0.92f, 1.0f, 1.0f)
        );
        addText(
            vertices,
            "K: MOSTRAR RUTA DIJKSTRA",
            phoneX + 24.0f,
            phoneY + 405.0f,
            1.35f,
            glm::vec4(0.20f, 0.92f, 1.0f, 1.0f)
        );
        addText(
            vertices,
            "E: RUTA A LA SALIDA",
            phoneX + 24.0f,
            phoneY + 432.0f,
            1.35f,
            glm::vec4(0.28f, 1.0f, 0.48f, 1.0f)
        );

        std::string routeText;
        if (guidanceTarget == GuidanceTarget::ParkingSlot && reservedSlotIndex >= 0)
        {
            routeText = "GUIA A: " + parkingSlots[reservedSlotIndex].id;
        }
        else if (guidanceTarget == GuidanceTarget::Exit)
        {
            routeText = "GUIA A: SALIDA";
        }
        else if (mainVehicleParkedSlotIndex >= 0 &&
                 mainVehicleParkedSlotIndex < static_cast<int>(parkingSlots.size()))
        {
            routeText = "MAZDA EN: " + parkingSlots[mainVehicleParkedSlotIndex].id;
        }
        else if (controlledVehicle != nullptr)
        {
            const glm::vec3 position = controlledVehicle->getPosition();
            std::ostringstream location;
            location << "MAZDA X" << static_cast<int>(std::round(position.x))
                     << " Z" << static_cast<int>(std::round(position.z));
            routeText = location.str();
        }
        else
        {
            routeText = "SIN GUIA ACTIVA";
        }

        addText(
            vertices,
            routeText,
            phoneX + 24.0f,
            phoneY + 470.0f,
            1.45f,
            guidanceTarget == GuidanceTarget::None
                ? glm::vec4(0.62f, 0.72f, 0.82f, 1.0f)
                : glm::vec4(1.0f, 0.82f, 0.18f, 1.0f)
        );
    }

    if (noticeTimer > 0.0f)
    {
        const float panelWidth = std::min(
            noticeSuccessStyle ? 860.0f : 760.0f,
            static_cast<float>(framebufferWidth) - 40.0f
        );
        const float panelHeight = noticeSuccessStyle ? 145.0f : 112.0f;
        const float panelX =
            (static_cast<float>(framebufferWidth) - panelWidth) * 0.5f;
        const float panelY = noticeSuccessStyle
            ? (static_cast<float>(framebufferHeight) - panelHeight) * 0.32f
            : 45.0f;

        const glm::vec4 accent = noticeSuccessStyle
            ? glm::vec4(0.96f, 0.78f, 0.12f, 1.0f)
            : glm::vec4(0.95f, 0.72f, 0.10f, 0.95f);

        addRect(
            vertices,
            panelX,
            panelY,
            panelWidth,
            panelHeight,
            glm::vec4(0.008f, 0.010f, 0.015f, 0.92f)
        );
        addRect(vertices, panelX, panelY, panelWidth, 7.0f, accent);
        addRect(
            vertices,
            panelX,
            panelY + panelHeight - 7.0f,
            panelWidth,
            7.0f,
            accent
        );

        const float titleScale = noticeSuccessStyle ? 4.4f : 3.3f;
        const float titleWidth =
            static_cast<float>(noticeTitle.size()) * 6.0f * titleScale;
        addText(
            vertices,
            noticeTitle,
            panelX + std::max(18.0f, (panelWidth - titleWidth) * 0.5f),
            panelY + (noticeSuccessStyle ? 30.0f : 22.0f),
            titleScale,
            accent
        );

        const float subtitleScale = noticeSuccessStyle ? 2.1f : 1.8f;
        const float subtitleWidth =
            static_cast<float>(noticeSubtitle.size()) * 6.0f * subtitleScale;
        addText(
            vertices,
            noticeSubtitle,
            panelX + std::max(18.0f, (panelWidth - subtitleWidth) * 0.5f),
            panelY + (noticeSuccessStyle ? 94.0f : 70.0f),
            subtitleScale,
            glm::vec4(0.92f, 0.95f, 1.0f, 1.0f)
        );
    }

    if (vertices.empty())
        return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    uiShader.use();
    uiShader.setVec2(
        "screenSize",
        glm::vec2(
            static_cast<float>(framebufferWidth),
            static_cast<float>(framebufferHeight)
        )
    );

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(UIVertex)),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );
    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(vertices.size())
    );
    glBindVertexArray(0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}


void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    framebufferWidth = std::max(width, 1);
    framebufferHeight = std::max(height, 1);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}


glm::vec3 getSimulationPosition()
{
    if (controlledVehicle != nullptr)
        return controlledVehicle->getPosition();

    return camera.Position;
}

// =========================================================
// MANDO XBOX
// =========================================================
void processGamepadInput(Vehicle& vehicle)
{
    if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
        return;

    GLFWgamepadstate state{};
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
        return;

    float steering = -state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];

    constexpr float DEAD_ZONE = 0.15f;
    if (std::abs(steering) < DEAD_ZONE)
    {
        steering = 0.0f;
    }
    else
    {
        const float sign = steering > 0.0f ? 1.0f : -1.0f;
        steering =
            sign * ((std::abs(steering) - DEAD_ZONE) / (1.0f - DEAD_ZONE));
    }

    float cameraX = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
    float cameraY = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

    if (std::abs(cameraX) < DEAD_ZONE) cameraX = 0.0f;
    if (std::abs(cameraY) < DEAD_ZONE) cameraY = 0.0f;

    // Stick derecho: mirar en cámara libre.
    if (cameraMode == CameraMode::FREE)
    {
        constexpr float CAMERA_SPEED = 300.0f;
        camera.ProcessMouseMovement(
            cameraX * CAMERA_SPEED * deltaTime,
            -cameraY * CAMERA_SPEED * deltaTime
        );

        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS)
            moveCameraSafely(FORWARD);
        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS)
            moveCameraSafely(BACKWARD);
        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS)
            moveCameraSafely(LEFT);
        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS)
            moveCameraSafely(RIGHT);
    }

    float rightTrigger =
        glm::clamp(
            (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0f) * 0.5f,
            0.0f,
            1.0f
        );

    float leftTrigger =
        glm::clamp(
            (state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0f) * 0.5f,
            0.0f,
            1.0f
        );

    float throttle = rightTrigger - leftTrigger;
    if (std::abs(throttle) < 0.05f)
        throttle = 0.0f;

    const bool brake =
        state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS;

    const bool reset =
        state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;

    const bool leftBumper =
        state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;

    const bool rightBumper =
        state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS;

    if (rightBumper && !rightBumperPressed)
    {
        if (cameraMode == CameraMode::FREE)
            cameraMode = CameraMode::THIRD_PERSON;
        else if (cameraMode == CameraMode::THIRD_PERSON)
            cameraMode = CameraMode::DRIVER;
        else
            cameraMode = CameraMode::FREE;

        resetThirdPersonCameraOffset();
    }

    if (leftBumper && !leftBumperPressed)
    {
        if (cameraMode == CameraMode::DRIVER)
            cameraMode = CameraMode::THIRD_PERSON;
        else if (cameraMode == CameraMode::THIRD_PERSON)
            cameraMode = CameraMode::FREE;
        else
            cameraMode = CameraMode::DRIVER;

        resetThirdPersonCameraOffset();
    }

    leftBumperPressed = leftBumper;
    rightBumperPressed = rightBumper;

    const bool controllerDriving =
        std::abs(throttle) > 0.05f ||
        std::abs(steering) > 0.05f ||
        brake ||
        reset;

    if (controllerDriving)
    {
        vehicle.processControllerInput(
            throttle,
            steering,
            brake,
            reset
        );
    }
}

// =========================================================
// CÁMARAS DEL VEHÍCULO
// =========================================================
glm::mat4 getVehicleCameraView(const Vehicle& vehicle)
{
    const glm::vec3 vehiclePosition = vehicle.getPosition();
    const glm::vec3 forward = vehicle.getForwardVector();
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    if (cameraMode == CameraMode::FREE)
        return camera.GetViewMatrix();

    const bool reversing = vehicle.getSpeed() < -0.05f;
    const glm::vec3 cameraDirection = reversing ? -forward : forward;
    const float vehiclePitch = vehicle.getPitch();

    if (cameraMode == CameraMode::THIRD_PERSON)
    {
        const float rampFactor =
            glm::clamp(std::abs(vehiclePitch) / 14.0f, 0.0f, 1.0f);

        const float followDistance = glm::mix(9.0f, 2.8f, rampFactor);
        const float followHeight = glm::mix(4.2f, 2.6f, rampFactor);
        const float lookAhead = glm::mix(2.5f, 5.0f, rampFactor);

        const glm::mat4 yawRotation =
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(thirdPersonYawOffset),
                worldUp
            );

        glm::vec3 orbitDirection =
            glm::normalize(
                glm::vec3(
                    yawRotation * glm::vec4(cameraDirection, 0.0f)
                )
            );

        glm::vec3 rightAxis = glm::cross(orbitDirection, worldUp);
        if (glm::length(rightAxis) > 0.0001f)
        {
            rightAxis = glm::normalize(rightAxis);
            const glm::mat4 pitchRotation =
                glm::rotate(
                    glm::mat4(1.0f),
                    glm::radians(thirdPersonPitchOffset),
                    rightAxis
                );

            orbitDirection =
                glm::normalize(
                    glm::vec3(
                        pitchRotation * glm::vec4(orbitDirection, 0.0f)
                    )
                );
        }

        const glm::vec3 cameraPosition =
            vehiclePosition
            - orbitDirection * followDistance
            + worldUp * followHeight;

        const glm::vec3 target =
            vehiclePosition
            + cameraDirection * lookAhead
            + worldUp * 0.9f;

        return glm::lookAt(cameraPosition, target, worldUp);
    }

    if (cameraMode == CameraMode::DRIVER)
    {
        const glm::vec3 cameraPosition =
            vehiclePosition + forward * 1.85f + worldUp * 1.15f;

        const glm::vec3 target =
            cameraPosition + cameraDirection * 12.0f;

        return glm::lookAt(cameraPosition, target, worldUp);
    }

    return camera.GetViewMatrix();
}

glm::vec3 getActiveCameraPosition(const Vehicle& vehicle)
{
    const glm::vec3 vehiclePosition = vehicle.getPosition();
    const glm::vec3 forward = vehicle.getForwardVector();
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    if (cameraMode == CameraMode::FREE)
        return camera.Position;

    const bool reversing = vehicle.getSpeed() < -0.05f;
    const glm::vec3 cameraDirection = reversing ? -forward : forward;

    if (cameraMode == CameraMode::THIRD_PERSON)
    {
        const float rampFactor =
            glm::clamp(std::abs(vehicle.getPitch()) / 14.0f, 0.0f, 1.0f);

        const float followDistance = glm::mix(9.0f, 2.8f, rampFactor);
        const float followHeight = glm::mix(4.2f, 2.6f, rampFactor);

        const glm::mat4 yawRotation =
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(thirdPersonYawOffset),
                worldUp
            );

        glm::vec3 orbitDirection =
            glm::normalize(
                glm::vec3(
                    yawRotation * glm::vec4(cameraDirection, 0.0f)
                )
            );

        glm::vec3 rightAxis = glm::cross(orbitDirection, worldUp);
        if (glm::length(rightAxis) > 0.0001f)
        {
            rightAxis = glm::normalize(rightAxis);
            const glm::mat4 pitchRotation =
                glm::rotate(
                    glm::mat4(1.0f),
                    glm::radians(thirdPersonPitchOffset),
                    rightAxis
                );

            orbitDirection =
                glm::normalize(
                    glm::vec3(
                        pitchRotation * glm::vec4(orbitDirection, 0.0f)
                    )
                );
        }

        return
            vehiclePosition
            - orbitDirection * followDistance
            + worldUp * followHeight;
    }

    if (cameraMode == CameraMode::DRIVER)
    {
        return
            vehiclePosition
            + forward * 1.85f
            + worldUp * 1.15f;
    }

    return camera.Position;
}

void updateThirdPersonCameraReturn(float frameDeltaTime)
{
    if (cameraMode != CameraMode::THIRD_PERSON)
        return;

    const float currentTime = static_cast<float>(glfwGetTime());
    if (currentTime - lastThirdPersonMouseTime < THIRD_PERSON_RETURN_DELAY)
        return;

    const float returnFactor =
        1.0f - std::exp(-THIRD_PERSON_RETURN_SPEED * frameDeltaTime);

    thirdPersonYawOffset =
        glm::mix(thirdPersonYawOffset, 0.0f, returnFactor);

    thirdPersonPitchOffset =
        glm::mix(thirdPersonPitchOffset, 0.0f, returnFactor);

    if (std::abs(thirdPersonYawOffset) < 0.01f)
        thirdPersonYawOffset = 0.0f;

    if (std::abs(thirdPersonPitchOffset) < 0.01f)
        thirdPersonPitchOffset = 0.0f;
}

void resetThirdPersonCameraOffset()
{
    firstMouse = true;
    thirdPersonYawOffset = 0.0f;
    thirdPersonPitchOffset = 0.0f;
    lastThirdPersonMouseTime = static_cast<float>(glfwGetTime());
}

// =========================================================
// CALLBACKS
// =========================================================
void mouse_callback(GLFWwindow*, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
        return;
    }

    const float xOffset = static_cast<float>(xpos) - lastX;
    const float yOffset = lastY - static_cast<float>(ypos);

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    if (cameraMode == CameraMode::FREE)
    {
        camera.ProcessMouseMovement(xOffset, yOffset);
        return;
    }

    if (cameraMode == CameraMode::THIRD_PERSON)
    {
        thirdPersonYawOffset +=
            xOffset * THIRD_PERSON_MOUSE_SENSITIVITY;

        thirdPersonPitchOffset +=
            yOffset * THIRD_PERSON_MOUSE_SENSITIVITY;

        thirdPersonYawOffset =
            glm::clamp(thirdPersonYawOffset, -75.0f, 75.0f);

        thirdPersonPitchOffset =
            glm::clamp(thirdPersonPitchOffset, -20.0f, 30.0f);

        lastThirdPersonMouseTime =
            static_cast<float>(glfwGetTime());
    }
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
