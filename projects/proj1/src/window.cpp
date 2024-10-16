/*! \file window.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "window.hpp"
#include "app.hpp"
#include "cs237/cs237.hpp"
#include "obj.hpp"
#include "render-modes.hpp"
#include "shader-uniforms.hpp"
#include "vertex.hpp"

/// constants to define the near and far planes of the view frustum
constexpr float kNearZ = 0.5;   // how close to the origin you can get
constexpr float kFarZ = 500.0f;  // distance to far plane

Proj1Window::Proj1Window (Proj1 *app)
  : cs237::Window (
        app,
        // resizable window with depth buffer
        cs237::CreateWindowInfo(
            app->scene()->width(),
            app->scene()->height(),
            "", true, true, false)),
    _mode(RenderMode::eWireframe)
{
    // initialize the camera from the scene
    this->_camPos = app->scene()->cameraPos();
    this->_camAt = app->scene()->cameraLookAt();
    this->_camUp = app->scene()->cameraUp();

    this->_initMeshes(app->scene());

    this->_initRenderPass ();

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers (this->_renderPass);

    // initialize the descriptor pools and layout
    this->_initDescriptorPools();

    /** HINT: add additional initialization for renderers */

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

/* virtual */
void Proj1Window::_init () /* override */
{
    /** HINT: additonal initialization */
}

Proj1Window::~Proj1Window ()
{
    auto device = this->device();

    device.destroyRenderPass(this->_renderPass);
    device.destroyDescriptorPool(this->_descPool);
    device.destroyDescriptorSetLayout(this->_dsLayout);

    for (int i = 0;  i < kNumRenderModes;  ++i) {
        delete this->_renderer[i];
    }

    /** HINT: release any other allocated objects */

}

void Proj1Window::_initRenderPass ()
{
    // initialize the attachment descriptors and references
    std::vector<vk::AttachmentDescription> atDescs;
    std::vector<vk::AttachmentReference> atRefs;
    this->_initAttachments (atDescs, atRefs);
    assert (atRefs.size() == 2);  /* expect a depth buffer */

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

void Proj1Window::_initRenderers ()
{
    /** HINT: create the renderers and initialize the `_renderer` array */
}

void Proj1Window::_initDescriptorPools ()
{
    auto device = this->device();

    // create the descriptor pool; we allocate one descriptor set per frame,
    // so we use the number of frames in the swap buffer as the max
    vk::DescriptorPoolSize poolSize(
        vk::DescriptorType::eUniformBuffer,
        cs237::kMaxFrames);
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        cs237::kMaxFrames, /* max sets */
        poolSize); /* pool sizes */
    this->_descPool = device.createDescriptorPool(poolInfo);

    // layout for the Scene UBO
    vk::DescriptorSetLayoutBinding layoutBinding(
        0, /* binding */
        vk::DescriptorType::eUniformBuffer,
        1, /* descriptor count */
        vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment,
        nullptr); /* samplers */
    vk::DescriptorSetLayoutCreateInfo layoutInfo(
        {}, /* flags */
        layoutBinding); /* bindings */
    this->_dsLayout = this->device().createDescriptorSetLayout(layoutInfo);

}

void Proj1Window::_initMeshes (const Scene *scene)
{
    /** HINT: put code to construct the meshes and instances
     *  from the scene here
     */
}

void Proj1Window::_recordCommandBuffer (uint32_t imageIdx)
{
    Renderer *rp = this->_renderer[static_cast<int>(this->_mode)];

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuf.begin(beginInfo);

    std::array<vk::ClearValue,2> clearValues = {
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f), /* clear the window to black */
            vk::ClearDepthStencilValue(1.0f, 0.0f)
        };
    vk::RenderPassBeginInfo renderPassInfo(
        this->_renderPass,
        this->_swap.fBufs[this->_currentFrame()->index],
        { {0, 0}, this->_swap.extent }, /* render area */
        clearValues);

    cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    /*** BEGIN COMMANDS ***/

    // set the viewport using the OpenGL convention
    this->_setViewportCmd (cmdBuf, true);

    rp->bindPipelineCmd(cmdBuf);

    /** HINT: bind the descriptor set for the uniform buffer **/

    // render the objects in the scene
    for (auto it : this->_objs) {
        auto mesh = it->mesh;

        /** HINT: set the push constants for the mesh */

        mesh->draw (cmdBuf);
    }

    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

