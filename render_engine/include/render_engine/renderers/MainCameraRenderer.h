#pragma once
#include <render_engine/renderers/AbstractRenderer.h>
#include <glm/glm.hpp>
#include <render_engine/window/Window.h>

#include <variant>
#include <filesystem>
#include <map>

namespace RenderEngine
{
	class Shader
	{
	public:
		class LoadedShader
		{
		public:
			LoadedShader(VkShaderModule module, VkDevice used_device)
				: _module(module),
				_device_loaded_on(used_device) {}
			LoadedShader(const LoadedShader&) = delete;
			LoadedShader(LoadedShader&&) = default;
			LoadedShader& operator=(const LoadedShader&) = delete;
			LoadedShader& operator=(LoadedShader&&) = default;
			~LoadedShader();

			VkShaderModule module() const { return _module; }
		private:
			VkShaderModule _module{ VK_NULL_HANDLE };
			VkDevice _device_loaded_on{ VK_NULL_HANDLE };
		};

		struct MetaData
		{
			struct Attribute
			{
				uint32_t location{ 0 };
				VkFormat format{};
				uint32_t offset{ 0 };
			};

			struct Uniforms
			{
				int32_t binding{ -1 };
				int32_t size{ -1 };

			};

			uint32_t stride;
			std::vector<Attribute> input_attributes;
			std::unordered_map<int32_t, Uniforms> uniform_buffers;
		};
		explicit Shader(std::filesystem::path spriv_path, MetaData meta_data)
			: _spirv_path(std::move(spriv_path))
			, _meta_data(std::move(meta_data))
		{}

		LoadedShader loadOn(VkDevice logical_device) const;
		const MetaData& metaData() const
		{
			return _meta_data;
		}
		void addUniform(int32_t binding, int32_t size)
		{
			_meta_data.uniform_buffers[binding].binding = binding;
			_meta_data.uniform_buffers[binding].size = size;
		}

	private:
		std::filesystem::path _spirv_path;
		MetaData _meta_data;
	};

	class Technique;
	class UniformBinding;
	class Mesh;
	class Material;
	class Geometry;

	class Material
	{
	public:
		Material(Shader verted_shader,
			Shader fragment_shader,
			std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> buffer_update_callback,
			std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> vertex_buffer_create_callback,
			uint32_t id);
		const Shader& vertexShader() const { return _vertex_shader; }
		const Shader& fragmentShader() const { return _fragment_shader; }

		std::unique_ptr<Technique> createTechnique(VkDevice logical_device,
			GpuResourceManager& gpu_resource_manager,
			VkRenderPass render_pass) const;

		uint32_t id() const { return _id; }

		std::vector<uint8_t> createVertexBufferFromGeometry(const Geometry& geometry) const
		{
			return _vertex_buffer_create_callback(geometry, *this);
		}
		void updateUniformBuffers(std::vector<UniformBinding>& ubo_container, uint32_t frame_number) const
		{
			_buffer_update_callback(ubo_container, frame_number);
		}
	private:
		std::vector<UniformBinding> createUniformBindings(GpuResourceManager& gpu_resource_manager,
			std::ranges::input_range auto&& uniforms_data,
			VkShaderStageFlags shader_stage) const;
		Shader _vertex_shader;
		Shader _fragment_shader;
		uint32_t _id;
		std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> _buffer_update_callback;
		std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> _vertex_buffer_create_callback;
	};

	template<typename T>
	class BackBuffer
	{
	public:
		explicit BackBuffer(size_t size)
			:_back_buffer()
		{
			_back_buffer.resize(size);
		}

		BackBuffer(const BackBuffer&) = delete;
		BackBuffer(BackBuffer&&) = default;
		BackBuffer& operator=(const BackBuffer&) = delete;
		BackBuffer& operator=(BackBuffer&&) = default;

		typename std::vector<T>::iterator begin() { return _back_buffer.begin(); }
		typename std::vector<T>::iterator end() { return _back_buffer.end(); }

		const T& operator[](size_t frame_number) const
		{
			return _back_buffer[frame_number % _back_buffer.size()];
		}
		T& operator[](size_t frame_number)
		{
			return _back_buffer[frame_number % _back_buffer.size()];
		}
		size_t size() const { return _back_buffer.size(); }
	private:
		std::vector<T> _back_buffer;
	};
	class UniformBinding
	{
	public:
		struct FrameData
		{
			VkDescriptorSet _descriptor_set{ VK_NULL_HANDLE };
			std::unique_ptr<Buffer> _buffer{};
		};

