/*! \file main.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 2.  This file is the main program
 * for Lab2.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237/cs237.hpp"

#ifdef CS237_BINARY_DIR
/// location of the shaders for Lab 2
const std::string kShaderDir = CS237_BINARY_DIR "/labs/lab2/shaders/";
#else
# error CS237_BINARY_DIR not defined
#endif

/// the period between animation steps (60 FPS)
constexpr double kUpdatePeriod = 1.0 / 60.0;
/// the rotation rate (degrees per second)
constexpr double kRotationRate = 90.0;


/******************** Vertex data ********************/

/// 2D vertices with color
struct Vertex {
    glm::vec2 pos;      ///< the vertex position
    glm::vec3 color;    ///< the vertex color

    /// constructor
    /// \param p  the position
    /// \param c  the color
    Vertex (glm::vec2 p, glm::vec3 c) : pos(p), color(c) { }

    /// static method for getting the input-binding description for this class
    static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions()
    {
        std::vector<vk::VertexInputBindingDescription> bindings(1);
        bindings[0].binding = 0;
        bindings[0].stride = sizeof(Vertex);
        bindings[0].inputRate = vk::VertexInputRate::eVertex;

        return bindings;
    }

    /// static method for getting the input-attribute description for this class
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<vk::VertexInputAttributeDescription> attrs(2);

        // pos
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = vk::Format::eR32G32Sfloat;
        attrs[0].offset = offsetof(Vertex, pos);

        // color
        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = vk::Format::eR32G32B32Sfloat;
        attrs[1].offset = offsetof(Vertex, color);

        return attrs;
    }
};

/// A 2D triangle to draw
const std::array<Vertex,3> vertices = {
        Vertex({ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}),
        Vertex({ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}),
        Vertex({-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f})
    };

/******************** Push constant buffer ********************/

struct PCBuffer {
    float sinTheta;
    float cosTheta;

    /// initialize the values in the buffer
    /// \param deg  the angle of rotation in degrees
    void init (double deg)
    {
        deg = ::fmod (deg, 360.0); /* clamp to one rotation */
        double angle = glm::radians (deg); /* convert angle to radians */
        this->sinTheta = float(::sin(angle));
        this->cosTheta = float(::cos(angle));
    }

};

/******************** derived classes ********************/

/// The Lab 2 Application class
class Lab2 : public cs237::Application {
public:
    /// constructor
    /// \param args  the command-line arguments
    Lab2 (std::vector<std::string> const &args);

    /// destructor
    ~Lab2 ();

    /// run the application code
    void run () override;
};

/// The Lab 2 Window class
class Lab2Window : public cs237::Window {
public:
    /// constructor
    /// \param app  pointer to the application object
    Lab2Window (Lab2 *app);

    /// destructor
    ~Lab2Window () override;

    /// reshape the window
    void reshape (int wid, int ht) override;

    /// render the contents of the window
    void draw () override;

    /// handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:
    vk::RenderPass _renderPass;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _graphicsPipeline;
    PCBuffer _pConsts;                  ///< push constants for vertex shader
    cs237::VertexBuffer<Vertex> *_vertBuffer;

    /// initialize the `_renderPass` field
    void _initRenderPass();
    /// initialize the `_pipelineLayout` and `_graphicsPipeline` fields
    void _initPipeline();
    /// initialize the vertex buffer
    void _initVertexBuffer();
    /// record the rendering commands
    void _recordCommandBuffer();

};

/******************** Lab2Window methods ********************/

Lab2Window::Lab2Window (Lab2 *app)
  : cs237::Window (app, cs237::CreateWindowInfo(800, 600))
{
    this->_initRenderPass();
    this->_initPipeline();

    // create framebuffers for the swap chain
    this->_swap.initFramebuffers(this->_renderPass);

    // initialize the vertex buffer
    this->_initVertexBuffer();

}

Lab2Window::~Lab2Window ()
{
    // delete the pipeline and render pass
    this->device().destroyPipeline(this->_graphicsPipeline);
    this->device().destroyPipelineLayout(this->_pipelineLayout);
    this->device().destroyRenderPass(this->_renderPass);
}

void Lab2Window::_initRenderPass ()
{
    // we have a single output framebuffer as the attachment
    vk::AttachmentDescription colorAttachment(
        {}, /* flags */
        this->_swap.imageFormat, /* image-view format */
        vk::SampleCountFlagBits::e1, /* number of samples */
        vk::AttachmentLoadOp::eClear, /* load op */
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

    this->_renderPass = this->device().createRenderPass(renderPassInfo);

}

void Lab2Window::_initPipeline ()
{
}

void Lab2Window::_initVertexBuffer ()
{
}

void Lab2Window::_recordCommandBuffer ()
{
    auto cmdBuf = this->_currentFrame()->cmdBuf;

    cmdBuf.reset();

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuf.begin(beginInfo);

    vk::ClearValue blackColor(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
    vk::RenderPassBeginInfo renderPassInfo(
        this->_renderPass,
        this->_swap.fBufs[this->_currentFrame()->index],
        { {0, 0}, this->_swap.extent }, /* render area */
        blackColor); /* clear the window to black */

    cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    /*** BEGIN COMMANDS ***/
    cmdBuf.bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        this->_graphicsPipeline);

    /*** END COMMANDS ***/

    cmdBuf.endRenderPass();

    cmdBuf.end();

}

void Lab2Window::reshape (int wid, int ht)
{
}

void Lab2Window::draw ()
{
    // next buffer from the swap chain
    auto sts = this->_acquireNextImage();
    if (sts != vk::Result::eSuccess) {
        ERROR("inable to acquire next image");
    }

    auto frame = this->_currentFrame();

    frame->resetFence();

    this->_recordCommandBuffer();

    // set up submission for the graphics queue
    frame->submitDrawingCommands();

    // set up submission for the presentation queue
    frame->present();
}

void Lab2Window::key (int key, int scancode, int action, int mods)
{
}

/******************** Lab2 class ********************/

Lab2::Lab2 (std::vector<std::string> const &args)
  : cs237::Application (args, "CS237 Lab 2")
{ }

Lab2::~Lab2 () { }

void Lab2::run ()
{
    Lab2Window *win = new Lab2Window (this);

    // set the base time to zero
    glfwSetTime(0.0);

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
    Lab2 app(args);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
