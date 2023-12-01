#include <scene/Camera.h>
#include <imgui.h>

namespace Scene
{
	glm::mat4 Camera::getView() const
	{
		const glm::mat4 rotation = glm::mat4_cast(getTransformation().getRotation());
		const glm::vec3 direction = rotation* glm::vec4(_scene_setup.forward, 1.0f);
		const glm::vec3 position = getTransformation().getPosition();

		return glm::lookAtRH(position,
			position + direction,
            _scene_setup.up);
	}
	
	void Camera::onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup)
	{
		if (old_lookup != nullptr)
		{
			old_lookup->unregisterCamera(getName());
		}
		new_lookup->registerCamera(this, this->getName());
	}
	
	void Camera::onSceneChanged(Scene* new_scene, Scene* old_scene)
	{
		_scene_setup = new_scene->getSceneSetup();
	}
    void Camera::onGui()
    {
        ImGui::Begin("Camera");
        const glm::mat4 rotation = glm::mat4_cast(getTransformation().getRotation());
        const glm::vec3 rotationAngles = getTransformation().getEulerAngles();
        const glm::vec3 direction = rotation * glm::vec4(_scene_setup.forward, 1.0f);
        const glm::vec3 position = getTransformation().getPosition();

        ImGui::LabelText("Position",
                         "%0.2f %0.2f, %0.2f", position.x, position.y, position.z);
        ImGui::LabelText("Rotation",
                         "%0.2f %0.2f, %0.2f", rotationAngles.x, rotationAngles.y, rotationAngles.z);
        ImGui::LabelText("Direction",
                         "%0.2f %0.2f, %0.2f", direction.x, direction.y, direction.z);

        ImGui::End();
    }
}