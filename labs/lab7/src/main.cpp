/*! \file main.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 7.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237/cs237.hpp"
#include <ostream>

#ifdef CS237_BINARY_DIR
/// location of the shaders for Lab 7
const std::string kShaderDir = CS237_BINARY_DIR "/labs/lab7/shaders/";
#else
# error CS237_BINARY_DIR not defined
#endif

constexpr int kMaxIter = 1024; // must agree with value in compute shader!
constexpr int kResolution = 1024;
constexpr float kTwoThirdsPi = (2.0 * M_PI) / 3.0;

/// the time period between steps; use larger values to slow the animation
constexpr double kUpdatePeriod = 1.0 / 60.0;

/******************** Compute Uniforms ********************/

struct CompUB {
    glm::vec2 center;           /// center point of image
    glm::vec2 fov;              /// field of view
    glm::vec2 size;             /// global compute-grid size
    glm::vec3 color[kMaxIter];  /// color map
};

using CompUBO = cs237::UniformBuffer<CompUB>;

struct CompPC {
    int iter;

    /// update the iteration count
    void step ()
    {
        if (this->iter < kMaxIter) { this->iter++; }
    }

};

/******************** Image Buffers ********************/

struct ImageBuffer {
    vk::Image img;              ///< Vulkan image
    vk::DeviceMemory mem;       ///< device memory for the image
    vk::ImageView view;         ///< image view

    void destroy (vk::Device device)
    {
        device.destroyImageView(this->view);
        device.freeMemory(this->mem);
        device.destroyImage(this->img);
    }
};

/******************** derived classes ********************/

/// The Lab 2 Application class
class Lab7 : public cs237::Application {
public:
    /// constructor
    /// \param args  the command-line arguments
    Lab7 (std::vector<std::string> const &args);

    /// destructor
    ~Lab7 ();

    /// run the application code
    void run () override;
};

/// The Lab 2 Window class
class Lab7Window : public cs237::Window {
public:
    /// constructor
    /// \param app  pointer to the application object
    Lab7Window (Lab7 *app);

    /// destructor
    ~Lab7Window () override;

    /// render the contents of the window
    void draw () override;

    /// handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:

    // extend the per-frame data type with the stuff we need for Lab 4
    struct FrameData : public cs237::Window::FrameData {
        vk::DescriptorSet computeDS;    ///< descriptor set for the compute shader
        vk::CommandBuffer computeCmdBuf; ///< compute command buffer for the frame
        vk::Fence computeInFlight;      ///< fence for in-flight computation commands
        vk::Semaphore computeFinished;  ///< semaphore to signal that the frame's
                                        ///  computation is finished

        ImageBuffer computeOutState;    ///< the compute output-state buffer
        ImageBuffer computeImage;       ///< the computed image buffer

        vk::DescriptorSet renderDS;     ///< descriptor set for the render shader

        FrameData (cs237::Window *w);
        virtual ~FrameData () override;

    private:
        void _allocImageBuffer (vk::Format fmt, vk::ImageUsageFlags usage, ImageBuffer &ib);

    };

    /// Vulkan resources needed for computing the image
    struct {
        vk::DescriptorSetLayout ubDSLayout;     ///< layout for compute-shader UBO
        vk::DescriptorSet uboDS;                ///< descriptor for compute-shader UBO
        vk::DescriptorSetLayout imgDSLayout;    ///< layout for compute-shader images
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
        CompUBO *ubo;
        CompPC pc;
    } _compute;

    /// Vulkan resources needed for rendering the image
    struct {
        vk::DescriptorSetLayout imgDSLayout;    ///< layout for computed image
        vk::Sampler imgSampler;                 ///< sampler for the computed image
        vk::RenderPass renderPass;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    } _render;

    vk::DescriptorPool _dsPool; ///< descriptor pool

    /// initialize the descriptor pool and layouts
    void _initDescriptorSetLayouts ();

    /// initialize the compute-shader UBO
    void _initComputeUBO ();

    /// initialize the compute pipeline
    void _initComputePipeline();
    /// record the compute commands
    void _recordComputeCommands(FrameData *frame);

    /// initialize the `_render.renderPass` field
    void _initRenderPass();
    /// initialize the graphics pipeline
    void _initRenderPipeline();
    /// record the rendering commands
    void _recordRenderCommands(FrameData *frame);

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `FrameData` struct.
    Window::FrameData *_allocFrameData (Window *w) override;

