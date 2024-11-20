/*! \file main.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 6.  This file is the main program
 * for Lab 6.
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
#include "gbuffer.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_structs.hpp"

#ifdef CS237_BINARY_DIR
/// location of the shaders for Lab 6
const std::string kShaderDir = CS237_BINARY_DIR "/labs/lab6/shaders/";
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
constexpr glm::vec3 kLightColor(0.85f, 0.85f, 0.85f);
constexpr glm::vec3 kAmbLightColor(0.15f, 0.15f, 0.15f);
constexpr glm::vec3 kBackgroundColor(0.05f, 0.05f, 0.20f);

/// the distance to the light's near plane
constexpr float kLightNearZ = 0.2f;

/******************** view renderer ********************/

/// the information needed to render the view in a given mode (shadows or no shadows)
struct Renderer {
    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;

    Renderer ()
    : renderPass(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE)
    { }

    void destroy (vk::Device device)
    {
        device.destroyPipeline(this->pipeline);
        device.destroyPipelineLayout(this->pipelineLayout);
        device.destroyRenderPass(this->renderPass);
    }
};

/******************** derived classes ********************/

/// The Lab 5 Application class
class Lab6 : public cs237::Application {
public:
    Lab6 (std::vector<std::string> const &args);
    ~Lab6 ();

    void run () override;
};

/// The Lab 5 Window class
class Lab6Window : public cs237::Window {
public:
    Lab6Window (Lab6 *app);

    ~Lab6Window () override;

    /// reshape the window
    void reshape (int wid, int ht) override;

    void draw () override;

    /// handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:

    // extend the per-frame data type with the stuff we need for Lab 4
    struct FrameData : public cs237::Window::FrameData {
        GeomUBO_t *geomUBO;             ///< uniform buffer for geometry-pass
        FinalUBO_t *finalUBO;           ///< uniform buffer for lighting
                                        ///  information used in the final pass
        vk::DescriptorSet geomDS;       ///< descriptor set for geometry-pass UBO
        vk::DescriptorSet finalDS;      ///< descriptor set for geometry-pass UBO
        bool valid;                     ///< are the UBOs up to date?

        FrameData (cs237::Window *w);
        virtual ~FrameData () override;

        void updateUBO (Lab6Window *win)
        {
            this->geomUBO->copyTo (win->_geomUBCache);
            this->finalUBO->copyTo (win->_finalUBCache);
            this->valid = true;
        }
    };

    // geometry-pass resources
    GBuffer *_gBuf;                             ///< the G-buffer
    Renderer _geomPass;                         ///< the pipelines for the geometry pass
    vk::Framebuffer _geomFramebuffer;           ///< the target frame buffer for the
                                                ///  geometry pass

    // final-pass resources
    Renderer _finalPass;

    std::vector<vk::Framebuffer> _framebuffers;

    // descriptors
    vk::DescriptorPool _descPool;               ///< descriptor-set pool
    vk::DescriptorSetLayout _geomDSLayout;      ///< the layout for the geometry-pass
                                                ///  per-frame UBO
    vk::DescriptorSetLayout _drawableDSLayout;  ///< the layout for the per-drawable
                                                ///  color-map descriptor sets
    vk::DescriptorSetLayout _finalDSLayout;     ///< the layout for final-pass
                                                ///  per-frame UBO

    std::vector<Drawable *> _objs;              ///< the drawable objects with their
                                                ///< uniforms, etc.

    // Camera state
    float _angle;                               ///< rotation angle for camera
    glm::vec3 _camPos;                          ///< camera position in world space
    glm::vec3 _camAt;                           ///< camera look-at point in world space
    glm::vec3 _camUp;                           ///< camera up vector in world space

    // cache information for the UBOs
    GeomUB _geomUBCache;                        ///< cache the per-frame geometry-pass
                                                ///  UBO data
    FinalUB _finalUBCache;                      ///< final-pass uniform data cache

    /// initialize the descriptor-set pools and layouts
    void _initDescriptorSetLayouts ();

