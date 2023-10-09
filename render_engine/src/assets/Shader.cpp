#include <render_engine/assets/Shader.h>

#include <fstream>
#include <stdexcept>
#include <vector>
#include <render_engine/resources/ShaderModule.h>

namespace
{
	std::vector<char> readFile(std::string_view filename) {
		std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

		if (!file.is_open()) {

			throw std::runtime_error("failed to open file " + std::string{ filename });
		}

		size_t fileSize = file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
}
namespace RenderEngine
{
	ShaderModule Shader::loadOn(VkDevice logical_device) const
	{
		const std::vector<char> code = readFile(_spirv_path.string());
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shader_module;
		if (vkCreateShaderModule(logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return ShaderModule(shader_module, logical_device);
	}
}
