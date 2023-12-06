#pragma once

#include <volk.h>

namespace RenderEngine
{

    class ShaderModule
    {
    public:
        ShaderModule(VkShaderModule module, VkDevice used_device)
            : _module(module),
            _device_loaded_on(used_device)
        {}
        ShaderModule(const ShaderModule&) = delete;
        ShaderModule(ShaderModule&&) = default;
        ShaderModule& operator=(const ShaderModule&) = delete;
        ShaderModule& operator=(ShaderModule&&) = default;
        ~ShaderModule();

        VkShaderModule getModule() const { return _module; }
    private:
        VkShaderModule _module{ VK_NULL_HANDLE };
        VkDevice _device_loaded_on{ VK_NULL_HANDLE };
    };
}