    /// initialize the per-drawable and shadow-map-sampler descriptor sets
    void _initDescriptorSets ();

    /// initialize the `_gBuf` field
    void _initGBuffer ();

    /// initialize the geometry render pass
    void _initGeomRenderPass ();

    /// initialize the final render pass
    void _initFinalRenderPass ();

    /// initialize the geometry render pass
    void _initGeomPipeline ();

    /// create and initialize a view renderer
    void _initFinalPipeline ();

    /// initialize the geometry-pass frame buffer
    /// \param wid  the width of the buffer
    /// \param ht   the height of the buffer
    void _initGeomFramebuffer (uint32_t wid, uint32_t ht);

    /// allocate and initialize the drawables
    void _initDrawables ();

    /// record the rendering commands
    void _recordCommandBuffer (Lab6Window::FrameData *frame);

    /// set the camera position based on the current angle
    void _setCameraPos ()
    {
        float rAngle = glm::radians(this->_angle);
        float camX = kRadius * sin(rAngle);
        float camZ = kRadius * cos(rAngle);
        this->_camPos = glm::vec3(camX, kCamPosY, camZ);

        // update the UBO cache
        this->_geomUBCache.viewMat = glm::lookAt(this->_camPos, this->_camAt, this->_camUp);
    }

    /// set the projection matrix based on the current window size
    void _setProjMat ()
    {
        this->_geomUBCache.projMat = glm::perspectiveFov(
            glm::radians(kFOV),
            float(this->_wid), float(this->_ht),
            kNearZ, kFarZ);
    }

    /// invalidate the per-frame scene-data UBOs and update the scene-data cache
    void _invalidateFrameUBOs ()
    {
        // mark the uniform buffers as being out-of-sync with the cache
        for (auto it : this->_frames) {
            reinterpret_cast<Lab6Window::FrameData *>(it)->valid = false;
        }
    }

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `FrameData` struct.
    Window::FrameData *_allocFrameData (Window *w) override;

};

/******************** Lab6Window methods ********************/

