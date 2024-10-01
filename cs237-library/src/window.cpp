/*! \file window.cpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237/cs237.hpp"

namespace cs237 {

/******************** local helper functions ********************/

// wrapper function for Refresh callback
static void refreshCB (GLFWwindow *win)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->refresh ();
}

// wrapper function for Reshape callback
static void reshapeCB (GLFWwindow *win, int wid, int ht)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->reshape (wid, ht);
}

// wrapper function for Iconify callback
static void iconifyCB (GLFWwindow *win, int iconified)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->iconify (iconified != 0);
}

// wrapper function for Key callback
static void keyCB (GLFWwindow *win, int key, int scancode, int action, int mods)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->key (key, scancode, action, mods);
}

// wrapper function for CursorPos callback
static void cursorPosCB (GLFWwindow *win, double xpos, double ypos)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->cursorPos (xpos, ypos);
}

// wrapper function for CursorEnter callback
static void cursorEnterCB (GLFWwindow *win, int entered)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->cursorEnter (entered == GLFW_TRUE);
}

// wrapper function for MouseButton callback
static void mouseButtonCB (GLFWwindow *win, int button, int action, int mods)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->mouseButton (button, action, mods);
}

// wrapper function for Scroll callback
static void scrollCB (GLFWwindow *win, double xoffset, double yoffset)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->scroll (xoffset, yoffset);
}

/******************** class Window methods ********************/

