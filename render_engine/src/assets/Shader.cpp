#include <render_engine/assets/Shader.h>

#include <render_engine/resources/ShaderModule.h>

#include <fstream>
#include <stdexcept>
#include <vector>

namespace
{
    std::vector<uint32_t> readSpirvCode(const std::filesystem::path& filepath)
    {
        std::ifstream file(filepath.c_str(), std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {

            throw std::runtime_error("failed to open file " + filepath.string());
        }

        size_t fileSize = file.tellg();
        std::vector<uint32_t> buffer(std::ceil(fileSize / 4.0));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        file.close();

        return buffer;
    }
}
namespace RenderEngine
{

    void Shader::readFromFile(const std::filesystem::path& spriv_path)
    {
        _spirv_code = readSpirvCode(spriv_path);
    }
    ShaderModule Shader::loadOn(LogicalDevice& logical_device) const
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = _spirv_code.size() * sizeof(uint32_t);
        create_info.pCode = _spirv_code.data();

        VkShaderModule shader_module;
        if (logical_device->vkCreateShaderModule(*logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }

        return ShaderModule(shader_module, logical_device);
    }
}
