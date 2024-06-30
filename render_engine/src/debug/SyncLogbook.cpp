#include <render_engine/debug/SyncLogbook.h>

#include <render_engine/containers/Views.h>
#include <render_engine/debug/Profiler.h>

#include <array>
#include <format>
#include <numeric>
#include <ostream>
#include <ranges>
#include <sstream>

namespace RenderEngine
{

    namespace
    {
        constexpr std::array g_known_pipeline_flags =
        {
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
            VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
            VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_HOST_BIT,
            VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_PIPELINE_STAGE_2_RESOLVE_BIT,
            VK_PIPELINE_STAGE_2_BLIT_BIT,
            VK_PIPELINE_STAGE_2_CLEAR_BIT,
            VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
            VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
            VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR,
            VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
            VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT,
            VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT,
            VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR,
            VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT,
            VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV,
            VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV,
            VK_PIPELINE_STAGE_2_SUBPASS_SHADER_BIT_HUAWEI,
            VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI,
            VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR,
            VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT,
            VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI,
            VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV
        };

        constexpr std::array g_known_access_flags =
        {
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
            VK_ACCESS_2_INDEX_READ_BIT,
            VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
            VK_ACCESS_2_UNIFORM_READ_BIT,
            VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_ACCESS_2_HOST_READ_BIT,
            VK_ACCESS_2_HOST_WRITE_BIT,
            VK_ACCESS_2_MEMORY_READ_BIT,
            VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR,
            VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR,
            VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR,
            VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR,
            VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT,
            VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT,
            VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT,
            VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT,
            VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV,
            VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV,
            VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
            VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,
            VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
            VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT,
            VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT,
            VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT,
            VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI,
            VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
            VK_ACCESS_2_MICROMAP_READ_BIT_EXT,
            VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT,
            VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV,
            VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV
        };

        const char* stageToString(VkPipelineStageFlags2 flag)
        {
            switch (flag)
            {
                case VK_PIPELINE_STAGE_2_NONE: return "VK_PIPELINE_STAGE_2_NONE";
                case VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT";
                case VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT: return "VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT";
                case VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT: return "VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT";
                case VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT: return "VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT";
                case VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT: return "VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT";
                case VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT: return "VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT";
                case VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT: return "VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT";
                case VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT: return "VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT";
                case VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT: return "VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT";
                case VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT: return "VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT";
                case VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT: return "VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT";
                case VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT: return "VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT";
                case VK_PIPELINE_STAGE_2_TRANSFER_BIT: return "VK_PIPELINE_STAGE_2_TRANSFER_BIT";
                case VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT";
                case VK_PIPELINE_STAGE_2_HOST_BIT: return "VK_PIPELINE_STAGE_2_HOST_BIT";
                case VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT: return "VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT";
                case VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT: return "VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT";
                case VK_PIPELINE_STAGE_2_COPY_BIT: return "VK_PIPELINE_STAGE_2_COPY_BIT";
                case VK_PIPELINE_STAGE_2_RESOLVE_BIT: return "VK_PIPELINE_STAGE_2_RESOLVE_BIT";
                case VK_PIPELINE_STAGE_2_BLIT_BIT: return "VK_PIPELINE_STAGE_2_BLIT_BIT";
                case VK_PIPELINE_STAGE_2_CLEAR_BIT: return "VK_PIPELINE_STAGE_2_CLEAR_BIT";
                case VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT: return "VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT";
                case VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT: return "VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT";
                case VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT: return "VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT";
                case VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR: return "VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR";
                case VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR: return "VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR";
                case VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT: return "VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT";
                case VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT: return "VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT";
                case VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV: return "VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV";
                case VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR: return "VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR";
                case VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR: return "VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR";
                case VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR: return "VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR";
                case VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT: return "VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT";
                case VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV: return "VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV";
                case VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV: return "VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV";
                case VK_PIPELINE_STAGE_2_SUBPASS_SHADER_BIT_HUAWEI: return "VK_PIPELINE_STAGE_2_SUBPASS_SHADER_BIT_HUAWEI";
                case VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI: return "VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI";
                case VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR: return "VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR";
                case VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT: return "VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT";
                case VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI: return "VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI";
                case VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV: return "VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV";
            }
            return "UNKNOWN_STAGE_BIT";
        }

