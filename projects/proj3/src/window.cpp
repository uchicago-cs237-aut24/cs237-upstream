/*! \file window.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "window.hpp"
#include "vertex.hpp"

/// constants to define the near and far planes of the view frustum
constexpr float kNearZ = 0.5;   // how close to the origin you can get
constexpr float kFarZ = 500.0f;  // distance to far plane

Proj3Window::Proj3Window (Proj3 *app)
  : cs237::Window (
        app,
        // resizable window with depth buffer
        cs237::CreateWindowInfo(
            app->scene()->width(),
            app->scene()->height(),
            "", true, true, false)),
    _mode(RenderMode::eTextureWOShadows)
{
    auto scene = app->scene();

    // initialize the camera from the scene
    this->_camPos = scene->cameraPos();
    this->_camAt = scene->cameraLookAt();
    this->_camUp = scene->cameraUp();

    // initialize the per-frame cache
    this->_updateFrameUBCache();

    // initialize the scene and shadow UBOs
    SceneUB sceneUB;
    sceneUB.nLights = scene->numLights();
    sceneUB.ambLight = scene->ambientLight();
    for (int i = 0;  i < scene->numLights();  ++i) {
        // initialize the light info in the Scene UB
        sceneUB.lights[i].position = scene->light(i).pos;
        sceneUB.lights[i].direction = scene->light(i).dir;
        sceneUB.lights[i].intensity = scene->light(i).intensity;
        sceneUB.lights[i].atten = glm::vec3(
            scene->light(i).k0,
            scene->light(i).k1,
            scene->light(i).k2);
        sceneUB.lights[i].cosCutoff = ::cos(glm::radians(scene->light(i).cutoff));
        sceneUB.lights[i].exponent = scene->light(i).exponent;
        // initialize the shadow UBO for the light
        ShadowUB shadowUB;
        shadowUB.lightID = i;
        shadowUB.shadowFactor = scene->shadowFactor();
        /** HINT: construct the world to light transformation here */
        shadowUB.shadowMat = glm::mat4();
        this->_shadowUBO.push_back(new ShadowUBO (this->app(), shadowUB));
    }
    this->_sceneUBO = new SceneUBO (this->app(), sceneUB);

    // initialize the meshes for the scene objects
    this->_initMeshes(scene);

    this->_initRenderInfo ();

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers (this->_noShadowsRInfo.renderPass);

    // initialize the descriptor pool and layouts for the uniform buffers
    this->_initDescriptorPools();

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Proj3Window::~Proj3Window ()
{
    auto device = this->device();

    this->_noShadowsRInfo.destroy (device);
    this->_ambientWShadowsRInfo.destroy (device);
    this->_shadowMapRInfo.destroy (device);
    this->_lightRInfo.destroy (device);

    /** HINT: release any other allocated objects */

}

void Proj3Window::_initRenderInfo ()
{
    /** HINT: initialize the render-pass, pipelines, and other information for
     ** the different rendering-info structs.
     ** Remember that you will need to set the blending mode for the lighting
     ** passes when shadows are enabled.
     **/
}

void Proj3Window::_initDescriptorPools ()
{
    auto device = this->device();
    int nObjs = this->_objs.size();
    assert (nObjs > 0);

    // allocate the descriptor-set pool.  We have the following descriptor sets:
    //   * a single scene UBO
    //   * a UBO per light for shadowing (max 4)
    //   * a UBO per frame
    //   * two samplers per object
    //   * the depth sampler
    //
    int nUBOs = 1 + 4 + cs237::kMaxFrames;
    int nSamplers = 2 * this->_objs.size() + 1;
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, nUBOs),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, nSamplers)
        };
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        nUBOs + nSamplers, /* max sets */
        poolSizes); /* pool sizes */

    this->_descPool = device.createDescriptorPool(poolInfo);

    /** HINT: create the scene-data descriptor-set layout, which
     ** has a single binding
     **/

}

void Proj3Window::_initMeshes (const Scene *scene)
{
    /** HINT: put code to construct the meshes and instances
     *  from the scene here
     */
}

void Proj3Window::_updateFrameUBCache ()
{
    // update the parts of the vertex UB cache that depend on
    // the camera position and viewport
    this->_frameUBCache.viewM = glm::lookAt(this->_camPos, this->_camAt, this->_camUp);
    this->_frameUBCache.projM = glm::perspectiveFov(
        glm::radians(this->_scene()->horizontalFOV()),
        float(this->_wid), float(this->_ht),
        kNearZ, kFarZ);
    this->_frameUBCache.enableNormalMap =
        (normalMapEnabled(this->_mode) ? VK_TRUE : VK_FALSE);
}

