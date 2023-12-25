#include <ui/AssetBrowserUi.h>

#include <imgui.h>

#include <assets/CtVolumeMaterial.h>
#include <assets/NoLitMaterial.h>
#include <render_engine/assets/Material.h>
#include <scene/MeshObject.h>
#include <scene/VolumeObject.h>

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
            if (auto* ct_material = dynamic_cast<Assets::CtVolumeMaterial::Instance*>(material_instance))
            {
                int step_count = ct_material->getMaterialConstants().fragment_values.getNumSteps();
                float step_size = ct_material->getMaterialConstants().fragment_values.getStepSize();

                if (ImGui::SliderInt(("Num Steps - " + material_instance_name).c_str(), &step_count, 1, 1 / step_size))
                {
                    ct_material->getMaterialConstants().fragment_values.setNumSteps(step_count);
                }

                if (ImGui::SliderFloat(("Step Size - " + material_instance_name).c_str(), &step_size, 0.0001f, 0.1f))
                {
                    ct_material->getMaterialConstants().fragment_values.setStepSize(step_size);
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
            Scene::SceneObject* scene_object{ nullptr };
            Scene::Transformation transformation;
            if (auto* mesh_object = _scene.getNodeLookup().findMesh(mesh_instance->getId()); mesh_object != nullptr)
            {
                scene_object = mesh_object;
            }
            else if (auto* volume_object = _scene.getNodeLookup().findVolumObject(mesh_instance->getId()); volume_object != nullptr)
            {
                scene_object = volume_object;
            }
            glm::vec3 rotation = scene_object->getTransformation().getEulerAngles();
            if (ImGui::SliderFloat3(("Rotation - " + mesh_instance_name).c_str(), &rotation.x, -glm::pi<float>(), glm::pi<float>()))
            {
                scene_object->getTransformation().setEulerAngles(rotation);
            }
            glm::vec3 position = scene_object->getTransformation().getPosition();
            if (ImGui::InputFloat3(("Position - " + mesh_instance_name).c_str(), &position.x))
            {
                scene_object->getTransformation().setPosition(position);
            }
            ImGui::Separator();
            glm::vec3 scale = scene_object->getTransformation().getScale();
            if (ImGui::InputFloat3(("Scale - " + mesh_instance_name).c_str(), &scale.x))
            {
                scene_object->getTransformation().setScale(scale);
            }
        }
        ImGui::End();

    }

}