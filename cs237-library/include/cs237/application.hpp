/*! \file application.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_APPLICATION_HPP_
#define _CS237_APPLICATION_HPP_

#ifndef _CS237_HPP_
#error "cs237/application.hpp should not be included directly"
#endif

namespace cs237 {

namespace __detail { class TextureBase; }

/// the base class for applications
class Application {

friend class Window;
friend class Buffer;
friend class MemoryObj;
friend class __detail::TextureBase;
friend class Texture1D;
friend class Texture2D;
friend class DepthBuffer;
friend class Attachment;

public:

    /// \brief constructor for application base class
    /// \param args     vector of the command-line arguments
    /// \param name     optional name of the application
    Application (std::vector<std::string> const &args, std::string const &name = "CS237 App");

    virtual ~Application ();

    /// main function for running the application
    virtual void run () = 0;

    /// \brief return the application name
    std::string name () const { return this->_name; }

    /// \brief is the program in debug mode?
    bool debug () const { return this->_debug; }
    /// \brief is the program in verbose mode?
    bool verbose () const
    {
        return (this->_messages & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
            == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
    }

    /// \brief Get the list of supported Vulkan instance extensions
    /// \return The vector of vk::ExtensionProperties for the supported extensions
    static std::vector<vk::ExtensionProperties> supportedExtensions ()
    {
        return vk::enumerateInstanceExtensionProperties(nullptr);
    }

    /// \brief Get the list of supported Vulkan device extensions for the selected
    ///        physical device.
    /// \return The vector of vk::ExtensionProperties for the supported extensions
    std::vector<vk::ExtensionProperties> supportedDeviceExtensions ()
    {
        return this->_gpu.enumerateDeviceExtensionProperties();
    }

    /// \brief Get the list of supported layers
    /// \return The vector of vk::LayerProperties for the supported layers
    static std::vector<vk::LayerProperties> supportedLayers ()
    {
        return vk::enumerateInstanceLayerProperties();
    }

    /// Information for creating a sampler object.  This is more limited than Vulkan's
    /// vk::SamplerCreateInfo structure, but should cover the common cases used in this
    /// class.
    struct SamplerInfo {
        vk::Filter magFilter;
        vk::Filter minFilter;
        vk::SamplerMipmapMode mipmapMode;
        vk::SamplerAddressMode addressModeU;
        vk::SamplerAddressMode addressModeV;
        vk::SamplerAddressMode addressModeW;
        vk::BorderColor borderColor;

        SamplerInfo ()
          : magFilter(vk::Filter::eLinear), minFilter(vk::Filter::eLinear),
            mipmapMode(vk::SamplerMipmapMode::eLinear),
            addressModeU(vk::SamplerAddressMode::eRepeat),
            addressModeV(vk::SamplerAddressMode::eRepeat),
            addressModeW(vk::SamplerAddressMode::eRepeat),
            borderColor(vk::BorderColor::eIntOpaqueBlack)
        { }

        /// sampler info for 1D texture
        SamplerInfo (
            vk::Filter magF, vk::Filter minF, vk::SamplerMipmapMode mm,
            vk::SamplerAddressMode am, vk::BorderColor color)
          : magFilter(magF), minFilter(minF), mipmapMode(mm),
            addressModeU(am), addressModeV(vk::SamplerAddressMode::eRepeat),
            addressModeW(vk::SamplerAddressMode::eRepeat), borderColor(color)
        { }

        /// sampler info for 2D texture
        SamplerInfo (
            vk::Filter magF, vk::Filter minF, vk::SamplerMipmapMode mm,
            vk::SamplerAddressMode am1, vk::SamplerAddressMode am2,
            vk::BorderColor color)
          : magFilter(magF), minFilter(minF), mipmapMode(mm),
            addressModeU(am1), addressModeV(am2),
            addressModeW(vk::SamplerAddressMode::eRepeat), borderColor(color)
        { }

    };

    /// \brief Create a texture sampler as specified
    /// \param info  a simplified sampler specification
    /// \return the created sampler
    vk::Sampler createSampler (SamplerInfo const &info);

    /// \brief Create a depth-texture sampler as specified
    /// \param info  a simplified sampler specification
    /// \return the created depth-texture sampler
    vk::Sampler createDepthSampler (SamplerInfo const &info);

    /// \brief get the logical device
    vk::Device device () const { return this->_device; }

    /// get the physical-device properties pointer
    const vk::PhysicalDeviceProperties *props () const
    {
        if (this->_propsCache == nullptr) {
            this->_getPhysicalDeviceProperties();
        }
        return this->_propsCache;
    }

    /// \brief access function for the physical device limits
    const vk::PhysicalDeviceLimits *limits () const { return &this->props()->limits; }

    /// \brief access function for the physical device features
    const vk::PhysicalDeviceFeatures *features () const
    {
        if (this->_featuresCache == nullptr) {
            this->_getPhysicalDeviceFeatures();
        }
        return this->_featuresCache;
    }

    /// \brief access function for the properties of an image format
    vk::FormatProperties formatProps (vk::Format fmt) const
    {
        return this->_gpu.getFormatProperties(fmt);
    }

    /// \brief Create a pipeline layout
    /// \param descSets the descriptor sets for the pipeline
    /// \param pcrs the push-constant ranged for the pipeline
    /// \return the created pipeline layout object
    ///
    /// Note that the Vulkan specification recommends using multiple
    /// descriptor sets when you have uniforms that are updated at different
    /// frequencies (e.g., per-scene vs. per-object).  It also recommends
    /// putting the least frequently changing descriptor sets at the
    /// beginning.
    vk::PipelineLayout createPipelineLayout (
        vk::ArrayProxy<vk::DescriptorSetLayout> descSets,
        vk::ArrayProxy<vk::PushConstantRange> pcrs)
    {
        vk::PipelineLayoutCreateInfo layoutInfo(
            {}, /* flags */
            descSets, /* set layouts */
            pcrs); /* push-constant ranges */

        return this->_device.createPipelineLayout(layoutInfo);
    }

    /// \brief Create a pipeline layout for a single descriptor set
    /// \param descSet  the single descriptor sets for the pipeline
    /// \return the created pipeline layout object
    vk::PipelineLayout createPipelineLayout (vk::DescriptorSetLayout descSet)
    {
        vk::PipelineLayoutCreateInfo layoutInfo(
            {}, /* flags */
            descSet, /* set layouts */
            {}); /* push-constant ranges */

        return this->_device.createPipelineLayout(layoutInfo);
    }

    /// \brief Allocate a graphics pipeline with blending support
    /// \param shaders     shaders for the pipeline
    /// \param vertexInfo  vertex info
    /// \param prim        primitive topology
    /// \param primRestart true if primitive restart should be enabled
    /// \param viewports   vector of viewports; ignored if the viewport state is dynamic
    /// \param scissors    vector of scissor rectangles; ignored if the scissor state is dynamic
    /// \param depthClamp  true if depth clamping is enabled
    /// \param polyMode    polygon mode
    /// \param cullMode    primitive culling mode
    /// \param front       the winding order that defines the front face of a triangle
    /// \param layout      the pipeline layout
    /// \param renderPass  a render pass that is compatible with the render pass to be used
    /// \param subPass     the index of the subpass in the render pass where the pipeline
    ///                    will be used
    /// \param blending    color-blending information
    /// \param dynamic     vector that specifies which parts of the pipeline can be
    ///                    dynamically set during the
    /// \return the created pipeline
    ///
    /// This function creates a pipeline with the following properties:
    ///
    ///   - rasterization discard is disabled
    ///   - depth bias is disabled
    ///   - no multisampling
    ///   - depth-test is enabled
    ///   - depth-write is enabled
    ///   - `LESS` is the depth-compare operation
    ///   - bounds-test is disabled
    ///   - stencil-test is disabled
    vk::Pipeline createPipeline (
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
        vk::PipelineColorBlendStateCreateInfo const &blending,
        vk::ArrayProxy<vk::DynamicState> const &dynamic);

    /// \brief Allocate a graphics pipeline
    /// \param shaders     shaders for the pipeline
    /// \param vertexInfo  vertex info
    /// \param prim        primitive topology
    /// \param primRestart true if primitive restart should be enabled
    /// \param viewports   vector of viewports; ignored if the viewport state is dynamic
    /// \param scissors    vector of scissor rectangles; ignored if the scissor state is dynamic
    /// \param depthClamp  true if depth clamping is enabled
    /// \param polyMode    polygon mode
    /// \param cullMode    primitive culling mode
    /// \param front       the winding order that defines the front face of a triangle
    /// \param layout      the pipeline layout
    /// \param renderPass  a render pass that is compatible with the render pass to be used
    /// \param subPass     the index of the subpass in the render pass where the pipeline
    ///                    will be used
    /// \param dynamic     vector that specifies which parts of the pipeline can be
    ///                    dynamically set during the
    /// \return the created pipeline
    ///
    /// This function creates a pipeline with the following properties:
    ///
    ///   - rasterization discard is disabled
    ///   - depth bias is disabled
    ///   - no multisampling
    ///   - depth-test is enabled
    ///   - depth-write is enabled
    ///   - `LESS` is the depth-compare operation
    ///   - bounds-test is disabled
    ///   - stencil-test is disabled
    ///   - color blending is disabled
    ///   - blending logic-op is disabled
    vk::Pipeline createPipeline (
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
        vk::ArrayProxy<vk::DynamicState> const &dynamic);

    /// \brief Allocate a graphics pipeline using common defaults
    /// \param shaders     shaders for the pipeline
    /// \param vertexInfo  vertex info
    /// \param prim        primitive topology
    /// \param viewports   vector of viewports; ignored if the viewport state is dynamic
    /// \param scissors    vector of scissor rectangles; ignored if the scissor state is dynamic
    /// \param polyMode    polygon mode
    /// \param cullMode    primitive culling mode
    /// \param front       the winding order that defines the front face of a triangle
    /// \param layout      the pipeline layout
    /// \param renderPass  a render pass that is compatible with the render pass to be used
    /// \param subPass     the index of the subpass in the render pass where the pipeline
    ///                    will be used
    /// \param dynamic     vector that specifies which parts of the pipeline can be
    ///                    dynamically set during the
    /// \return the created pipeline
    ///
    /// This function creates a pipeline with the following properties:
    ///
    ///   - primitive restart is disabled
    ///   - depth clamping is disabled
    ///   - rasterization discard is disabled
    ///   - depth bias is disabled
    ///   - no multisampling
    ///   - depth-test is enabled
    ///   - depth-write is enabled
    ///   - `LESS` is the depth-compare operation
    ///   - bounds-test is disabled
    ///   - stencil-test is disabled
    ///   - color blending is disabled
    ///   - blending logic-op is disabled
    vk::Pipeline createPipeline (
        cs237::Shaders *shaders,
        vk::PipelineVertexInputStateCreateInfo const &vertexInfo,
        vk::PrimitiveTopology prim,
        vk::ArrayProxy<vk::Viewport> const &viewports,
        vk::ArrayProxy<vk::Rect2D> const &scissors,
        vk::PolygonMode polyMode,
        vk::CullModeFlags cullMode,
        vk::FrontFace front,
        vk::PipelineLayout layout,
        vk::RenderPass renderPass,
        uint32_t subPass,
        vk::ArrayProxy<vk::DynamicState> const &dynamic)
    {
        return this->createPipeline(
            shaders,
            vertexInfo,
            prim,
            false,
            viewports,
            scissors,
            false,
            polyMode,
            cullMode,
            front,
            layout,
            renderPass,
            subPass,
            dynamic);
    }