Lab6Window::Lab6Window (Lab6 *app)
  : cs237::Window (
        app,
        // resizable window without depth buffer or stencil
        cs237::CreateWindowInfo(1024, 768, app->name(), true, true, false)),
    _gBuf(nullptr), _geomFramebuffer(VK_NULL_HANDLE),
    _descPool(VK_NULL_HANDLE),
    _geomDSLayout(VK_NULL_HANDLE),
    _drawableDSLayout(VK_NULL_HANDLE),
    _finalDSLayout(VK_NULL_HANDLE)
{
    // initialize the camera
    this->_angle = 0.0;
    this->_camAt = glm::vec3(0.0f, 0.0f, 0.0f);
    this->_camUp = glm::vec3(0.0f, 1.0f, 0.0f);
    this->_setCameraPos ();

    // initialize the projection matrix
    this->_setProjMat();

    // initialize the UBO for the final pass
    _finalUBCache.lightDir = glm::normalize(-kLightDir);
    _finalUBCache.lightColor = kLightColor;
    _finalUBCache.ambient = kAmbLightColor;
    _finalUBCache.background = kBackgroundColor;
    _finalUBCache.mode = kRenderScene;

    this->_initDrawables ();

    // allocate the G-buffer
    this->_gBuf = new GBuffer(app, this->width(), this->height());

    // set up the rendering passes
    this->_initGeomRenderPass ();
    this->_initFinalRenderPass ();

    this->_initDescriptorSetLayouts ();

    this->_initGeomPipeline ();
    this->_initFinalPipeline ();

    this->_initDescriptorSets();

    // initialize the frame buffer for the geometry pass
    this->_initGeomFramebuffer(this->width(), this->height());

    // create framebuffers for the swap chain; these are used in the final pass
    if (this->app()->verbose() || this->app()->debug()) {
        std::cout << "# Lab6Window::Lab6Window: initFramebuffers\n";
    }
    this->_swap.initFramebuffers (this->_finalPass.renderPass);

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Lab6Window::~Lab6Window ()
{
    auto device = this->device();

    // clean up geometry-pass resources
    delete this->_gBuf;
    device.destroyFramebuffer(this->_geomFramebuffer);
    this->_geomPass.destroy(device);

    // clean up final-pass resources
    this->_finalPass.destroy(device);

    // delete the objects and associated resources
    for (auto obj : this->_objs) {
        delete obj;
    }

    // clean up other resources
    device.destroyDescriptorPool(this->_descPool);
    device.destroyDescriptorSetLayout(this->_geomDSLayout);
    device.destroyDescriptorSetLayout(this->_drawableDSLayout);
    device.destroyDescriptorSetLayout(this->_finalDSLayout);

}

void Lab6Window::_initDrawables ()
{
    Mesh *floorMesh = Mesh::floor();
    this->_objs.push_back (new Drawable (this->_app, floorMesh));

    Mesh *crateMesh = Mesh::crate();
    this->_objs.push_back (new Drawable (this->_app, crateMesh));

    // cleanup
    delete crateMesh;
    delete floorMesh;

}

/******************** Descriptor Set Initialization ********************/

void Lab6Window::_initDescriptorSetLayouts ()
{
    int nObjs = this->_objs.size();
    assert (nObjs > 0);

    // allocate the descriptor-set pool.  We have the following descriptors:
    //   * one GeomUBO + one FinalUBO per frame
    //   * one color-map sampler-descriptor per object
    // Note that the G-buffer has its own pool for its sampler descriptors
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {
            vk::DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                2*cs237::kMaxFrames),
            vk::DescriptorPoolSize
                (vk::DescriptorType::eCombinedImageSampler,
                nObjs+GBuffer::kNumBuffers)
        };
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        2*cs237::kMaxFrames+nObjs+2, /* max sets */
        poolSizes); /* pool sizes */
    this->_descPool = this->device().createDescriptorPool(poolInfo);

    // create the descriptor-set layout for the geometry-pass per-frame UBO
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            kGeomUBBind, /* binding */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eVertex, /* stages */
            nullptr); /* immutable samplers */
        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_geomDSLayout = this->device().createDescriptorSetLayout(layoutInfo);
    }

    // create the descriptor-set layout for the geometry-pass per-drawable sampler
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            kColorSamplerBind, /* binding */
            vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr); /* immutable samplers */

        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_drawableDSLayout = this->device().createDescriptorSetLayout(layoutInfo);
    }

    // create the descriptor-set layouts for the final-pass lighting UBO
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            kFinalUBBind, /* binding */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr); /* immutable samplers */
        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBinding);
        this->_finalDSLayout = this->device().createDescriptorSetLayout(layoutInfo);
    }

    // initialize the G-buffer descriptor pool and layout
    this->_gBuf->initDescriptorSetLayout();

}

void Lab6Window::_initDescriptorSets ()
{
    /* NOTE: the per-frame descriptor sets are initialized when the per-frame
     * data is created.
     */

    // create and initialize the per-drawable descriptor sets
    for (auto obj : this->_objs) {
        obj->initDescriptor (this->_descPool, this->_drawableDSLayout);
    }

    // create and initialize the G-buffer sampler descriptor set
    this->_gBuf->initDescriptorSet();

}

/******************** Render Pass Initialization ********************/

