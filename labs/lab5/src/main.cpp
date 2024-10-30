/*! \file main.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 5.  This file is the main program
 * for Lab 5.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "mesh.hpp"
#include "drawable.hpp"
#include "uniforms.hpp"

#ifdef CS237_BINARY_DIR
/// location of the shaders for Lab 5
const std::string kShaderDir = CS237_BINARY_DIR "/labs/lab5/shaders/";
#else
# error CS237_BINARY_DIR not defined
#endif

// view parameters; these are constants for now.
constexpr float kNearZ = 0.2f;       ///< distance to near plane
constexpr float kFarZ = 100.0f;      ///< distance to far plane
constexpr float kFOV = 90.0f;        ///< field of view angle in degrees
constexpr float kRadius = 10.0f;     ///< camera distance from Y axis
constexpr float kCamPosY = 8.0f;     ///< camera elevation
constexpr float kCameraSpeed = 2.0f; ///< camera rotation speed (in degrees)

/// the light's direction in world coordinates (pointing toward the scene)
constexpr glm::vec3 kLightDir(-0.75f, -1.0f, -0.5f);
/// the distance to the light's near plane
constexpr float kLightNearZ = 0.2f;

/// the dimensions of the depth texture
constexpr uint32_t kDepthTextureWid = 1024;
constexpr uint32_t kDepthTextureHt = 1024;

/******************** view renderer ********************/

/// the information needed to render the view in a given mode (shadows or no shadows)
struct Renderer {
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;

    void destroy (vk::Device device)
    {
        device.destroyPipeline(this->pipeline);
        device.destroyPipelineLayout(this->pipelineLayout);
    }
};

/******************** derived classes ********************/

/// The Lab 5 Application class
class Lab5 : public cs237::Application {
public:
    Lab5 (std::vector<std::string> const &args);
    ~Lab5 ();

    void run () override;
};

/// The Lab 5 Window class
class Lab5Window : public cs237::Window {
public:
    Lab5Window (Lab5 *app);

    ~Lab5Window () override;

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
        bool valid;                             ///< is the UBO up to date?

        FrameData (cs237::Window *w);
        virtual ~FrameData () override;

