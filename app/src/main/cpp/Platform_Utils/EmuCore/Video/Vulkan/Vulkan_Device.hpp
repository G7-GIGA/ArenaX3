#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace com::arenax3 {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    bool isCompleteCompute() const {
        return computeFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Vulkan_Device {
public:
    Vulkan_Device(VkInstance instance, VkSurfaceKHR surface, 
                  const std::vector<const char*>& requiredExtensions = {});
    ~Vulkan_Device();

    Vulkan_Device(const Vulkan_Device&) = delete;
    Vulkan_Device& operator=(const Vulkan_Device&) = delete;
    Vulkan_Device(Vulkan_Device&& other) noexcept;
    Vulkan_Device& operator=(Vulkan_Device&& other) noexcept;

    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice getLogicalDevice() const { return m_device; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkQueue getComputeQueue() const { return m_computeQueue; }
    VkQueue getTransferQueue() const { return m_transferQueue; }
    
    const QueueFamilyIndices& getQueueFamilyIndices() const { return m_queueFamilyIndices; }
    VkPhysicalDeviceProperties getProperties() const { return m_properties; }
    VkPhysicalDeviceFeatures getFeatures() const { return m_features; }
    VkPhysicalDeviceMemoryProperties getMemoryProperties() const { return m_memoryProperties; }

    bool isExtensionSupported(const std::string& extensionName) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    
    SwapChainSupportDetails getSwapChainSupportDetails() const;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
                                 VkImageTiling tiling, 
                                 VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;
    bool hasStencilComponent(VkFormat format) const;

    void waitIdle() const;

private:
    VkInstance m_instance;
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    
    QueueFamilyIndices m_queueFamilyIndices;
    VkPhysicalDeviceProperties m_properties{};
    VkPhysicalDeviceFeatures m_features{};
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};
    std::vector<std::string> m_supportedExtensions;

    void pickPhysicalDevice(const std::vector<const char*>& requiredExtensions);
    bool isDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    
    void createLogicalDevice(const std::vector<const char*>& requiredExtensions);
    void setupQueues();
};

} // namespace com::arenax3