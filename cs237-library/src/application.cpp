/*! \file application.cpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237/cs237.hpp"
#include <cstring>
#include <cstdlib>
#include <set>
#include <vector>

namespace cs237 {

static std::vector<const char *> requiredExtensions (bool debug);
static int graphicsQueueIndex (vk::PhysicalDevice dev);

// callback for debug messages
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback (
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT ty,
    const vk::DebugUtilsMessengerCallbackDataEXT* cbData,
    void* usrData);

const std::vector<const char *> kValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };


/******************** class Application methods ********************/

Application::Application (std::vector<std::string> const &args, std::string const &name)
  : _name(name),
    _messages(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning),
    _debug(0),
    _gpu(nullptr),
    _propsCache(nullptr),
    _featuresCache(nullptr)
{
    // process the command-line arguments
    for (auto it : args) {
        if (it == "-debug") {
            this->_messages |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
            this->_debug = true;
        } else if (it == "-verbose") {
            this->_messages = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
        }
    }

    // initialize GLFW
    glfwInit();

    // create a Vulkan instance
    this->_createInstance();

    // set up the debug handler
    if (this->_debug) {
        this->_initDebug ();
    }

    // initialize the command pool
    this->_initCommandPool();
}

Application::~Application ()
{
    if (this->_propsCache != nullptr) {
        delete this->_propsCache;
    }
    if (this->_featuresCache != nullptr) {
        delete this->_featuresCache;
    }

    // delete the command pool
    this->_device.destroyCommandPool(this->_cmdPool);

    // destroy the logical device
    this->_device.destroy();

    if (this->_debug) {
        this->_cleanupDebug();
    }

    // delete the instance
    this->_instance.destroy();

    // shut down GLFW
    glfwTerminate();

}

// create a Vulkan instance
void Application::_createInstance ()
{
    // set up the application information
    vk::ApplicationInfo appInfo(
        this->_name.c_str(), /* application name */
        VK_MAKE_VERSION(1, 0, 0), /* application version */
        nullptr, /* engine name */
        0, /* engine version */
        VK_API_VERSION_1_3); /* API version */

    // figure out what extensions we are going to need
    auto extensions = requiredExtensions(this->_debug);

    // intialize the creation info struct
    vk::InstanceCreateInfo createInfo(
        vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        &appInfo,
        (this->_debug
            // in debug mode, we add validation layer(s)
            ? kValidationLayers
            : vk::ArrayProxyNoTemporaries<const char * const>()),
        extensions);

    this->_instance = vk::createInstance(createInfo);

    // pick the physical device; we require fillModeNonSolid to support
    // wireframes and samplerAnisotropy for texture mapping
    vk::PhysicalDeviceFeatures reqs{};
    reqs.fillModeNonSolid = VK_TRUE;
    reqs.samplerAnisotropy = VK_TRUE;
    this->_selectDevice (&reqs);

    // create the logical device and get the queues
    this->_createLogicalDevice ();
}

// check that a device meets the requested features
//
static bool hasFeatures (vk::PhysicalDevice gpu, vk::PhysicalDeviceFeatures *reqFeatures)
{
    if (reqFeatures == nullptr) {
        return true;
    }
    vk::PhysicalDeviceFeatures availFeatures = gpu.getFeatures();

    if (reqFeatures->depthClamp == availFeatures.depthClamp) {
        return true;
    }
    else if (reqFeatures->fillModeNonSolid == availFeatures.fillModeNonSolid) {
        return true;
    }
    else if (reqFeatures->samplerAnisotropy == availFeatures.samplerAnisotropy) {
        return true;
    }
    else {
        return false;
    }
}

