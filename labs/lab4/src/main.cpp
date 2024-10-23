/*! \file main.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 4.  This file is the main program
 * for Lab 4.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "mesh.hpp"

#ifdef CS237_BINARY_DIR
/// location of the shaders for Lab 4
const std::string kShaderDir = CS237_BINARY_DIR "/labs/lab4/shaders/";
#else
# error CS237_BINARY_DIR not defined
#endif

// view parameters; these are constants for now.
static const float kNearZ = 0.2f;       ///< distance to near plane
static const float kFarZ = 100.0f;      ///< distance to far plane
static const float kFOV = 90.0f;        ///< field of view angle in degrees

// layout of the unform-buffer data for the vertex shader; we use the `alignas`
// annotations to ensure that the values are correctly aligned.  See
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout
// for details on the alignment requirements.
//
struct UBData {
    alignas(16) glm::mat4 MV;           ///< model-view transform
    alignas(16) glm::mat4 P;            ///< projection transform
};

using UBO_t = cs237::UniformBuffer<UBData>;


/******************** derived classes ********************/

/// The Lab 2 Application class
class Lab4 : public cs237::Application {
public:
    Lab4 (std::vector<std::string> const &args);
    ~Lab4 ();

    void run () override;

private:
    class Lab4Window *_win;
};

/// The Lab 4 Window class
class Lab4Window : public cs237::Window {
public:
    Lab4Window (Lab4 *app);

    ~Lab4Window () override;

    /// reshape the window
    void reshape (int wid, int ht) override;

    void draw () override;

    /// handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:

    // extend the per-frame data type with the stuff we need for Lab 4
    struct FrameData : public cs237::Window::FrameData {
        UBO_t *ubo;                             ///< uniform buffer for vertex shader
        vk::DescriptorSet descSet;              ///< descriptor set for the uniform
                                                ///  buffer and sampler

        FrameData (cs237::Window *w);
        virtual ~FrameData () override;
    };

    vk::RenderPass _renderPass;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _graphicsPipeline;
    UBData _uboCache;                           ///< current values for the UBO
    cs237::VertexBuffer<Vertex> *_vertBuffer;   ///< vertex buffer for cube vertices
    cs237::IndexBuffer<uint16_t> *_idxBuffer;   ///< index buffer for cube indices
    vk::DescriptorSetLayout _descSetLayout;     ///< descriptor-set layout for uniform buffer
    vk::DescriptorPool _descPool;               ///< descriptor-set pool
    // texture state; these values are invariant over the course of execution,
    // so we can allocate them per window, instead of per frame.
    cs237::Texture2D *_txt;                     ///< the texture image etc.
    vk::Sampler _txtSampler;                    ///< the texture sampler
    // Camera state
    glm::vec3 _camPos;                          ///< camera position in world space
    glm::vec3 _camAt;                           ///< camera look-at point in world space
    glm::vec3 _camUp;                           ///< camera up vector in world space

    /// initialize the uniform-buffer-object cache using the current camera
    /// state.
    void _updateUBO ();

    /// initialize the UBO descriptor pool and layout
    void _initDescriptorPools ();
    /// initialize the `_renderPass` field
    void _initRenderPass ();
    /// initialize the `_pipelineLayout` and `_graphicsPipeline` fields
    void _initPipeline ();
    /// allocate and initialize the data buffers
    void _initData ();
    /// record the rendering commands
    void _recordCommandBuffer (Lab4Window::FrameData *frame);

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `FrameData` struct.
    Window::FrameData *_allocFrameData (Window *w) override;

};

/******************** Lab4Window methods ********************/

