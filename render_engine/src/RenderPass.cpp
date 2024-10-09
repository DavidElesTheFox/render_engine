#include <render_engine/RenderPass.h>

#include <render_engine/FrameBuffer.h>
#include <render_engine/LogicalDevice.h>

#include <algorithm>
#include <cmath>
#include <ranges>

namespace RenderEngine
{
    namespace
    {
        uint32_t calculateSumOfFirstNNumbers(float n)
        {
            return std::round(n / 2.0f * (n + 1));
        }
    }

    RenderPassBuilder::SubPassGraph::SubPassGraph(uint32_t size)
        :_dependency_matrix(calculateSumOfFirstNNumbers(size - 1), VkSubpassDependency{})
    {
        // Setting 'no-link' between subpasses by default
        for (uint32_t i = 1; i < size; ++i)
        {
            for (uint32_t j = i + 1; j < size; ++j)
            {
                _dependency_matrix[getIndex(i, j)].srcSubpass = i;
                _dependency_matrix[getIndex(i, j)].dstSubpass = j;
                _dependency_matrix[getIndex(i, j)].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                _dependency_matrix[getIndex(i, j)].srcAccessMask = VK_ACCESS_NONE;

                _dependency_matrix[getIndex(i, j)].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                _dependency_matrix[getIndex(i, j)].dstAccessMask = VK_ACCESS_NONE;

                _dependency_matrix[getIndex(i, j)].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
        }
    }

    VkSubpassDependency& RenderPassBuilder::SubPassGraph::operator()(uint32_t from, const ExternalDependency&)
    {
        auto it = std::ranges::find_if(_dependency_matrix, [&](const VkSubpassDependency& info) { return info.srcSubpass == from; });
        if (it == _dependency_matrix.end())
        {
            VkSubpassDependency item{
                .srcSubpass = from,
                .dstSubpass = VK_SUBPASS_EXTERNAL,
                .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_NONE,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
            };
            _dependency_matrix.push_back(item);
            it = _dependency_matrix.end() - 1;
        }
        return *it;
    }

    VkSubpassDependency& RenderPassBuilder::SubPassGraph::operator()(const ExternalDependency&, uint32_t to)
    {
        auto it = std::ranges::find_if(_dependency_matrix, [&](const VkSubpassDependency& info) { return info.dstSubpass == to; });
        if (it == _dependency_matrix.end())
        {
            VkSubpassDependency item{
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = to,
                .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_NONE,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
            };
            _dependency_matrix.push_back(item);
            it = _dependency_matrix.end() - 1;
        }
        return *it;
    }

    VkRenderPassCreateInfo& RenderPassBuilder::SubPassGraph::fill(VkRenderPassCreateInfo& create_info) const
    {
        create_info.dependencyCount = _dependency_matrix.size();
        create_info.pDependencies = _dependency_matrix.data();
        return create_info;
    }
    /*
    * The idea is to modifying the well know linearized matrix indexing and restricting it to the upper triangle.
    *
    * | 0 1 2 |     | - 0 1 |
    * | 3 4 5 |  -> | - - 2 |
    * | 6 7 8 |     | - - - |
    *
    * | 00 01 02 03 |    | -- 00 01 02 |
    * | 04 05 06 07 |    | -- -- 03 04 |
    * | 08 09 10 11 |    | -- -- -- 05 |
    * | 12 13 14 15 |    | -- -- -- -- |
    *
    * The pattern that the new index is: idx(i, j) - {sum of the first i + 1 number}
    */
    uint32_t RenderPassBuilder::SubPassGraph::getIndex(uint32_t row, uint32_t col) const
    {
        if (col < row)
        {
            std::swap(row, col);
        }
        const uint32_t linear_index = row * _size + col;

        auto sum_of_firs_n_number = [](float n) -> uint32_t
            {
                return std::round(n / 2.0f * (n + 1));
            };

        return linear_index - sum_of_firs_n_number(row + 1);
    }
    RenderPass RenderPassBuilder::build(LogicalDevice& logical_device)
    {
        VkRenderPassCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        create_info.attachmentCount = _attachments.size();
        create_info.pAttachments = _attachments.data();

        std::vector<VkSubpassDescription> subpass_descriptions;
        for (const auto& subpass : _subpasses)
        {
            subpass_descriptions.push_back(subpass.get());
        }
        create_info.subpassCount = subpass_descriptions.size();
        create_info.pSubpasses = subpass_descriptions.data();

        _dependency_graph->fill(create_info);

        VkRenderPass render_pass{ VK_NULL_HANDLE };

        if (logical_device->vkCreateRenderPass(*logical_device, &create_info, VK_NULL_HANDLE, &render_pass) == false)
        {
            throw std::runtime_error("Can't create render pass");
        }
        return RenderPass(render_pass, *this);
    }
    RenderPassBuilder& RenderPassBuilder::addSubPass(SubPass sub_pass)
    {
        assert(_dependency_graph == std::nullopt && "Dependency graph is built but new subpass is added.");
        _subpasses.push_back(std::move(sub_pass));
        return *this;
    }
    RenderPassBuilder& RenderPassBuilder::addAttachment(VkAttachmentDescription attachment)
    {
        _attachments.push_back(std::move(attachment));
        return *this;
    }
    FrameBufferBuilder RenderPass::createFrameBufferBuilder() const
    {
        return FrameBufferBuilder(_render_pass, _origin.getAttachmentDescriptions());
    }
    RenderPass::EnableScope RenderPass::begin(LogicalDevice& logical_device,
                                              VkCommandBuffer command_buffer,
                                              VkRect2D render_area,
                                              FrameBuffer* frame_buffer,
                                              const std::vector<VkClearValue>& clear_colors)
    {
        VkRenderPassBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.clearValueCount = clear_colors.size();
        begin_info.pClearValues = clear_colors.data();
        begin_info.framebuffer = frame_buffer->get();
        begin_info.renderArea = render_area;
        begin_info.renderPass = _render_pass;

        return EnableScope(logical_device,
                           command_buffer,
                           begin_info,
                           frame_buffer,
                           this);
    }
    RenderPass::EnableScope::EnableScope(LogicalDevice& logical_device,
                                         VkCommandBuffer command_buffer,
                                         const VkRenderPassBeginInfo& begin_info,
                                         FrameBuffer* frame_buffer,
                                         RenderPass* render_pass)
        : _logical_device(logical_device)
        , _command_buffer(command_buffer)
        , _frame_buffer(frame_buffer)
        , _render_pass(render_pass)
    {
        logical_device->vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        const auto& attachment_descriptions = _render_pass->_origin.getAttachmentDescriptions();
        for (uint32_t i = 0; i < attachment_descriptions.size(); ++i)
        {
            const VkAttachmentDescription& description = attachment_descriptions[i];
            Texture& attachment = frame_buffer->getAttachmentTexture(i);
            auto new_state = attachment.getResourceState(_current_scope).clone()
                .setImageLayout(description.initialLayout);
            _render_pass->overrideResourceState(attachment, _current_scope, std::move(new_state));
        }
    }
    RenderPass::EnableScope::~EnableScope()
    {
        auto& logical_device = *_logical_device;
        logical_device->vkCmdEndRenderPass(_command_buffer);
        const auto& attachment_descriptions = _render_pass->_origin.getAttachmentDescriptions();
        for (uint32_t i = 0; i < attachment_descriptions.size(); ++i)
        {
            const VkAttachmentDescription& description = attachment_descriptions[i];
            Texture& attachment = _frame_buffer->getAttachmentTexture(i);
            auto new_state = attachment.getResourceState(_current_scope).clone()
                .setImageLayout(description.finalLayout);
            _render_pass->overrideResourceState(attachment, _current_scope, std::move(new_state));
        }
    }
}