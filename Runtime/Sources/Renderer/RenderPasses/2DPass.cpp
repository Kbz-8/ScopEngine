#include <Renderer/RenderPasses/2DPass.h>
#include <Renderer/Pipelines/Graphics.h>
#include <Renderer/ViewerData.h>
#include <Renderer/Renderer.h>
#include <Graphics/Scene.h>
#include <Core/Engine.h>
#include <Maths/Mat4.h>

namespace Scop
{
	struct SpriteData
	{
		Mat4f model_matrix;
		Vec4f color;
	};

	struct ViewerData2D
	{
		Mat4f projection;
	};

	void Render2DPass::Init()
	{
		ShaderLayout vertex_shader_layout(
			{
				{ 0,
					ShaderSetLayout({ 
						{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
					})
				}
			}, { ShaderPushConstantLayout({ 0, sizeof(SpriteData) }) }
		);
		p_vertex_shader = LoadShaderFromFile(ScopEngine::Get().GetAssetsPath() / "Shaders/Build/2DVertex.spv", ShaderType::Vertex, std::move(vertex_shader_layout));
		ShaderLayout fragment_shader_layout(
			{
				{ 1,
					ShaderSetLayout({ 
						{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
					})
				}
			}, {}
		);
		p_fragment_shader = LoadShaderFromFile(ScopEngine::Get().GetAssetsPath() / "Shaders/Build/2DFragment.spv", ShaderType::Fragment, std::move(fragment_shader_layout));

		std::function<void(const EventBase&)> functor = [this](const EventBase& event)
		{
			if(event.What() == Event::ResizeEventCode || event.What() == Event::SceneHasChangedEventCode)
				m_pipeline.Destroy();
		};
		EventBus::RegisterListener({ functor, "__ScopRender2DPass" });

		p_viewer_data_set = RenderCore::Get().GetDescriptorPoolManager().GetAvailablePool().RequestDescriptorSet(p_vertex_shader->GetShaderLayout().set_layouts.at(0), ShaderType::Vertex);
		p_texture_set = RenderCore::Get().GetDescriptorPoolManager().GetAvailablePool().RequestDescriptorSet(p_fragment_shader->GetShaderLayout().set_layouts.at(1), ShaderType::Fragment);

		p_viewer_data_buffer = std::make_shared<UniformBuffer>();
		p_viewer_data_buffer->Init(sizeof(ViewerData2D));
		for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			p_viewer_data_set->SetUniformBuffer(i, 0, p_viewer_data_buffer->Get(i));
			p_viewer_data_set->Update(i);
		}
	}

	void Render2DPass::Pass(Scene& scene, Renderer& renderer, Texture& render_target)
	{
		if(m_pipeline.GetPipeline() == VK_NULL_HANDLE)
		{
			GraphicPipelineDescriptor pipeline_descriptor;
			pipeline_descriptor.vertex_shader = p_vertex_shader;
			pipeline_descriptor.fragment_shader = p_fragment_shader;
			pipeline_descriptor.color_attachments = { &render_target };
			pipeline_descriptor.culling = CullMode::None;
			pipeline_descriptor.clear_color_attachments = false;
			pipeline_descriptor.name = "2D_pass_pipeline";
			m_pipeline.Init(std::move(pipeline_descriptor));
		}

		std::uint32_t frame_index = renderer.GetCurrentFrameIndex();

		ViewerData2D viewer_data;
		viewer_data.projection = Mat4f::Ortho(0.0f, render_target.GetWidth(), render_target.GetHeight(), 0.0f);
		static CPUBuffer buffer(sizeof(ViewerData2D));
		std::memcpy(buffer.GetData(), &viewer_data, buffer.GetSize());
		p_viewer_data_buffer->SetData(buffer, frame_index);

		VkCommandBuffer cmd = renderer.GetActiveCommandBuffer();
		m_pipeline.BindPipeline(cmd, 0, {});
		for(const auto& [_, sprite] : scene.GetSprites())
		{
			SpriteData sprite_data;
			sprite_data.color = sprite.GetColor();

			Mat4f translation_matrix = Mat4f::Identity().ApplyTranslation(Vec3f{ Vec2f(sprite.GetPosition()), 0.0f });
			Mat4f scale_matrix = Mat4f::Identity().ApplyScale(Vec3f{ sprite.GetScale(), 1.0f });

			sprite_data.model_matrix = Mat4f::Identity();
			sprite_data.model_matrix.ConcatenateTransform(scale_matrix);
			sprite_data.model_matrix.ConcatenateTransform(translation_matrix);

			if(!sprite.IsSetInit())
				const_cast<Sprite&>(sprite).UpdateDescriptorSet(p_texture_set);
			const_cast<Sprite&>(sprite).Bind(frame_index, cmd);
			std::array<VkDescriptorSet, 2> sets = { p_viewer_data_set->GetSet(frame_index), sprite.GetSet(frame_index) };
			RenderCore::Get().vkCmdBindDescriptorSets(cmd, m_pipeline.GetPipelineBindPoint(), m_pipeline.GetPipelineLayout(), 0, sets.size(), sets.data(), 0, nullptr);
			RenderCore::Get().vkCmdPushConstants(cmd, m_pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SpriteData), &sprite_data);
			sprite.GetMesh()->Draw(cmd, renderer.GetDrawCallsCounterRef(), renderer.GetPolygonDrawnCounterRef());
		}
		for(const auto& [_, text] : scene.GetTexts())
		{
			SpriteData sprite_data;
			sprite_data.color = text.GetColor();

			Mat4f translation_matrix = Mat4f::Identity().ApplyTranslation(Vec3f{ Vec2f(text.GetPosition()), 0.0f });
			Mat4f scale_matrix = Mat4f::Identity().ApplyScale(Vec3f{ text.GetScale(), 1.0f });

			sprite_data.model_matrix = Mat4f::Identity();
			sprite_data.model_matrix.ConcatenateTransform(scale_matrix);
			sprite_data.model_matrix.ConcatenateTransform(translation_matrix);

			if(!text.IsSetInit())
				const_cast<Text&>(text).UpdateDescriptorSet(p_texture_set);
			const_cast<Text&>(text).Bind(frame_index, cmd);
			std::array<VkDescriptorSet, 2> sets = { p_viewer_data_set->GetSet(frame_index), text.GetSet(frame_index) };
			RenderCore::Get().vkCmdBindDescriptorSets(cmd, m_pipeline.GetPipelineBindPoint(), m_pipeline.GetPipelineLayout(), 0, sets.size(), sets.data(), 0, nullptr);
			RenderCore::Get().vkCmdPushConstants(cmd, m_pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SpriteData), &sprite_data);
			text.GetMesh()->Draw(cmd, renderer.GetDrawCallsCounterRef(), renderer.GetPolygonDrawnCounterRef());
		}
		m_pipeline.EndPipeline(cmd);
	}

	void Render2DPass::Destroy()
	{
		RenderCore::Get().WaitDeviceIdle();
		m_pipeline.Destroy();
		p_vertex_shader.reset();
		p_fragment_shader.reset();
		p_viewer_data_set.reset();
		p_viewer_data_buffer->Destroy();
		p_texture_set.reset();
	}
}