void Lab6Window::_initGeomRenderPass ()
{
    if (this->app()->verbose() || this->app()->debug()) {
        std::cout << "# Lab6Window::_initGeomRenderPass\n";
    }

    std::vector<vk::AttachmentDescription> atDescs;
    std::vector<vk::AttachmentReference> atRefs;

    this->_gBuf->initAttachments (atDescs, atRefs);
    assert (atDescs.size() == GBuffer::kNumBuffers && "something strange is going on");

    // the attachment for the depth buffer
    vk::AttachmentDescription dbDesc{
            {}, /* flags */
            this->_swap.dsBuf->format, /* format */
            vk::SampleCountFlagBits::e1, /* samples */
            vk::AttachmentLoadOp::eClear, /* load op */
            vk::AttachmentStoreOp::eStore, /* store op */
            vk::AttachmentLoadOp::eDontCare, /* stencil load op */
            vk::AttachmentStoreOp::eDontCare, /* stencil store op */
            vk::ImageLayout::eUndefined, /* initial layout */
            vk::ImageLayout::eDepthStencilAttachmentOptimal /* final layout */
        };
    vk::AttachmentReference dbRef{
            GBuffer::kNumBuffers,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        };

    // subpass for output
    vk::SubpassDescription subpass(
        {}, /* flags */
        vk::PipelineBindPoint::eGraphics, /* pipeline bind point */
        0, nullptr, /* input attachments */
        GBuffer::kNumBuffers, &(atRefs[0]), /* color attachments */
        nullptr, /* resolve attachments */
        &dbRef, /* depth-stencil attachment */
        0, nullptr); /* preserve attachments */

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL, /* src subpass */
        0, /* dst subpass */
        vk::PipelineStageFlagBits::eColorAttachmentOutput, /* src stage mask */
        vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dst stage mask */
        {}, /* src access mask */
        { vk::AccessFlagBits::eColorAttachmentWrite }, /* dst access mask */
        {}); /* dependency flags */

    // add depth buffer to attachments
    atDescs.push_back(dbDesc);
    vk::RenderPassCreateInfo renderPassInfo(
        {}, /* flags */
        atDescs, /* attachments */
        subpass, /* subpasses_ */
        dependency); /* dependencies */

    this->_geomPass.renderPass = this->device().createRenderPass(renderPassInfo);

}

void Lab6Window::_initFinalRenderPass ()
{
    if (this->app()->verbose() || this->app()->debug()) {
        std::cout << "# Lab6Window::_initFinalRenderPass\n";
    }

    // the final pass only renders to the color buffer, but since the swap buffer
    // was created with a depth buffer, there are still two attachments.
    std::vector<vk::AttachmentDescription> atDescs;
    std::vector<vk::AttachmentReference> atRefs;
    this->_initAttachments (atDescs, atRefs);

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

    this->_finalPass.renderPass = this->device().createRenderPass(renderPassInfo);

}

/******************** Graphics Pipeline Initialization ********************/

void Lab6Window::_initGeomPipeline ()
{
    if (this->app()->verbose() || this->app()->debug()) {
        std::cout << "# Lab6Window::_initGeomPipeline\n";
    }

    // create the pipeline for the geometry pass
    vk::PushConstantRange pcRange = {
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(PC),
        };
    std::array<vk::DescriptorSetLayout,2> dsLayouts = {
            this->_geomDSLayout, /* set 0 */
            this->_drawableDSLayout /* set 1 */
        };

    vk::PipelineLayoutCreateInfo layoutInfo(
        {}, /* flags */
        dsLayouts, /* set layouts */
        pcRange); /* push constants */
    this->_geomPass.pipelineLayout = this->device().createPipelineLayout(layoutInfo);

    // load the shaders
    vk::ShaderStageFlags stages =
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    std::string shaderName = kShaderDir + "geom-pass";
    auto shaders = new cs237::Shaders(this->device(), shaderName, stages);

    // vertex input info
    auto vertexInfo = cs237::vertexInputInfo (
        Vertex::getBindingDescriptions(),
        Vertex::getAttributeDescriptions());

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    // we need one blending mode per G-buffer attachment
    std::array<vk::PipelineColorBlendAttachmentState, GBuffer::kNumBuffers> colorBlendAttachments = {
            vk::PipelineColorBlendAttachmentState(
                VK_FALSE, /* blend enable */
                vk::BlendFactor::eZero, vk::BlendFactor::eZero,
                vk::BlendOp::eAdd, /* color blend op */
                vk::BlendFactor::eZero, vk::BlendFactor::eZero,
                vk::BlendOp::eAdd, /* alpha blend op */
                vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags), /* color write mask */
            vk::PipelineColorBlendAttachmentState(
                VK_FALSE, /* blend enable */
                vk::BlendFactor::eZero, vk::BlendFactor::eZero,
                vk::BlendOp::eAdd, /* color blend op */
                vk::BlendFactor::eZero, vk::BlendFactor::eZero,
                vk::BlendOp::eAdd, /* alpha blend op */
                vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags) /* color write mask */
        };

    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {}, /* flags */
        VK_FALSE, /* logic-op enable */
        vk::LogicOp::eClear, /* logic op */
        colorBlendAttachments, /* attachments */
        { 0.0f, 0.0f, 0.0f, 0.0f }); /* blend constants */

    this->_geomPass.pipeline = this->_app->createPipeline(
        shaders,
        vertexInfo,
        vk::PrimitiveTopology::eTriangleList,
        false,
        // the viewport and scissor rectangles are specified dynamically, but we need
        // to specify the counts
        vk::ArrayProxy<vk::Viewport>(1, nullptr), /* viewports */
        vk::ArrayProxy<vk::Rect2D>(1, nullptr), /* scissor rects */
        false,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        // we are following the OpenGL convention for front faces
        vk::FrontFace::eCounterClockwise,
        this->_geomPass.pipelineLayout,
        this->_geomPass.renderPass,
        0,
        colorBlending,
        dynamicStates);

    cs237::destroyVertexInputInfo (vertexInfo);
    delete shaders;
}

