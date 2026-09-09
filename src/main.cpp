#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_inverse.hpp>

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, float& rotationX, float& rotationY, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }

    const float rotationSpeed = 2.5f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        rotationX += rotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        rotationX -= rotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        rotationY -= rotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        rotationY += rotationSpeed;
    }
}

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";
// Define a struct to hold the line's start and end points, as well as the time of collision
struct CollisionLine {
    glm::vec3 start;
    glm::vec3 position;
    glm::vec3 velocity;

    std::vector<glm::vec3> trail;

    float startTime;
    bool collided;
    bool active;
    bool escaped;
};

// Function to check if a line segment intersects with a sphere

bool segmentHitsSphere(
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec3& sphereCenter,
    float radius,
    float& hitTime
){
    glm::vec3 direction = end - start;
    glm::vec3 offset = start - sphereCenter;

    float a = glm::dot(direction, direction);

    if (a < 0.000001f) {
        return false;
    }

    float b = 2.0f * glm::dot(offset, direction);
    float c = glm::dot(offset, offset) - radius * radius;

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) {
        return false;
    }

    float squareRoot = std::sqrt(discriminant);

    float firstHit = (-b - squareRoot) / (2.0f * a);
    float secondHit = (-b + squareRoot) / (2.0f * a);

    if (firstHit >= 0.0f && firstHit <= 1.0f) {
        hitTime = firstHit;
        return true;
    }

    if (secondHit >= 0.0f && secondHit <= 1.0f) {
        hitTime = secondHit;
        return true;
    }

    return false;
}

struct ClickLaunchContext {
    std::vector<CollisionLine>* lines;
    glm::mat4 inverseMvp;
    glm::vec3 sphereCenter;
    float launchPlaneZ;
    float initialVelocityWorld;
};

void mouseButtonCallback(
    GLFWwindow* window,
    int button,
    int action,
    int mods
){
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
        return;
    }

    auto* context = reinterpret_cast<ClickLaunchContext*>(glfwGetWindowUserPointer(window));

    if (context == nullptr) {
        return;
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int windowWidth, windowHeight;
    int framebufferWidth, framebufferHeight;

    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    if (windowWidth == 0 || windowHeight == 0 || framebufferWidth == 0 || framebufferHeight == 0) {
        return;
    }

    //Convert the mouse position into framebuffer pixels

    mouseX *= static_cast<double>(framebufferWidth) / windowWidth;
    mouseY *= static_cast<double>(framebufferHeight) / windowHeight;

    //Convert pixels into OpenGl normalized device coordinates
    float ndcX = static_cast<float>((2.0 * mouseX) / framebufferWidth - 1.0);
    float ndcY = static_cast<float>(1.0 - (2.0 * mouseY) / framebufferHeight);

    glm::vec4 nearPoint = context->inverseMvp * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPoint = context->inverseMvp * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    glm::vec3 rayStart = glm::vec3(nearPoint);
    glm::vec3 rayDirection = glm::normalize(glm::vec3(farPoint) - rayStart);

    //Put the new line on a plane in front of the sphere
    if (std::abs(rayDirection.z) < 0.0001f) {
        return;
    }

    float distanceToLaunchPlane = (context->launchPlaneZ - rayStart.z) / rayDirection.z;

    if (distanceToLaunchPlane < 0.0f) {
        return;
    }

    glm::vec3 launchPosition = rayStart + rayDirection * distanceToLaunchPlane;

    CollisionLine line;
    line.start = launchPosition;
    line.position = launchPosition;
    line.velocity = glm::vec3(context->initialVelocityWorld, 0.0f, 0.0f);
    line.trail.push_back(launchPosition);
    line.startTime = static_cast<float>(glfwGetTime());
    line.active = true;
    line.collided = false;
    line.escaped = false;

    context->lines->push_back(line);
}

