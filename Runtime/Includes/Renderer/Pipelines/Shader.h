#ifndef __SCOP_SHADER__
#define __SCOP_SHADER__

#include <vector>
#include <cstdint>
#include <filesystem>
#include <unordered_map>

#include <kvf.h>

#include <Maths/Mat4.h>
#include <Utils/NonOwningPtr.h>

namespace Scop
{
	struct ShaderSetLayout
	{
		std::unordered_map<int, VkDescriptorType> binds;

		ShaderSetLayout(std::unordered_map<int, VkDescriptorType> b) : binds(std::move(b)) {}

		inline bool operator==(const ShaderSetLayout& rhs) const { return binds == rhs.binds; }
	};

	struct ShaderPushConstantLayout
	{
		std::size_t offset;
		std::size_t size;

		ShaderPushConstantLayout(std::size_t o, std::size_t s) : offset(o), size(s) {}
	};

	struct ShaderLayout
	{
		std::unordered_map<int, ShaderSetLayout> set_layouts;
		std::vector<ShaderPushConstantLayout> push_constants;

		ShaderLayout(std::unordered_map<int, ShaderSetLayout> s, std::vector<ShaderPushConstantLayout> pc) : set_layouts(std::move(s)), push_constants(std::move(pc)) {}
	};

	enum class ShaderType
	{
		Vertex,
		Fragment,
		Compute
	};

	struct ShaderPipelineLayoutPart
	{
		std::vector<VkPushConstantRange> push_constants;
		std::vector<VkDescriptorSetLayout> set_layouts;
	};

	class Shader
	{
		public:
			Shader(const std::vector<std::uint32_t>& bytecode, ShaderType type, ShaderLayout layout, std::string shader_name = {});

			[[nodiscard]] inline const ShaderLayout& GetShaderLayout() const { return m_layout; }
			[[nodiscard]] inline const std::vector<std::uint32_t>& GetByteCode() const noexcept { return m_bytecode; }
			[[nodiscard]] inline const ShaderPipelineLayoutPart& GetPipelineLayout() const noexcept { return m_pipeline_layout_part; }
			[[nodiscard]] inline VkShaderModule GetShaderModule() const noexcept { return m_module; }
			[[nodiscard]] inline VkShaderStageFlagBits GetShaderStage() const noexcept { return m_stage; }
			[[nodiscard]] inline NonOwningPtr<class GraphicPipeline> GetGraphicPipelineInUse() const noexcept { return p_pipeline_in_use; }

			inline void SetPipelineInUse(NonOwningPtr<class GraphicPipeline> pipeline) noexcept { p_pipeline_in_use = pipeline; }

			void Destroy();

			~Shader();

		private:
			void GeneratePipelineLayout(ShaderLayout layout);

		private:
			std::string m_name;
			ShaderLayout m_layout;
			ShaderPipelineLayoutPart m_pipeline_layout_part;
			std::vector<std::uint32_t> m_bytecode;
			std::vector<VkDescriptorSetLayout> m_set_layouts;
			VkShaderStageFlagBits m_stage;
			VkShaderModule m_module = VK_NULL_HANDLE;
			NonOwningPtr<class GraphicPipeline> p_pipeline_in_use = nullptr;
	};

	std::shared_ptr<Shader> LoadShaderFromFile(const std::filesystem::path& filepath, ShaderType type, ShaderLayout layout);

	static const ShaderLayout DefaultForwardVertexShaderLayout(
		{
			{ 0,
				ShaderSetLayout({ 
					{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
					{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
				})
			}
		}, { ShaderPushConstantLayout({ 0, sizeof(Mat4f) * 2 }) }
	);

	static const Scop::ShaderLayout DefaultShaderLayout(
		{
			{ 1,
				Scop::ShaderSetLayout({ 
					{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
					{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
				})
			}
		}, {}
	);

	static const Scop::ShaderLayout PostProcessShaderLayout(
		{
			{ 0,
				Scop::ShaderSetLayout({ 
					{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
					{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
					{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
				})
			}
		}, {}
	);
}

#endif