        const char* accessFlagToString(VkAccessFlagBits2 flag)
        {
            switch (flag)
            {
                case VK_ACCESS_2_NONE: return "VK_ACCESS_2_NONE";
                case VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT: return "VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT";
                case VK_ACCESS_2_INDEX_READ_BIT: return "VK_ACCESS_2_INDEX_READ_BIT";
                case VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT: return "VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT";
                case VK_ACCESS_2_UNIFORM_READ_BIT: return "VK_ACCESS_2_UNIFORM_READ_BIT";
                case VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT: return "VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT";
                case VK_ACCESS_2_SHADER_READ_BIT: return "VK_ACCESS_2_SHADER_READ_BIT";
                case VK_ACCESS_2_SHADER_WRITE_BIT: return "VK_ACCESS_2_SHADER_WRITE_BIT";
                case VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT: return "VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT";
                case VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT: return "VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT";
                case VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT: return "VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT";
                case VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT: return "VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT";
                case VK_ACCESS_2_TRANSFER_READ_BIT: return "VK_ACCESS_2_TRANSFER_READ_BIT";
                case VK_ACCESS_2_TRANSFER_WRITE_BIT: return "VK_ACCESS_2_TRANSFER_WRITE_BIT";
                case VK_ACCESS_2_HOST_READ_BIT: return "VK_ACCESS_2_HOST_READ_BIT";
                case VK_ACCESS_2_HOST_WRITE_BIT: return "VK_ACCESS_2_HOST_WRITE_BIT";
                case VK_ACCESS_2_MEMORY_READ_BIT: return "VK_ACCESS_2_MEMORY_READ_BIT";
                case VK_ACCESS_2_MEMORY_WRITE_BIT: return "VK_ACCESS_2_MEMORY_WRITE_BIT";
                case VK_ACCESS_2_SHADER_SAMPLED_READ_BIT: return "VK_ACCESS_2_SHADER_SAMPLED_READ_BIT";
                case VK_ACCESS_2_SHADER_STORAGE_READ_BIT: return "VK_ACCESS_2_SHADER_STORAGE_READ_BIT";
                case VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT: return "VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT";
                case VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR: return "VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR";
                case VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR: return "VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR";
                case VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR: return "VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR";
                case VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR: return "VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR";
                case VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT: return "VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT";
                case VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT: return "VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT";
                case VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT: return "VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT";
                case VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT: return "VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT";
                case VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV: return "VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV";
                case VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV: return "VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV";
                case VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR: return "VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR";
                case VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR: return "VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR";
                case VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR: return "VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR";
                case VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT: return "VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT";
                case VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT: return "VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT";
                case VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT: return "VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT";
                case VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI: return "VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI";
                case VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR: return "VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR";
                case VK_ACCESS_2_MICROMAP_READ_BIT_EXT: return "VK_ACCESS_2_MICROMAP_READ_BIT_EXT";
                case VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT: return "VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT";
                case VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV: return "VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV";
                case VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV: return "VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV";
            }
            return "UNKNOWN_ACCESS_FLAG";
        }
        std::ostream& operator<<(std::ostream& os, const SyncLogbook::SemaphoreType& value)
        {
            switch (value)
            {
                case SyncLogbook::SemaphoreType::Binary: os << "Binary Semaphore"; break;
                case SyncLogbook::SemaphoreType::Timeline: os << "Timeline Semaphore"; break;
            }
            return os;
        }

    }

