#ifndef __SCOP_VULKAN_MEMORY_DEVICE_ALLOCATOR__
#define __SCOP_VULKAN_MEMORY_DEVICE_ALLOCATOR__

#include <mutex>
#include <vector>
#include <memory>
#include <cstdint>

#include <Renderer/Memory/Block.h>
#include <Renderer/Memory/Chunk.h>

namespace Scop
{
	constexpr std::size_t SMALL_HEAP_MAX_SIZE = (1024ULL * 1024 * 1024); // 1GB
	constexpr std::size_t DEFAULT_LARGE_HEAP_BLOCK_SIZE = (256ULL * 1024 * 1024); // 256MiB
	constexpr std::uint32_t NEW_BLOCK_SIZE_SHIFT_MAX = 3;

	class DeviceAllocator
	{
		public:
			DeviceAllocator() = default;

			void AttachToDevice(VkDevice device, VkPhysicalDevice physical) noexcept;
			inline void DetachFromDevice() noexcept { m_chunks.clear(); m_device = VK_NULL_HANDLE; m_physical = VK_NULL_HANDLE; }
			[[nodiscard]] inline std::size_t GetAllocationsCount() const noexcept { return m_allocations_count; }

			[[nodiscard]] MemoryBlock Allocate(VkDeviceSize size, VkDeviceSize alignment, std::int32_t memory_type_index, bool dedicated_chunk = false);
			void Deallocate(const MemoryBlock& block);

			[[nodiscard]] inline std::uint32_t GetVramUsage() const noexcept { return m_vram_usage; }
			[[nodiscard]] inline std::uint32_t GetVramHostVisibleUsage() const noexcept { return m_vram_host_visible_usage; }

			~DeviceAllocator() = default;

		private:
			VkDeviceSize CalcPreferredChunkSize(std::uint32_t mem_type_index);

		private:
			std::vector<std::unique_ptr<MemoryChunk>> m_chunks;
			VkPhysicalDeviceMemoryProperties m_mem_props;
			VkDevice m_device = VK_NULL_HANDLE;
			VkPhysicalDevice m_physical = VK_NULL_HANDLE;
			std::size_t m_allocations_count = 0;
			std::mutex m_alloc_mutex;
			std::mutex m_dealloc_mutex;
			std::uint32_t m_vram_usage = 0;
			std::uint32_t m_vram_host_visible_usage = 0;
			bool m_last_chunk_creation_failed = false;
	};
}

#endif