Lab4Window::Lab4Window (Lab4 *app)
  : cs237::Window (
        app,
        // resizable window with depth buffer and no stencil
        cs237::CreateWindowInfo(800, 600, app->name(), true, true, false))
{
    // create the texture sampler
    /** HINT: define this->_txtSampler here */

    // initialize the camera
    this->_camPos = glm::vec3(0.0f, 0.0f, 4.0f);
    this->_camAt = glm::vec3(0.0f, 0.0f, 0.0f);
    this->_camUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // set the initial uniform values
    this->_updateUBO ();

    // initialize the vertex buffer, and index buffer, and texture
    this->_initData();

    // create the descriptor-set pool and layout
    this->_initDescriptorPools();

    this->_initRenderPass ();
    this->_initPipeline ();

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers (this->_renderPass);

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Lab4Window::~Lab4Window ()
{
    auto device = this->device();

    device.destroyPipeline(this->_graphicsPipeline);
    device.destroyPipelineLayout(this->_pipelineLayout);
    device.destroyRenderPass(this->_renderPass);

    device.destroyDescriptorPool(this->_descPool);
    device.destroyDescriptorSetLayout(this->_descSetLayout);
    device.destroySampler(this->_txtSampler);

    delete this->_idxBuffer;
    delete this->_vertBuffer;
    delete this->_txt;

}

void Lab4Window::_initDescriptorPools ()
{
    auto device = this->device();

    // create the descriptor pool
    std::array<vk::DescriptorPoolSize,2> poolSizes = {
            // UBO pool
            vk::DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                cs237::kMaxFrames),
            // sampler pool
            vk::DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                cs237::kMaxFrames)
        };
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        cs237::kMaxFrames, /* max sets */
        poolSizes); /* pool sizes */
    this->_descPool = device.createDescriptorPool(poolInfo);

    // the descriptor-set layout for the uniform buffer
    vk::DescriptorSetLayoutBinding uboLayout(
        0, /* binding */
        vk::DescriptorType::eUniformBuffer, /* descriptor type */
        1, /* descriptor count */
        vk::ShaderStageFlagBits::eVertex, /* stages */
        nullptr); /* samplers */

    // the descriptor-set layout for the sampler
/** HINT: define the layout binding for the sampler here */
    vk::DescriptorSetLayoutBinding samplerLayout();

    // two layouts; one for the UBO and one for the sampler
    std::array<vk::DescriptorSetLayoutBinding, 2> layoutBindings = {
            uboLayout, samplerLayout
        };
    vk::DescriptorSetLayoutCreateInfo layoutInfo(
        {}, /* flags */
        layoutBindings); /* bindings */
    this->_descSetLayout = device.createDescriptorSetLayout(layoutInfo);

}

void Lab4Window::_initRenderPass ()
{
    // we have both color and depth-buffer attachments
    std::vector<vk::AttachmentDescription> atDescs;
    std::vector<vk::AttachmentReference> atRefs;
    this->_initAttachments (atDescs, atRefs);
    assert (atRefs.size() == 2);  /* expect a depth buffer */

    // subpass for output
    vk::SubpassDescription subpass(
        {}, /* flags */
        vk::PipelineBindPoint::eGraphics, /* pipeline bind point */
        0, nullptr, /* input attachments */
        1, &(atRefs[0]), /* color attachments */
        nullptr, /* resolve attachments */
        &(atRefs[1]), /* depth-stencil attachment */
        0, nullptr); /* preserve attachments */

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
        atDescs, /* attachments */
        subpass, /* subpasses_ */
        dependency); /* dependencies */

    this->_renderPass = this->device().createRenderPass(renderPassInfo);

}

void Lab4Window::_initPipeline ()
{
    // load the shaders
    vk::ShaderStageFlags stages =
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    auto shaders = new cs237::Shaders(this->device(), kShaderDir + "shader", stages);

    // vertex input info
    auto vertexInfo = cs237::vertexInputInfo (
        Vertex::getBindingDescriptions(),
        Vertex::getAttributeDescriptions());

    this->_pipelineLayout = this->_app->createPipelineLayout(this->_descSetLayout);

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    this->_graphicsPipeline = this->_app->createPipeline(
        shaders,
        vertexInfo,
        vk::PrimitiveTopology::eTriangleList,
        // the viewport and scissor rectangles are specified dynamically, but we need
        // to specify the counts
        vk::ArrayProxy<vk::Viewport>(1, nullptr), /* viewports */
        vk::ArrayProxy<vk::Rect2D>(1, nullptr), /* scissor rects */
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        // we are following the OpenGL convention for front faces
        vk::FrontFace::eCounterClockwise,
        this->_pipelineLayout,
        this->_renderPass,
        0,
        dynamicStates);

    cs237::destroyVertexInputInfo (vertexInfo);
    delete shaders;

}

void Lab4Window::_initData ()
{
    Mesh mesh;

    // create and set up the vertex buffer
    /** HINT: create and initialize this->_vertBuffer */

    // create and set up the index buffer
    /** HINT: create and initialize this->_idxBuffer */

    // initialize the texture
    /** HINT: create a texture from `mesh.image` */

}