        void updateUBO (Lab5Window *win)
        {
            this->ubo->copyTo (win->_uboCache);
            this->valid = true;
        }
    };

    // depth (aka shadow) rendering pass
    cs237::DepthBuffer *_depthBuf;              ///< depth-buffer
    vk::RenderPass _depthRenderPass;            ///< render pass for depth texture
    vk::PipelineLayout _depthPipelineLayout;    ///< pipeline layout for depth pass
    vk::Pipeline _depthPipeline;                ///< pipeline for depth pass
    vk::Framebuffer _depthFrameBuffer;          ///< output buffer for depth pass

    // view rendering pass
    vk::RenderPass _viewRenderPass;             ///< the view render pass
    Renderer *_noShadowRenderer;                ///< renderer for when shadows are
                                                ///  disabled
    Renderer *_shadowRenderer;                  ///< renderer for when shadows are
                                                ///  enabled
    std::vector<vk::Framebuffer> _framebuffers;

    // descriptors
    vk::DescriptorPool _descPool;               ///< descriptor-set pool
    vk::DescriptorSetLayout _uboDSLayout;       ///< the layout for the per-frame
                                                ///  uniform buffer
    vk::DescriptorSetLayout _drawableDSLayout;  ///< the layout for the per-drawable
                                                ///  color-map descriptor sets
    vk::DescriptorSetLayout _depthDSLayout;     ///< the layout for the
                                                ///  depth-buffer descriptor
                                                ///  set (`_depthDS`)
    vk::DescriptorSet _depthDS;                 ///< the depth-buffer descriptor set

    std::vector<Drawable *> _objs;              ///< the drawable objects with their
                                                ///< uniforms, etc.
    cs237::AABBf_t _bbox;                       ///< axis-aligned box that contains
                                                ///  the objects of the scene

    // Camera state
    float _angle;                               ///< rotation angle for camera
    glm::vec3 _camPos;                          ///< camera position in world space
    glm::vec3 _camAt;                           ///< camera look-at point in world space
    glm::vec3 _camUp;                           ///< camera up vector in world space

    // cache information for the UBOs
    glm::mat4 _worldToLight;                    ///< world-space to light-space transform
    UB _uboCache;                               ///< cache the per-drawable UBO data

    /// get the current renderer
    Renderer *_renderer ()
    {
        if (this->_uboCache.enableShadows == VK_TRUE) {
            return this->_shadowRenderer;
        } else {
            return this->_noShadowRenderer;
        }
    }

    /// toggle the current texture mode
    void toggleTextureMode ()
    {
        this->_uboCache.enableTexture =
            (this->_uboCache.enableTexture == VK_FALSE) ? VK_TRUE : VK_FALSE;
        this->_invalidateFrameUBOs();
    }

    /// toggle the current shadow mode
    void toggleShadowMode ()
    {
        this->_uboCache.enableShadows =
            (this->_uboCache.enableShadows == VK_FALSE) ? VK_TRUE : VK_FALSE;
        this->_invalidateFrameUBOs();
    }

    /// initialize the descriptor-set pools and layouts
    void _initDescriptorSetLayouts ();

    /// initialize the per-drawable and shadow-map-sampler descriptor sets
    void _initDescriptorSets ();

    /// initialize the `_depthRenderPass` field
    void _initDepthRenderPass ();
    /// initialize the `_depthPipelineLayout` and `_depthGraphicsPipeline` fields
    void _initDepthPipeline ();

    /// initialize the `_viewRenderPass` field
    void _initViewRenderPass ();
    /// create and initialize a view renderer
    Renderer *_createViewRenderer (bool shadows);

    /// allocate and initialize the drawables
    void _initDrawables ();

    /// initialize the shadow matrix
    void _initShadowMatrix ();

    /// record the rendering commands
    void _recordCommandBuffer (Lab5Window::FrameData *frame);

    /// set the camera position based on the current angle
    void _setCameraPos ()
    {
        float rAngle = glm::radians(this->_angle);
        float camX = kRadius * sin(rAngle);
        float camZ = kRadius * cos(rAngle);
        this->_camPos = glm::vec3(camX, kCamPosY, camZ);

        // update the UBO cache
        this->_uboCache.viewMat = glm::lookAt(this->_camPos, this->_camAt, this->_camUp);
    }

    /// set the projection matrix based on the current window size
    void _setProjMat ()
    {
        this->_uboCache.projMat = glm::perspectiveFov(
            glm::radians(kFOV),
            float(this->_wid), float(this->_ht),
            kNearZ, kFarZ);
    }

    /// invalidate the per-frame scene-data UBOs and update the scene-data cache
    void _invalidateFrameUBOs ()
    {
        // mark the uniform buffers as being out-of-sync with the cache
        for (auto it : this->_frames) {
            reinterpret_cast<Lab5Window::FrameData *>(it)->valid = false;
        }
    }

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `FrameData` struct.
    Window::FrameData *_allocFrameData (Window *w) override;

};

/******************** Lab5Window methods ********************/

