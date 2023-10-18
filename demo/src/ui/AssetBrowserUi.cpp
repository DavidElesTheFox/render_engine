#include <ui/AssetBrowserUi.h>

#include <imgui.h>

#include <scene/MeshObject.h>
#include <assets/NoLitMaterial.h>
#include <render_engine/assets/Material.h>

namespace Ui
{
	void AssetBrowserUi::onGui()
	{
		ImGui::Begin("Assets");
		ImGui::Text("Material Instances");
		ImGui::Separator();
		for (auto material_instance_name : _assets.getMaterialInstanceNames())
		{
			auto* material_instance = _assets.getMaterialInstance(material_instance_name);
			ImGui::LabelText("Name", material_instance_name.c_str());

			if (auto* nolit_material = dynamic_cast<Assets::NoLitMaterial::Instance*>(material_instance))
			{
				ImGui::SliderFloat("Color offset", &nolit_material->getMaterialData().color_offset, 0.0f, 1.0f);
			}
			ImGui::Separator();
		}
		ImGui::Text("Mesh instances");
		ImGui::Separator();
		for (auto mesh_instance_name : _assets.getMeshInstanceNames())
		{
			ImGui::LabelText("Name", mesh_instance_name.c_str());

			auto* mesh_instance = _assets.getMeshInstance(mesh_instance_name);
			auto* mesh_object = _scene.getNodeLookup().findMesh(mesh_instance->getId());
			glm::vec3 rotation = mesh_object->getTransformation().getEulerAngles();
			if (ImGui::SliderFloat3("Rotation", &rotation.x, -glm::pi<float>(), glm::pi<float>()))
			{
				mesh_object->getTransformation().setEulerAngles(rotation);
			}
			glm::vec3 position = mesh_object->getTransformation().getPosition();
			if (ImGui::InputFloat3("Position", &position.x))
			{
				mesh_object->getTransformation().setPosition(position);
			}
			ImGui::Separator();
			glm::vec3 scale = mesh_object->getTransformation().getScale();
			if (ImGui::InputFloat3("Scale", &scale.x))
			{
				mesh_object->getTransformation().setScale(scale);
			}
		}
		ImGui::End();

	}

}