void Lab4Window::_recordCommandBuffer (Lab4Window::FrameData *frame)
{
    auto cmdBuf = frame->cmdBuf;

    cmdBuf.reset();

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuf.begin(beginInfo);

    std::array<vk::ClearValue,2> clearValues = {
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f), /* clear the window to black */
            vk::ClearDepthStencilValue(1.0f, 0.0f)
        };
    vk::RenderPassBeginInfo renderPassInfo(
        this->_renderPass,
        this->_swap.fBufs[frame->index],
        { {0, 0}, this->_swap.extent }, /* render area */
        clearValues);

    cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    /*** BEGIN COMMANDS ***/
    cmdBuf.bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        this->_graphicsPipeline);

    // set the viewport using the OpenGL convention
    this->_setViewportCmd(cmdBuf, true);

    // bind the descriptor sets
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        this->_pipelineLayout,
        0,
        frame->descSet,
        nullptr);

    // bind the vertex buffer
    vk::Buffer vertBuffers[] = {this->_vertBuffer->vkBuffer()};
    vk::DeviceSize offsets[] = {0};
    cmdBuf.bindVertexBuffers(0, vertBuffers, offsets);

    // bind the index buffer
    cmdBuf.bindIndexBuffer(
        this->_idxBuffer->vkBuffer(), 0, vk::IndexType::eUint16);

    cmdBuf.drawIndexed(this->_idxBuffer->nIndices(), 1, 0, 0, 0);
    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

void Lab4Window::_updateUBO ()
{
    // the model-view: MV = V*M = V*I = V
    this->_uboCache.MV = glm::lookAt(this->_camPos, this->_camAt, this->_camUp);

    // the projection matrix
    this->_uboCache.P = glm::perspectiveFov(
        glm::radians(kFOV),
        float(this->_wid), float(this->_ht),
        kNearZ, kFarZ);

}

void Lab4Window::reshape (int wid, int ht)
{
    // invoke the super-method reshape method
    this->cs237::Window::reshape(wid, ht);

    // recreate the new framebuffers
    this->_swap.initFramebuffers(this->_renderPass);

    // update the perspective project matrix
    this->_updateUBO();
}

void Lab4Window::draw ()
{
    // next buffer from the swap chain
    auto sts = this->_acquireNextImage();
    if (sts != vk::Result::eSuccess) {
        ERROR("inable to acquire next image");
    }

    auto frame = reinterpret_cast<FrameData *>(this->_currentFrame());

    frame->resetFence();

    // set the frame's UBO
    frame->ubo->copyTo(this->_uboCache);

    // record the rendering commands
    this->_recordCommandBuffer(frame);

    // set up submission for the graphics queue
    frame->submitDrawingCommands();

    // set up submission for the presentation queue
    frame->present();
}

void Lab4Window::key (int key, int scancode, int action, int mods)
{
    // ignore releases, control keys, command keys, etc.
    if ((action == GLFW_PRESS)
    && (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER)) == 0) {
        switch (key) {
            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
                break;

            /** HINT: handle keys for camera control */

            default: // ignore all other keys
                return;
        }
    }

}

/******************** Lab4Window::FrameData struct ********************/

/* virtual */
cs237::Window::FrameData *Lab4Window::_allocFrameData (Window *w) /* override */
{
    return new Lab4Window::FrameData (w);
}

Lab4Window::FrameData::FrameData (Window *w)
: Window::FrameData(w)
{
    auto win = reinterpret_cast<Lab4Window *>(w);
    auto device = win->device();

    // allocate the UBO for the frame
    this->ubo = new UBO_t(win->app());

    // allocate a descriptor set for the frame
    vk::DescriptorSetAllocateInfo allocInfo(
        win->_descPool,
        win->_descSetLayout);
    this->descSet = (device.allocateDescriptorSets(allocInfo))[0];

    // info about the UBO
    auto bufferInfo = this->ubo->descInfo();

    // write operation for the uniform buffer
    vk::WriteDescriptorSet uboWriteDS(
        this->descSet, /* descriptor set */
        0, /* binding */
        0, /* array element */
        vk::DescriptorType::eUniformBuffer, /* descriptor type */
        nullptr, /* image info */
        bufferInfo, /* buffer info */
        nullptr); /* texel buffer view */

    // info about the sampler
    /** HINT: define the info for the sampler here */

    // write operation for the sampler
    /** HINT: define the write operation for the sampler **/
    vk::WriteDescriptorSet samplerWriteDS();

    // write the descriptor set
    std::array<vk::WriteDescriptorSet,2> descriptorWrites = {
            uboWriteDS, samplerWriteDS
        };
    device.updateDescriptorSets (descriptorWrites, nullptr);

}

/* virtual */
Lab4Window::FrameData::~FrameData ()
{
    delete this->ubo;
}

/******************** Lab4 class ********************/

Lab4::Lab4 (std::vector<std::string> const &args)
  : cs237::Application (args, "CS237 Lab 4"), _win(nullptr)
{ }

Lab4::~Lab4 () { }

void Lab4::run ()
{
    auto win = new Lab4Window (this);

    // complete the lab-specific window initialization
    win->initialize ();

    // wait until the window is closed
    while(! win->windowShouldClose()) {
        // wait for events
        glfwPollEvents();

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
    Lab4 app(args);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
