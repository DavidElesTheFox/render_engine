#pragma once

#include <render_engine/resources/Texture.h>
#include <render_engine/synchronization/ResourceStates.h>

#include <cassert>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <volk.h>

namespace RenderEngine
{
    class RenderPass;
    class LogicalDevice;

    class RenderPassBuilder
    {
    public:
        class SubPass;
        /*
        * Representation of the dependencies. Its goal is to fulfill the requirements of vulkan
        * subpass dependencies. I.e.: Having defined dependencies between all existing subpasses.
        *
        * The basic idea is to having a matrix representation of the graph.
        *  - We doesn't need a full matrix. I.e.: between subpass A and B must have a connection, but only one
        *    from A to B or B to A. The direction is encoded inside the struct, so it doesn't need to be coded in the graph.
        *    Thus, we can work with the upper triangle matrix.
        *  - We need a linear representation of the data. To avoiding copy during fill.
        *  - External dependencies can be pushed after the dependency graph
        */
        class SubPassGraph
        {
        public:
            struct ExternalDependency {};
            explicit SubPassGraph(uint32_t size);
            VkSubpassDependency& operator()(uint32_t from, uint32_t to)
            {
                return _dependency_matrix[getIndex(from, to)];
            }
            VkSubpassDependency& operator()(uint32_t from, const ExternalDependency&);
            VkSubpassDependency& operator()(const ExternalDependency&, uint32_t to);
            VkRenderPassCreateInfo& fill(VkRenderPassCreateInfo& create_info) const;
            uint32_t getSize() const { return _dependency_matrix.size(); }
        private:
            uint32_t getIndex(uint32_t row, uint32_t col) const;
            uint32_t _size;
            std::vector<VkSubpassDependency> _dependency_matrix;
        };
        RenderPass build(LogicalDevice& logical_device);
        RenderPassBuilder& addSubPass(SubPass sub_pass);
        RenderPassBuilder& addAttachment(VkAttachmentDescription attachment);
        SubPassGraph& getDependencyGraph()
        {
            if (_dependency_graph == std::nullopt)
            {
                _dependency_graph = SubPassGraph(_subpasses.size());
            }
            return *_dependency_graph;
        }

        const std::vector<VkAttachmentDescription> getAttachmentDescriptions() const { return _attachments; }
    private:

        std::vector<VkAttachmentDescription> _attachments;
        std::vector<SubPass> _subpasses;
        std::optional<SubPassGraph> _dependency_graph;
    };

    class RenderPassBuilder::SubPass
    {
    public:
        SubPass(std::string name, VkPipelineBindPoint bind_point)
            : _name(std::move(name))
        {
            _description.pipelineBindPoint = bind_point;
        }
        SubPass& addColorAttachment(VkAttachmentReference reference)
        {
            _color_attachments.push_back(std::move(reference));
            _description.pColorAttachments = _color_attachments.data();
            _description.colorAttachmentCount = _color_attachments.size();
        }
        SubPass& addInputAttachment(VkAttachmentReference reference)
        {
            _input_attachment.push_back(std::move(reference));
            _description.pInputAttachments = _input_attachment.data();
            _description.inputAttachmentCount = _input_attachment.size();
        }
        const VkSubpassDescription& get() const
        {
            return _description;
        }
    private:
        std::string _name;
        VkSubpassDescription _description{};
        std::vector<VkAttachmentReference> _color_attachments;
        std::vector<VkAttachmentReference> _input_attachment;
    };

    class FrameBufferBuilder;
    class FrameBuffer;
    class RenderPass
    {
    public:
        class EnableScope;

        RenderPass(VkRenderPass render_pass, RenderPassBuilder origin)
            : _render_pass(render_pass)
            , _origin(std::move(origin))
        {}
        RenderPass(RenderPass&&) = default;
        RenderPass(const RenderPass&) = delete;

        RenderPass& operator=(RenderPass&&) = default;
        RenderPass& operator=(const RenderPass&) = delete;

        FrameBufferBuilder createFrameBufferBuilder() const;
        EnableScope begin(LogicalDevice& logical_device,
                          VkCommandBuffer command_buffer,
                          VkRect2D render_area,
                          FrameBuffer* frame_buffer,
                          const std::vector<VkClearValue>& clear_colors);

        VkRenderPass get() { return _render_pass; }
    private:
        void overrideResourceState(Texture& texture, SubmitScope& scope, TextureState new_state)
        {
            texture.overrideResourceState(std::move(new_state), scope, {});
        }

        VkRenderPass _render_pass{ VK_NULL_HANDLE };
        RenderPassBuilder _origin;

    };

    class RenderPass::EnableScope
    {
    public:
        EnableScope(LogicalDevice& logical_device,
                    VkCommandBuffer command_buffer,
                    const VkRenderPassBeginInfo& begin_info,
                    FrameBuffer* frame_buffer,
                    RenderPass* render_pass);
        EnableScope(EnableScope&&) = default;
        EnableScope(const EnableScope&) = delete;

        EnableScope& operator=(EnableScope&&) = default;
        EnableScope& operator=(const EnableScope&) = delete;

        SubmitScope& getSubmitScope() { return _current_scope; }

        ~EnableScope();
    private:
        RefObj<LogicalDevice> _logical_device;
        VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
        RenderPass* _render_pass;
        FrameBuffer* _frame_buffer;
        SubmitScope _current_scope;
    };

}