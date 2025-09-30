#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <numbers>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "./header/Shader.h"
#include "./header/Object.h"

// Settings
const int INITIAL_SCR_WIDTH = 800;
const int INITIAL_SCR_HEIGHT = 600;
const float AQUARIUM_BOUND_X = 35.0f;
const float AQUARIUM_BOUND_Z = 20.0f;

// Animation constants
const float TAIL_ANIMATION_SPEED = 5.0f;
const float WAVE_FREQUENCY = 1.5f;

int SCR_WIDTH = INITIAL_SCR_WIDTH;
int SCR_HEIGHT = INITIAL_SCR_HEIGHT;

// Global objects
Shader* shader = nullptr;
Object* cube = nullptr;
Object* fish1 = nullptr;
Object* fish2 = nullptr;
Object* fish3 = nullptr;

struct Fish {
    glm::vec3 position;
    glm::vec3 direction;
    std::string fishType = "fish1";
    float angle = 0.0f;
    float speed = 3.0f;
    glm::vec3 scale = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 color = glm::vec3(1.0f, 0.5f, 0.3f);
};

struct SeaweedSegment {
    glm::vec3 localPos;
    glm::vec3 color;
    float phase;
    glm::vec3 scale;
    SeaweedSegment* next = nullptr;
};

struct Seaweed {
    glm::vec3 basePosition;
    SeaweedSegment* rootSegment = nullptr;
    float swayOffset = 0.0f;
};

struct playerFish {
    glm::vec3 position = glm::vec3(0.0f, 5.0f, 0.0f);
    float angle = 0.0f; // Heading direction in radians
    float speed = 2.0f;
    float rotationSpeed = 2.0f;
    bool mouthOpen = false; 
    float tailAnimation = 0.0f;
    // for tooth
    float duration = 1.0f;     
    float elapsed = 0.0f;    
    struct tooth{
        glm::vec3 pos0, pos1;
    }toothUpperLeft, toothUpperRight, toothLowerLeft, toothLowerRight;
   
} playerFish;

// Aquarium elements
std::vector<Seaweed> seaweeds;
std::vector<Fish> schoolFish;

float globalTime = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window, float deltaTime);
void drawModel(std::string type, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color);
void drawPlayerFish(const glm::vec3& position, float angle, float tailPhase,
                    const glm::mat4& view, const glm::mat4& projection, bool mouthOpen, float deltaTime);
void updateSchoolFish(float deltaTime);
void initializeAquarium();
void cleanup();
void init();

