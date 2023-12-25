#include <render_engine/assets/VolumeMaterial.h>

namespace RenderEngine
{
    std::vector<uint8_t> VolumeMaterial::createVertexBuffer(const Geometry& geometry, const Material&)
    {
        std::vector<float> vertex_buffer_data;
        vertex_buffer_data.reserve(geometry.positions.size() * 6);
        for (uint32_t i = 0; i < geometry.positions.size(); ++i)
        {
            vertex_buffer_data.push_back(geometry.positions[i].x);
            vertex_buffer_data.push_back(geometry.positions[i].y);
            vertex_buffer_data.push_back(geometry.positions[i].z);

            vertex_buffer_data.push_back(geometry.texture_coord_3d[i].x);
            vertex_buffer_data.push_back(geometry.texture_coord_3d[i].y);
            vertex_buffer_data.push_back(geometry.texture_coord_3d[i].z);
        }
        auto begin = reinterpret_cast<uint8_t*>(vertex_buffer_data.data());
        std::vector<uint8_t> vertex_buffer(begin, begin + vertex_buffer_data.size() * sizeof(float));
        return vertex_buffer;
    }
    std::unique_ptr<Material> VolumeMaterial::createForFrontFace(std::unique_ptr<Shader> fragment_shader,
                                                                 uint32_t id) const
    {
        std::unique_ptr<Shader> vertex_shader = std::make_unique<Shader>(getVertexShader());
        std::unique_ptr<Material> result(new Material(std::move(vertex_shader),
                                                      std::move(fragment_shader),
                                                      { [](const Geometry& geometry, const Material& material) { return createVertexBuffer(geometry, material); } },
                                                      id));
        result->setColorBlending(result->getColorBlending().clone()
                                 .setEnabled(false));
        result->setAlphaBlending(result->getAlpheBlending().clone()
                                 .setEnabled(false));
        result->setRasterizationInfo(result->getRasterizationInfo().clone()
                                     .setCullMode(VK_CULL_MODE_BACK_BIT));
        return result;
    }
    std::unique_ptr<Material> VolumeMaterial::createForBackFace(std::unique_ptr<Shader> fragment_shader,
                                                                uint32_t id) const
    {
        std::unique_ptr<Shader> vertex_shader = std::make_unique<Shader>(getVertexShader());
        std::unique_ptr<Material> result(new Material(std::move(vertex_shader),
                                                      std::move(fragment_shader),
                                                      { [](const Geometry& geometry, const Material& material) { return createVertexBuffer(geometry, material); } },
                                                      id));
        result->setColorBlending(result->getColorBlending().clone()
                                 .setEnabled(false));
        result->setAlphaBlending(result->getAlpheBlending().clone()
                                 .setEnabled(false));
        result->setRasterizationInfo(result->getRasterizationInfo().clone()
                                     .setCullMode(VK_CULL_MODE_FRONT_BIT));
        return result;
    }
}