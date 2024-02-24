#include <render_engine/assets/Material.h>

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/Device.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/GpuResourceSet.h>
#include <render_engine/resources/Technique.h>


#include <functional>
#include <numeric>
#include <ranges>
#include <set>

namespace RenderEngine
{

    Material::Material(std::unique_ptr<Shader> verted_shader,
                       std::unique_ptr<Shader> fragment_shader,
                       CallbackContainer callbacks,
                       uint32_t id,
                       std::string name)
        : _vertex_shader(std::move(verted_shader))
        , _fragment_shader(std::move(fragment_shader))
        , _id{ id }
        , _callbacks(std::move(callbacks))
        , _name(std::move(name))
    {}
}
