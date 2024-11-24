/*! \file window.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 4
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "window.hpp"
#include "cs237/cs237.hpp"
#include "mesh.hpp"
#include "render-modes.hpp"
#include "scene.hpp"
#include "shader-uniforms.hpp"
#include "vertex.hpp"

const std::string kShaderDir = PROJ4_BINARY_ROOT "/shaders/";

/// we use vertex + fragment shader stages for all renderers
constexpr vk::ShaderStageFlags kStages =
    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

Proj4Window::Proj4Window (Proj4 *app)
  : cs237::Window (
        app,
        // resizable window with depth buffer
        cs237::CreateWindowInfo(
            app->scene()->width(),
            app->scene()->height(),
            "", true, true, false)),
    _renderFlags()
{
    auto scene = app->scene();

    // initialize the camera from the scene
    this->_camPos = scene->cameraPos();
    this->_camAt = scene->cameraLookAt();
    this->_camUp = scene->cameraUp();
    this->_angle = 0.0;
    this->_radius = glm::length(scene->cameraLookAt() - scene->cameraPos());

    this->_setCameraPos ();
    this->_setProjMat();

    // initialize the meshes for the scene objects
    this->_initMeshes(scene);

    // initialize the descriptor pool and layouts for the uniform buffers
    this->_initDescriptorPools();

    // initialize the lighting UBO
    this->_initLighting ();

    this->_initForwardRenderInfo ();

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers (this->_renderPass);

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Proj4Window::~Proj4Window ()
{
    auto device = this->device();

/** HINT: add code to release any additional resources that
 ** you have allocated.
 **/

    this->_wireFramePipeline.destroy (device);
    this->_texturePipeline.destroy (device);
    device.destroyRenderPass(this->_renderPass);

    // clean up other resources
    device.destroyDescriptorPool(this->_descPool);
    device.destroyDescriptorSetLayout(this->_lightingLayout);
    delete this->_lightingUBO;
    delete this->_meshFactory;

    // delete the mesh data
    for (auto inst : this->_objs) {
        delete inst;
    }
    for (auto mesh : this->_meshes) {
        delete mesh;
    }

}

void Proj4Window::_initMeshes (const Scene *scene)
{
    Proj4 *app = reinterpret_cast<Proj4 *>(this->_app);

    // first we count the number of meshes so that we can create the factory
    int nMeshes = 1; /* ground */
    for (auto model = scene->beginModels();  model != scene->endModels();  ++model) {
        nMeshes += (*model)->numGroups();
    }
    this->_meshFactory = new MeshFactory(app, nMeshes);

    // create the meshes and record the start mesh index for each model
    std::vector<int> modelFirstGrpIdx;
    modelFirstGrpIdx.reserve(scene->numModels());
    this->_meshes.reserve(scene->numModels());
    for (auto model = scene->beginModels();  model != scene->endModels();  ++model) {
        modelFirstGrpIdx.push_back(this->_meshes.size());
        // load the meshes for the group
        for (int i = 0;  i < (*model)->numGroups();  ++i) {
            auto mesh = this->_meshFactory->alloc(*model, i);
            this->_meshes.push_back(mesh);
        }
    }

    // create the instances
    this->_objs.reserve (scene->numObjects());
    for (auto it = scene->beginObjs();  it!= scene->endObjs();  ++it) {
        int modelId = it->model;
        const OBJ::Model *model = scene->model(modelId);
        int firstMesh = modelFirstGrpIdx[modelId];
        Instance *inst = new Instance(it->toWorld, it->normToWorld());
        for (int i = 0;  i < model->numGroups();  ++i) {
            inst->pushMesh(this->_meshes[firstMesh+i]);
        }
        this->_objs.push_back(inst);
    }

    if (scene->ground() != nullptr) {
        // create the ground mesh
        auto groundMesh = this->_meshFactory->alloc(scene->ground());
        auto *groundInst = new Instance {
                groundMesh,
                glm::mat4(1), /* identity, since positions are in world space */
                glm::mat3(1) /* identity, since normals are in world space */
            };
        // add the ground to the vectors
        this->_meshes.push_back(groundMesh);
        this->_objs.push_back(groundInst);
    }

}

