#pragma once
#include <arjet/Component.h>
#include <glm/glm.hpp>
#include <iostream>

using glm::vec3;
using glm::vec2;

class Transform : public Component{
public:
    vec3 position = { 0.0f, 0.0f, 0.0f };
    vec3 rotation = { 0.0f, 0.0f, 0.0f };
    vec3 size = { 1.0f, 1.0f, 1.0f };

    void translate(vec3 tr) {
        //std::cout << position.x << std::endl;
        position += tr;
    }
    void rotate(vec3 rot) {
        rotation += rot;
    }
    void scale(vec3 scale){
        size *= scale;
    }
    void scale(float scale) {
        size *= scale;
    }


    Transform() { type = "Transform"; }
};
