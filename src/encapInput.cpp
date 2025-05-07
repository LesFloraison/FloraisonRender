#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "encapVk.h"
#include "MTracer.h"
using namespace std;
int kUp = 1, lUp = 1;
int nUp = 1, mUp = 1;
bool spaceUp = 1;
bool wDown, sDown, aDown, dDown, spaceSignal;

void cul_mouseDir(glm::vec3* dir) {
    if (!MTracer::isTracerActivating && displayID != 16) {
        (*(dir)).x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        (*(dir)).y = sin(glm::radians(pitch));
        (*(dir)).z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    }
    else if (!MTracer::isTracerActivating && displayID == 16) {
        //do nothing
    }
    else {
        *dir = MTracer::direction;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // 注意这里是相反的，因为y坐标是从底部往顶部依次增大的
    lastX = xpos;
    lastY = ypos;
    
    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
}

void processInput(GLFWwindow* window) {
    wDown = sDown = aDown = dDown = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (!MTracer::isTracerActivating) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            if (freeCam) {
                invCameraPos = invCameraPos + (-cameraDirection) * deltaTime * cameraSpeed;
            }
            wDown = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            if (freeCam) {
                invCameraPos = invCameraPos + cameraDirection * deltaTime * cameraSpeed;
            }
            sDown = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            if (freeCam) {
                invCameraPos = invCameraPos + glm::cross(cameraDirection, glm::vec3(0, 1, 0)) * deltaTime * cameraSpeed;
            }
            aDown = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            if (freeCam) {
                invCameraPos = invCameraPos + -glm::cross(cameraDirection, glm::vec3(0, 1, 0)) * deltaTime * cameraSpeed;
            }
            dDown = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        UIEnable = 0;
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        UIEnable = 1;
    }

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        debugVal = 0;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        debugVal = 1;
    }

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE) {
        kUp = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        lUp = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spaceUp = 1;
    }

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        if (kUp == 1) {
            kUp = 0;
            if (displayID > -1) {
                displayID--;
            }
            std::cout << displayID << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (lUp == 1) {
            lUp = 0;
            displayID++;
            std::cout << displayID << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (spaceUp == 1) {
            spaceUp = 0;
            spaceSignal = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS) {
        consoleInput();
    }
}