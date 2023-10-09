#include "DemoApplication.h"

void DemoApplication::init()
{
	using namespace RenderEngine;
	std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
	renderers->registerRenderer(UIRenderer::kRendererId,
		[](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
		{return std::make_unique<UIRenderer>(window, swap_chain, back_buffer_count, previous_renderer == nullptr); });

	Shader::MetaData nolit_vertex_meta_data;
	nolit_vertex_meta_data.attributes_stride = 5 * sizeof(float);
	nolit_vertex_meta_data.input_attributes.push_back({ .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 });
	nolit_vertex_meta_data.input_attributes.push_back({ .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 2 * sizeof(float) });

	Shader::MetaData nolit_frament_meta_data;
	nolit_frament_meta_data.global_uniform_buffers.insert({ 0, Shader::MetaData::Uniforms{.binding = 0, .size = sizeof(float)} });

	Shader nolit_vertex_shader(BASE_VERT_SHADER, nolit_vertex_meta_data);
	Shader nolit_fretment_shader(BASE_FRAG_SHADER, nolit_frament_meta_data);
	_nolit_material = std::make_unique<Material>(nolit_vertex_shader,
		nolit_fretment_shader,
		[&](std::vector<UniformBinding>& uniform_bindings, uint32_t frame_number) {uniform_bindings[0].getBuffer(frame_number).uploadMapped(_nolit_shader_controller.data()); },
		[](const Geometry& geometry, const Material& material)
		{
			std::vector<float> vertex_buffer_data;
			vertex_buffer_data.reserve(geometry.positions.size() * 5);
			for (uint32_t i = 0; i < geometry.positions.size(); ++i)
			{
				vertex_buffer_data.push_back(geometry.positions[i].x);
				vertex_buffer_data.push_back(geometry.positions[i].y);
				vertex_buffer_data.push_back(geometry.colors[i].r);
				vertex_buffer_data.push_back(geometry.colors[i].g);
				vertex_buffer_data.push_back(geometry.colors[i].b);
			}
			auto begin = reinterpret_cast<uint8_t*>(vertex_buffer_data.data());
			std::vector<uint8_t> vertex_buffer(begin, begin + vertex_buffer_data.size() * sizeof(float));
			return vertex_buffer;
		}, 0
		);


	renderers->registerRenderer(ForwardRenderer::kRendererId,
		[&](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
		{return std::make_unique<ForwardRenderer>(window, swap_chain, has_next, std::vector{ _nolit_material.get() }); });

	RenderContext::initialize({ "VK_LAYER_KHRONOS_validation" }, std::move(renderers));
	createMesh();

	_window = RenderContext::context().getEngine(0).createWindow("Secondary Window", 3);
	_window->registerRenderers({ ForwardRenderer::kRendererId, UIRenderer::kRendererId });
	_window->getRendererAs<ForwardRenderer>(ForwardRenderer::kRendererId).addMesh(_triangle.get(), 0);

	_window->getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
		[&] {
			float value = _nolit_shader_controller.color_offset;
			ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
			_nolit_shader_controller.color_offset = value; });
}

void DemoApplication::run()
{
	while (_window->isClosed() == false)
	{
		_window->update();
	}
}

void DemoApplication::createMesh()
{
	_triangle_geometry = std::make_unique<RenderEngine::Geometry>();
	_triangle_geometry->positions.push_back({ -0.5f, -0.5f, 0.0f });
	_triangle_geometry->positions.push_back({ 0.5f, -0.5f, 0.0f });
	_triangle_geometry->positions.push_back({ 0.5f, 0.5f, 0.0f });
	_triangle_geometry->positions.push_back({ -0.5f, 0.5f, 0.0f });
	_triangle_geometry->colors.push_back({ .0f, 0.0f, 0.0f });
	_triangle_geometry->colors.push_back({ .0f, 1.0f, 0.0f });
	_triangle_geometry->colors.push_back({ .0f, 0.0f, 1.0f });
	_triangle_geometry->colors.push_back({ 1.0f, 1.0f, 1.0f });
	_triangle_geometry->indexes = {
		0, 1, 2, 2, 3, 0
	};
	_triangle = std::make_unique<RenderEngine::Mesh>(_triangle_geometry.get(), _nolit_material.get(), 0);
}