    /// override the `Window::_init` method to initialize the per-frame
    /// descriptors
    void _init () override;

};

/******************** Lab7Window methods ********************/

Lab7Window::Lab7Window (Lab7 *app)
  : cs237::Window (app, cs237::CreateWindowInfo(kResolution, kResolution))
{
    this->_initComputeUBO();
    this->_compute.pc.iter = 0;

    // initialize the descriptor pool.
    this->_initDescriptorSetLayouts();

    this->_initComputePipeline();

    this->_initRenderPass();
    this->_initRenderPipeline();

    // create the sampler for the computed image
    cs237::Application::SamplerInfo samplerInfo(
        vk::Filter::eLinear,                    // magnification filter
        vk::Filter::eLinear,                    // minification filter
        vk::SamplerMipmapMode::eLinear,         // mipmap mode
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for U coordinates
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for V coordinates
        vk::BorderColor::eIntOpaqueBlack);      // border color
    this->_render.imgSampler = this->_app->createSampler (samplerInfo);

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers(this->_render.renderPass);

    // enable handling of keyboard events
    this->enableKeyEvent (true);

}

Lab7Window::~Lab7Window ()
{
    this->device().destroyDescriptorPool(this->_dsPool);

    this->device().destroyPipeline(this->_compute.pipeline);
    this->device().destroyPipelineLayout(this->_compute.pipelineLayout);
    this->device().destroyDescriptorSetLayout(this->_compute.ubDSLayout);
    this->device().destroyDescriptorSetLayout(this->_compute.imgDSLayout);
    delete this->_compute.ubo;

    this->device().destroySampler(this->_render.imgSampler);
    this->device().destroyPipeline(this->_render.pipeline);
    this->device().destroyPipelineLayout(this->_render.pipelineLayout);
    this->device().destroyRenderPass(this->_render.renderPass);
    this->device().destroyDescriptorSetLayout(this->_render.imgDSLayout);
}

/* virtual */
void Lab7Window::_init ()
{
    if (this->_app->verbose()) {
        std::cout << "# Lab7Window::_init\n";
    }

    auto device = this->device();

    // allocate and initialize the per-frame descriptors.  For the compute shader,
    // the `stateIn` image is the previous frame's image.
    auto prevFrame = reinterpret_cast<FrameData *>(this->_frames[cs237::kMaxFrames-1]);
    for (int i = 0;  i < cs237::kMaxFrames;  ++i) {
        auto frame = reinterpret_cast<FrameData *>(this->_frames[i]);

        // allocate a descriptor set for the compute-shader images
        {
            vk::DescriptorSetAllocateInfo allocInfo(
                this->_dsPool,
                this->_compute.imgDSLayout);
            frame->computeDS = (device.allocateDescriptorSets(allocInfo))[0];
        }

        // allocate a descriptor set for the fragment-shader image
        {
            vk::DescriptorSetAllocateInfo allocInfo(
                this->_dsPool,
                this->_render.imgDSLayout);
            frame->renderDS = (device.allocateDescriptorSets(allocInfo))[0];
        }

        // update the per-frame descriptors
        {
            // write operation for the input state (the previous frame's output)
            vk::DescriptorImageInfo stateInInfo(
                {},
                prevFrame->computeOutState.view,
                vk::ImageLayout::eGeneral);
            vk::WriteDescriptorSet stateIn(
                frame->computeDS, /* descriptor set */
                0, /* binding */
                0, /* array element */
                vk::DescriptorType::eStorageImage, /* descriptor type */
                stateInInfo, /* image info */
                nullptr, /* buffer info */
                nullptr); /* texel buffer view */
            // write operation for the output state
            vk::DescriptorImageInfo stateOutInfo(
                {},
                frame->computeOutState.view,
                vk::ImageLayout::eGeneral);
            vk::WriteDescriptorSet stateOut(
                frame->computeDS, /* descriptor set */
                1, /* binding */
                0, /* array element */
                vk::DescriptorType::eStorageImage, /* descriptor type */
                stateOutInfo, /* image info */
                nullptr, /* buffer info */
                nullptr); /* texel buffer view */
            // write operation for the computed image
            vk::DescriptorImageInfo imageInfo(
                {},
                frame->computeImage.view,
                vk::ImageLayout::eGeneral);
            vk::WriteDescriptorSet image(
                frame->computeDS, /* descriptor set */
                2, /* binding */
                0, /* array element */
                vk::DescriptorType::eStorageImage, /* descriptor type */
                imageInfo, /* image info */
                nullptr, /* buffer info */
                nullptr); /* texel buffer view */
            std::array<vk::WriteDescriptorSet,3> descriptorWrites = {
                    stateIn, stateOut, image
                };
            device.updateDescriptorSets (descriptorWrites, nullptr);
        }

        // update the rendering descriptors
        {
            vk::DescriptorImageInfo imageInfo(
                this->_render.imgSampler,
                frame->computeImage.view,
                vk::ImageLayout::eGeneral);
            vk::WriteDescriptorSet descriptorWrite(
                frame->renderDS,
                0, /* binding */
                0, /* array element */
                vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
                imageInfo, /* image info */
                nullptr, /* buffer info */
                nullptr); /* texel buffer view */
            device.updateDescriptorSets (descriptorWrite, nullptr);
        }

        prevFrame = frame;
    }

    // allocate and update the UBO and descriptors
    {
        vk::DescriptorSetAllocateInfo allocInfo(
            this->_dsPool,
            this->_compute.ubDSLayout);
        this->_compute.uboDS = (device.allocateDescriptorSets(allocInfo))[0];

        // write operation for the compute-shader UBO
        vk::DescriptorBufferInfo uboInfo = this->_compute.ubo->descInfo();
        vk::WriteDescriptorSet uboWrite(
            this->_compute.uboDS,
            0, /* binding */
            0, /* array element */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            nullptr, /* image info */
            uboInfo, /* buffer info */
            nullptr); /* texel buffer view */
        device.updateDescriptorSets (uboWrite, nullptr);
    }

}

