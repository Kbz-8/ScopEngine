#ifndef __SCOP_VULKAN_MEMORY_CHUNK__
#define __SCOP_VULKAN_MEMORY_CHUNK__

#include <vector>
#include <cstdint>
#include <optional>

#include <Renderer/Memory/Block.h>

namespace Scop
{
	class MemoryChunk
	{
		public:
			MemoryChunk(VkDevice device, VkPhysicalDevice physical, VkDeviceSize size, std::int32_t memory_type_index, bool is_dedicated, std::uint32_t& vram_usage, std::uint32_t& vram_host_visible_usage);

			[[nodiscard]] std::optional<MemoryBlock> Allocate(VkDeviceSize size, VkDeviceSize alignment);
			void Deallocate(const MemoryBlock& block);
			[[nodiscard]] inline bool Has(const MemoryBlock& block) const noexcept { return block.memory == m_memory; }
			[[nodiscard]] inline std::int32_t GetMemoryTypeIndex() const noexcept { return m_memory_type_index; }
			[[nodiscard]] inline bool IsDedicated() const noexcept { return m_is_dedicated; }
			[[nodiscard]] inline void* GetMap() const noexcept { return p_map; }
			[[nodiscard]] inline VkDeviceSize GetSize() const noexcept { return m_size; }

			~MemoryChunk();

		protected:
			std::vector<MemoryBlock> m_blocks;
			VkDevice m_device = VK_NULL_HANDLE;
			VkPhysicalDevice m_physical = VK_NULL_HANDLE;
			VkDeviceMemory m_memory = VK_NULL_HANDLE;
			void* p_map = nullptr;
			VkDeviceSize m_size = 0;
			std::int32_t m_memory_type_index;
			bool m_is_dedicated;
	};
}

#endif
