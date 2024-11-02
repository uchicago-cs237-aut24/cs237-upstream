/*! \file window.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 2
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "window.hpp"
#include "obj.hpp"
#include "render-modes.hpp"
#include "renderer.hpp"
#include "shader-uniforms.hpp"
#include "vertex.hpp"

/// constants to define the near and far planes of the view frustum
constexpr float kNearZ = 0.5;   // how close to the origin you can get
constexpr float kFarZ = 500.0f;  // distance to far plane

Proj2Window::Proj2Window (Proj2 *app)
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

    // initialize the cached lighting info
    this->_sceneUBCache.ambLight = this->_scene()->ambientLight();
    this->_sceneUBCache.lightDir = this->_scene()->lightDir();
    this->_sceneUBCache.lightColor = this->_scene()->lightIntensity();

    // initialize the scene-data cache
    this->_updateUBCache();

    // initialize the meshes for the scene objects
    this->_initMeshes(app->scene());

    this->_initRenderPass ();

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers (this->_renderPass);

    // initialize the descriptor pool and layouts for the uniform buffers
    this->_initDescriptorPools();

    // create the renderer objects
    this->_initRenderers();

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Proj2Window::~Proj2Window ()
{
    auto device = this->device();

    device.destroyRenderPass(this->_renderPass);
    device.destroyDescriptorPool(this->_descPool);
    device.destroyDescriptorSetLayout(this->_descSetLayout);

    /** HINT: release any other allocated objects */

}

void Proj2Window::_initRenderPass ()
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

void Proj2Window::_initDescriptorPools ()
{
    auto device = this->device();

    // create the descriptor pool; we have one descriptor per frame
    vk::DescriptorPoolSize poolSz(
        vk::DescriptorType::eUniformBuffer,
        cs237::kMaxFrames);
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        cs237::kMaxFrames, /* max sets */
        poolSz); /* pool sizes */
    this->_descPool = device.createDescriptorPool(poolInfo);

    /** HINT: create the scene-data descriptor-set layout, which
     ** has a single binding
     **/

}

void Proj2Window::_initRenderers ()
{
    Proj2 *app = reinterpret_cast<Proj2 *>(this->_app);

    // create the renderers
    /** HINT: create the renderer objects for each of the modes here */

}

void Proj2Window::_initMeshes (const Scene *scene)
{
    /** HINT: put code to construct the meshes and instances
     *  from the scene here
     */
}

void Proj2Window::_updateUBCache ()
{
    // update the parts of the vertex UB cache that depend on
    // the camera position and viewport
    this->_sceneUBCache.viewM = glm::lookAt(this->_camPos, this->_camAt, this->_camUp);
    this->_sceneUBCache.projM = glm::perspectiveFov(
        glm::radians(this->_scene()->horizontalFOV()),
        float(this->_wid), float(this->_ht),
        kNearZ, kFarZ);
}

void Proj2Window::_invalidateSceneUBOs ()
{
    // mark the uniform buffers as being out-of-sync with the cache
    for (auto it : this->_frames) {
        reinterpret_cast<Proj2Window::FrameData *>(it)->valid = false;
    }

    // recompute the correct values and update the cache
    this->_updateUBCache();
}

void Proj2Window::_invalidateVertexUBOs ()
{
    // mark the uniform buffers as being out-of-sync with the cache
    for (auto it : this->_frames) {
        reinterpret_cast<Proj2Window::FrameData *>(it)->vertexUBO.valid = false;
    }

    // recompute the correct values and update the cache
    this->_updateVertexUBCache();
}

void Proj2Window::_recordCommandBuffer (Proj2Window::FrameData *frame)
{
    Renderer *rp = this->_renderer[static_cast<int>(this->_mode)];

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

    // set the viewport using the OpenGL convention
    this->_setViewportCmd (cmdBuf, true);

    rp->bindPipelineCmd(cmdBuf);

    // bind per-frame descriptors
    rp->bindFrameDescriptorSets (cmdBuf, frame->ds);

    // render the objects in the scene
    for (auto it : this->_objs) {
        // bind the descriptors for the object
        rp->bindMeshDescriptorSets (cmdBuf, it);

        /** HINT: set the push constants for the mesh */

        it->mesh->draw (cmdBuf);
    }

    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

void Proj2Window::draw ()
{
    // next buffer from the swap chain
    auto sts = this->_acquireNextImage();
    if (sts != vk::Result::eSuccess) {
        ERROR("Unable to acquire next image");
    }

    auto frame = reinterpret_cast<Proj2Window::FrameData *>(this->_currentFrame());

    frame->resetFence();

    /** HINT: update the UBOs, if they are invalid */

    // record drawing commands
    this->_recordCommandBuffer (frame);

    // set up submission for the graphics queue
    frame->submitDrawingCommands();

    // set up submission for the presentation queue
    frame->present();
}

void Proj2Window::reshape (int wid, int ht)
{
    // invoke the super-method reshape method
    this->cs237::Window::reshape(wid, ht);

    // recreate the new framebuffers
    this->_swap.initFramebuffers (this->_renderPass);

    /** HINT: update the uniform cache and invalidate the buffers */

}

void Proj2Window::key (int key, int scancode, int action, int mods)
{
    // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_F:  // 'f' or 'F' ==> switch to flat mode
                this->_mode = RenderMode::eFlatShading;
                break;
            case GLFW_KEY_N:  // 'n' or 'n' ==> switch to normal-mapping mode
                this->_mode = RenderMode::eNormalMapShading;
                break;
            case GLFW_KEY_T:  // 't' or 'T' ==> switch to texturing mode
                this->_mode = RenderMode::eTextureShading;
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

/******************** Window::FrameData struct ********************/

/* virtual */
cs237::Window::FrameData *Proj2Window::_allocFrameData (cs237::Window *w) /* override */
{
    return new Proj2Window::FrameData (w);
}

Proj2Window::FrameData::FrameData (cs237::Window *w)
: cs237::Window::FrameData(w)
{
    auto win = reinterpret_cast<Proj2Window *>(w);
    auto device = win->device();

    /** HINT: allocate the UBO and descriptor set for the scene-data UBO */

    /** HINT: write the descriptor sets */
}

/* virtual */
Proj2Window::FrameData::~FrameData ()
{
    delete this->ubo;
}
