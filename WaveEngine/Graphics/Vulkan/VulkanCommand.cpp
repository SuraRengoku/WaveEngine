#include "VulkanCommand.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

////////////////////////////////////////////// VULKAN COMMAND POOL /////////////////////////////////////////////

bool vulkanCommandPool::initialize(VkDevice device, u32 queueFamilyIndex, const void* next,  VkCommandPoolCreateFlags flags) {
    assert(_pool == VK_NULL_HANDLE && "::VULKAN:ERROR: Can not reinitialize the command pool\n");
    _device = device;
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.pNext = next;

    VKCall(vkCreateCommandPool(_device, &createInfo, nullptr, &_pool), "::VULKAN:ERROR Failed to create a command pool\n");
    return true;
}

bool vulkanCommandPool::initialize(const deviceContext& dCtx, const void* next, VkCommandPoolCreateFlags flags) {
    assert(_pool == VK_NULL_HANDLE && "::VULKAN:ERROR: Can not reinitialize the command pool\n");
    _device = dCtx._device;
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = next;
    createInfo.queueFamilyIndex = dCtx._graphicsQueue.familyIndex();
    createInfo.flags = flags;

    VKCall(vkCreateCommandPool(_device, &createInfo, dCtx._allocator, &_pool), "::VULKAN:ERROR Failed to create a command pool\n");
    return true;
}

VkResult vulkanCommandPool::allocateBuffers(UTL::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level) const {
    assert(_device != VK_NULL_HANDLE && _pool != VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _pool;
    allocateInfo.commandBufferCount = buffers.size();
    allocateInfo.level = level;
    if (VkResult result = vkAllocateCommandBuffers(_device, &allocateInfo, buffers.data())) {
        debug_error("::VULKAN:ERROR Failed to allocate command buffers\n");
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
        debug_error("::VULKAN:ERROR Failed to allocate command buffers\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanCommandPool::freeBuffers(UTL::vector<VkCommandBuffer>& buffers) const {
    if (buffers.empty()) {
        debug_error(":VULKAN:WARNING the container of command buffer is empty\n");
        return;
    }
    vkFreeCommandBuffers(_device, _pool, buffers.size(), buffers.data());
    memset(buffers.data(), 0, buffers.size() * sizeof(VkCommandBuffer));
}

void vulkanCommandPool::freeBuffers(UTL::vector<vulkanCommandBuffer>& buffers) const {
    if (buffers.empty()) {
        debug_error(":VULKAN:WARNING the container of command buffer is empty\n");
        return;
    }
    // TODO if we use ArrayRef we can just implicitly transfer vulkanCommandBuffer to VkCommandBuffer
    // TODO make sure vulkanCommandBuffer only has one element and no virtual functions
    static_assert(sizeof(vulkanCommandBuffer) == sizeof(VkCommandBuffer),
        "::VULKAN:ERROR vulkanCommandBuffer must have the same size as VkCommandBuffer\n");

    vkFreeCommandBuffers(_device, _pool, buffers.size(), reinterpret_cast<const VkCommandBuffer*>(buffers.data()));
    memset(buffers.data(), 0, buffers.size() * sizeof(vulkanCommandBuffer));
}

void vulkanCommandPool::release() {
    assert(_pool != VK_NULL_HANDLE);
    VK_DESTROY_PTR_BY(vkDestroyCommandPool, _device, _pool);
}

////////////////////////////////////////////// VULKAN COMMAND BUFFER /////////////////////////////////////////////

VkResult vulkanCommandBuffer::begin(VkCommandBufferUsageFlags usageFlags,
                                    const VkCommandBufferInheritanceInfo& inheritanceInfo) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    if (VkResult result = vkBeginCommandBuffer(_commandBuffer, &beginInfo)) {
        debug_error("::VULKAN:ERROR Failed to begin a command buffer\n");
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
        debug_error("::VULKAN:ERROR Failed to begin a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanCommandBuffer::reset() const {
    vkResetCommandBuffer(_commandBuffer, 0);
}

VkResult vulkanCommandBuffer::end() const {
    if (VkResult result = vkEndCommandBuffer(_commandBuffer)) {
        debug_error("::VULKAN:ERROR Failed to end a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}
}