void Lab6Window::_initFinalPipeline ()
{
    if (this->app()->verbose() || this->app()->debug()) {
        std::cout << "# Lab6Window::_initFinalPipeline\n";
    }

    // create the pipeline layout for view rendering
    std::array<vk::DescriptorSetLayout,2> dsLayouts = {
            this->_finalDSLayout, /* set 0 */
            this->_gBuf->descriptorSetLayout() /* set 1 */
        };

    vk::PipelineLayoutCreateInfo layoutInfo(
        {}, /* flags */
        dsLayouts, /* set layouts */
        {}); /* push constants */
    this->_finalPass.pipelineLayout = this->device().createPipelineLayout(layoutInfo);

    // load the shaders
    vk::ShaderStageFlags stages =
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    std::string shaderName = kShaderDir + "final-pass";
    auto shaders = new cs237::Shaders(this->device(), shaderName, stages);

    // vertex input info; use defaults, since there is no vertex data
    vk::PipelineVertexInputStateCreateInfo vertexInfo;

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    this->_finalPass.pipeline = this->_app->createPipeline(
        shaders,
        vertexInfo,
        vk::PrimitiveTopology::eTriangleList,
        // the viewport and scissor rectangles are specified dynamically, but we need
        // to specify the counts
        vk::ArrayProxy<vk::Viewport>(1, nullptr), /* viewports */
        vk::ArrayProxy<vk::Rect2D>(1, nullptr), /* scissor rects */
        vk::PolygonMode::eFill,
        // Note: we are using the Vulkan viewport and winding-order conventions
        // for the full-screen triangle (see final-pass.vert)
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        this->_finalPass.pipelineLayout,
        this->_finalPass.renderPass,
        0,
        dynamicStates);

    delete shaders;
}

/******************** Frame buffers ********************/

void Lab6Window::_initGeomFramebuffer (uint32_t wid, uint32_t ht)
{
    if (this->app()->verbose() || this->app()->debug()) {
        std::cout << "# Lab6Window::_initGeomFramebuffer\n";
    }

    auto attachments = this->_gBuf->attachments();

    // we also need the depth buffer, although it is not part of the G-buffer
    // in this lab
    auto dbImageView = this->_swap.depthImageView();
    assert (dbImageView.has_value() && "missing depth buffer");
    attachments.push_back(*dbImageView);

    vk::FramebufferCreateInfo fbInfo (
        {}, /* flags */
        this->_geomPass.renderPass, /* render pass */
        attachments, /* attachments */
        wid, /* width */
        ht, /* height */
        1); /* layers */

    this->_geomFramebuffer = this->_app->device().createFramebuffer (fbInfo);
}