// A helper function to pick the physical device when there is more than one.
// Currently, we ignore the features and favor discrete GPUs over other kinds
//
void Application::_selectDevice (vk::PhysicalDeviceFeatures *reqFeatures)
{
    // get the available devices
    auto devices = this->_instance.enumeratePhysicalDevices();

    if (devices.empty()) {
        ERROR("no available GPUs");
    }

    // Select a device that supports graphics and presentation
    // This code is brute force, but we only expect one or two devices.
    // Future versions will support checking for properties.

// FIXME: we should also check that the device supports swap chains!!!!

    // we first look for a discrete GPU
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            vk::PhysicalDeviceProperties props = dev.getProperties();
            if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                this->_gpu = dev;
                return;
            }
        }
    }
    // no discrete GPU, so look for an integrated GPU
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            vk::PhysicalDeviceProperties props = dev.getProperties();
            if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
                this->_gpu = dev;
                return;
            }
        }
    }
    // check for any device that supports graphics and presentation
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            this->_gpu = dev;
            return;
        }
    }

    ERROR("no available GPUs that support graphics");

}

// helper to set the properties cache variable
void Application::_getPhysicalDeviceProperties () const
{
    assert (this->_propsCache == nullptr);

    this->_propsCache = new vk::PhysicalDeviceProperties(this->_gpu.getProperties());
}

void Application::_getPhysicalDeviceFeatures () const
{
    assert (this->_featuresCache == nullptr);

    this->_featuresCache = new vk::PhysicalDeviceFeatures;
    this->_gpu.getFeatures (this->_featuresCache);
}

int32_t Application::_findMemory (
    uint32_t reqTypeBits,
    vk::MemoryPropertyFlags reqProps) const
{
    // get the memory properties for the device
    vk::PhysicalDeviceMemoryProperties memProps = this->_gpu.getMemoryProperties();

    for (int32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((reqTypeBits & (1 << i))
        && (memProps.memoryTypes[i].propertyFlags & reqProps) == reqProps)
        {
            return i;
        }
    }

    return -1;

}

vk::Format Application::_findBestFormat (
    std::vector<vk::Format> candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features)
{
    for (vk::Format fmt : candidates) {
        vk::FormatProperties props = this->_gpu.getFormatProperties(fmt);
        if (tiling == vk::ImageTiling::eLinear) {
            if ((props.linearTilingFeatures & features) == features) {
                return fmt;
            }
        } else { // vk::ImageTiling::eOptimal
            if ((props.optimalTilingFeatures & features) == features) {
                return fmt;
            }
        }
    }
    return vk::Format::eUndefined;
}

vk::Format Application::_depthStencilBufferFormat (bool depth, bool stencil)
{
    if (!depth && !stencil) {
        return vk::Format::eUndefined;
    }

    // construct a list of valid candidate formats in best-to-worst order
    std::vector<vk::Format> candidates;
    if (!depth) {
        candidates.push_back(vk::Format::eS8Uint);              // 8-bit stencil; no depth
    }
    if (!stencil) {
        candidates.push_back(vk::Format::eD32SfloatS8Uint);     // 32-bit depth; no stencil
    }
    candidates.push_back(vk::Format::eD32SfloatS8Uint);         // 32-bit depth + 8-bit stencil
    if (!stencil) {
        candidates.push_back(vk::Format::eX8D24UnormPack32);    // 24-bit depth; no stencil
        candidates.push_back(vk::Format::eD16UnormS8Uint);      // 16-bit depth; no stencil
    }
    candidates.push_back(vk::Format::eD16UnormS8Uint);          // 16-bit depth + 8-bit stencil

    return this->_findBestFormat(
        candidates,
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);

}