Lab5Window::Lab5Window (Lab5 *app)
  : cs237::Window (
        app,
        // resizable window with depth buffer and no stencil
        cs237::CreateWindowInfo(1024, 768, app->name(), true, true, false))
{
    // initialize the camera
    this->_angle = 0.0;
    this->_camAt = glm::vec3(0.0f, 0.0f, 0.0f);
    this->_camUp = glm::vec3(0.0f, 1.0f, 0.0f);
    this->_setCameraPos ();

    // initialize the projection matrix
    this->_setProjMat();

    // cache the unit vector that points toward the light
    this->_uboCache.lightDir = glm::normalize(-kLightDir);

    // initially, texturing and shadowing are disabled
    this->_uboCache.enableTexture = VK_FALSE;
    this->_uboCache.enableShadows = VK_FALSE;

    this->_initDrawables ();

    this->_initShadowMatrix();

    // create the depth buffer; note that this step needs to be
    // done before calling _initDepthRenderPass, since we need
    // to determine the format of the depth buffer
    this->_depthBuf = new cs237::DepthBuffer (app, kDepthTextureWid, kDepthTextureHt);

    this->_initDepthRenderPass ();
    this->_initViewRenderPass ();

    this->_initDescriptorSetLayouts ();

    this->_initDepthPipeline ();
    this->_noShadowRenderer = this->_createViewRenderer (false);
    this->_shadowRenderer = this->_createViewRenderer (true);

    this->_initDescriptorSets();

    // create the depth-buffer framebuffer
    this->_depthFrameBuffer = this->_depthBuf->createFramebuffer (this->_depthRenderPass);

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers (this->_viewRenderPass);

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Lab5Window::~Lab5Window ()
{
    auto device = this->device();

    // clean up view resources
    this->_noShadowRenderer->destroy(device);
    this->_shadowRenderer->destroy(device);

    device.destroyRenderPass(this->_viewRenderPass);

    // clean up depth-buffer resources
    device.destroyFramebuffer(this->_depthFrameBuffer);
    delete this->_depthBuf;
    device.destroyPipeline(this->_depthPipeline);
    device.destroyRenderPass(this->_depthRenderPass);
    device.destroyPipelineLayout(this->_depthPipelineLayout);

    // delete the objects and associated resources
    for (auto obj : this->_objs) {
        delete obj;
    }

    // clean up other resources
    device.destroyDescriptorPool(this->_descPool);
    device.destroyDescriptorSetLayout(this->_uboDSLayout);
    device.destroyDescriptorSetLayout(this->_drawableDSLayout);
    device.destroyDescriptorSetLayout(this->_depthDSLayout);
}

void Lab5Window::_initDrawables ()
{
    Mesh *floorMesh = Mesh::floor();
    this->_objs.push_back (new Drawable (this->_app, floorMesh));

    Mesh *crateMesh = Mesh::crate();
    this->_objs.push_back (new Drawable (this->_app, crateMesh));

    // compute the bounding box for the scene
    this->_bbox += floorMesh->bbox();
    this->_bbox += crateMesh->bbox();

    // cleanup
    delete crateMesh;
    delete floorMesh;

}

void Lab5Window::_initShadowMatrix ()
{
    /** HINT: the code for setting up the shadow matrix goes here */
}

/******************** Descriptor Set Initialization ********************/

void Lab5Window::_initDescriptorSetLayouts ()
{
    int nObjs = this->_objs.size();
    assert (nObjs > 0);

    // allocate the descriptor-set pool.  We have one UBO descriptor per frame,
    // one sampler descriptor per object, and the depth-buffer sampler
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, cs237::kMaxFrames),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, nObjs+1)
        };
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        cs237::kMaxFrames+nObjs+1, /* max sets */
        poolSizes); /* pool sizes */
    this->_descPool = this->device().createDescriptorPool(poolInfo);

    // create the descriptor-set layout for the per-frame UBO (set 0)
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            0, /* binding */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eVertex /* stages */
                | vk::ShaderStageFlagBits::eFragment,
            nullptr); /* immutable samplers */
        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_uboDSLayout = this->device().createDescriptorSetLayout(layoutInfo);
    }

    // create the descriptor-set layout for the per-drawable sampler (set 1)
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            0, /* binding */
            vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr); /* immutable samplers */

        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_drawableDSLayout = this->device().createDescriptorSetLayout(layoutInfo);
    }

    // create the descriptor-set layout for the depth-buffer sampler (set 2)
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            0, /* binding */
            vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr); /* immutable samplers */

        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_depthDSLayout = this->device().createDescriptorSetLayout(layoutInfo);
    }
}

void Lab5Window::_initDescriptorSets ()
{
    /* NOTE: the per-frame descriptor sets are initialized when the per-frame
     * data is created.
     */

    // create and initialize the per-drawable descriptor sets
    for (auto obj : this->_objs) {
        obj->initDescriptor (this->_descPool, this->_drawableDSLayout);
    }

    // create the depth-buffer descriptor set
    vk::DescriptorSetAllocateInfo allocInfo(this->_descPool, this->_depthDSLayout);
    this->_depthDS = (this->device().allocateDescriptorSets(allocInfo))[0];

    // initialize the depth-buffer descriptor set
    vk::DescriptorImageInfo depthImgInfo = this->_depthBuf->imageInfo();
    vk::WriteDescriptorSet depthDescWrite(
        this->_depthDS, /* descriptor set */
        0, /* binding */
        0, /* array element */
        vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
        depthImgInfo, /* image info */
        nullptr, /* buffer info */
        nullptr); /* texel buffer view */
    this->device().updateDescriptorSets(depthDescWrite, nullptr);
}

/******************** Render Pass Initialization ********************/

void Lab5Window::_initDepthRenderPass ()
{
    /** HINT: the code for initializing the render pass for the depth-buffer
     ** rendering pass goes here
     */
}

void Lab5Window::_initViewRenderPass ()
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

    this->_viewRenderPass = this->device().createRenderPass(renderPassInfo);

}

/******************** Graphics Pipeline Initialization ********************/

