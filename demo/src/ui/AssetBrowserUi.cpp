#include <ui/AssetBrowserUi.h>

#include <imgui.h>

#include <assets/NoLitMaterial.h>
#include <render_engine/assets/Material.h>
#include <scene/MeshObject.h>

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
                glm::vec3 color = nolit_material->getMaterialConstants().fragment_values.getInstanceColor();
                if (ImGui::SliderFloat3(("Instance_color - " + material_instance_name).c_str(), &color.x, 0.0f, 1.0f))
                {
                    nolit_material->getMaterialConstants().fragment_values.setInstanceColor(color);
                }
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
            if (ImGui::SliderFloat3(("Rotation - " + mesh_instance_name).c_str(), &rotation.x, -glm::pi<float>(), glm::pi<float>()))
            {
                mesh_object->getTransformation().setEulerAngles(rotation);
            }
            glm::vec3 position = mesh_object->getTransformation().getPosition();
            if (ImGui::InputFloat3(("Position - " + mesh_instance_name).c_str(), &position.x))
            {
                mesh_object->getTransformation().setPosition(position);
            }
            ImGui::Separator();
            glm::vec3 scale = mesh_object->getTransformation().getScale();
            if (ImGui::InputFloat3(("Scale - " + mesh_instance_name).c_str(), &scale.x))
            {
                mesh_object->getTransformation().setScale(scale);
            }
        }
        ImGui::End();

    }

}