int main() {

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        800,
        600,
        "CplusIntercept OpenGL",
        nullptr,
        nullptr
    );

    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //keep back from overlapping front
    glEnable(GL_DEPTH_TEST);

    // Define the parameters for the sphere and the lines
    // I had to add in some parameters to convert the real world values into the world space values, this is because the sphere is not at a 1:1 scale with the real world.
    constexpr float radius = 0.75f;
    constexpr double gravitationalConstant = 6.67430e-11;
    constexpr double earthMass = 5.972e24;
    constexpr double earthRadiusMeters = 6.371e6;

    constexpr double earthMu = gravitationalConstant * earthMass;
    const double metersPerWorldUnit = earthRadiusMeters / static_cast<double>(radius);
    const double muWorld = earthMu / (metersPerWorldUnit * metersPerWorldUnit * metersPerWorldUnit);
    constexpr double initialVelocityMetersPerSecond = 8000.0;
    float initialVelocityWorld = static_cast<float>(initialVelocityMetersPerSecond / metersPerWorldUnit);
    const double visualTimeScale = 300.0;
    constexpr double physicsStep = 0.25;
    constexpr int latitudeSegments = 32;
    constexpr int longitudeSegments = 32;
    constexpr float pi = 3.14159265359f;
    std::vector<float> vertices;
    std::vector<CollisionLine> lines;
    constexpr int lineCount = 25;
    constexpr float lineInterval = 0.2f;
    constexpr float lineStartX = -2.0f;

    for (int i = 0; i < lineCount; ++i) {
        float y = -1.5f + static_cast<float>(i) * 0.125f;
        CollisionLine line;
        line.start = glm::vec3(lineStartX, y, 0.0f);
        line.position = line.start;
        line.velocity = glm::vec3(initialVelocityWorld, 0.0f, 0.0f);
        line.trail.push_back(line.start);
        line.startTime = static_cast<float>(i) * 0.5f;
        line.active = true;
        line.collided = false;
        line.escaped = false;

        lines.push_back(line);


    }

    for(int lat = 0; lat <= latitudeSegments; ++lat) {
        float latitude = pi * static_cast<float>(lat) / static_cast<float>(latitudeSegments);

        float y = radius * std::cos(latitude);
        float ringRadius = radius * std::sin(latitude);

        for(int lon = 0; lon <= longitudeSegments; ++lon) {
            float longitude = 2.0f * pi * static_cast<float>(lon) / longitudeSegments;

            float x = ringRadius * std::cos(longitude);
            float z = ringRadius * std::sin(longitude);

            float red = static_cast<float>(lat) / static_cast<float>(latitudeSegments);
            float green = static_cast<float>(lon) / static_cast<float>(longitudeSegments);
            float blue = 1.0f - red;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            vertices.push_back(red);
            vertices.push_back(green);
            vertices.push_back(blue);
        }
    }




    std::vector<unsigned int> indices;

    for(int lat = 0; lat < latitudeSegments; ++lat) {
        for(int lon = 0; lon < longitudeSegments; ++lon) {
            unsigned int current = (lat * (longitudeSegments + 1)) + lon;
            unsigned int next = current + longitudeSegments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(next);
            indices.push_back(next + 1);
            indices.push_back(current + 1);
        }
    }

    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);


    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );

    glEnableVertexAttribArray(1);

    // Collision settings for the growing lines.
    glm::vec3 sphereCenter(0.0f, 0.0f, 0.0f);

    //Shader compilation and linking
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int modelLocation = glGetUniformLocation(shaderProgram, "model");
    int viewLocation = glGetUniformLocation(shaderProgram, "view");
    int projectionLocation = glGetUniformLocation(shaderProgram, "projection");

    // Camera setup
    glm::vec3 cameraPosition(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float lastFrameTime = 0.0f;
    double physicsAccumulator = 0.0;


    // Set up the mouse click callback to launch lines
    ClickLaunchContext clickContext;
    clickContext.lines = &lines;
    clickContext.sphereCenter = sphereCenter;
    clickContext.inverseMvp = glm::mat4(1.0f);
    clickContext.launchPlaneZ = 1.5f;
    clickContext.initialVelocityWorld = initialVelocityWorld;

    glfwSetWindowUserPointer(window, &clickContext);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Create a separate VAO and VBO for the line
    unsigned int lineVao;
    unsigned int lineVbo;

    glGenVertexArrays(1, &lineVao);
    glGenBuffers(1, &lineVbo);

    glBindVertexArray(lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        lineCount * 2 * 6 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    //Render loop



    while (!glfwWindowShouldClose(window)) {
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        processInput(window, rotationX, rotationY, deltaTime);

        double limitedDeltaTime =
            std::min(
                static_cast<double>(deltaTime),
                0.05
            );

        physicsAccumulator +=
            limitedDeltaTime * visualTimeScale;

        while (physicsAccumulator >= physicsStep) {
            for (CollisionLine& line : lines) {
                if (currentFrameTime < line.startTime) {
                    continue;
                }

                if (!line.active) {
                    continue;
                }

                glm::vec3 previousPosition =
                    line.position;

                glm::vec3 offset =
                    line.position - sphereCenter;

                float distance =
                    glm::length(offset);

                if (distance <= radius) {
                    line.collided = true;
                    line.active = false;
                    continue;
                }

                glm::vec3 acceleration =
                    static_cast<float>(-muWorld) *
                    offset /
                    (distance * distance * distance);

                float step =
                    static_cast<float>(physicsStep);

                line.velocity += acceleration * step;

                glm::vec3 nextPosition =
                    line.position + line.velocity * step;

                float hitTime = 0.0f;

                if (segmentHitsSphere(
                        previousPosition,
                        nextPosition,
                        sphereCenter,
                        radius,
                        hitTime)) {

                    line.position =
                        previousPosition +
                        (nextPosition - previousPosition)
                        * hitTime;

                    line.trail.push_back(line.position);

                    line.collided = true;
                    line.active = false;
                } else {
                    line.position = nextPosition;
                    line.trail.push_back(line.position);

                    if (glm::length(
                            line.position - sphereCenter
                        ) > 8.0f) {

                        line.escaped = true;
                        line.active = false;
                    }
                }
            }

            physicsAccumulator -= physicsStep;
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::rotate(model, rotationX, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotationY, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);

        // Get the framebuffer size to calculate the aspect ratio for the projection matrix
        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight),
            0.1f,
            100.0f
        );
    //Draw the sphere

        glUniformMatrix4fv(
            modelLocation,
            1,
            GL_FALSE,
            glm::value_ptr(model)
        );

        glUniformMatrix4fv(
            viewLocation,
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            projectionLocation,
            1,
            GL_FALSE,
            glm::value_ptr(projection)
        );
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

        // Draw the curved line trails.
        glBindVertexArray(lineVao);
        glBindBuffer(GL_ARRAY_BUFFER, lineVbo);

        glm::mat4 lineModel = model;
        glUniformMatrix4fv(
            modelLocation,
            1,
            GL_FALSE,
            glm::value_ptr(lineModel)
        );

        glLineWidth(2.0f);

        for (const CollisionLine& line : lines) {
            if (currentFrameTime < line.startTime) {
                continue;
            }

            if (line.trail.size() < 2) {
                continue;
            }

            std::vector<float> trailVertices;

            for (const glm::vec3& point : line.trail) {
                trailVertices.push_back(point.x);
                trailVertices.push_back(point.y);
                trailVertices.push_back(point.z);

                if (line.collided) {
                    trailVertices.push_back(0.2f);
                    trailVertices.push_back(1.0f);
                    trailVertices.push_back(0.2f);
                } else if (line.escaped) {
                    trailVertices.push_back(0.2f);
                    trailVertices.push_back(0.4f);
                    trailVertices.push_back(1.0f);
                } else {
                    trailVertices.push_back(1.0f);
                    trailVertices.push_back(0.2f);
                    trailVertices.push_back(0.2f);
                }
            }

            glBufferData(
                GL_ARRAY_BUFFER,
                trailVertices.size() * sizeof(float),
                trailVertices.data(),
                GL_DYNAMIC_DRAW
            );

            glDrawArrays(
                GL_LINE_STRIP,
                0,
                static_cast<GLsizei>(line.trail.size())
            );
        }

        clickContext.inverseMvp = glm::inverse(
            projection * view * model
        );

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    
//clean up gpu resources
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &lineVao);
    glDeleteBuffers(1, &lineVbo);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