    void SyncLogbook::registerSemaphore(std::string name, SemaphoreType type, void* handler, std::string sync_object_name)
    {
        std::unique_lock lock(_semaphore_mutex);
        _semaphores.insert({ handler,
                           SemaphoreEntry
                               {
                                   .name = std::move(name),
                                   .type = type,
                                   .handler = handler,
                                   .sync_object_name = std::move(sync_object_name)
                               }
                           });
    }
    void SyncLogbook::signaledSemaphoreFromHost(void* handler, uint64_t value)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_stack_mutex);
        _stack.push_front(StackLine
                          {
                              .operation = Operation::SignalFromHost,
                              .handler = handler,
                              .value = value
                          });
        shrink();
    }
    void SyncLogbook::imageAcquire(void* handler)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_stack_mutex);
        _stack.push_front(StackLine
                          {
                              .operation = Operation::ImageAcquire,
                              .handler = handler,
                          });
        shrink();
    }
    void SyncLogbook::usedAtSubmitForSignal(void* handler, VkPipelineStageFlagBits2 stage)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_stack_mutex);
        _stack.push_front(StackLine
                          {
                              .operation = Operation::Signal,
                              .handler = handler,
                              .stage_info = StageInfo{.stage_flag = stage}
                          });
        shrink();
    }
    void SyncLogbook::usedAtSubmitForWait(void* handler, VkPipelineStageFlagBits2 stage)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_stack_mutex);
        _stack.push_front(StackLine
                          {
                              .operation = Operation::Wait,
                              .handler = handler,
                              .stage_info = StageInfo{.stage_flag = stage}
                          });
        shrink();
    }
    void SyncLogbook::usedAtPresentForWait(void* handler)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_stack_mutex);
        _stack.push_front(StackLine
                          {
                              .operation = Operation::WaitAtPresent,
                              .handler = handler
                          });
        shrink();
    }
    std::string SyncLogbook::toString() const
    {
        return (std::ostringstream{} << *this).str();
    }
    void SyncLogbook::shrink()
    {
        while (_stack.size() > _stack_max_size)
        {
            _stack.pop_back();
        }
    }
    std::ostream& operator<<(std::ostream& os, const SyncLogbook& logbook)
    {
        PROFILE_SCOPE();
        os << "Semaphores:" << std::endl;
        const std::vector<SyncLogbook::SemaphoreEntry> entries = [&]
            {
                std::shared_lock lock{ logbook._semaphore_mutex };
                return logbook._semaphores | std::views::values | views::to<std::vector< SyncLogbook::SemaphoreEntry>>();
            }();

        for (const auto& entry : entries)
        {
            os << entry << std::endl;
        }

        os << "Stack: " << std::endl;
        const std::deque<SyncLogbook::StackLine> stack = [&]
            {
                std::shared_lock lock{ logbook._stack_mutex };
                return logbook._stack;
            }();
        for (uint32_t i = 0; i < stack.size(); ++i)
        {
            os << "#" << i << ": " << stack[i] << std::endl;
        }
        return os;
    }
    std::ostream& operator<<(std::ostream& os, const SyncLogbook::Operation& value)
    {
        switch (value)
        {
            case SyncLogbook::Operation::Signal: os << "Signal"; break;
            case SyncLogbook::Operation::Wait: os << "Wait"; break;
            case SyncLogbook::Operation::WaitAtPresent: os << "WaitAtPresent"; break;
            case SyncLogbook::Operation::SignalFromHost: os << "SignalFromHost"; break;
            case SyncLogbook::Operation::ImageAcquire: os << "ImageAcquire"; break;
        }
        return os;
    }
    std::ostream& operator<<(std::ostream& os, const SyncLogbook::SemaphoreEntry& value)
    {
        os << std::format("{:s} ({:#018x}): ", value.name, reinterpret_cast<uintptr_t>(value.handler)) << "): "
            << "Owner: " << value.sync_object_name << " [" << value.type << "]";
        return os;
    }
    std::ostream& operator<<(std::ostream& os, const SyncLogbook::StackLine& value)
    {
        os << value.operation << std::format(": {:#018x}", reinterpret_cast<uintptr_t>(value.handler));
        if (value.stage_info != std::nullopt)
        {
            os << " @" << *value.stage_info;
        }
        if (value.value != std::nullopt)
        {
            os << " Value: " << *value.value;
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const SyncLogbook::StageInfo& value)
    {
        std::vector<VkPipelineStageFlags2> stages;
        for (auto stage_flag : g_known_pipeline_flags)
        {
            if (stage_flag & value.stage_flag)
            {
                stages.push_back(stage_flag);
            }
        }
        auto stages_str_list = stages | std::views::transform([](const auto flag) { return stageToString(flag); });
        const std::string stages_str = std::accumulate(stages_str_list.begin(), stages_str_list.end(), std::string{ " | " });

        os << "Stages: (" << stages_str << ")";
        return os;
    }
}
