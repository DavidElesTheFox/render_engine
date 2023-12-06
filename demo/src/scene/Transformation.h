#pragma once

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
namespace Scene
{
    class Transformation
    {
    public:
        Transformation() = default;

        const glm::vec3& getPosition() const { return _position; }
        glm::vec3 getEulerAngles() const { return glm::eulerAngles(_rotation); }
        const glm::vec3& getScale() const { return _scale; }

        void setPosition(glm::vec3 position) { _position = std::move(position); }
        void setEulerAngles(glm::vec3 euler_angles) { _rotation = std::move(euler_angles); }
        void setScale(glm::vec3 scale) { _scale = std::move(scale); }

        glm::mat4 calculateTransformation() const { return glm::translate(glm::mat4{ 1.0f }, _position) * glm::mat4_cast(getRotation()) * glm::scale(glm::mat4{ 1.0f }, _scale); }
        const glm::quat& getRotation() const { return _rotation; }

        void rotate(const glm::quat& rotation)
        {
            _rotation *= rotation;
        }

        void translate(const glm::vec3& translation)
        {
            _position += translation;
        }

        void scale(const glm::vec3& scale)
        {
            _scale *= scale;
        }
    private:
        glm::vec3 _position{ 0.0f };
        glm::quat _rotation{ glm::vec3{0.0f, 0.0f, 0.0f} };
        glm::vec3 _scale{ 1.0f };
    };
}