void Lab7Window::_initDescriptorSetLayouts ()
{
    if (this->_app->verbose()) {
        std::cout << "# Lab7Window::_initDescriptorSetLayouts\n";
    }

    // allocate the descriptor-set pool.  We have the following descriptors:
    //   * one UBO for the compute shader
    //   * three image buffers per frame for the compute shader
    //   * one image sampler per frame for the render shader
    std::array<vk::DescriptorPoolSize,3> poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
            vk::DescriptorPoolSize(
                vk::DescriptorType::eStorageImage,
                3*cs237::kMaxFrames),
            vk::DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                cs237::kMaxFrames)
        };
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        3*cs237::kMaxFrames + 1, /* max sets */
        poolSizes); /* pool sizes */
    this->_dsPool = this->device().createDescriptorPool(poolInfo);

    // create the descriptor-set layout for the compute-shader UBO
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            0, /* binding */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eCompute, /* stages */
            nullptr); /* immutable samplers */
        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_compute.ubDSLayout =
            this->device().createDescriptorSetLayout(layoutInfo);
    }

    // create the descriptor-set layout for the compute-shader image buffers
    {
        std::array<vk::DescriptorSetLayoutBinding, 3> layoutBindings = {
                /* stateIn image */
                vk::DescriptorSetLayoutBinding(
                    0, /* binding */
                    vk::DescriptorType::eStorageImage, /* descriptor type */
                    1, /* descriptor count */
                    vk::ShaderStageFlagBits::eCompute, /* stages */
                    nullptr), /* immutable samplers */
                /* stateOut image */
                vk::DescriptorSetLayoutBinding(
                    1, /* binding */
                    vk::DescriptorType::eStorageImage, /* descriptor type */
                    1, /* descriptor count */
                    vk::ShaderStageFlagBits::eCompute, /* stages */
                    nullptr), /* immutable samplers */
                /* output image */
                vk::DescriptorSetLayoutBinding(
                    2, /* binding */
                    vk::DescriptorType::eStorageImage, /* descriptor type */
                    1, /* descriptor count */
                    vk::ShaderStageFlagBits::eCompute, /* stages */
                    nullptr), /* immutable samplers */
            };

        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBindings);
        this->_compute.imgDSLayout =
            this->device().createDescriptorSetLayout(layoutInfo);
    }

    // create the descriptor-set layout for the render-shader image buffer
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            0, /* binding */
            vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr);

        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_render.imgDSLayout =
            this->device().createDescriptorSetLayout(layoutInfo);
    }


}

/***** Compute Initialization *****/