/* TODO: define a ComputeShader class, since compute shaders
 * only have one stage and get used in different contexts.
 */
    /// \brief Create a compute pipeline
    /// \param layout   the pipeline layout
    /// \param shaders  shaders for the pipeline
    /// \return the created pipeline
    vk::Pipeline createComputePipeline (vk::PipelineLayout layout, cs237::Shaders *shaders);

    /// \brief create and initialize a command buffer
    /// \return the fresh command buffer
    vk::CommandBuffer newCommandBuf ()
    {
        vk::CommandBufferAllocateInfo allocInfo(
            this->_cmdPool,
            vk::CommandBufferLevel::ePrimary,
            1); /* buffer count */

        return (this->_device.allocateCommandBuffers(allocInfo))[0];
    }

    /// \brief begin recording commands in the given command buffer
    /// \param cmdBuf the command buffer to use for recording commands
    /// \param oneTime true if this command buffer is only going to be used once
    void beginCommands (vk::CommandBuffer cmdBuf, bool oneTime = false)
    {
        vk::CommandBufferBeginInfo beginInfo(
            oneTime
              ? vk::CommandBufferUsageFlagBits::eOneTimeSubmit
              : vk::CommandBufferUsageFlags());
        cmdBuf.begin(beginInfo);
    }

    /// \brief end the recording of commands in the give command buffer
    /// \param cmdBuf the command buffer that we are recording in
    void endCommands (vk::CommandBuffer cmdBuf) { cmdBuf.end(); }

    /// \brief end the commands and submit the buffer to the graphics queue.
    /// \param cmdBuf the command buffer to submit
    void submitCommands (vk::CommandBuffer cmdBuf)
    {
        vk::CommandBuffer cmdBufs[1] = { cmdBuf };
        vk::SubmitInfo submitInfo(
            {},
            {},
            vk::ArrayProxyNoTemporaries<const vk::CommandBuffer>(1, cmdBufs),
            {});
        this->_queues.graphics.submit ({submitInfo});
        this->_queues.graphics.waitIdle();
    }

    /// \brief free the command buffer
    /// \param cmdBuf the command buffer to free
    void freeCommandBuf (vk::CommandBuffer & cmdBuf)
    {
        this->_device.freeCommandBuffers (this->_cmdPool, cmdBuf);
    }

    /// information about queue families
    template <typename T>
    struct Queues {
        T graphics;     ///< the queue family that supports graphics
        T present;      ///< the queue family that supports presentation
        T compute;      ///< the queue family that supports compute
    };

    /// get the queue indices
    Queues<uint32_t> getQIndices () const { return this->_qIdxs; }

