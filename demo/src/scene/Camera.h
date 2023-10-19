#pragma once

#include <scene/SceneObject.h>
#include <scene/Scene.h>

#include <memory>

namespace Scene
{
	class Camera : public SceneObject
	{
	public:
		static std::unique_ptr<Camera> createOrtho(std::string name,
			glm::vec2 leftBottomCorner,
			glm::vec2 rightUpCorner,
			float near,
			float far)
		{
			return std::unique_ptr<Camera>(new Camera(std::move(name),
				glm::orthoRH_ZO(leftBottomCorner.x, rightUpCorner.x,
				leftBottomCorner.y, rightUpCorner.y,
					near, far)));
		}
		static std::unique_ptr<Camera> createPerspective(std::string name,
			float fov,
			float width,
			float height,
			float near, 
			float far)
		{
			return std::unique_ptr<Camera>(new Camera(std::move(name),
				glm::perspectiveFovRH_ZO(fov, width, height, near, far)));
		}
		const glm::mat4& getProjection() const { return _projection; }
		glm::mat4 getView() const;

		void onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup) override;
		void onSceneChanged(Scene* new_scene, Scene* old_scene) override;

	private:
		Camera(std::string name, glm::mat4 projection)
			: SceneObject(std::move(name))
			, _projection(std::move(projection))
		{}
		glm::mat4 _projection;
		Scene::SceneSetup _scene_setup{};
	};
}