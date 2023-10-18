#pragma once
#include <memory>

#include <glm/mat4x4.hpp>

#include <render_engine/assets/Material.h>

#include <assets/IMaterial.h>
#include <scene/Scene.h>

namespace Assets
{

	class NoLitMaterial : public IMaterial
	{
	public:

		struct Data
		{
			float color_offset{ 0.0f };
		};

		class PushConstants
		{
			friend class NoLitMaterial;
		public:
		private:
			glm::mat4 projection;
			glm::mat4 model_view;
		};

		class Instance : public IInstance
		{
			friend class NoLitMaterial;
		public:
			Instance() = default;
			~Instance() override = default;
			Data& getMaterialData() { return _material_data; }
			const Data& getMaterialData() const { return _material_data; }
			PushConstants& getMaterialConstants() { return _material_constants; }
			const PushConstants& getMaterialConstants() const { return _material_constants; }
			RenderEngine::MaterialInstance* getMaterialInstance() { return _material_instance.get(); }
		private:
			Data _material_data{};
			PushConstants _material_constants;
			std::unique_ptr<RenderEngine::MaterialInstance> _material_instance;
		};
		static const std::string& GetName() 
		{
			static std::string _name = "NoLit";
			return _name;
		}

		NoLitMaterial();
		~NoLitMaterial() override = default;

		std::unique_ptr<Instance> createInstance(Data data, Scene::Scene* scene);

		RenderEngine::Material* getMaterial() { return _material.get(); }

		const std::string& getName() const override
		{
			return GetName();
		}
	private:
		std::unique_ptr<RenderEngine::Material> _material;
	};
}