void Lab5Window::_initDepthPipeline ()
{
    /** HINT: the code for initializing the pipelin for the depth-buffer
     ** rendering pass goes here
     */
}

Renderer *Lab5Window::_createViewRenderer (bool shadows)
{
    Renderer *renderer = new Renderer;

    // create the pipeline layout for view rendering
    vk::PushConstantRange pcRange = {
            vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(PC),
        };
    std::vector dsLayouts = {
            this->_uboDSLayout, /* set 0 */
            this->_drawableDSLayout /* set 1 */
        };
    if (shadows) {
        dsLayouts.push_back(this->_depthDSLayout); /* set 2 */
    }
    vk::PipelineLayoutCreateInfo layoutInfo(
        {}, /* flags */
        dsLayouts, /* set layouts */
        pcRange); /* push constants */
    renderer->pipelineLayout = this->device().createPipelineLayout(layoutInfo);

    // load the shaders
    vk::ShaderStageFlags stages =
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    std::string shaderName = kShaderDir + (shadows ? "shadow" : "no-shadow");
    auto shaders = new cs237::Shaders(this->device(), shaderName, stages);

    // vertex input info
    auto vertexInfo = cs237::vertexInputInfo (
        Vertex::getBindingDescriptions(),
        Vertex::getAttributeDescriptions());

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    renderer->pipeline = this->_app->createPipeline(
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
        renderer->pipelineLayout,
        this->_viewRenderPass,
        0,
        dynamicStates);

    cs237::destroyVertexInputInfo (vertexInfo);
    delete shaders;

    return renderer;
}

/******************** Rendering ********************/

void Lab5Window::_recordCommandBuffer (Lab5Window::FrameData *frame)
{
    auto cmdBuf = frame->cmdBuf;

    cmdBuf.reset();

    vk::ClearValue depthClearValue = vk::ClearDepthStencilValue(1.0f, 0);

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuf.begin(beginInfo);

    if (this->_uboCache.enableShadows) {
        /* Shadow pass */
        vk::RenderPassBeginInfo depthPassInfo(
            this->_depthRenderPass,
            this->_depthFrameBuffer,
            vk::Rect2D({0, 0}, {kDepthTextureWid, kDepthTextureHt}),
            depthClearValue);
        cmdBuf.beginRenderPass(depthPassInfo, vk::SubpassContents::eInline);

        /*** BEGIN COMMANDS ***/

        // bind the depth-pass pipeline
        cmdBuf.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            this->_depthPipeline);

        // bind the descriptor for the uniform buffer
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            this->_depthPipelineLayout,
            kUBODescSetID,
            frame->descSet,
            nullptr);

        // draw the objects
        for (auto obj : this->_objs) {
            // set the push constants
            obj->emitPushConstants(cmdBuf,
                this->_depthPipelineLayout,
                vk::ShaderStageFlagBits::eVertex);
            // render the drawable to the shadow buffer
            obj->draw (cmdBuf);
        }
        /*** END COMMANDS ***/

        cmdBuf.endRenderPass();
    }

    /* Render pass */
    std::array<vk::ClearValue,2> clearValues = {
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f), /* clear the window to black */
            depthClearValue
        };
    vk::RenderPassBeginInfo renderPassInfo(
        this->_viewRenderPass,
        this->_swap.fBufs[frame->index],
        { {0, 0}, this->_swap.extent }, /* render area */
        clearValues);
    cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    auto renderer = this->_renderer();

    /*** BEGIN COMMANDS ***/
    cmdBuf.bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        this->_renderer()->pipeline);

    // set the viewport using the OpenGL convention
    this->_setViewportCmd (frame->cmdBuf, true);

    // bind the descriptor for the uniform buffer
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        renderer->pipelineLayout,
        kUBODescSetID,
        frame->descSet,
        nullptr);

    // conditionally bind the descriptor for the shadow map (aka depth buffer)
    if (this->_uboCache.enableShadows) {
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            renderer->pipelineLayout,
            kSMapDescSetID,
            this->_depthDS,
            nullptr);
    }

    for (auto obj : this->_objs) {
        // bind the descriptor set for the color-map sampler
        obj->bindDescriptorSet(frame->cmdBuf, renderer->pipelineLayout);
        // set the push constants
        obj->emitPushConstants(cmdBuf,
            renderer->pipelineLayout,
            vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment);
        // render the drawable
        obj->draw (frame->cmdBuf);
    }
    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

