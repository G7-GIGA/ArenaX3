#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <optional>

namespace com::arenax3 {

class Vulkan_Swapchain {
public:
    struct SwapchainDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    struct SwapchainImages {
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> imageViews;
        vk::Format format;
        vk::Extent2D extent;
    };

    Vulkan_Swapchain() = default;
    ~Vulkan_Swapchain();

    Vulkan_Swapchain(const Vulkan_Swapchain&) = delete;
    Vulkan_Swapchain& operator=(const Vulkan_Swapchain&) = delete;
    Vulkan_Swapchain(Vulkan_Swapchain&& other) noexcept;
    Vulkan_Swapchain& operator=(Vulkan_Swapchain&& other) noexcept;

    void create(vk::PhysicalDevice physicalDevice, vk::Device device, 
                vk::SurfaceKHR surface, vk::Queue presentQueue,
                uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex,
                uint32_t width, uint32_t height, bool vsync = true);

    void recreate(uint32_t width, uint32_t height, bool vsync = true);

    void destroy();

    vk::SwapchainKHR get() const { return m_swapchain.get(); }
    vk::Format getImageFormat() const { return m_images.format; }
    vk::Extent2D getExtent() const { return m_images.extent; }
    const std::vector<vk::ImageView>& getImageViews() const { return m_images.imageViews; }
    size_t getImageCount() const { return m_images.images.size(); }

    vk::Result acquireNextImage(uint32_t& imageIndex, vk::Semaphore semaphore, 
                                 vk::Fence fence = nullptr);

    vk::Result present(uint32_t imageIndex, vk::Semaphore waitSemaphore = nullptr);

    bool isOutOfDate() const { return m_outOfDate; }

private:
    SwapchainDetails querySwapchainDetails(vk::PhysicalDevice device, vk::SurfaceKHR surface) const;

    vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const;
    vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& modes, bool vsync) const;
    vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const;

    void createImageViews();

    struct SwapchainDeleter {
        vk::Device device;
        vk::SwapchainKHR swapchain;
        void operator()(vk::SwapchainKHR* sw) const;
    };

    std::unique_ptr<vk::SwapchainKHR, SwapchainDeleter> m_swapchain;
    vk::Device m_device;
    vk::SurfaceKHR m_surface;
    vk::Queue m_presentQueue;
    SwapchainImages m_images;
    bool m_outOfDate = false;

    vk::PhysicalDevice m_physicalDevice;
    uint32_t m_graphicsQueueFamilyIndex;
    uint32_t m_presentQueueFamilyIndex;
};

} // namespace com::arenax3