//! helper function to check if named extension is in a vector of extension properties
static bool extInList (const char *name, std::vector<vk::ExtensionProperties> const &props)
{
    for (auto it = props.cbegin();  it != props.cend();  ++it) {
        if (strcmp(it->extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

void Application::_createLogicalDevice ()
{
    // set up the device queues info struct; the graphics and presentation queues may
    // be different or the same, so we have to initialize either one or two create-info
    // structures
    std::vector<vk::DeviceQueueCreateInfo> qCreateInfos;
    std::set<uint32_t> uniqueQIndices = {
            this->_qIdxs.graphics,
            this->_qIdxs.compute,
            this->_qIdxs.present
        };

    float qPriority = 1.0f;
    for (auto qix : uniqueQIndices) {
        vk::DeviceQueueCreateInfo qCreateInfo(
            {}, /* flags */
            qix, /* queue-family index */
            1, /* queue count */
            &qPriority); /* queue priority */
        qCreateInfos.push_back(qCreateInfo);
    }

    // get the extensions that are supported by the device
    auto supportedExts = this->supportedDeviceExtensions();

    // set up the extension vector to have swap chains and portability subset (if
    // supported)
    std::vector<const char*> kDeviceExts;
    if (extInList(VK_KHR_SWAPCHAIN_EXTENSION_NAME, supportedExts)) {
        kDeviceExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    else {
        ERROR("required " VK_KHR_SWAPCHAIN_EXTENSION_NAME " extension is not supported");
    }
    if (extInList("VK_KHR_portability_subset", supportedExts)) {
        kDeviceExts.push_back("VK_KHR_portability_subset");
    }

    // for now, we are only enabling a couple of extra features
    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // initialize the create info
    vk::DeviceCreateInfo createInfo(
        {}, /* flags */
        qCreateInfos,
        (this->_debug // validation layers (if in debug mode)
            ? kValidationLayers
            : vk::ArrayProxyNoTemporaries<const char * const>()),
        kDeviceExts, /* enabled extension names */
        &deviceFeatures); /* enabled device features */

    // create the logical device
    this->_device = this->_gpu.createDevice(createInfo);

    // get the queues
    this->_queues.graphics = this->_device.getQueue(this->_qIdxs.graphics, 0);
    this->_queues.present = this->_device.getQueue(this->_qIdxs.present, 0);
    this->_queues.compute = this->_device.getQueue(this->_qIdxs.compute, 0);

}

// create a Vulkan image; used for textures, depth buffers, etc.
vk::Image Application::_createImage (
    uint32_t wid,
    uint32_t ht,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::ImageLayout layout,
    uint32_t mipLvls)
{
    vk::ImageCreateInfo imageInfo(
        {}, /* flags */
        vk::ImageType::e2D,
        format,
        { wid, ht, 1 }, /* extend: wid, ht, depth */
        mipLvls, /* mip levels */
        1, /* array layers */
        vk::SampleCountFlagBits::e1, /* samples */
        tiling,
        usage,
        vk::SharingMode::eExclusive, /* sharing mode */
        {},
        layout);

    return this->_device.createImage(imageInfo);
}

vk::DeviceMemory Application::_allocImageMemory (vk::Image img, vk::MemoryPropertyFlags props)
{
    auto memRequirements = this->_device.getImageMemoryRequirements(img);

    vk::MemoryAllocateInfo allocInfo(
        memRequirements.size,
        this->_findMemory(memRequirements.memoryTypeBits, props));

    vk::DeviceMemory mem = this->_device.allocateMemory(allocInfo);

    this->_device.bindImageMemory(img, mem, 0);

    return mem;
}

vk::ImageView Application::_createImageView (
    vk::Image img,
    vk::Format fmt,
    vk::ImageAspectFlags aspectFlags)
{
    assert (img);

    vk::ImageViewCreateInfo viewInfo(
        {}, /* flags */
        img,
        vk::ImageViewType::e2D,
        fmt,
        {}, /* component mapping */
        { aspectFlags, 0, 1, 0, 1 });

    return this->_device.createImageView(viewInfo);

}

vk::Buffer Application::_createBuffer (size_t size, vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo bufferInfo(
        {}, /* flags */
        size,
        usage,
        vk::SharingMode::eExclusive,
        {});

    return this->_device.createBuffer(bufferInfo);
}

vk::DeviceMemory Application::_allocBufferMemory (vk::Buffer buf, vk::MemoryPropertyFlags props)
{
    auto memRequirements = this->_device.getBufferMemoryRequirements(buf);

    vk::MemoryAllocateInfo allocInfo(
        memRequirements.size,
        this->_findMemory(memRequirements.memoryTypeBits, props));

    vk::DeviceMemory mem = this->_device.allocateMemory(allocInfo);

    this->_device.bindBufferMemory(buf, mem, 0);

    return mem;

}

void Application::_transitionImageLayout (
    vk::Image image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout)
{
    vk::CommandBuffer cmdBuf = this->newCommandBuf();

    this->beginCommands(cmdBuf);

    vk::ImageMemoryBarrier barrier(
        {}, /* src access mask */
        {}, /* dst access mask */
        oldLayout,
        newLayout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;

    if ((oldLayout == vk::ImageLayout::eUndefined)
    && (newLayout == vk::ImageLayout::eTransferDstOptimal)) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if ((oldLayout == vk::ImageLayout::eTransferDstOptimal)
    && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        srcStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else {
        ERROR("unsupported layout transition!");
    }

    cmdBuf.pipelineBarrier(
        srcStage, dstStage,
        {},
        {},
        {},
        {barrier});

    this->endCommands(cmdBuf);
    this->submitCommands(cmdBuf);
    this->freeCommandBuf(cmdBuf);

}

void Application::_copyBuffer (
    vk::Buffer srcBuf, vk::Buffer dstBuf,
    size_t offset, size_t size)
{
    vk::CommandBuffer cmdBuf = this->newCommandBuf();

    this->beginCommands(cmdBuf);

    vk::BufferCopy copyRegion(0, offset, size); /* args: src offset, dst offset, size */
    cmdBuf.copyBuffer(srcBuf, dstBuf, {copyRegion});

    this->endCommands(cmdBuf);
    this->submitCommands(cmdBuf);
    this->freeCommandBuf(cmdBuf);

}

void Application::_copyBufferToImage (
        vk::Image dstImg, vk::Buffer srcBuf, size_t size,
        uint32_t wid, uint32_t ht, uint32_t depth)
{
    vk::CommandBuffer cmdBuf = this->newCommandBuf();

    this->beginCommands(cmdBuf);

    vk::BufferImageCopy region(
        0, /* offset */
        0, /* row length */
        0, /* image height */
        { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
        { 0, 0, 0 },
        { wid, ht, depth });

    cmdBuf.copyBufferToImage(
        srcBuf, dstImg,
        vk::ImageLayout::eTransferDstOptimal,
        {region});

    this->endCommands(cmdBuf);
    this->submitCommands(cmdBuf);
    this->freeCommandBuf(cmdBuf);

}

void Application::_initCommandPool ()
{
    vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        this->_qIdxs.graphics);

    this->_cmdPool = this->_device.createCommandPool(poolInfo);
}


vk::Sampler Application::createSampler (Application::SamplerInfo const &info)
{
    vk::SamplerCreateInfo samplerInfo(
        {}, /* flags */
        info.magFilter,
        info.minFilter,
        info.mipmapMode,
        info.addressModeU,
        info.addressModeV,
        info.addressModeW,
        0.0, /* mip LOD bias */
        VK_TRUE, /* anisotropy enable */
        this->limits()->maxSamplerAnisotropy,
        VK_FALSE, /* compare enable */
        vk::CompareOp::eNever, /* compare op */
        0, /* min LOD */
        0, /* max LOD */
        info.borderColor, /* borderColor */
        VK_FALSE); /* unnormalized coordinates */

    return this->_device.createSampler(samplerInfo);
}

vk::Sampler Application::createDepthSampler (SamplerInfo const &info)
{
    vk::SamplerCreateInfo samplerInfo(
        {}, /* flags */
        info.magFilter,
        info.minFilter,
        info.mipmapMode,
        info.addressModeU,
        info.addressModeV,
        info.addressModeW,
        0.0, /* mip LOD bias */
        VK_TRUE, /* anisotropy enable */
        this->limits()->maxSamplerAnisotropy,
/* FIXME: need VkPhysicalDevicePortabilitySubsetFeaturesKHR::mutableComparisonSamplers
        VK_TRUE, vk::CompareOp::eLessOrEqual,
*/
        VK_FALSE, /* compare enable */
        vk::CompareOp::eAlways, /* compare op */
        0, /* min LOD */
        0, /* max LOD */
        info.borderColor, /* borderColor */
        VK_FALSE); /* unnormalized coordinates */

    return this->_device.createSampler(samplerInfo);
}

vk::Pipeline Application::createPipeline (
    cs237::Shaders *shaders,
    vk::PipelineVertexInputStateCreateInfo const &vertexInfo,
    vk::PrimitiveTopology prim,
    bool primRestart,
    vk::ArrayProxy<vk::Viewport> const &viewports,
    vk::ArrayProxy<vk::Rect2D> const &scissors,
    bool depthClamp,
    vk::PolygonMode polyMode,
    vk::CullModeFlags cullMode,
    vk::FrontFace front,
    vk::PipelineLayout layout,
    vk::RenderPass renderPass,
    uint32_t subPass,
    vk::ArrayProxy<vk::DynamicState> const &dynamic)
{
    vk::PipelineInputAssemblyStateCreateInfo asmInfo(
        {}, /* flags */
        prim, /* topology */
        primRestart ? VK_TRUE : VK_FALSE); /* primitive restart */

    vk::PipelineViewportStateCreateInfo viewportState(
        {}, /* flags */
        viewports, /* viewport */
        scissors); /* scissor rects */

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        depthClamp ? VK_TRUE : VK_FALSE, /* depth clamp */
        VK_FALSE, /* rasterizer discard */
        polyMode, /* polygon mode */
        cullMode, /* cull mode */
        front, /* front face orientation */
        VK_FALSE, /* depth bias enable */
        0.0f, 0.0f, 0.0f, /* depth bias: constant, clamp, and slope */
        1.0f); /* line width */

    // no multisampling, which is the default
    vk::PipelineMultisampleStateCreateInfo multisampling{};

    vk::PipelineDepthStencilStateCreateInfo depthStencil(
        {}, /* flags */
        VK_TRUE, /* depth-test enable */
        VK_TRUE, /* depth-write enable */
        vk::CompareOp::eLess, /* depth-compare operation */
        VK_FALSE, /* bounds-test enable */
        VK_FALSE); /* stencil-test enable */
        /* defaults for remaining fields */

    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        VK_FALSE, /* blend enable */
        vk::BlendFactor::eZero, vk::BlendFactor::eZero,
        vk::BlendOp::eAdd, /* color blend op */
        vk::BlendFactor::eZero, vk::BlendFactor::eZero,
        vk::BlendOp::eAdd, /* alpha blend op */
        vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags); /* color write mask */

    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {}, /* flags */
        VK_FALSE, /* logic-op enable */
        vk::LogicOp::eClear, /* logic op */
        colorBlendAttachment, /* attachments */
        { 0.0f, 0.0f, 0.0f, 0.0f }); /* blend constants */

    vk::PipelineDynamicStateCreateInfo dynamicState(
        {}, /* flags */
        dynamic);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {}, /* flags */
        shaders->stages(), /* stages */
        &vertexInfo, /* vertex-input state */
        &asmInfo, /* input-assembly state */
        {}, /* tesselation state */
        &viewportState, /* viewport state */
        &rasterizer, /* rasterization state */
        &multisampling, /* multisample state */
        &depthStencil, /* depth-stencil state */
        &colorBlending, /* color-blend state */
        &dynamicState, /* dynamic state */
        layout, /* layout */
        renderPass, /* render pass */
        0, /* subpass */
        nullptr); /* base pipeline */

    // create the pipeline
    auto pipes = this->device().createGraphicsPipelines(
        nullptr,
        pipelineInfo);
    if (pipes.result != vk::Result::eSuccess) {
        ERROR("unable to create graphics pipeline!");
    }
    return pipes.value[0];
}

/* TODO: define a ComputeShader class, since compute shaders
 * only have one stage and get used in different contexts.
 */
vk::Pipeline Application::createComputePipeline (
    vk::PipelineLayout layout,
    cs237::Shaders *shaders)
{
    vk::ComputePipelineCreateInfo pipelineInfo(
        {}, /* flags */
        shaders->stages()[0], /* stages */
        layout, /* layout */
        nullptr, /* base pipeline */
        -1); /* base pipeline index */

    // create the pipeline
    auto pipes = this->device().createComputePipelines(
        nullptr,
        pipelineInfo);
    if (pipes.result != vk::Result::eSuccess) {
        ERROR("unable to create compute pipeline!");
    }
    return pipes.value[0];
}

/******************** local utility functions ********************/

// A helper function for determining the extensions that are required
// when creating an instance. These include the extensions required
// by GLFW and the extensions required for debugging support when
// `debug` is true.
//
static std::vector<const char *> requiredExtensions (bool debug)
{
    uint32_t extCount;

    // extensions required by GLFW
    const char **glfwReqExts = glfwGetRequiredInstanceExtensions(&extCount);

    // in debug mode we need the debug utilities
    uint32_t debugExtCount = debug ? 1 : 0;

    // initialize the vector of extensions with the GLFW extensions
    std::vector<const char *> reqExts (glfwReqExts, glfwReqExts+extCount);

    reqExts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    reqExts.push_back("VK_KHR_get_physical_device_properties2");

    // add debug extensions
    if (debug) {
        reqExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return reqExts;
}

// check the device's queue families for graphics and presentation support
//
bool Application::_getQIndices (vk::PhysicalDevice dev)
{
    // get the queue family info
    auto qFamilies = dev.getQueueFamilyProperties();

    Application::Queues<int32_t> indices = { -1, -1, -1 };
    for (int i = 0;  i < qFamilies.size();  ++i) {
        // check for graphics support
        if ((indices.graphics < 0)
        && (qFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
            indices.graphics = i;
        }
        // check for presentation support
        if (indices.present < 0) {
            if (glfwGetPhysicalDevicePresentationSupport(this->_instance, dev, i)) {
                indices.present = i;
            }
        }
        // Check for compute support
        if (indices.compute < 0
        && (qFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)) {
            indices.compute = i;
        }
        // check if we are finished
        if ((indices.graphics >= 0) && (indices.present >= 0)) {
            this->_qIdxs.graphics = static_cast<uint32_t>(indices.graphics);
            this->_qIdxs.present = static_cast<uint32_t>(indices.present);
            this->_qIdxs.compute = static_cast<uint32_t>(indices.compute);
            return true;
        }
    }

    return false;

}

/***** Debug callback support *****/

constexpr int kMaxErrorCount = 20;
constexpr int kMaxWarningCount = 50;

static int errorCount = 0;
static int warningCount = 0;

// callback for debug messages
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback (
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT ty,
    const VkDebugUtilsMessengerCallbackDataEXT* cbData,
    void* usrData)
{
    std::cerr << "# " << cbData->pMessage << std::endl;

    // check to see if we should terminate
    if (severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        if (++errorCount > kMaxErrorCount) {
            ERROR("too many validation errors");
        }
    } else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        if (++warningCount > kMaxErrorCount) {
            ERROR("too many validation warnings");
        }
    }

    return VK_FALSE;
}

// set up the debug callback
void Application::_initDebug ()
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr (
        this->_instance,
        "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        VkDebugUtilsMessengerCreateInfoEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = debugCallback;

        VkDebugUtilsMessengerEXT debugMessenger;
        auto sts = func (this->_instance, &info, nullptr, &this->_debugMessenger);
        if (sts != VK_SUCCESS) {
            ERROR("unable to set up debug messenger!");
        }
    } else {
        ERROR("unable to get vkCreateDebugUtilsMessengerEXT address");
    }

}

// clean up the debug callback
void Application::_cleanupDebug ()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        this->_instance,
        "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func (this->_instance, this->_debugMessenger, nullptr);
    }

}

} // namespace cs237