void Proj3Window::_invalidateFrameUBOs ()
{
    // mark the per-frame UBOs as being out-of-sync with the cache
    for (auto it : this->_frames) {
        reinterpret_cast<Proj3Window::FrameData *>(it)->valid = false;
    }

    // recompute the correct values and update the cache
    this->_updateFrameUBCache();
}

void Proj3Window::_recordCommandBuffer (Proj3Window::FrameData *frame)
{
    auto cmdBuf = frame->cmdBuf;

    cmdBuf.reset();

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuf.begin(beginInfo);

    std::array<vk::ClearValue,2> clearValues = {
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f), /* clear the window to black */
            vk::ClearDepthStencilValue(1.0f, 0.0f)
        };

    if (shadowsEnabled(this->_mode)) {
        /** HINT: render the scene using ambient lighting and then, for each light,
         ** compute the shadow map and then use it to compute the light's contribution
         ** to the scene.
         **/
    } else {
        /** Rendering w/o shadows */
        vk::RenderPassBeginInfo renderPassInfo(
            this->_noShadowsRInfo.renderPass,
            this->_swap.fBufs[frame->index],
            { {0, 0}, this->_swap.extent }, /* render area */
            clearValues);

        cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        /*** BEGIN COMMANDS ***/

        // set the viewport using the OpenGL convention
        this->_setViewportCmd (cmdBuf, true);

        this->_noShadowsRInfo.bindPipelineCmd(cmdBuf);

        // bind per-frame descriptors
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            this->_noShadowsRInfo.pipelineLayout,
            0, /* first set */
            frame->ds, /* descriptor sets */
            nullptr);

        // render the objects in the scene
        for (auto it : this->_objs) {
            // bind the descriptors for the object
            cmdBuf.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                this->_noShadowsRInfo.pipelineLayout,
                1, /* second set */
                it->mesh->descSet, /* descriptor sets */
                nullptr);

            /** HINT: set the push constants for the mesh */

            it->mesh->draw (cmdBuf);
        }

        /*** END COMMANDS ***/

        cmdBuf.endRenderPass();
    }

    cmdBuf.end();

}

void Proj3Window::draw ()
{
    // next buffer from the swap chain
    auto sts = this->_acquireNextImage();
    if (sts != vk::Result::eSuccess) {
        ERROR("Unable to acquire next image");
    }

    auto frame = reinterpret_cast<Proj3Window::FrameData *>(this->_currentFrame());

    frame->resetFence();

    /** HINT: update the UBOs, if they are invalid */

    // record drawing commands
    this->_recordCommandBuffer (frame);

    // set up submission for the graphics queue
    frame->submitDrawingCommands();

    // set up submission for the presentation queue
    frame->present();
}

void Proj3Window::reshape (int wid, int ht)
{
    // invoke the super-method reshape method
    this->cs237::Window::reshape(wid, ht);

    // recreate the new framebuffers
    this->_swap.initFramebuffers (this->_noShadowsRInfo.renderPass);

    /** HINT: update the frame UB cache and invalidate the buffers */

}

void Proj3Window::key (int key, int scancode, int action, int mods)
{
    // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_N:  // 'n' or 'n' ==> switch to normal-mapping mode
                this->_mode = enableNormalMap(this->_mode);
                this->_invalidateFrameUBOs();
                break;
            case GLFW_KEY_T:  // 't' or 'T' ==> switch to texturing mode
                this->_mode = disableNormalMap(this->_mode);
                this->_invalidateFrameUBOs();
                break;
            case GLFW_KEY_S:  // 's' or 'S' ==> toggle shadows
                this->_mode = toggleShadows(this->_mode);
                this->_invalidateFrameUBOs();
                break;
            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
                this->_invalidateFrameUBOs();
                break;

/** HINT: add cases for optional camera controls */

            default: // ignore all other keys
                return;
        }
    }

}

/******************** Window::FrameData struct ********************/

/* virtual */
cs237::Window::FrameData *Proj3Window::_allocFrameData (cs237::Window *w) /* override */
{
    return new Proj3Window::FrameData (w);
}

Proj3Window::FrameData::FrameData (cs237::Window *w)
: cs237::Window::FrameData(w)
{
    auto win = reinterpret_cast<Proj3Window *>(w);
    auto device = win->device();

    /** HINT: allocate the UBO and descriptor set for the scene-data UBO */

    /** HINT: write the descriptor sets */
}

/* virtual */
Proj3Window::FrameData::~FrameData ()
{
    delete this->ubo;
}
