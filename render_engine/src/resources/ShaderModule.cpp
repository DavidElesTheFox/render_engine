#include "render_engine/resources/ShaderModule.h"


#include <render_engine/assets/Shader.h>

namespace RenderEngine
{
    ShaderModule::~ShaderModule()
    {
        vkDestroyShaderModule(_device_loaded_on, _module, nullptr);
    }

}
