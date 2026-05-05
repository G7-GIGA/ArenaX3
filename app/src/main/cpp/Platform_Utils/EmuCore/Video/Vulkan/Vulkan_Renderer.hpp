#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <optional>
#include <array>
#include <unordered_map>

namespace com::arenax3 {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    bool isComputeComplete() const {
        return computeFamily.has_value();
    }

    bool isTransferComplete() const {
        return transferFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[3].offset = offsetof(Vertex, normal);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 lightPosition;
    glm::vec4 cameraPosition;
    float time;
};

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    void init(GLFWwindow* window);
    void cleanup();
    void recreateSwapChain();
    void waitForDevice();

    void beginFrame();
    void endFrame();
    void drawFrame();

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, 
                      vk::MemoryPropertyFlags properties, vk::Buffer& buffer, 
                      vk::DeviceMemory& bufferMemory);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, 
                           uint32_t height, uint32_t layerCount);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, 
                     vk::SampleCountFlagBits numSamples, vk::Format format, 
                     vk::ImageTiling tiling, vk::ImageUsageFlags usage, 
                     vk::MemoryPropertyFlags properties, vk::Image& image, 
                     vk::DeviceMemory& imageMemory);
    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

    vk::ImageView createImageView(vk::Image image, vk::Format format, 
                                  vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
    vk::ShaderModule createShaderModule(const std::vector<char>& code);

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);

    // Getters
    vk::Device getDevice() const { return m_device; }
    vk::PhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    vk::CommandPool getCommandPool() const { return m_commandPool; }
    vk::Queue getGraphicsQueue() const { return m_graphicsQueue; }
    vk::Queue getPresentQueue() const { return m_presentQueue; }
    vk::Queue getComputeQueue() const { return m_computeQueue; }
    vk::SwapchainKHR getSwapChain() const { return m_swapChain; }
    vk::Extent2D getSwapChainExtent() const { return m_swapChainExtent; }
    vk::Format getSwapChainImageFormat() const { return m_swapChainImageFormat; }
    vk::RenderPass getRenderPass() const { return m_renderPass; }
    vk::SampleCountFlagBits getMsaaSamples() const { return m_msaaSamples; }

    void updateUniformBuffer(uint32_t currentImage, const UniformBufferObject& ubo);
    void setViewportScissor(vk::CommandBuffer commandBuffer, vk::Extent2D extent);

private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createColorResources();
    void createDepthResources();
    void createTextureSampler();
    void loadModels();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    bool isDeviceSuitable(vk::PhysicalDevice device);
    vk::SampleCountFlagBits getMaxUsableSampleCount();

    GLFWwindow* m_window;

    vk::Instance m_instance;
    vk::DebugUtilsMessengerEXT m_debugMessenger;
    vk::SurfaceKHR m_surface;

    vk::PhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    vk::Device m_device;

    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;
    vk::Queue m_computeQueue;
    vk::Queue m_transferQueue;

    vk::SwapchainKHR m_swapChain;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::ImageView> m_swapChainImageViews;

    vk::RenderPass m_renderPass;
    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;

    std::vector<vk::Framebuffer> m_swapChainFramebuffers;

    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;

    vk::Image m_colorImage;
    vk::DeviceMemory m_colorImageMemory;
    vk::ImageView m_colorImageView;

    vk::Image m_depthImage;
    vk::DeviceMemory m_depthImageMemory;
    vk::ImageView m_depthImageView;
    vk::Format m_depthFormat;

    vk::SampleCountFlagBits m_msaaSamples;

    vk::Buffer m_vertexBuffer;
    vk::DeviceMemory m_vertexBufferMemory;
    vk::Buffer m_indexBuffer;
    vk::DeviceMemory m_indexBufferMemory;

    std::vector<vk::Buffer> m_uniformBuffers;
    std::vector<vk::DeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    vk::Image m_textureImage;
    vk::DeviceMemory m_textureImageMemory;
    vk::ImageView m_textureImageView;
    vk::Sampler m_textureSampler;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;
    std::vector<vk::Fence> m_imagesInFlight;
    uint32_t m_currentFrame = 0;

    bool m_framebufferResized = false;

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
};

} // namespace com::arenax3

#endif // VULKAN_RENDERER_HPP