void Proj4Window::_initForwardRenderInfo ()
{
    auto dev = this->_app->device();

    /* create the render pass */
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

        // note that we create two separate render passes because we do not have
        // a mechanism for sharing them w/o double deletion
        this->_renderPass = dev.createRenderPass(renderPassInfo);
    }

    /* create the pipeline layouts for the wire-frame and texture renderers */
    {
        vk::PushConstantRange pcRange(
            vk::ShaderStageFlagBits::eVertex, /* just used in vertex shader */
            0, /* offset */
            sizeof(WireFramePushConsts));

        vk::PipelineLayoutCreateInfo layoutInfo(
            {}, /* flags */
            nullptr, /* no descriptor sets */
            pcRange); /* push constant ranges */
        this->_wireFramePipeline.layout = dev.createPipelineLayout(layoutInfo);


    }

    /* create the pipeline layout for the texture renderer */
    {
        std::array<vk::DescriptorSetLayout, 2> dsLayouts = {
                this->_lightingLayout, this->_meshFactory->materialLayout()
            };

        vk::PushConstantRange pcRange(
            vk::ShaderStageFlagBits::eVertex, /* just used in vertex shader */
            0, /* offset */
            sizeof(TexturePushConsts));

        vk::PipelineLayoutCreateInfo layoutInfo(
            {}, /* flags */
            dsLayouts, /* set layouts */
            pcRange); /* push constant ranges */
        this->_texturePipeline.layout = dev.createPipelineLayout(layoutInfo);
    }

    // vertex info is the same for both pipelines
    auto vertexInfo = cs237::vertexInputInfo (
        Vertex::getBindingDescriptions(),
        Vertex::getAttributeDescriptions());

    // dynamic state is the same for both pipelines
    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    /* create the pipeline for the wire-frame renderers */
    {
        auto shaders = new cs237::Shaders(
            dev,
            std::string(kShaderDir) + "wire-frame",
            kStages);

        this->_wireFramePipeline.pipe = this->_app->createPipeline (
            shaders,
            vertexInfo,
            vk::PrimitiveTopology::eTriangleList,
            // the viewport and scissor rectangles are specified dynamically,
            // but we need to specify the counts
            vk::ArrayProxy<vk::Viewport>(1, nullptr), /* viewports */
            vk::ArrayProxy<vk::Rect2D>(1, nullptr), /* scissor rects */
            vk::PolygonMode::eLine,
            vk::CullModeFlagBits::eNone,
            vk::FrontFace::eCounterClockwise,
            this->_wireFramePipeline.layout,
            this->_renderPass,
            0,
            dynamicStates);

        delete shaders;
    }

    /* create the pipeline for the texture renderers */
    {
        auto shaders = new cs237::Shaders(
            dev,
            std::string(kShaderDir) + "texture",
            kStages);

        this->_texturePipeline.pipe = this->_app->createPipeline (
            shaders,
            vertexInfo,
            vk::PrimitiveTopology::eTriangleList,
            // the viewport and scissor rectangles are specified dynamically,
            // but we need to specify the counts
            vk::ArrayProxy<vk::Viewport>(1, nullptr), /* viewports */
            vk::ArrayProxy<vk::Rect2D>(1, nullptr), /* scissor rects */
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            this->_texturePipeline.layout,
            this->_renderPass,
            0,
            dynamicStates);

        delete shaders;
    }

    cs237::destroyVertexInputInfo (vertexInfo);

}