void Lab5Window::draw ()
{
    // next buffer from the swap chain
    auto sts = this->_acquireNextImage();
    if (sts != vk::Result::eSuccess) {
        ERROR("Unable to acquire next image");
    }

    auto frame = reinterpret_cast<FrameData *>(this->_currentFrame());

    frame->resetFence();

    if (! frame->valid) {
        // first we update the UBOs for the objects
        frame->updateUBO (this);
    }

    // record the rendering commands
    this->_recordCommandBuffer(frame);

    // set up submission for the graphics queue
    frame->submitDrawingCommands();

    // set up submission for the presentation queue
    frame->present();
}

/******************** User Interaction ********************/

void Lab5Window::reshape (int wid, int ht)
{
    // invoke the super-method reshape method
    this->cs237::Window::reshape(wid, ht);

    // recreate the new framebuffers
    this->_swap.initFramebuffers (this->_viewRenderPass);

    // update the projection matrix
    this->_setProjMat();

    // mark per-frame UBOs as invalid
    this->_invalidateFrameUBOs();

}

void Lab5Window::key (int key, int scancode, int action, int mods)
{
  // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
                break;

            case GLFW_KEY_T:  // 't' or 'T' ==> toggle texturing
                this->toggleTextureMode();
                std::cout << "Toggle texturing "
                    << (this->_uboCache.enableTexture ? "on\n" : "off\n");
                break;

            case GLFW_KEY_S:  // 's' or 'S' ==> toggle shadowing
                this->toggleShadowMode();
                std::cout << "Toggle shadows "
                    << (this->_uboCache.enableShadows ? "on\n" : "off\n");
                break;

            case GLFW_KEY_LEFT:
                this->_angle -= kCameraSpeed;
                this->_setCameraPos();
                this->_invalidateFrameUBOs();
                break;

            case GLFW_KEY_RIGHT:
                this->_angle += kCameraSpeed;
                this->_setCameraPos();
                this->_invalidateFrameUBOs();
                break;

            default: // ignore all other keys
                return;
        }
    }

}

/******************** Lab5Window::FrameData struct ********************/

/* virtual */
cs237::Window::FrameData *Lab5Window::_allocFrameData (Window *w) /* override */
{
    return new Lab5Window::FrameData (w);
}

Lab5Window::FrameData::FrameData (Window *w)
: Window::FrameData(w), valid(false)
{
    auto win = reinterpret_cast<Lab5Window *>(w);
    auto device = win->device();

    // allocate the UBO for the frame
    this->ubo = new UBO_t(win->app());

    // allocate a descriptor set for the frame
    vk::DescriptorSetAllocateInfo allocInfo(
        win->_descPool,
        win->_uboDSLayout);
    this->descSet = (device.allocateDescriptorSets(allocInfo))[0];
std::cout << "# Lab5Window::FrameData::FrameData: this = " << (void*)this
<< "; descSet = " << (void*)this->descSet << "\n";

    // info about the UBO
    auto bufferInfo = this->ubo->descInfo();

    // write operation for the uniform buffer
    vk::WriteDescriptorSet writeDS(
        this->descSet, /* descriptor set */
        0, /* binding */
        0, /* array element */
        vk::DescriptorType::eUniformBuffer, /* descriptor type */
        nullptr, /* image info */
        bufferInfo, /* buffer info */
        nullptr); /* texel buffer view */

    // write the descriptor set
    device.updateDescriptorSets (writeDS, nullptr);

}

/* virtual */
Lab5Window::FrameData::~FrameData ()
{
    delete this->ubo;
}


/******************** Lab5 class ********************/

Lab5::Lab5 (std::vector<std::string> const &args)
  : cs237::Application (args, "CS237 Lab 5")
{ }

Lab5::~Lab5 ()
{ }

void Lab5::run ()
{
    auto win = new Lab5Window (this);

    // complete the lab-specific window initialization
    win->initialize ();

    // wait until the window is closed
    while(! win->windowShouldClose()) {
        win->draw ();
        glfwWaitEvents();
    }

    // wait until any in-flight rendering is complete
    this->_device.waitIdle();

    delete win;
}

/******************** main ********************/

int main(int argc, char *argv[])
{
    std::vector<std::string> args(argv, argv+argc);
    Lab5 app(args);

    // print information about the keyboard commands
    std::cout
        << "# Lab 5 User Interface\n"
        << "#  't' to toggle textures\n"
        << "#  's' to toggle shadows\n"
        << "#  'q' to quit\n"
        << "#  left and right arrow keys to rotate view\n";

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
