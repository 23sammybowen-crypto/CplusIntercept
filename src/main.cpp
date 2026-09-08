#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

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
    glm::vec3 end;
    float startTime;
    bool collided;
};


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
    constexpr float radius = 0.75f;
    constexpr int latitudeSegments = 32;
    constexpr int longitudeSegments = 32;
    constexpr float pi = 3.14159265359f;
    std::vector<float> vertices;
    std::vector<CollisionLine> lines;
    constexpr int lineCount = 21;
    constexpr float lineInterval = 0.2f;
    constexpr float lineStartX = -2.0f;
    constexpr float lineMaxX = 2.0f;

    for (int i = 0; i < lineCount; ++i) {
        float y = -1.0f + static_cast<float>(i) * 0.1f;

        lines.push_back({
            glm::vec3(lineStartX, y, 0.0f),
            glm::vec3(lineStartX, y, 0.0f),
            i * lineInterval,
            false
        });
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
    float lineSpeed = 0.5f;

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

        std::vector<float> lineVertices;
        lineVertices.reserve(lines.size() * 2 * 6);

        for (CollisionLine& line : lines) {
            if (currentFrameTime < line.startTime) {
                continue;
            }

            if (!line.collided) {
                float yOffset = line.start.y - sphereCenter.y;
                float inside = radius * radius - yOffset * yOffset;

                if (inside >= 0.0f) {
                    float hitX = sphereCenter.x - std::sqrt(inside);

                    line.end.x += lineSpeed * deltaTime;

                    if (line.end.x >= hitX) {
                        line.end.x = hitX;
                        line.collided = true;
                    }
                } else {
                    line.end.x += lineSpeed * deltaTime;

                    if (line.end.x >= lineMaxX) {
                        line.end.x = lineMaxX;
                        line.collided = true;
                    }
                }
            }

            float red = line.collided ? 0.2f : 1.0f;
            float green = line.collided ? 1.0f : 0.2f;
            float blue = 0.2f;

            lineVertices.push_back(line.start.x);
            lineVertices.push_back(line.start.y);
            lineVertices.push_back(line.start.z);
            lineVertices.push_back(1.0f);
            lineVertices.push_back(1.0f);
            lineVertices.push_back(1.0f);

            lineVertices.push_back(line.end.x);
            lineVertices.push_back(line.end.y);
            lineVertices.push_back(line.end.z);
            lineVertices.push_back(red);
            lineVertices.push_back(green);
            lineVertices.push_back(blue);
        }

        glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
        if (!lineVertices.empty()) {
            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                lineVertices.size() * sizeof(float),
                lineVertices.data()
            );
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::rotate(model, rotationX, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotationY, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            800.0f / 600.0f,
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

        //Draw the line
        glm::mat4 lineModel = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(lineModel));

        
        glBindVertexArray(lineVao);
        glDrawArrays(
            GL_LINES,
            0,
            static_cast<GLsizei>(lineVertices.size() / 6)
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