		UniformBinding(VkDescriptorSetLayout descriptor_set_layout,
			BackBuffer<FrameData>&& back_buffer,
			VkDevice logical_device)
			:_logical_device(logical_device)
			, _descriptor_set_layout(descriptor_set_layout)
			, _back_buffer(std::move(back_buffer))
		{}
		~UniformBinding();
		UniformBinding(const UniformBinding&) = delete;
		UniformBinding(UniformBinding&&) = default;
		UniformBinding& operator=(const UniformBinding&) = delete;
		UniformBinding& operator=(UniformBinding&&) = default;

		Buffer& buffer(size_t frame_number)
		{
			return *_back_buffer[frame_number]._buffer;
		}
		VkDescriptorSetLayout descriptorSetLayout() const { return _descriptor_set_layout; }
		VkDescriptorSet getDescriptorSet(size_t frame_number)
		{
			return _back_buffer[frame_number]._descriptor_set;
		}
	private:
		VkDevice _logical_device;
		VkDescriptorSetLayout _descriptor_set_layout{ VK_NULL_HANDLE };
		BackBuffer<FrameData> _back_buffer;
	};

	class Technique
	{
	public:
		Technique(VkDevice logical_device,
			const Material* material,
			std::vector<UniformBinding>&& uniform_buffers,
			VkRenderPass render_pass);
		~Technique();
		VkPipeline getPipeline()
		{
			return _pipeline;
		}
		VkPipelineLayout getPipelineLayout()
		{
			return _pipeline_layout;
		}
		std::vector<VkDescriptorSet> collectDescriptorSets(size_t frame_number)
		{
			std::vector<VkDescriptorSet> result;
			for (auto& binding : _uniform_buffers)
			{
				result.push_back(binding.getDescriptorSet(frame_number));
			}
			return result;
		}

		void update(uint32_t frame_number)
		{
			_material->updateUniformBuffers(_uniform_buffers, frame_number);
		}
	private:

		const Material* _material;

		std::vector<UniformBinding> _uniform_buffers;
		VkDevice _logical_device;
		VkPipeline _pipeline{ VK_NULL_HANDLE };
		VkPipelineLayout _pipeline_layout{ VK_NULL_HANDLE };
	};

	struct Geometry
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> colors;
		std::vector<glm::vec3> normals;
		std::vector<int16_t> indexes;
		std::vector<glm::vec2> uv;
	};
	class Mesh
	{
	public:
		Mesh(Geometry* geometry, Material* material, int32_t id)
			: _geometry(std::move(geometry))
			, _material(std::move(material))
			, _id(id)
		{}

		const Geometry& geometry() const { return *_geometry; }
		const Material& material() const { return *_material; }
		int getId() const { return _id; }
		std::vector<uint8_t> createVertexBuffer() const
		{
			return material().createVertexBufferFromGeometry(geometry());
		}
	private:
		Geometry* _geometry{ nullptr };
		Material* _material{ nullptr };
		int32_t _id{ 0 };
	};

	class ForwardRenderer : public AbstractRenderer
	{
	private:
		struct MeshBuffers
		{
			std::unique_ptr<Buffer> vertex_buffer;
			std::unique_ptr<Buffer> index_buffer;
			std::unique_ptr<Buffer> color_buffer;
			std::unique_ptr<Buffer> normal_buffer;
			std::unique_ptr<Buffer> texture_buffer;
		};
		struct MeshGroup
		{
			Technique* technique;
			std::map<Mesh*, MeshBuffers> mesh_buffers;
		};
		struct FrameData
		{
			VkCommandBuffer command_buffer;
		};
		using MeshGroupMap = std::unordered_map<uint32_t, MeshGroup>;
	public:
		static constexpr uint32_t kRendererId = 2u;
		ForwardRenderer(Window& parent,
			const SwapChain& swap_chain,
			bool last_renderer,
			std::vector<Material*> supported_materials);
		~ForwardRenderer();
		void addMesh(Mesh* mesh, int32_t priority);
		void draw(uint32_t swap_chain_image_index, uint32_t frame_number) override;
		std::vector<VkCommandBuffer> getCommandBuffers(uint32_t frame_number) override
		{
			return { _back_buffer[frame_number].command_buffer };
		}
	private:
		void createFrameBuffers(const SwapChain& swap_chain);
		void createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index);
		void createCommandPool(uint32_t render_queue_family);
		void createCommandBuffer();

		void resetFrameBuffers();
		void beforeReinit() override;
		void finalizeReinit(const SwapChain& swap_chain) override;
	private:
		std::map<int32_t, MeshGroupMap> _meshes;
		std::unordered_map<uint32_t, std::unique_ptr<Technique>> _technique_map;
		BackBuffer<FrameData> _back_buffer;

		VkRect2D _render_area;
		Window& _window;

		std::vector<VkFramebuffer> _frame_buffers;
		VkRenderPass _render_pass{ VK_NULL_HANDLE };

		VkCommandPool _command_pool{ VK_NULL_HANDLE };
	};
}