void Proj1Window::draw ()
{
    // next buffer from the swap chain
    auto sts = this->_acquireNextImage();
    if (sts != vk::Result::eSuccess) {
        ERROR("inable to acquire next image");
    }

    // cast the current frame to our FrameData type
    Proj1Window::FrameData *frame = reinterpret_cast<FrameData *>(this->_currentFrame());

    frame->resetFence();

    // update the UBO, if necessary
    if (! frame->uboInfo.valid) {
        frame->uboInfo.ubo->copyTo(this->_ubCache);
        frame->uboInfo.valid = true;
    }

    this->_recordCommandBuffer ();

    // set up submission for the graphics queue
    frame->submitDrawingCommands();

    // set up submission for the presentation queue
    frame->present();
}

void Proj1Window::reshape (int wid, int ht)
{
    // invoke the super-method reshape method
    this->cs237::Window::reshape(wid, ht);

    // recreate the swapchain framebuffers
    this->_swap.initFramebuffers (this->_renderPass);

    // update the uniform cache and invalidate the buffers
    this->_updateUBCache();
}

void Proj1Window::key (int key, int scancode, int action, int mods)
{
  // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_F:  // 'f' or 'F' ==> switch to flat mode
                this->_mode = RenderMode::eFlatShading;
                break;
            case GLFW_KEY_G:  // 'g' or 'G' ==> switch to Gouraud mode
                this->_mode = RenderMode::eGouraudShading;
                break;
            case GLFW_KEY_P:  // 'p' or 'P' ==> switch to Phong mode
                this->_mode = RenderMode::ePhongShading;
                break;
            case GLFW_KEY_W:  // 'w' or 'W' ==> switch to wireframe mode
                this->_mode = RenderMode::eWireframe;
                break;
            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
                break;

/** HINT: add cases for optional camera controls */

            default: // ignore all other keys
                return;
        }
    }

}

/* virtual */
cs237::Window::FrameData *Proj1Window::_allocFrameData (Window *w) /* override */
{
    return new Proj1Window::FrameData (w);
}

Proj1Window::FrameData::FrameData (Window *w)
: Window::FrameData(w), uboInfo()
{
    auto win = reinterpret_cast<Proj1Window *>(w);
    auto device = win->device();

    // the descriptor set allocation info used in defining descriptor set
    vk::DescriptorSetAllocateInfo allocInfo(
        win->_descPool,
        win->_dsLayout);

    // allocate the UBO for the frame
    auto ubo = new SceneUBO_t(win->_app);
    // allocate a descriptor set for the frame
    auto ds = (device.allocateDescriptorSets(allocInfo))[0];
    // connect the uniform buffer to the descriptor set
    auto descInfo = ubo->descInfo();
    vk::WriteDescriptorSet descriptorWrite(
        ds, /* descriptor set */
        0, /* binding */
        0, /* array element */
        vk::DescriptorType::eUniformBuffer, /* descriptor type */
        nullptr, /* image info */
        descInfo, /* buffer info */
        nullptr); /* texel buffer view */
    device.updateDescriptorSets (descriptorWrite, nullptr);

    // initialize the uniform-buffer-object info
    this->uboInfo.valid = false;
    this->uboInfo.ubo = ubo;
    this->uboInfo.descSet = ds;

}

/* virtual */
Proj1Window::FrameData::~FrameData ()
{
    delete this->uboInfo.ubo;
}