/******************** Rendering ********************/

void Lab6Window::_recordCommandBuffer (Lab6Window::FrameData *frame)
{
    auto cmdBuf = frame->cmdBuf;

    cmdBuf.reset();

    vk::ClearValue depthClearValue = vk::ClearDepthStencilValue(1.0f, 0);

    vk::CommandBufferBeginInfo beginInfo{};
    cmdBuf.begin(beginInfo);

    /***** Geometry Pass *****/

    // the clear values for the GBuffer
    auto clearVals = this->_gBuf->clearValues();
    // add the clear value for the depth buffer, which is not part of the GBuffer
    // in this lab.
    clearVals.push_back(vk::ClearDepthStencilValue(1.0f, 0));

    vk::RenderPassBeginInfo geomPassInfo(
        this->_geomPass.renderPass,
        this->_geomFramebuffer,
        { {0, 0}, this->_swap.extent }, /* render area */
        clearVals);
    cmdBuf.beginRenderPass(geomPassInfo, vk::SubpassContents::eInline);

    /*** BEGIN COMMANDS ***/
    {
        // bind the geometry-pass pipeline
        cmdBuf.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            this->_geomPass.pipeline);

        // set the geometry-pass viewport using the OpenGL convention
        this->_setViewportCmd (cmdBuf, true);

        // bind the descriptor for the geometry-pass UBO
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            this->_geomPass.pipelineLayout,
            kGeomUBSet,
            frame->geomDS,
            nullptr);

        // draw the objects
        for (auto obj : this->_objs) {
            // bind the descriptor set for the color-map sampler
            obj->bindDescriptorSet(cmdBuf, this->_geomPass.pipelineLayout);
            // set the push constants
            obj->emitPushConstants(cmdBuf,
                this->_geomPass.pipelineLayout,
                vk::ShaderStageFlagBits::eVertex);
            // render the drawable to the shadow buffer
            obj->draw (cmdBuf);
        }
    }
    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    /***** Final Pass *****/

    std::array<vk::ClearValue,2> clearValues = {
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f), /* clear the window to black */
            depthClearValue
        };
    vk::RenderPassBeginInfo renderPassInfo(
        this->_finalPass.renderPass,
        this->_swap.fBufs[frame->index],
        { {0, 0}, this->_swap.extent }, /* render area */
        clearValues);
    cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    /*** BEGIN COMMANDS ***/
    {
        cmdBuf.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            this->_finalPass.pipeline);

        // set the final-pass viewport using the Vulkan convention
        // (see the _initFinalPipeline method and final-pass.vert)
        this->_setViewportCmd (cmdBuf, false);

        std::array<vk::DescriptorSet, 2> finalDS = {
                frame->finalDS,                     // set 0
                this->_gBuf->descriptorSet()        // set 1
            };

        // bind the descriptor for the geometry buffer textures
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            this->_finalPass.pipelineLayout,
            0,
            finalDS,
            nullptr);

        // draw the screen triangle
        cmdBuf.draw(3, 1, 0, 0);
    }
    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

void Lab6Window::draw ()
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

void Lab6Window::reshape (int wid, int ht)
{
    // invoke the super-method reshape method
    this->cs237::Window::reshape(wid, ht);

    // resize the G-buffer
    this->_gBuf->resize(wid, ht);

    // recreate the new framebuffers
    this->_swap.initFramebuffers (this->_finalPass.renderPass);
    this->_initGeomFramebuffer(wid, ht);

    // update the projection matrix
    this->_setProjMat();

    // mark per-frame UBOs as invalid
    this->_invalidateFrameUBOs();

}