protected:

    // information about swap-chain support
    struct SwapChainDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    std::string _name;          ///< the application name
    vk::DebugUtilsMessageSeverityFlagsEXT _messages;
                                ///< set to the message severity level
    bool _debug;                ///< set when validation layers should be enabled
    vk::Instance _instance;     ///< the Vulkan instance used by the application
    vk::PhysicalDevice _gpu;    ///< the graphics card (aka device) that we are using
    mutable vk::PhysicalDeviceProperties *_propsCache;
    mutable vk::PhysicalDeviceFeatures *_featuresCache;
                                ///< a cache of the physical device properties
    vk::Device _device;         ///< the logical device that we are using to render
    Queues<uint32_t> _qIdxs;    ///< the queue family indices
    Queues<vk::Queue> _queues;  ///< the device queues that we are using
    vk::CommandPool _cmdPool;   ///< pool for allocating command buffers

    /// \brief A helper function to create and initialize the Vulkan instance
    /// used by the application.
    void _createInstance ();

    /// \brief function that gets the physical-device properties and caches the
    ///        pointer in the `_propsCache` field.
    void _getPhysicalDeviceProperties () const;

    /// \brief function that gets the physical-device features and caches the
    ///        pointer in the `_featuresCache` field.
    void _getPhysicalDeviceFeatures () const;

    /// \brief A helper function to select the GPU to use
    /// \param reqFeatures  points to a structure specifying the required features
    ///                     of the selected device.
    void _selectDevice (vk::PhysicalDeviceFeatures *reqFeatures = nullptr);

    /// \brief A helper function to identify the index of a device memory type
    ///        that has the required type and properties
    /// \param reqTypeBits  bit mask that specifies the possible memory types
    /// \param reqProps     memory property bit mask
    /// \return the index of the lowest set bit in reqTypeBits that has the
    ///         required properties.  If no such memory exists, then -1 is returned.
    int32_t _findMemory (uint32_t reqTypeBits, vk::MemoryPropertyFlags reqProps) const;

    /// \brief A helper function to identify the best image format supported by the
    ///        device from an ordered list of candidate formats
    /// \param candidates   list of candidates in order of preference
    /// \param tiling       how pixels are to be tiled in the image (linear vs optimal)
    /// \param features     required features for the format
    /// \return the first format in the list of candidates that has the required features
    ///         for the specified tiling.  VK_FORMAT_UNDEFINED is returned if there is
    ///         no valid candidate
    vk::Format _findBestFormat (
        std::vector<vk::Format> candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features);

    /// \brief allocate the command pool for the application
    void _initCommandPool ();

    /// \brief A helper function to identify the best depth/stencil-buffer attachment
    ///        format for the device
    /// \param depth    set to true if requesting depth-buffer support
    /// \param stencil  set to true if requesting stencil-buffer support
    /// \return the format that has the requested buffer support and the best precision.
    ///         Returns VK_FORMAT_UNDEFINED is `depth` and `stencil` are both false or
    ///         if there s no depth-buffer support
    vk::Format _depthStencilBufferFormat (bool depth, bool stencil);

    /// \brief A helper function to identify the queue-family indices for the
    ///        physical device that we are using.
    /// \return `true` if the device supports all of the required queue types and `false`
    ///        otherwise.
    ///
    /// If this function returns `true`, then the `_qIdxs` instance variable will be
    /// initialized to the queue family indices that were detected.
    bool _getQIndices (vk::PhysicalDevice dev);

    /// \brief A helper function to create the logical device during initialization
    ///
    /// This function initializes the `_device`, `_qIdxs`, and `_queues`
    /// instance variables.
    void _createLogicalDevice ();

    /// \brief A helper function for creating a Vulkan image that can be used for
    ///        textures or depth buffers
    /// \param wid      the image width
    /// \param ht       the image height
    /// \param format   the pixel format for the image
    /// \param tiling   the tiling method for the pixels (device optimal vs linear)
    /// \param usage    flags specifying the usage of the image
    /// \param layout   the image layout
    /// \param mipLvls  number of mipmap levels for the image (default = 1)
    /// \return the created image
    vk::Image _createImage (
        uint32_t wid,
        uint32_t ht,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::ImageLayout layout,
        uint32_t mipLvls = 1);

    /// \brief A helper function for creating a Vulkan image that can be used for
    ///        textures or depth buffers
    /// \param wid      the image width
    /// \param ht       the image height
    /// \param format   the pixel format for the image
    /// \param tiling   the tiling method for the pixels (device optimal vs linear)
    /// \param usage    flags specifying the usage of the image
    /// \param mipLvls  number of mipmap levels for the image
    /// \return the created image
    vk::Image _createImage (
        uint32_t wid,
        uint32_t ht,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        uint32_t mipLvls = 1)
    {
        return this->_createImage (
            wid, ht, format, tiling, usage,
            vk::ImageLayout::eUndefined,
            mipLvls);
    }

    /// \brief A helper function for allocating and binding device memory for an image
    /// \param img    the image to allocate memory for
    /// \param props  requred memory properties
    /// \return the device memory that has been bound to the image
    vk::DeviceMemory _allocImageMemory (vk::Image img, vk::MemoryPropertyFlags props);

    /// \brief A helper function for creating a Vulkan image view object for an image
    vk::ImageView _createImageView (
        vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);

    /// \brief A helper function for changing the layout of an image
    void _transitionImageLayout (
        vk::Image image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout);

    /// \brief create a vk::Buffer object
    /// \param size   the size of the buffer in bytes
    /// \param usage  the usage of the buffer
    /// \return the allocated buffer
    vk::Buffer _createBuffer (size_t size, vk::BufferUsageFlags usage);

    /// \brief A helper function for allocating and binding device memory for a buffer
    /// \param buf    the buffer to allocate memory for
    /// \param props  requred memory properties
    /// \return the device memory that has been bound to the buffer
    vk::DeviceMemory _allocBufferMemory (vk::Buffer buf, vk::MemoryPropertyFlags props);

    /// \brief copy data from one buffer to another using the GPU
    /// \param dstBuf the destination buffer
    /// \param srcBuf the source buffer
    /// \param offset the offset in the destination buffer to copy to
    /// \param size   the size (in bytes) of data to copy
    void _copyBuffer (vk::Buffer dstBuf, vk::Buffer srcBuf, size_t offset, size_t size);

    /// \brief copy data from a buffer to an image
    /// \param dstImg the destination image
    /// \param srcBuf the source buffer
    /// \param size   the size (in bytes) of data to copy
    /// \param wid    the image width
    /// \param ht     the image height (default 1)
    /// \param depth  the image depth (default 1)
    void _copyBufferToImage (
        vk::Image dstImg, vk::Buffer srcBuf, size_t size,
        uint32_t wid, uint32_t ht=1, uint32_t depth=1);

    /* debug-message support */
    VkDebugUtilsMessengerEXT _debugMessenger;
    void _initDebug ();
    void _cleanupDebug ();
};

} // namespace cs237

#endif // !_CS237_APPLICATION_HPP_