int main() {
    // Initialize random seed for aquarium elements
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // GLFW: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GPU-Accelerated Aquarium", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // TODO: Enable depth test, face culling
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    init();
    initializeAquarium();

    float lastFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        globalTime = currentFrame;

        playerFish.tailAnimation += deltaTime * TAIL_ANIMATION_SPEED;

        glClearColor(0.2f, 0.5f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 10.0f, 25.0f), glm::vec3(0.0f, 8.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 1000.0f);

        glm::mat4 baseModel = glm::mat4(1.0f);
        baseModel = glm::translate(baseModel, glm::vec3(0.0f, 0.0f, 0.0f));
        baseModel = glm::scale(baseModel, glm::vec3(70.0f, 1.0f, 40.0f));
        drawModel("cube", baseModel, view, projection, glm::vec3(0.9f, 0.8f, 0.6f));
        
        for (const auto& seaweed : seaweeds) {
            glm::mat4 currentModel = glm::translate(glm::mat4(1.0f), seaweed.basePosition);
            SeaweedSegment* seg = seaweed.rootSegment;
            int index = 0;
            while (seg) {
                float swayAngle = 0.2f * sin(globalTime * WAVE_FREQUENCY + seg->phase + seaweed.swayOffset);
                currentModel = glm::rotate(currentModel, swayAngle, glm::vec3(0.0f, 0.0f, 1.0f));
                
                glm::vec3 basePos = glm::vec3(0.0f, seg->scale.y / 2.0f, 0.0f);
                glm::vec3 swayPos = glm::vec3(0.1f * sin(globalTime + seg->phase), seg->scale.y / 2.0f, 0.0f);
                glm::vec3 finalPos = glm::mix(basePos, swayPos, -0.8f);
                
                glm::mat4 segModel = glm::translate(currentModel, finalPos);
                segModel = glm::scale(segModel, seg->scale);
                drawModel("cube", segModel, view, projection, seg->color);
                currentModel = glm::translate(currentModel, glm::vec3(0.0f, seg->scale.y, 0.0f));
                seg = seg->next;
                index++;
            }
        }

        for (const auto& fish : schoolFish) {
            glm::mat4 model(1.0f);
            model = glm::translate(model, fish.position);
            model = glm::rotate(model, fish.angle, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, fish.scale);
            drawModel(fish.fishType, model, view, projection, fish.color);
        }
        updateSchoolFish(deltaTime);

        drawPlayerFish(playerFish.position, playerFish.angle, playerFish.tailAnimation,
                        view, projection, playerFish.mouthOpen, deltaTime);

        processInput(window, deltaTime);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanup();
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

void processInput(GLFWwindow* window, float deltaTime) {
    glm::vec3 moveDir(0.0f);
    glm::vec3 faceDir(0.0f);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        moveDir.z -= 1.0f;
        faceDir.z += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        moveDir.z += 1.0f;
        faceDir.z -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        moveDir.x -= 1.0f;
        faceDir.x -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        moveDir.x += 1.0f;
        faceDir.x += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        moveDir.y += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        moveDir.y -= 1.0f;
    }

    float moveLength = glm::length(moveDir);
    if (moveLength > 0.0f) {
        moveDir /= moveLength;
        playerFish.position += moveDir * playerFish.speed * deltaTime;
    }
    
    float faceLength = glm::length(glm::vec2(faceDir.x, faceDir.z));
    if (faceLength > 0.0f) {
        glm::vec3 horizFaceDir = glm::normalize(glm::vec3(faceDir.x, 0.0f, faceDir.z));
        playerFish.angle = glm::atan(horizFaceDir.z, horizFaceDir.x);
    }

    // TODO: Keep fish within aquarium bounds
    playerFish.position.y = glm::max(playerFish.position.y, 1.5f);
    playerFish.position.y = glm::clamp(playerFish.position.y, 1.5f, 18.0f);
    playerFish.position.x = glm::clamp(playerFish.position.x, -AQUARIUM_BOUND_X+20, AQUARIUM_BOUND_X-20);
    playerFish.position.z = glm::clamp(playerFish.position.z, -AQUARIUM_BOUND_Z, AQUARIUM_BOUND_Z);
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // The action is one of GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE. 
    // Events with GLFW_PRESS and GLFW_RELEASE actions are emitted for every key press.
    // Most keys will also emit events with GLFW_REPEAT actions while a key is held down.
    // https://www.glfw.org/docs/3.3/input_guide.html
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // TODO: Implement mouth toggle logic
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        playerFish.mouthOpen = !playerFish.mouthOpen;
        if (playerFish.mouthOpen) {
            playerFish.elapsed = 0.0f;
        }
    }
}

void drawModel(std::string type, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color) {
    shader->set_uniform("projection", projection);
    shader->set_uniform("view", view);
    shader->set_uniform("model", model);
    shader->set_uniform("objectColor", color);
    if (type == "fish1") {
        fish1->draw();
    } else if (type == "fish2") {
        fish2->draw();
    } else if (type == "fish3") {
        fish3->draw();
    }else if (type == "cube") {
        cube->draw();
    }
}

void init() {
#if defined(__linux__) || defined(__APPLE__)
    std::string dirShader = "shaders/";
    std::string dirAsset = "asset/";
#else
    std::string dirShader = "shaders\\";
    std::string dirAsset = "asset\\";
#endif

    shader = new Shader((dirShader + "easy.vert").c_str(), (dirShader + "easy.frag").c_str());
   
    cube = new Object(dirAsset + "cube.obj");
    fish1 = new Object(dirAsset + "fish1.obj");
    fish2 = new Object(dirAsset + "fish2.obj");
    fish3 = new Object(dirAsset + "fish3.obj");
}

void cleanup() {
    if (shader) {
        delete shader;
        shader = nullptr;
    }
    
    if (cube) {
        delete cube;
        cube = nullptr;
    }
    
    for (auto& seaweed : seaweeds) {
        SeaweedSegment* current = seaweed.rootSegment;
        while (current != nullptr) {
            SeaweedSegment* next = current->next;
            delete current;
            current = next;
        }
    }
    seaweeds.clear();
    
    schoolFish.clear();
}