void Lab7Window::_initComputeUBO ()
{
    if (this->_app->verbose()) {
        std::cout << "# Lab7Window::_initComputeUBO\n";
    }

    CompUB ub;
    // NOTE: you can set the center to (-1,0) and the field of view to (1.5,1.5)
    // to see the whole image
//    ub.center = glm::vec2(-1,0);
//    ub.fov = glm::vec2(1.5,1.5);
    ub.center = glm::vec2(-1.2, -0.3);
    ub.fov = glm::vec2(0.15, 0.15);
    ub.size = glm::vec2(kResolution, kResolution);

    // compute the color map
    for (int i = 0;  i < kMaxIter;  ++i) {
        auto t = ::sinf(11.0 * ::powf(float(i), 0.2));
        auto r = (::sin(t) + 1.7) / 2.7;
        auto g = (::sin(t + kTwoThirdsPi) + 1.7) / 2.7;
        auto b = (::sin(t - kTwoThirdsPi) + 1.7) / 2.7;
        ub.color[i] = glm::vec3(r, g, b);
    }

    this->_compute.ubo = new CompUBO(this->_app, ub);
}

void Lab7Window::_initComputePipeline ()
{
    if (this->_app->verbose()) {
        std::cout << "# Lab7Window::_initComputePipeline\n";
    }

    // load the compute shaders
    vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eCompute;
    auto shaders = new cs237::Shaders(this->device(), kShaderDir + "mandelbrot", stages);

    // create the pipeline layout for view rendering
    std::array<vk::DescriptorSetLayout,2> dsLayouts = {
            this->_compute.ubDSLayout, /* set 0 */
            this->_compute.imgDSLayout /* set 1 */
        };

    // pipeline layout
    vk::PushConstantRange pcRange = {
            vk::ShaderStageFlagBits::eCompute,
            0,
            sizeof(CompPC),
        };

    this->_compute.pipelineLayout =
        this->app()->createPipelineLayout(dsLayouts, pcRange);

    this->_compute.pipeline =
        this->_app->createComputePipeline(this->_compute.pipelineLayout, shaders);

    // cleanup temporaries
    delete shaders;

}

void Lab7Window::_recordComputeCommands (FrameData *frame)
{
    auto cmdBuf = frame->computeCmdBuf;

    cmdBuf.reset();

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuf.begin(beginInfo);

    /*** BEGIN COMMANDS ***/
    {
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, this->_compute.pipeline);

        cmdBuf.bindDescriptorSets(
                vk::PipelineBindPoint::eCompute,
                this->_compute.pipelineLayout,
                0,
                { this->_compute.uboDS, frame->computeDS },
                nullptr);

        cmdBuf.pushConstants(
            this->_compute.pipelineLayout,
            vk::ShaderStageFlagBits::eCompute,
            0,
            sizeof(CompPC),
            &this->_compute.pc);

        // we divide the image into local work groups of size 16x16
        cmdBuf.dispatch(kResolution / 16, kResolution / 16, 1);
    }
    /*** END COMMANDS ***/

    cmdBuf.end();

}

/***** Render Initialization *****/

void Lab7Window::_initRenderPass ()
{
    // we have a single color buffer as the attachment
    vk::AttachmentDescription colorAttachment(
        {}, /* flags */
        this->_swap.imageFormat, /* image-view format */
        vk::SampleCountFlagBits::e1, /* number of samples */
        vk::AttachmentLoadOp::eLoad, /* load op */
        vk::AttachmentStoreOp::eStore, /* store op */
        vk::AttachmentLoadOp::eDontCare, /* stencil load op */
        vk::AttachmentStoreOp::eDontCare, /* stencil store op */
        vk::ImageLayout::eUndefined, /* initial layout */
        vk::ImageLayout::ePresentSrcKHR); /* final layout */

    vk::AttachmentReference colorAttachmentRef(
        0, /* index */
        vk::ImageLayout::eColorAttachmentOptimal); /* layout */

    vk::SubpassDescription subpass(
        {}, /* flags */
        vk::PipelineBindPoint::eGraphics, /* pipeline bind point */
        {}, /* input attachments */
        colorAttachmentRef, /* color attachments */
        {}, /* resolve attachments */
        {}, /* depth-stencil attachment */
        {}); /* preserve attachments */

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL, /* src subpass */
        0, /* dst subpass */
        vk::PipelineStageFlagBits::eColorAttachmentOutput, /* src stage mask */
        vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dst stage mask */
        {}, /* src access mask */
        { vk::AccessFlagBits::eColorAttachmentWrite }, /* dst access mask */
        {}); /* dependency flags */

    vk::RenderPassCreateInfo renderPassInfo(
        {}, /* flags */
        colorAttachment, /* attachments */
        subpass, /* subpasses_ */
        dependency); /* dependencies */

    this->_render.renderPass = this->device().createRenderPass(renderPassInfo);

}

