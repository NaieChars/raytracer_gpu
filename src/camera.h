#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>

class Camera {
public:
    // ---------- 位置与朝向 ----------
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    // ---------- 移动/灵敏度参数 ----------
    float movementSpeed;
    float mouseSensitivity;

    // ---------- 镜头参数(景深用) ----------
    float fovDegrees;
    float aperture;
    float focusDist;

    bool useFocusTarget = false;   // 是否启用自动跟焦
    glm::vec3 focusTarget;         // 对焦的世界坐标点

    // 设置一个固定的世界坐标点作为对焦目标,之后每帧自动重新计算focusDist,
    // 不管相机怎么移动,永远保持这个点在焦平面上
    void setFocusTarget(glm::vec3 target) {
        focusTarget = target;
        useFocusTarget = true;
        moved = true; // 对焦点变了,画面会变,需要重置累积
    }

    // 关闭自动跟焦,恢复手动指定focusDist
    void clearFocusTarget() {
        useFocusTarget = false;
        moved = true;
    }


    bool dofEnabled = true;    // 景深开关状态
    float savedAperture;       // 关闭时,把aperture存起来,方便重新开启时恢复原来的虚化强度

    // 切换景深开关;
    void toggleDOF() {
        if (dofEnabled) {
            savedAperture = aperture;
            aperture = 0.0f;   // 光圈为0
            dofEnabled = false;
        } else {
            aperture = savedAperture;
            dofEnabled = true;
        }
        moved = true; 
    }
    void markMoved() {
        moved = true;
    }

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 0.5f, 3.0f),
           float startYaw = -90.0f, float startPitch = 0.0f)
        : position(startPos), worldUp(0.0f, 1.0f, 0.0f),
          yaw(startYaw), pitch(startPitch),
          movementSpeed(2.5f), mouseSensitivity(0.1f),
          fovDegrees(50.0f), aperture(0.12f), focusDist(3.0f),
          moved(true), firstMouse(true), lastMouseX(0.0f), lastMouseY(0.0f)
    {
        savedAperture = aperture; 
        updateCameraVectors();
    }

    // 每帧调用,处理WASD/QE/Shift加速
    void processKeyboard(GLFWwindow* window, float deltaTime) {
        float speed = movementSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 3.0f;

        glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
        glm::vec3 oldPos = position;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += front * speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= front * speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * speed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) position += worldUp * speed;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) position -= worldUp * speed;

        if (position != oldPos) moved = true;
    }

    // GLFW鼠标回调里调用这个,传入原始xpos/ypos(屏幕像素坐标)
    void processMouseMovement(float xpos, float ypos) {
        if (firstMouse) {
            lastMouseX = xpos;
            lastMouseY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastMouseX;
        float yoffset = lastMouseY - ypos; // 反转,让鼠标上移=pitch增加
        lastMouseX = xpos;
        lastMouseY = ypos;

        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw   += xoffset;
        pitch += yoffset;
        pitch = std::clamp(pitch, -89.0f, 89.0f); // 防止万向锁

        updateCameraVectors();
        moved = true;
    }

    void getGPUParams(float aspectRatio,
                       glm::vec3& outOrigin, glm::vec3& outLowerLeftCorner,
                       glm::vec3& outHorizontal, glm::vec3& outVertical,
                       glm::vec3& outU, glm::vec3& outV, float& outLensRadius) const
    {
        // 如果启用了自动跟焦,每次调用都重新算一次当前相机位置到目标点的距离,
        if (useFocusTarget) {
            const_cast<Camera*>(this)->focusDist = glm::length(focusTarget - position);
        }
        glm::vec3 w = glm::normalize(-front);
        glm::vec3 u = glm::normalize(glm::cross(worldUp, w));
        glm::vec3 v = glm::cross(w, u);

        float theta = glm::radians(fovDegrees);
        float halfHeight = tan(theta / 2.0f);
        float halfWidth = aspectRatio * halfHeight;

        outOrigin = position;
        outLowerLeftCorner = position - halfWidth * focusDist * u - halfHeight * focusDist * v - focusDist * w;
        outHorizontal = 2.0f * halfWidth * focusDist * u;
        outVertical = 2.0f * halfHeight * focusDist * v;
        outU = u;
        outV = v;
        outLensRadius = aperture / 2.0f;
    }

    // 检查相机这一帧是否移动过;调用后自动清零标记
    bool consumeMovedFlag() {
        bool result = moved;
        moved = false;
        return result;
    }

private:
    bool moved;
    bool firstMouse;
    float lastMouseX, lastMouseY;

    void updateCameraVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        up = worldUp;
    }
};