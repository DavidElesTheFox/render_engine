#include <scene/Camera.h>

namespace Scene
{
	glm::mat4 Camera::getView() const
	{
		const glm::mat4 transformation = getTransformation().calculateNoScaleTransformation();
		const glm::vec3 direction = glm::vec4(_scene_setup.forward, 1.0f)
			* transformation;
		const glm::vec3 up = glm::vec4(_scene_setup.up, 1.0f)
			* transformation;
		const glm::vec3 position = getTransformation().getPosition();

		return glm::lookAt(position,
			position + direction,
			up);
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
}