Window::Window (Application *app, CreateWindowInfo const &info)
: _app(app), _win(nullptr), _surf(nullptr), _swap(app->_device), _curFrameIdx(0)
{
    glfwWindowHint(GLFW_RESIZABLE, info.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(
        info.wid, info.ht,
        info.title.c_str(),
        nullptr, nullptr);
    if (window == nullptr) {
        ERROR("unable to create window!");
    }

    // set the user data for the window to this object
    glfwSetWindowUserPointer (window, this);

    // set up window-system callbacks
    glfwSetWindowRefreshCallback (window, refreshCB);
    if (info.resizable) {
        glfwSetWindowSizeCallback (window, reshapeCB);
    }
    glfwSetWindowIconifyCallback (window, iconifyCB);

    this->_win = window;
    this->_wid = info.wid;
    this->_ht = info.ht;
    this->_isVis = true;
    this->_keyEnabled = false;
    this->_cursorPosEnabled = false;
    this->_cursorEnterEnabled = false;
    this->_mouseButtonEnabled = false;
    this->_scrollEnabled = false;

    // set up the Vulkan surface for the window
    VkSurfaceKHR surf;
    if (glfwCreateWindowSurface(app->_instance, window, nullptr, &surf) != VK_SUCCESS) {
        ERROR("unable to create window surface!");
    }
    this->_surf = vk::SurfaceKHR(surf);

    // set up the swap chain for the surface
    this->_createSwapChain (info.depth, info.stencil);

    // allocate the per-frame render state
    for (int i = 0;  i < kMaxFrames;  ++i) {
        this->_frames[i] = this->_allocFrameData (this);
    }
}

Window::~Window ()
{
    // destroy the swap chain as associated state
    this->_swap.cleanup ();

    // delete the surface
    this->_app->_instance.destroySurfaceKHR(this->_surf);

    glfwDestroyWindow (this->_win);
}

void Window::reshape (int wid, int ht)
{
    this->_wid = wid;
    this->_ht = ht;

    // recreate the swap chain, etc.  Note that we leave recreating the framebuffers
    // to the subclass, since we do not have access to the render pass here
    this->_recreateSwapChain ();
}

void Window::iconify (bool iconified)
{
    this->_isVis = !iconified;
}

/* default event handlers just ignore the events */
void Window::key (int key, int scancode, int action, int mods) { }
void Window::cursorPos (double xpos, double ypos) { }
void Window::cursorEnter (bool entered) { }
void Window::mouseButton (int button, int action, int mods) { }
void Window::scroll (double xoffset, double yoffset) { }

void Window::enableKeyEvent (bool enable)
{
    if (this->_keyEnabled && (! enable)) {
        // disable the callback
        this->_keyEnabled = false;
        glfwSetKeyCallback(this->_win, nullptr);
    }
    else if ((! this->_keyEnabled) && enable) {
        // enable the callback
        this->_keyEnabled = true;
        glfwSetKeyCallback(this->_win, keyCB);
    }

}

void Window::setCursorMode (int mode)
{
    glfwSetInputMode (this->_win, GLFW_CURSOR, mode);
}

void Window::enableCursorPosEvent (bool enable)
{
    if (this->_cursorPosEnabled && (! enable)) {
        // disable the callback
        this->_cursorPosEnabled = false;
        glfwSetCursorPosCallback(this->_win, nullptr);
    }
    else if ((! this->_cursorPosEnabled) && enable) {
        // enable the callback
        this->_cursorPosEnabled = true;
        glfwSetCursorPosCallback(this->_win, cursorPosCB);
    }

}

void Window::enableCursorEnterEvent (bool enable)
{
    if (this->_cursorEnterEnabled && (! enable)) {
        // disable the callback
        this->_cursorEnterEnabled = false;
        glfwSetCursorEnterCallback(this->_win, nullptr);
    }
    else if ((! this->_cursorEnterEnabled) && enable) {
        // enable the callback
        this->_cursorEnterEnabled = true;
        glfwSetCursorEnterCallback(this->_win, cursorEnterCB);
    }

}

void Window::enableMouseButtonEvent (bool enable)
{
    if (this->_mouseButtonEnabled && (! enable)) {
        // disable the callback
        this->_mouseButtonEnabled = false;
        glfwSetMouseButtonCallback(this->_win, nullptr);
    }
    else if ((! this->_mouseButtonEnabled) && enable) {
        // enable the callback
        this->_mouseButtonEnabled = true;
        glfwSetMouseButtonCallback(this->_win, mouseButtonCB);
    }

}

void Window::enableScrollEvent (bool enable)
{
    if (this->_scrollEnabled && (! enable)) {
        // disable the callback
        this->_scrollEnabled = false;
        glfwSetScrollCallback(this->_win, nullptr);
    }
    else if ((! this->_scrollEnabled) && enable) {
        // enable the callback
        this->_scrollEnabled = true;
        glfwSetScrollCallback(this->_win, scrollCB);
    }

}

Window::SwapChainDetails Window::_getSwapChainDetails ()
{
    auto dev = this->_app->_gpu;
    auto surf = this->_surf;
    Window::SwapChainDetails details;

    details.capabilities = dev.getSurfaceCapabilitiesKHR(surf);
    details.formats = dev.getSurfaceFormatsKHR(surf);
    details.presentModes = dev.getSurfacePresentModesKHR(surf);

    return details;

}

void Window::_createSwapChain (bool depth, bool stencil)
{
    // determine the required depth/stencil-buffer format
    vk::Format dsFormat = this->_app->_depthStencilBufferFormat(depth, stencil);
    if ((dsFormat == vk::Format::eUndefined) && (depth || stencil)) {
        ERROR("depth/stencil buffer requested but not supported by device");
    }
    this->_swap.numAttachments = (dsFormat == vk::Format::eUndefined) ? 1 : 2;

    SwapChainDetails swapChainSupport = this->_getSwapChainDetails ();

    // choose the best aspects of the swap chain
    vk::SurfaceFormatKHR surfaceFormat = swapChainSupport.chooseSurfaceFormat();
    vk::PresentModeKHR presentMode = swapChainSupport.choosePresentMode();
    vk::Extent2D extent = swapChainSupport.chooseExtent(this->_win);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if ((swapChainSupport.capabilities.maxImageCount > 0)
    && (imageCount > swapChainSupport.capabilities.maxImageCount)) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    auto qIdxs = this->_app->_qIdxs;
    uint32_t qIndices[] = {qIdxs.graphics, qIdxs.present};

    vk::SwapchainCreateInfoKHR swapInfo(
        {},
        this->_surf,
        imageCount, /* min image count */
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1, /* image array layers */
        vk::ImageUsageFlagBits::eColorAttachment,
        // check if the graphics and presentation queues are distinct
        (qIdxs.graphics != qIdxs.present
            ? vk::SharingMode::eConcurrent
            : vk::SharingMode::eExclusive),
        (qIdxs.graphics != qIdxs.present
            ? vk::ArrayProxyNoTemporaries<const uint32_t>(2, qIndices)
            : nullptr),
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE, /* clipped */
        {}); /* old swapchain */

    auto dev = this->device();
    this->_swap.chain = dev.createSwapchainKHR(swapInfo);

    // get the vector of images that represent the swap chain
    this->_swap.images = dev.getSwapchainImagesKHR(this->_swap.chain);

    this->_swap.imageFormat = surfaceFormat.format;
    this->_swap.extent = extent;

    // create an image view per swap-chain image
    this->_swap.views.resize(this->_swap.images.size());
    for (int i = 0; i < this->_swap.images.size(); ++i) {
        this->_swap.views[i] = this->_app->_createImageView(
            this->_swap.images[i],
            this->_swap.imageFormat,
            vk::ImageAspectFlagBits::eColor);
    }

    if (dsFormat != vk::Format::eUndefined) {
        // initialize the depth/stencil-buffer
        DepthStencilBuffer dsBuf;
        dsBuf.depth = depth;
        dsBuf.stencil = stencil;
        dsBuf.format = dsFormat;
        dsBuf.image = this->_app->_createImage(
            extent.width, extent.height,
            dsFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment);
        dsBuf.imageMem = this->_app->_allocImageMemory(
            dsBuf.image,
            vk::MemoryPropertyFlagBits::eDeviceLocal);
        dsBuf.view = this->_app->_createImageView (
            dsBuf.image,
            dsFormat,
            vk::ImageAspectFlagBits::eDepth);
        this->_swap.dsBuf = dsBuf;
    }
}

void Window::_recreateSwapChain ()
{
    // wait until any in-flight rendering is complete
    this->device().waitIdle();

    // remember the configuration
    bool hasDB = this->_swap.hasDepthBuffer();
    bool hasStencil = this->_swap.hasStencilBuffer();

    // cleanup the existing swapchain
    this->_swap.cleanup();

    // initialize a new swap-chain etc.
    this->_createSwapChain(hasDB, hasStencil);
}

/* virtual */
Window::FrameData *Window::_allocFrameData (Window *w)
{
    return new FrameData(w);
}

vk::Result Window::_acquireNextImage ()
{
    FrameData *frame = this->_currentFrame();
    auto sts = this->device().waitForFences({frame->inFlight}, VK_TRUE, UINT64_MAX);
    if (sts != vk::Result::eSuccess) {
        // the command failed
        frame->index = -1;
        return sts;
    }

    auto res = this->device().acquireNextImageKHR(
        this->_swap.chain,
        UINT64_MAX,
        frame->imageAvail,
        nullptr);

    if (res.result == vk::Result::eSuccess) {
        frame->index = res.value;
    } else {
        frame->index = -1;
    }

    return res.result;

}


void Window::_initAttachments (
    std::vector<vk::AttachmentDescription> &descs,
    std::vector<vk::AttachmentReference> &refs)
{
    descs.resize(this->_swap.numAttachments);
    refs.resize(this->_swap.numAttachments);

    // the output color buffer
    descs[0].format = this->_swap.imageFormat;
    descs[0].samples = vk::SampleCountFlagBits::e1;
    descs[0].loadOp = vk::AttachmentLoadOp::eClear;
    descs[0].storeOp = vk::AttachmentStoreOp::eStore;
    descs[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    descs[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    descs[0].initialLayout = vk::ImageLayout::eUndefined;
    descs[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    refs[0].attachment = 0;
    refs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    if (this->_swap.dsBuf.has_value()) {
        // the optional depth-stencil buffer
        descs[1].format = this->_swap.dsBuf->format;
        descs[1].samples = vk::SampleCountFlagBits::e1;
        descs[1].loadOp = vk::AttachmentLoadOp::eClear;
        descs[1].storeOp = vk::AttachmentStoreOp::eDontCare;
/* FIXME: if we need stencil support, then the following is incorrect! */
        descs[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        descs[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        descs[1].initialLayout = vk::ImageLayout::eUndefined;
        descs[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        refs[1].attachment = 1;
        refs[1].layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
}

void Window::_setViewportCmd (
    vk::CommandBuffer cmdBuf,
    int32_t x, int32_t y,
    int32_t wid, int32_t ht)
{
    vk::Viewport viewport(
        float(x), float(y),
        float(wid), float(ht),
        0.0f, /* min depth */
        1.0f); /* max depth */
    cmdBuf.setViewport (0, viewport);

    vk::Rect2D scissor(
        {0, 0},
        {static_cast<uint32_t>(std::abs(wid)), static_cast<uint32_t>(std::abs(ht))});
    cmdBuf.setScissor(0, { scissor });
}


/******************** struct Window::SwapChainDetails methods ********************/

// choose the surface format for the buffers
vk::SurfaceFormatKHR Window::SwapChainDetails::chooseSurfaceFormat ()
{
    for (const auto& fmt : this->formats) {
        if ((fmt.format == vk::Format::eB8G8R8A8Srgb)
        && (fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)) {
            return fmt;
        }
    }

    return this->formats[0];
}

// choose a presentation mode for the buffers
vk::PresentModeKHR Window::SwapChainDetails::choosePresentMode ()
{
    // the prefered presentation mode depends on the number of frames in flight
    vk::PresentModeKHR preferredModes[4];
    switch (kMaxFrames) {
    case 1:
        preferredModes[0] = vk::PresentModeKHR::eImmediate;
        preferredModes[1] = vk::PresentModeKHR::eMailbox;
        preferredModes[2] = vk::PresentModeKHR::eFifo;
        preferredModes[3] = vk::PresentModeKHR::eFifoRelaxed;
        break;
    case 2:
        preferredModes[0] = vk::PresentModeKHR::eMailbox;
        preferredModes[1] = vk::PresentModeKHR::eFifo;
        preferredModes[2] = vk::PresentModeKHR::eFifoRelaxed;
        preferredModes[3] = vk::PresentModeKHR::eImmediate;
        break;
    default:
        assert (kMaxFrames == 3);
        preferredModes[0] = vk::PresentModeKHR::eFifoRelaxed;
        preferredModes[1] = vk::PresentModeKHR::eFifo;
        preferredModes[2] = vk::PresentModeKHR::eMailbox;
        preferredModes[3] = vk::PresentModeKHR::eImmediate;
        break;
    }

    for (int i = 0;  i < 4;  ++i) {
        for (const auto& mode : this->presentModes) {
            if (mode == preferredModes[i]) {
                return mode;
            }
        }
    }

    // we should never get here, since the surface should support at least one
    // of the standard modes!
    ERROR("impossble: no valid presentation mode");

}

// compute the extent of the buffers
vk::Extent2D Window::SwapChainDetails::chooseExtent (GLFWwindow *win)
{
    if (this->capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return this->capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(win, &width, &height);

        vk::Extent2D actualExtent(
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height));

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }

}

/******************** struct Window::SwapChain methods ********************/

void Window::SwapChain::initFramebuffers (vk::RenderPass renderPass)
{
    assert (this->size() > 0);
    assert (this->fBufs.size() == 0);

    // the framebuffer attachments
    vk::ImageView attachments[2] = {nullptr, nullptr};

    if (this->dsBuf.has_value()) {
        assert (this->numAttachments == 2);
        // include the depth buffer
        attachments[1] = this->dsBuf->view;
    } else {
        assert (this->numAttachments == 1);
    }

    // initialize the invariant parts of the create info structure
    vk::FramebufferCreateInfo fbInfo(
        {}, /* flags */
        renderPass,
        vk::ArrayProxyNoTemporaries<const vk::ImageView>(
            this->numAttachments, attachments),
        this->extent.width, this->extent.height, 1);

    // create a frambuffer per swap-chain image
    this->fBufs.reserve(this->size());
    for (size_t i = 0; i < this->size(); i++) {
        attachments[0] = this->views[i];
        this->fBufs.push_back(this->device.createFramebuffer(fbInfo));
    }

}

void Window::SwapChain::cleanup ()
{
    // destroy framebuffers
    for (auto fb : this->fBufs) {
        this->device.destroyFramebuffer(fb);
    }
    this->fBufs.clear();

    // destroy the views
    for (auto view : this->views) {
        this->device.destroyImageView(view, nullptr);
    }
    this->views.clear();
    /* note that the images are owned by the swap chain object, so we do not have
     * to destroy them.
     */

    // cleanup the depth buffer (if present)
    if (this->dsBuf.has_value()) {
        this->device.destroyImageView(this->dsBuf->view);
        this->device.destroyImage(this->dsBuf->image);
        this->device.freeMemory(this->dsBuf->imageMem);
    }

    this->device.destroySwapchainKHR(this->chain);
}

/******************** struct Window::FrameData methods ********************/

Window::FrameData::FrameData (Window *w)
: win(w), index(-1), imageAvail(nullptr), finished(nullptr), inFlight(nullptr)
{
    auto device = this->win->device();

    vk::SemaphoreCreateInfo semInfo;
    this->imageAvail = device.createSemaphore (semInfo);
    this->finished = device.createSemaphore (semInfo);

    // we initialize the fence to signaled so that it does not block the first time
    // that we wait for it
    vk::FenceCreateInfo fenceInfo (vk::FenceCreateFlagBits::eSignaled);
    this->inFlight = device.createFence (fenceInfo);

    // allocate the main command buffer
    this->cmdBuf = this->win->_app->newCommandBuf();

}

/* virtual */
Window::FrameData::~FrameData ()
{
    auto device = this->win->device();

    // delete synchronization objects
    device.destroyFence (this->inFlight);
    device.destroySemaphore (this->imageAvail);
    device.destroySemaphore (this->finished);

    // delete the main command buffer
    this->win->_app->freeCommandBuf (this->cmdBuf);

}

void Window::FrameData::submitDrawingCommands ()
{
    vk::PipelineStageFlags pipeFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo(
        this->imageAvail,
        pipeFlags,
        this->cmdBuf,
        this->finished);

    this->win->graphicsQ().submit({ submitInfo }, this->inFlight);

}

} // namespace cs237
