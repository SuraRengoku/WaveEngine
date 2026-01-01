#include "VulkanCommand.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

////////////////////////////////////////////// VULKAN COMMAND POOL /////////////////////////////////////////////

VkResult vulkanCommandPool::allocateBuffers(UTL::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level) const {
    assert(_device != VK_NULL_HANDLE && _pool != VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _pool;
    allocateInfo.commandBufferCount = buffers.size();
    allocateInfo.level = level;
    if (VkResult result = vkAllocateCommandBuffers(_device, &allocateInfo, buffers.data())) {
        debug_output("::VULKAN:ERROR Failed to allocate command buffers\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanCommandPool::allocateBuffers(UTL::vector<vulkanCommandBuffer>& buffers, VkCommandBufferLevel level) const {
    assert(_device != VK_NULL_HANDLE && _pool != VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _pool;
    allocateInfo.commandBufferCount = buffers.size();
    allocateInfo.level = level;
    if (VkResult result = vkAllocateCommandBuffers(_device, &allocateInfo, reinterpret_cast<VkCommandBuffer*>(buffers.data()))) {
        debug_output("::VULKAN:ERROR Failed to allocate command buffers\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanCommandPool::freeBuffers(UTL::vector<VkCommandBuffer>& buffers) const {
    if (buffers.empty()) {
        debug_output(":VULKAN:WARNING the container of command buffer is empty\n");
        return;
    }
    vkFreeCommandBuffers(_device, _pool, buffers.size(), buffers.data());
    memset(buffers.data(), 0, buffers.size() * sizeof(VkCommandBuffer));
}

void vulkanCommandPool::freeBuffers(UTL::vector<vulkanCommandBuffer>& buffers) const {
    if (buffers.empty()) {
        debug_output(":VULKAN:WARNING the container of command buffer is empty\n");
        return;
    }
    // TODO if we use ArrayRef we can just implicitly transfer vulkanCommandBuffer to VkCommandBuffer
    // TODO make sure vulkanCommandBuffer only has one element and no virtual functions
    static_assert(sizeof(vulkanCommandBuffer) == sizeof(VkCommandBuffer),
        "::VULKAN:ERROR vulkanCommandBuffer must have th same size as VkCommandBuffer\n");

    vkFreeCommandBuffers(_device, _pool, buffers.size(), reinterpret_cast<const VkCommandBuffer*>(buffers.data()));
    memset(buffers.data(), 0, buffers.size() * sizeof(vulkanCommandBuffer));
}

VkResult vulkanCommandPool::create(const VkCommandPoolCreateInfo& createInfo) {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkCreateCommandPool(_device, &createInfo, nullptr, &_pool)) {
        debug_output("::VULKAN:ERROR Failed to create a command pool\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanCommandPool::create(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.flags = flags;
    return create(createInfo);
}

////////////////////////////////////////////// VULKAN COMMAND BUFFER /////////////////////////////////////////////

VkResult vulkanCommandBuffer::begin(VkCommandBufferUsageFlags usageFlags,
                                    const VkCommandBufferInheritanceInfo& inheritanceInfo) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    if (VkResult result = vkBeginCommandBuffer(_commandBuffer, &beginInfo)) {
        debug_output("::VULKAN:ERROR Failed to begin a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanCommandBuffer::begin(VkCommandBufferUsageFlags usageFlags) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags;
    beginInfo.pInheritanceInfo = VK_NULL_HANDLE;
    if (VkResult result = vkBeginCommandBuffer(_commandBuffer, &beginInfo)) {
        debug_output("::VULKAN:ERROR Failed to begin a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanCommandBuffer::end() const {
    if (VkResult result = vkEndCommandBuffer(_commandBuffer)) {
        debug_output("::VULKAN:ERROR Failed to end a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}
}