void Proj4Window::_initDescriptorPools ()
{
    auto device = this->device();
    int nObjs = this->_objs.size();
    assert (nObjs > 0);

    /** HINT: redo this computation to account for the descriptors used in the
     ** deferred rendering passes.
     **/

    // allocate the descriptor-set pool.  For forward rendering, we have one UBO for
    // the lighting state.  The mesh descriptors are handled by the mesh factory.
    int nUBOs = 1;
    int nSamplers = 0;
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, nUBOs)
        };
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        1, /* max sets */
        poolSizes); /* pool sizes */

    this->_descPool = device.createDescriptorPool(poolInfo);

    // create the layout for the lighting UBO
    {
        vk::DescriptorSetLayoutBinding layoutBinding(
            0, /* binding */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr); /* samplers */

        vk::DescriptorSetLayoutCreateInfo layoutInfo(
            {}, /* flags */
            layoutBinding); /* bindings */
        this->_lightingLayout = device.createDescriptorSetLayout(layoutInfo);
    }

}

void Proj4Window::_initLighting ()
{
    Scene const *scene = this->_scene();

    if (this->app()->verbose()) {
        std::cout << "# Lighting UB:\n"
            << "## lightDir = " << to_string(-scene->lightDir()) << "\n"
            << "## lightIntensity = " << to_string(scene->lightIntensity()) << "\n"
            << "## ambIntensity = " << to_string(scene->ambientLight()) << "\n"
            << "## shadowFactor = " << scene->shadowFactor() << "\n";
    }

    LightingUB ub = {
            -scene->lightDir(), /* negate direction */
            scene->lightIntensity(),
            scene->ambientLight(),
            scene->shadowFactor()
        };

    this->_lightingUBO = new cs237::UniformBuffer<LightingUB>(this->app(), ub);

    // allocate the lighting descriptor set
    vk::DescriptorSetAllocateInfo allocInfo(this->_descPool, this->_lightingLayout);
    this->_lightingDS = (this->_app->device().allocateDescriptorSets(allocInfo))[0];

    // update the lighting descriptor set
    auto lightingInfo = this->_lightingUBO->descInfo();
    vk::WriteDescriptorSet descWrite(
        this->_lightingDS, /* descriptor set */
        0, /* binding */
        0, /* array element */
        vk::DescriptorType::eUniformBuffer, /* descriptor type */
        nullptr, /* image info */
        lightingInfo, /* buffer info */
        nullptr); /* texel buffer view */

    this->_app->device().updateDescriptorSets (descWrite, nullptr);

}

void Proj4Window::_invalidateFrameUBOs ()
{
    // mark the per-frame UBOs as being out-of-sync with the cache
    for (auto it : this->_frames) {
        reinterpret_cast<Proj4Window::FrameData *>(it)->valid = false;
    }

    /** HINT: update the cache for per-frame data */
}

void Proj4Window::_recordForwardCommands (Proj4Window::FrameData *frame)
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
    {
        // set the viewport using the OpenGL convention
        this->_setViewportCmd (cmdBuf, true);

        switch (this->_renderFlags.mode) {
        case RenderMode::eWireFrame:
            cmdBuf.bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                this->_wireFramePipeline.pipe);
            break;
        case RenderMode::eTextured:
            cmdBuf.bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                this->_texturePipeline.pipe);
            cmdBuf.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                this->_texturePipeline.layout,
                0, /* first set */
                this->_lightingDS,
                nullptr);
            break;
        default:
            ERROR("Proj4Window::_recordForwardCommands: invalid render mode");
        } /* switch */

        // render the objects in the scene
        for (auto it : this->_objs) {
            for (auto mesh : it->meshes) {
                // initialize the uniforms
                if (this->_renderFlags.mode == RenderMode::eWireFrame) {
                    WireFramePushConsts pc = {
                            this->_projM * this->_viewM * it->toWorld,
                            mesh->albedoColor
                        };
                    cmdBuf.pushConstants(
                        this->_texturePipeline.layout,
                        vk::ShaderStageFlagBits::eVertex,
                        0,
                        sizeof(WireFramePushConsts),
                        &pc);
                } else { // texture mode
                    // bind the descriptors for the object
                    cmdBuf.bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics,
                        this->_texturePipeline.layout,
                        1, /* second set */
                        mesh->descSet, /* descriptor sets */
                        nullptr);
                    // push constants for the mesh
                    TexturePushConsts pc = {
                            this->_projM * this->_viewM * it->toWorld,
                            glm::mat4(it->normToWorld)
                        };
                    cmdBuf.pushConstants(
                        this->_texturePipeline.layout,
                        vk::ShaderStageFlagBits::eVertex,
                        0,
                        sizeof(TexturePushConsts),
                        &pc);
                }
                mesh->draw (cmdBuf);
            }
        }

    }
    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