void drawPlayerFish(const glm::vec3& position, float angle, float tailPhase,
    const glm::mat4& view, const glm::mat4& projection, bool mouthOpen, float deltaTime) {
    glm::mat4 model(1.0f);

    model = glm::translate(model, position);
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 fishModel = model;
    
    glm::mat4 bodyModel = glm::scale(fishModel, glm::vec3(5.0f, 3.0f, 2.5f));
    drawModel("cube", bodyModel, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
  
    glm::vec3 upperJawConnection = glm::vec3(3.0f, 0.3f, 0.0f);
    glm::vec3 lowerJawConnection = glm::vec3(2.3f, -1.0f, 0.0f);
    
    glm::mat4 headModel = glm::translate(fishModel, upperJawConnection);
    headModel = glm::rotate(headModel, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    headModel = glm::translate(headModel, glm::vec3(0.0f, 0.0f, 0.0f));
    headModel = glm::scale(headModel, glm::vec3(2.7f, 1.5f, 2.0f));
    drawModel("cube", headModel, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));

    glm::mat4 mouthModel = glm::translate(fishModel, lowerJawConnection);
    float mouthRotation = mouthOpen ? glm::radians(-20.0f) : glm::radians(10.0f);
    mouthModel = glm::rotate(mouthModel, mouthRotation, glm::vec3(0.0f, 0.0f, 1.0f));

    if (mouthOpen) {
        playerFish.elapsed += deltaTime;
        float t = glm::min(0.85f, playerFish.elapsed / playerFish.duration);

        glm::mat4 headForTeeth = glm::translate(headModel, glm::vec3(0.7f, -1.9f, 0.0f));
        headForTeeth = glm::rotate(headForTeeth, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        headForTeeth = glm::translate(headForTeeth, glm::vec3(-1.25f, 0.7f, 0.0f));

        glm::mat4 toothURModel = glm::translate(headForTeeth, glm::mix(playerFish.toothUpperRight.pos0, playerFish.toothUpperRight.pos1, t));
        toothURModel = glm::scale(toothURModel, glm::vec3(0.15f, 0.3f, 0.1f));
        drawModel("cube", toothURModel, view, projection, glm::vec3(1.0f, 1.0f, 1.0f));

        glm::mat4 toothULModel = glm::translate(headForTeeth, glm::mix(playerFish.toothUpperLeft.pos0, playerFish.toothUpperLeft.pos1, t));
        toothULModel = glm::scale(toothULModel, glm::vec3(0.15f, 0.3f, 0.1f));
        drawModel("cube", toothULModel, view, projection, glm::vec3(1.0f, 1.0f, 1.0f));

        glm::mat4 mouthForTeeth = glm::translate(fishModel, lowerJawConnection);
        mouthForTeeth = glm::rotate(mouthForTeeth, glm::radians(-10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mouthForTeeth = glm::translate(mouthForTeeth, glm::vec3(0.5f, 0.75f, 0.0f));

        glm::mat4 toothLRModel = glm::translate(mouthForTeeth, glm::mix(playerFish.toothLowerRight.pos0, playerFish.toothLowerRight.pos1, t));
        toothLRModel = glm::scale(toothLRModel, glm::vec3(0.2f, 0.4f, 0.2f));
        drawModel("cube", toothLRModel, view, projection, glm::vec3(1.0f, 1.0f, 1.0f));

        glm::mat4 toothLLModel = glm::translate(mouthForTeeth, glm::mix(playerFish.toothLowerLeft.pos0, playerFish.toothLowerLeft.pos1, t));
        toothLLModel = glm::scale(toothLLModel, glm::vec3(0.2f, 0.4f, 0.2f));
        drawModel("cube", toothLLModel, view, projection, glm::vec3(1.0f, 1.0f, 1.0f));
    }

    mouthModel = glm::scale(mouthModel, glm::vec3(2.5f, 0.6f, 1.8f));
    drawModel("cube", mouthModel, view, projection, glm::vec3(1.0f, 1.0f, 1.0f));

    glm::mat4 eyeBaseModel = glm::translate(fishModel, upperJawConnection);
    eyeBaseModel = glm::rotate(eyeBaseModel, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 eyeLeftModel = glm::translate(eyeBaseModel, glm::vec3(0.3f, 0.2f, -1.0f));
    eyeLeftModel = glm::scale(eyeLeftModel, glm::vec3(0.4f, 0.4f, 0.2f));
    drawModel("cube", eyeLeftModel, view, projection, glm::vec3(1.0f, 1.0f, 1.0f));

    glm::mat4 eyeRightModel = glm::translate(eyeBaseModel, glm::vec3(0.3f, 0.2f, 1.0f));
    eyeRightModel = glm::scale(eyeRightModel, glm::vec3(0.4f, 0.4f, 0.2f));
    drawModel("cube", eyeRightModel, view, projection, glm::vec3(1.0f, 1.0f, 1.0f));

    glm::mat4 pupilLeftModel = glm::translate(eyeBaseModel, glm::vec3(0.3f, 0.2f, -1.1f));
    pupilLeftModel = glm::scale(pupilLeftModel, glm::vec3(0.2f, 0.2f, 0.2f));
    drawModel("cube", pupilLeftModel, view, projection, glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 pupilRightModel = glm::translate(eyeBaseModel, glm::vec3(0.3f, 0.2f, 1.1f));
    pupilRightModel = glm::scale(pupilRightModel, glm::vec3(0.2f, 0.2f, 0.2f));
    drawModel("cube", pupilRightModel, view, projection, glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 leftFinModel = glm::translate(fishModel, glm::vec3(0.8f, -1.0f, -1.5f));
    leftFinModel = glm::rotate(leftFinModel, glm::radians(-30.0f), glm::vec3(1.0f, 1.0f, 0.0f));
    leftFinModel = glm::scale(leftFinModel, glm::vec3(3.0f, 0.5f, 1.0f));
    drawModel("cube", leftFinModel, view, projection, glm::vec3(0.35f, 0.35f, 0.55f));

    glm::mat4 rightFinModel = glm::translate(fishModel, glm::vec3(0.8f, -1.0f, 1.5f));
    rightFinModel = glm::rotate(rightFinModel, glm::radians(30.0f), glm::vec3(1.0f, 1.0f, 0.0f));
    rightFinModel = glm::scale(rightFinModel, glm::vec3(3.0f, 0.5f, 1.0f));
    drawModel("cube", rightFinModel, view, projection, glm::vec3(0.35f, 0.35f, 0.55f));

    glm::mat4 dorsalFinModel = glm::translate(fishModel, glm::vec3(1.0f, 1.5f, 0.0f));
    dorsalFinModel = glm::rotate(dorsalFinModel, glm::radians(60.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    dorsalFinModel = glm::scale(dorsalFinModel, glm::vec3(1.0f, 1.5f, 1.0f));
    drawModel("cube", dorsalFinModel, view, projection, glm::vec3(0.35f, 0.35f, 0.55f));

    glm::mat4 tailModel = glm::translate(fishModel, glm::vec3(-2.0f, 0.0f, 0.0f));
    float tailScales[4] = {2.0f, 2.5f, 3.0f, 3.5f};
    for (int i = 0; i < 4; ++i) {
        float sway = (i < 5) ? 0.3f * sin(tailPhase + i * 0.5f) : 0.0f;
        tailModel = glm::rotate(tailModel, sway, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 segModel = glm::translate(tailModel, glm::vec3(-tailScales[i] / 2.0f, 0.0f, 0.0f));
        if (i == 3) {
            glm::mat4 upperLobe = glm::translate(segModel, glm::vec3(0.8f, 0.0f, 0.0f));
            upperLobe = glm::scale(upperLobe, glm::vec3(1.5f, 6.0f, 0.5f));
            drawModel("cube", upperLobe, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
        } else {
            segModel = glm::scale(segModel, glm::vec3(tailScales[i], 1.5f - i * 0.25f, 2.2f - i * 0.3f));
            drawModel("cube", segModel, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
        }
        tailModel = glm::translate(tailModel, glm::vec3(-tailScales[i] * 0.8f, 0.0f, 0.0f));
    }
}


 
void updateSchoolFish(float deltaTime) {
    for (auto& fish : schoolFish) {
        fish.position += fish.direction * fish.speed * deltaTime;
        
        if (fish.position.x > AQUARIUM_BOUND_X - 20.0f || fish.position.x < -AQUARIUM_BOUND_X + 20.0f) {
            fish.direction.x *= -1;
            fish.angle = (fish.direction.x > 0) ? 0.0f : glm::pi<float>();
        }
        
        fish.position.z = glm::clamp(fish.position.z, -AQUARIUM_BOUND_Z + 8.0f, AQUARIUM_BOUND_Z - 8.0f);
        fish.position.y = glm::max(fish.position.y, 1.0f);
    }
}


void initializeAquarium() {
    srand(static_cast<unsigned int>(time(nullptr)));

    playerFish.toothUpperLeft.pos0 = glm::vec3(0.0f, 0.5f, -0.4f);
    playerFish.toothUpperLeft.pos1 = glm::vec3(0.5f, 0.5f, -0.4f);
    playerFish.toothUpperRight.pos0 = glm::vec3(0.0f, 0.5f, 0.4f);
    playerFish.toothUpperRight.pos1 = glm::vec3(0.5f, 0.5f, 0.4f);
    playerFish.toothLowerLeft.pos0 = glm::vec3(0.0f, -0.5f, -0.4f);
    playerFish.toothLowerLeft.pos1 = glm::vec3(0.5f, -0.5f, -0.4f);
    playerFish.toothLowerRight.pos0 = glm::vec3(0.0f, -0.5f, 0.4f);
    playerFish.toothLowerRight.pos1 = glm::vec3(0.5f, -0.5f, 0.4f);

    schoolFish.clear();
    Fish f1;
    f1.position = glm::vec3(0.0f, 15.0f, 0.0f);
    f1.fishType = "fish1";
    f1.direction = glm::vec3((rand() % 2 == 0) ? 1.0f : -1.0f, 0.0f, 0.0f);
    f1.angle = (f1.direction.x > 0) ? 0.0f : glm::pi<float>();
    f1.color = glm::vec3(static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX);
    schoolFish.push_back(f1);

    Fish f2;
    f2.position = glm::vec3(7.0f, 3.0f, 0.0f);
    f2.fishType = "fish2";
    f2.direction = glm::vec3((rand() % 2 == 0) ? 1.0f : -1.0f, 0.0f, 0.0f);
    f2.angle = (f2.direction.x > 0) ? 0.0f : glm::pi<float>();
    f2.color = glm::vec3(static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX);
    schoolFish.push_back(f2);

    Fish f3;
    f3.position = glm::vec3(-3.0f, 7.0f, -7.0f);
    f3.fishType = "fish3";
    f3.direction = glm::vec3((rand() % 2 == 0) ? 1.0f : -1.0f, 0.0f, 0.0f);
    f3.angle = (f3.direction.x > 0) ? 0.0f : glm::pi<float>();
    f3.color = glm::vec3(static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX);
    schoolFish.push_back(f3);

    std::vector<glm::vec3> seaweedPos = {glm::vec3(7.0f, 0.0f, 0.0f), glm::vec3(-7.0f, 0.0f, -10.0f), glm::vec3(-7.0f, 0.0f, 5.0f)};
    for (const auto& pos : seaweedPos) {
        Seaweed sw;
        sw.basePosition = pos;
        sw.swayOffset = static_cast<float>(rand()) / RAND_MAX * 3.0f * glm::pi<float>();
        SeaweedSegment* prev = nullptr;
        for (int i = 0; i < 7; ++i) {
            SeaweedSegment* seg = new SeaweedSegment();
            seg->localPos = glm::vec3(0.0f, 0.0f, 0.0f);
            seg->scale = glm::vec3(0.5f - i * 0.02f, 1.0f, 0.5f - i * 0.02f);
            seg->color = glm::vec3(0.0f, 0.6f - i * 0.05f, 0.1f);
            seg->phase = -i * 0.35f;
            seg->next = nullptr;
            if (i == 0) {
                sw.rootSegment = seg;
            } else {
                prev->next = seg;
            }
            prev = seg;
        }
        seaweeds.push_back(sw);
    }
}