void Lab7Window::_initRenderPipeline ()
{
    // load the rendering shaders
    vk::ShaderStageFlags stages =
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    auto shaders = new cs237::Shaders(this->device(), kShaderDir + "render", stages);

    // vertex input info; use defaults, since there is no vertex data
    vk::PipelineVertexInputStateCreateInfo vertexInfo;

    this->_render.pipelineLayout =
        this->app()->createPipelineLayout(this->_render.imgDSLayout, nullptr);

    // use the natural viewport/scissors rectangle that covers the whole window
    vk::Viewport viewport = this->_getViewport();
    vk::Rect2D scissors = this->_getScissorsRect();

    this->_render.pipeline = this->_app->createPipeline(
        shaders,
        vertexInfo,
        vk::PrimitiveTopology::eTriangleList,
        viewport, /* viewports */
        scissors, /* scissor rects */
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        this->_render.pipelineLayout,
        this->_render.renderPass,
        0,
        {});

    // cleanup temporaries
    delete shaders;

}

void Lab7Window::_recordRenderCommands (FrameData *frame)
{
    auto cmdBuf = frame->cmdBuf;

    cmdBuf.reset();

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuf.begin(beginInfo);

    vk::ClearValue blackColor(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
    vk::RenderPassBeginInfo renderPassInfo(
        this->_render.renderPass,
        this->_swap.fBufs[this->_currentFrame()->index],
        { {0, 0}, this->_swap.extent }, /* render area */
        blackColor);

    cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    /*** BEGIN COMMANDS ***/
    {
        cmdBuf.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            this->_render.pipeline);

        cmdBuf.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                this->_render.pipelineLayout,
                0,
                frame->renderDS,
                nullptr);

        // draw the screen triangle
        cmdBuf.draw(3, 1, 0, 0);
    }
    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

/***** Computing and Drawing *****/

void Lab7Window::draw ()
{
    if (this->_app->verbose()) {
        if (this->_compute.pc.iter == 0) {
            std::cout << "# Lab7Window::draw: ";
        } else {
            std::cout << ".";
        }
        std::flush(std::cout);
    }

    // first we advance the frame
    this->_advanceFrame();

    auto frame = reinterpret_cast<FrameData *>(this->_currentFrame());

    /***** Compute phase *****/

    // wait until the frame's compute resources are available
    auto sts = this->device().waitForFences(frame->computeInFlight, VK_TRUE, UINT64_MAX);
    if (sts != vk::Result::eSuccess) {
        ERROR("Synchronization error");
    }
    this->device().resetFences(frame->computeInFlight);

    this->_recordComputeCommands(frame);

    vk::SubmitInfo computeSubmitInfo(
        nullptr, /* wait semaphores */
        nullptr, /* destination stage mask */
        frame->computeCmdBuf, /* command buffer */
        frame->computeFinished); /* signal semaphores */

    this->computeQ().submit(computeSubmitInfo, frame->computeInFlight);

    /***** Render phase *****/

    // wait for the frame's `inFlight` fence
    frame->waitForFence();
    frame->resetFence();

    // get the next buffer from the swap chain
    auto res = this->device().acquireNextImageKHR(
        this->_swap.chain,
        UINT64_MAX,
        frame->imageAvail,
        nullptr);
    if (res.result == vk::Result::eSuccess) {
        frame->index = res.value;
    } else {
        ERROR("Unable to acquire next image");
    }

    this->_recordRenderCommands(frame);

    // set up submission for the graphics queue
    std::array<vk::Semaphore,2> waitSemaphores = {
            frame->computeFinished, frame->imageAvail
        };
    std::array<vk::PipelineStageFlags,2> stageMask = {
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        };
    vk::SubmitInfo renderSubmitInfo(
        waitSemaphores, /* wait semaphores */
        stageMask, /* destination stage mask */
        frame->cmdBuf, /* command buffer */
        frame->finished); /* signal semaphores */

    this->computeQ().submit(renderSubmitInfo, frame->inFlight);

    // set up submission for the presentation queue
    frame->present();

    // record that we have taken a step
    this->_compute.pc.step();

}