void Proj4Window::draw ()
{
    // next buffer from the swap chain
    auto sts = this->_acquireNextImage();
    if (sts != vk::Result::eSuccess) {
        ERROR("Unable to acquire next image");
    }

    auto frame = reinterpret_cast<Proj4Window::FrameData *>(this->_currentFrame());

    frame->resetFence();

    if (this->_renderFlags.mode == RenderMode::eDeferred) {
        /** HINT: record commands for deferred-rendering mode here */
    } else {
        // record drawing commands
        this->_recordForwardCommands (frame);
    }

    // set up submission for the graphics queue
    frame->submitDrawingCommands();

    // set up submission for the presentation queue
    frame->present();
}

void Proj4Window::reshape (int wid, int ht)
{
    // invoke the super-method reshape method
    this->cs237::Window::reshape(wid, ht);

    // recreate the new framebuffers
    this->_swap.initFramebuffers (this->_renderPass);

    // recompute the projection
    this->_setProjMat();

    this->_invalidateFrameUBOs();

}

void Proj4Window::key (int key, int scancode, int action, int mods)
{
    // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            /* Rendering-mode controls */
            case GLFW_KEY_T:  // 't' or 'T' ==> switch to texturing mode
                this->_renderFlags.mode = RenderMode::eTextured;
                break;
            case GLFW_KEY_W:  // 'w' or 'W' ==> switch to wire-frame mode
                this->_renderFlags.mode = RenderMode::eWireFrame;
                break;

            /* Lighting controls for deferred rendering */
            case GLFW_KEY_L:  // 'l' or 'L' ==> toggle the direct light
                this->_renderFlags.toggleDirLight();
                this->_invalidateFrameUBOs();
                break;
            case GLFW_KEY_P:  // 'p' or 'P' ==> toggle the spot lights
                this->_renderFlags.toggleSpotLights();
                this->_invalidateFrameUBOs();
                break;
            case GLFW_KEY_E:  // 'e' or 'E' ==> toggle emissive lighting
                this->_renderFlags.toggleEmissiveLighting();
                this->_invalidateFrameUBOs();
                break;
            case GLFW_KEY_S:  // 's' or 'S' ==> toggle shadows
                this->_renderFlags.toggleShadows();
                this->_invalidateFrameUBOs();
                break;

            /* Other controls */
            case GLFW_KEY_H:  // 'h' or 'H' ==> display help message
                reinterpret_cast<Proj4 *>(this->_app)->controlsHelpMessage();
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
/** HINT: add other camera controls */

            default: // ignore all other keys
                return;
        }
    }

}

/******************** Window::FrameData struct ********************/

/* virtual */
cs237::Window::FrameData *Proj4Window::_allocFrameData (cs237::Window *w) /* override */
{
    return new Proj4Window::FrameData (w);
}

Proj4Window::FrameData::FrameData (cs237::Window *w)
: cs237::Window::FrameData(w)
{
    auto win = reinterpret_cast<Proj4Window *>(w);
    auto device = win->device();

    /** HINT: allocate the UBO and descriptor set for the scene-data UBO */

    /** HINT: write the descriptor sets */
}

/* virtual */
Proj4Window::FrameData::~FrameData ()
{
    /** HINT: delete descriptor sets and UBOs */
}