void Lab6Window::key (int key, int scancode, int action, int mods)
{
  // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_A:  // 'a' or 'A' ==> show the contents of the albedo buffer
                if (this->_finalUBCache.mode != kShowAlbedoBuffer) {
                    if (this->app()->verbose()) {
                        std::cout << "# switch mode: show albedo buffer\n";
                    }
                    this->_finalUBCache.mode = kShowAlbedoBuffer;
                    this->_invalidateFrameUBOs();
                }
                break;

            case GLFW_KEY_D:  // 'd' or 'D' ==> deferred rendering of the scene
                if (this->_finalUBCache.mode != kRenderScene) {
                    if (this->app()->verbose()) {
                        std::cout << "# switch mode: deferred rendering of scene\n";
                    }
                    this->_finalUBCache.mode = kRenderScene;
                    this->_invalidateFrameUBOs();
                }
                break;

            case GLFW_KEY_N:  // 'n' or 'N' ==> show the contents of the normal buffer
                if (this->_finalUBCache.mode != kShowNormalBuffer) {
                    if (this->app()->verbose()) {
                        std::cout << "# switch mode: show normal buffer\n";
                    }
                    this->_finalUBCache.mode = kShowNormalBuffer;
                    this->_invalidateFrameUBOs();
                }
                break;

            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
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

/******************** Lab6Window::FrameData struct ********************/

/* virtual */
cs237::Window::FrameData *Lab6Window::_allocFrameData (Window *w) /* override */
{
    return new Lab6Window::FrameData (w);
}

Lab6Window::FrameData::FrameData (Window *w)
: Window::FrameData(w), valid(false)
{
    auto win = reinterpret_cast<Lab6Window *>(w);
    auto device = win->device();

    // allocate the UBOs
    this->geomUBO = new GeomUBO_t(win->app());
    this->finalUBO = new FinalUBO_t(win->app());

    // allocate and write the geometry-pass UBO descriptor set
    {
        vk::DescriptorSetAllocateInfo allocInfo(
            win->_descPool,
            win->_geomDSLayout);
        this->geomDS = (device.allocateDescriptorSets(allocInfo))[0];

        // info about the UBOs
        auto bufferInfo = this->geomUBO->descInfo();

        // write operation for the uniform buffer
        vk::WriteDescriptorSet writeDS(
            this->geomDS, /* descriptor set */
            kGeomUBBind, /* binding */
            0, /* array element */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            nullptr, /* image info */
            bufferInfo, /* buffer info */
            nullptr); /* texel buffer view */

        // write the descriptor set
        device.updateDescriptorSets (writeDS, nullptr);
    }

    // allocate and write the final-pass UBO descriptor set
    {
        vk::DescriptorSetAllocateInfo allocInfo(
            win->_descPool,
            win->_finalDSLayout);
        this->finalDS = (device.allocateDescriptorSets(allocInfo))[0];

        // info about the UBOs
        auto bufferInfo = this->finalUBO->descInfo();

        // write operation for the uniform buffer
        vk::WriteDescriptorSet writeDS(
            this->finalDS, /* descriptor set */
            0, /* binding */
            0, /* array element */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            nullptr, /* image info */
            bufferInfo, /* buffer info */
            nullptr); /* texel buffer view */

        // write the descriptor set
        device.updateDescriptorSets (writeDS, nullptr);
    }
}

/* virtual */
Lab6Window::FrameData::~FrameData ()
{
    delete this->geomUBO;
    delete this->finalUBO;
}


/******************** Lab6 class ********************/

Lab6::Lab6 (std::vector<std::string> const &args)
  : cs237::Application (args, "CS237 Lab 5")
{ }

Lab6::~Lab6 ()
{ }

void Lab6::run ()
{
    auto win = new Lab6Window (this);

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
    Lab6 app(args);

    // print information about the keyboard commands
    std::cout
        << "# Lab 6 User Interface\n"
        << "#  'a' to show the contents of the albedo buffer\n"
        << "#  'd' for deferred rendering of scene (initial mode)\n"
        << "#  'n' to show the contents of the normal buffer\n"
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