void Lab7Window::key (int key, int scancode, int action, int mods)
{
    // ignore releases, control keys, command keys, etc.
    if ((action == GLFW_PRESS)
    && (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER)) == 0) {
        switch (key) {
            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose(this->_win, GLFW_TRUE);
                break;
            default: // ignore all other keys
                return;
        }
    }

}

/******************** Lab6Window::FrameData struct ********************/

/* virtual */
cs237::Window::FrameData *Lab7Window::_allocFrameData (Window *w) /* override */
{
    return new Lab7Window::FrameData (w);
}

// helper function for initializing image buffers
void Lab7Window::FrameData::_allocImageBuffer (
    vk::Format fmt,
    vk::ImageUsageFlags usage,
    ImageBuffer &ib)
{
    auto win = reinterpret_cast<Lab7Window *>(this->win);

    std::vector<uint32_t> qFamilyIndices;
    vk::SharingMode sharingMode;
    if (win->app()->getQIndices().compute != win->app()->getQIndices().graphics) {
        // The compute and graphics queue family indices differ, so we create
        // an image that can be shared between them
        sharingMode = vk::SharingMode::eConcurrent;
        qFamilyIndices.push_back(win->app()->getQIndices().graphics);
        qFamilyIndices.push_back(win->app()->getQIndices().compute);
    } else {
        sharingMode = vk::SharingMode::eExclusive;
    }

    vk::ImageCreateInfo info(
        {}, /* flags */
        vk::ImageType::e2D,
        fmt, /* format */
        { kResolution, kResolution, 1 }, /* extent */
        1, /* mip levels */
        1, /* array layers */
        vk::SampleCountFlagBits::e1, /* samples */
        vk::ImageTiling::eOptimal,
        usage, /* usage */
        sharingMode, /* sharing mode */
        qFamilyIndices, /* queue family indices */
        vk::ImageLayout::eUndefined); /* initial layout */

    ib.img = win->device().createImage(info);
    ib.mem = win->_allocImageMemory(ib.img, vk::MemoryPropertyFlagBits::eDeviceLocal);

    // transition the image layout
    win->_transitionImageLayout(
        ib.img, fmt,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral);

    ib.view = win->_createImageView(ib.img, fmt, vk::ImageAspectFlagBits::eColor);

}

Lab7Window::FrameData::FrameData (Window *w)
: Window::FrameData(w)
{
    if (win->app()->verbose()) {
        std::cout << "# Lab7Window::FrameData constructor\n";
    }

    auto win = reinterpret_cast<Lab7Window *>(w);

    // allocate a command buffer for the compute pipeline
    this->computeCmdBuf = win->app()->newCommandBuf();

    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
    this->computeInFlight = win->device().createFence(fenceInfo);

    vk::SemaphoreCreateInfo semaphoreInfo{};
    this->computeFinished = win->device().createSemaphore(semaphoreInfo);

    // allocate the image buffer used to hold the work-item states in the compute shader
    if (win->app()->verbose()) {
        std::cout << "# Lab7Window::FrameData constructor: allocate output state\n";
    }
    this->_allocImageBuffer(
        vk::Format::eR32G32B32A32Sfloat, /* format: == vec4 */
        vk::ImageUsageFlagBits::eStorage,
        this->computeOutState);

    // allocate the image buffer used to hold the computed image
    if (win->app()->verbose()) {
        std::cout << "# Lab7Window::FrameData constructor: allocate image\n";
    }
    this->_allocImageBuffer(
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
        this->computeImage);

}

/* virtual */
Lab7Window::FrameData::~FrameData ()
{
    auto device = this->win->device();

    device.destroyFence(this->computeInFlight);
    device.destroySemaphore(this->computeFinished);

    this->computeOutState.destroy (device);
    this->computeImage.destroy (device);

}

/******************** Lab7 class ********************/

Lab7::Lab7 (std::vector<std::string> const &args)
  : cs237::Application (args, "CS237 Lab 7")
{ }

Lab7::~Lab7 () { }

void Lab7::run ()
{
    Lab7Window *win = new Lab7Window (this);

    // complete window intialization
    win->initialize();

    // wait until the window is closed
    while(! win->windowShouldClose()) {
        // wait for events, but not longer that the update period
        glfwWaitEventsTimeout(kUpdatePeriod);

        win->draw();
    }

    // wait until any in-flight rendering is complete
    this->_device.waitIdle();

    // cleanup
    delete win;
}

/******************** main ********************/

int main(int argc, char *argv[])
{
    std::vector<std::string> args(argv, argv + argc);
    Lab7 app(args);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
