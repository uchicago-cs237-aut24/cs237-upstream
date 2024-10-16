/*! \file window.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_WINDOW_HPP_
#define _CS237_WINDOW_HPP_

#ifndef _CS237_HPP_
#error "cs237-window.hpp should not be included directly"
#endif

#include <optional>

namespace cs237 {

#ifdef CS237_MAX_FRAMES_IN_FLIGHT
#  define MAX_FRAMES CS237_MAX_FRAMES_IN_FLIGHT
#else
#  define MAX_FRAMES 2
#endif

//// the maximum number of frames allowed "in flight".  This value defaults
/// to 2 (for double buffering), but can be overridden by defining the macro
/// `CS237_MAX_FRAMES_IN_FLIGHT` to some other value.
constexpr int kMaxFrames = MAX_FRAMES;

/// structure containing parameters for creating windows
//
struct CreateWindowInfo {
    int wid;            ///< the window width
    int ht;             ///< the window height
    std::string title;  ///< window title
    bool resizable;     ///< should the window support resizing
    bool depth;         ///< do we need depth-buffer support?
    bool stencil;       ///< do we need stencil-buffer support?

    /// constructor
    /// \param[in] w  window width in pixels
    /// \param[in] h  window height in pixels
    /// \param[in] t  window title string
    /// \param[in] r  boolean flag to specify that the window is resizable
    /// \param[in] d  boolean flag to specify that the window has a depth buffer
    /// \param[in] s  boolean flag to specify that the window has a stencil buffer
    CreateWindowInfo (int w, int h, std::string const &t, bool r, bool d, bool s)
        : wid(w), ht(h), title(t), resizable(r), depth(d), stencil(s)
    { }

    /// simple constructor for fixed-size window without depth or stencil
    /// \param[in] w  window width in pixels
    /// \param[in] h  window height in pixels
    CreateWindowInfo (int w, int h)
        : wid(w), ht(h), title(""), resizable(false), depth(false), stencil(false)
    { }

    /// do we need a depth/stencil buffer for the window?
    bool needsDepthBuf () const { return this->depth || this->stencil; }
};

/// abstract base class for simple GLFW windows used to view buffers, etc.
//
class Window {
public:

    /// destructor: it destroys the underlying GLFW window
    virtual ~Window ();

    /// method for completing the initialization of a window.  This function allocates
    /// the per-frame data array and also invokes the virtual `_init` method to handle
    /// and sub-class specific initialization that was not handled in the constructor.
    void initialize ();

    /// return the application pointer
    Application *app () { return this->_app; }

    /// return the logical device for this window
    vk::Device device () const { return this->_app->_device; }

    /// the graphics queue
    vk::Queue graphicsQ () const { return this->_app->_queues.graphics; }

    /// the presentation queue
    vk::Queue presentationQ () const { return this->_app->_queues.present; }

    /// the compute queue
    vk::Queue computeQ () const { return this->_app->_queues.compute; }

    /// Refresh the contents of the window.  This method is also invoked
    /// on Refresh events.
    void refresh ()
    {
        if (this->_isVis) {
            this->draw();
        }
    }

    /// Hide the window
    void hide ()
    {
      glfwHideWindow (this->_win);
      this->_isVis = false;
    }

    /// Show the window (a no-op if it is already visible)
    void show ()
    {
      glfwShowWindow (this->_win);
      this->_isVis = true;
    }

    /// virtual draw method provided by derived classes to draw the contents of the
    /// window.  It is called by Refresh.
    virtual void draw () = 0;

    /// method invoked on Reshape events.
    /// \param wid  specifies the width of the viewport
    /// \param ht   specifies the height of the viewport
    ///
    /// This method takes care of updating the cached size of the window and
    /// recreating the swap chain.  Other updates, including allocating new
    /// framebuffers should be handled by overriding this method in the subclass.
    virtual void reshape (int wid, int ht);

    /// method invoked on Iconify events.
    virtual void iconify (bool iconified);

    /// get the value of the "close" flag for the window
    bool windowShouldClose ()
    {
        return glfwWindowShouldClose (this->_win);
    }

    ///{
    /// Input handling methods; override these in the derived window
    /// classes to do something useful.
    virtual void key (int key, int scancode, int action, int mods);
    virtual void cursorPos (double xpos, double ypos);
    virtual void cursorEnter (bool entered);
    virtual void mouseButton (int button, int action, int mods);
    virtual void scroll (double xoffset, double yoffset);
    //}

    ///{
    /// enable/disable handling of events
    void enableKeyEvent (bool enable);
    void setCursorMode (int mode);
    void enableCursorPosEvent (bool enable);
    void enableCursorEnterEvent (bool enable);
    void enableMouseButtonEvent (bool enable);
    void enableScrollEvent (bool enable);
    //}

protected:
    /// information about swap-chain support
    struct SwapChainDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;

        /// \brief choose a surface format from the available formats
        vk::SurfaceFormatKHR chooseSurfaceFormat ();
        /// \brief choose a presentation mode from the available modes; we prefer
        ///        "mailbox" (aka triple buffering)
        vk::PresentModeKHR choosePresentMode ();
        /// \brief get the extent of the window subject to the limits of
        ///        the Vulkan device
        vk::Extent2D chooseExtent (GLFWwindow *win);
    };

    /// information about the optional depth/stencil buffers for the window
    struct DepthStencilBuffer {
        bool depth;                     ///< true if depth-buffer is supported
        bool stencil;                   ///< true if stencil-buffer is supported
        vk::Format format;              ///< the depth/image-buffer format
        vk::Image image;                ///< depth/image-buffer image
        vk::DeviceMemory imageMem;      ///< device memory for depth/image-buffer
        vk::ImageView view;             ///< image view for depth/image-buffer
    };

    /// the collected information about the swap-chain for a window
    struct SwapChain {
        vk::Device device;              ///< the owning logical device
        vk::SwapchainKHR chain;         ///< the swap chain object
        vk::Format imageFormat;         ///< pixel format of image buffers
        vk::Extent2D extent;            ///< size of swap buffer images
        int numAttachments;             ///< the number of framebuffer attachments
        // the following vectors hold the state for each of the buffers in the
        // swap chain.
        std::vector<vk::Image> images;  ///< images for the swap buffers
        std::vector<vk::ImageView> views; ///< image views for the swap buffers
        std::optional<DepthStencilBuffer> dsBuf; ///< optional depth/stencil-buffer
        std::vector<vk::Framebuffer> fBufs; ///< frame buffers

        SwapChain (vk::Device dev)
          : device(dev), dsBuf(std::nullopt)
        { }

        /// \brief return the number of buffers in the swap chain
        int size () const { return this->images.size(); }

        /// \brief allocate frame buffers for a rendering pass
        void initFramebuffers (vk::RenderPass renderPass);

        /// \brief does the swap-chain support a depth buffer?
        bool hasDepthBuffer () const
        {
            return this->dsBuf.has_value() && this->dsBuf->depth;
        }

        /// \brief does the swap-chain support a stencil buffer?
        bool hasStencilBuffer () const
        {
            return this->dsBuf.has_value() && this->dsBuf->stencil;
        }

        /// \brief destroy the Vulkan state for the swap chain
        void cleanup ();
    };

    /// \brief A container for the per-frame rendering state
    ///
    /// There is one of these objects per frame.  Subclasses of the
    /// Window class may want to extend this structure with additional information
    /// so the window class defines a virtual function for allocating FrameData
    /// objects.
    struct FrameData {
        Window *win;                    ///< the owning window
        vk::CommandBuffer cmdBuf;       ///< the main command buffer for drawing to
                                        ///  this frame
        vk::Semaphore imageAvail;       ///< semaphore for signaling when the
                                        ///  image object is available
        vk::Semaphore finished;         ///< semaphore for signaling when render pass
                                        ///  is finished
        vk::Fence inFlight;             ///< fence for synchronizing on the termination
                                        ///  of the rendering operation
        uint32_t index;                 ///< the swap-chain image index for this frame
                                        ///  This field is set by the window's
                                        ///  `_acquireNextImage` method.

        /// Constructor
        /// \param win  the owning window
        ///
        /// This constructor initializes the `win` and  synchronization components.
        //
        FrameData (Window *w);

        FrameData () = delete;
        FrameData (FrameData &) = delete;
        FrameData (FrameData const &) = delete;
        FrameData (FrameData &&) = delete;

        /// Destructor
        //
        virtual ~FrameData ();

        /// reset this frame's fence
        void resetFence ()
        {
            this->win->device().resetFences({this->inFlight});
        }

        /// submit drawing commands for this frame using the main command buffer
        void submitDrawingCommands ();

        /// \brief present this frame
        /// \return the return status of presenting the image
        vk::Result present ()
        {
            vk::PresentInfoKHR presentInfo(
                this->finished,
                this->win->_swap.chain,
                this->index,
                nullptr);

            return this->win->presentationQ().presentKHR(presentInfo);
        }

    }; // struct FrameData

    Application *_app;                  ///< the owning application
    GLFWwindow *_win;                   ///< the underlying window
    int _wid, _ht;	                ///< window dimensions
    bool _isVis;                        ///< true when the window is visible
    bool _keyEnabled;                   ///< true when the Key callback is enabled
    bool _cursorPosEnabled;             ///< true when the CursorPos callback is enabled
    bool _cursorEnterEnabled;           ///< true when the CursorEnter callback is enabled
    bool _mouseButtonEnabled;           ///< true when the MouseButton callback is enabled
    bool _scrollEnabled;                ///< true when the Scroll callback is enabled
    // Vulkan state for rendering
    vk::SurfaceKHR _surf;               ///< the Vulkan surface to render to
    SwapChain _swap;                    ///< buffer-swapping information
    FrameData *_frames[kMaxFrames];     ///< the per-frame rendering state
    uint32_t _curFrameIdx;              ///< index into `_frames` array for current
                                        ///  frame data

    /// \brief the Window base-class constructor
    /// \param app      the owning application
    /// \param info     information for creating the window, such as size and title
    Window (Application *app, CreateWindowInfo const &info);

    /// subclasses can override this method to handle any additional initialization
    /// that needs to be run after the window object is constructed.
    virtual void _init ();

    /// \brief Get the swap-chain details for a physical device
    SwapChainDetails _getSwapChainDetails ();

    /// \brief Create the swap chain for this window; this initializes the _swap
    ///        instance variable.
    /// \param depth    set to true if requesting depth-buffer support
    /// \param stencil  set to true if requesting stencil-buffer support
    void _createSwapChain (bool depth, bool stencil);

    /// \brief Recreate the swap chain for this window; this redefines the _swap
    ///        instance variable and is used when some aspect of the presentation
    ///        surface changes.
    void _recreateSwapChain ();

    /// virtual function for allocating a `FrameData` object.  Subclasses of the
    /// `Window` class can define a subclass of `FrameData` and then override this
    /// method to allocate the subclass objects.
    ///
    virtual FrameData *_allocFrameData (Window *w);

    /// get a pointer to the current per-frame rendering state
    FrameData *_currentFrame () { return this->_frames[this->_curFrameIdx]; }

    /// advance the current frame
    void _nextFrame () { this->_curFrameIdx = (this->_curFrameIdx + 1) % kMaxFrames; }

    /// acquire the next image from the swap chain.  This method has the side effect
    /// of setting the `index` of the current frame to the index of the swap-chain
    /// image that we are using.
    /// \return the status of the request; `Result::eSuccess` for success
    vk::Result _acquireNextImage ();

    /// \brief initialize the attachment descriptors and references for the color and
    ///        optional depth/stencil-buffer
    /// \param[out] descs  vector that will contain the attachment descriptors
    /// \param[out] refs   vector that will contain the attachment references
    void _initAttachments (
        std::vector<vk::AttachmentDescription> &descs,
        std::vector<vk::AttachmentReference> &refs);

    /// \brief the graphics queue-family index
    ///
    /// This method is a wrapper to allow subclasses access to this information
    uint32_t _graphicsQIdx () const { return this->_app->_qIdxs.graphics; }

    /// \brief the presentation queue
    ///
    /// This method is a wrapper to allow subclasses access to this information
    uint32_t _presentationQIdx () const { return this->_app->_qIdxs.present; }

    /// \brief the compute queue index
    ///
    /// This method is a wrapper to allow subclasses access to this information
    uint32_t _computeQIdx () const { return this->_app->_qIdxs.compute; }

    /// \brief get the natural viewport for the window
    /// \param oglView  if `true` then use the **OpenGL** convention where
    ///                 Y = 0 maps to the bottom of the screen and Y increases
    ///                 going up.
    /// \return a viewport that covers the extent of the window
    vk::Viewport _getViewport (bool oglView = false)
    {
        if (oglView) {
            // to get the OpenGL-style viewport, we set the origin at Y = height
            // and use a negative height
            return vk::Viewport(
                0.0f,
                float(this->_swap.extent.width),
                float(this->_swap.extent.width),
                -float(this->_swap.extent.height),
                0.0f, /* min depth */
                1.0f);
        } else {
            return vk::Viewport(
                0.0f, 0.0f,
                float(this->_swap.extent.width),
                float(this->_swap.extent.height),
                0.0f, /* min depth */
                1.0f); /* max depth */
        }
    }

    /// \brief get the scissors rectangle for this window
    /// \return a rectangle that covers the extent of the window
    vk::Rect2D _getScissorsRect ()
    {
        return vk::Rect2D(
            {0, 0},
            {this->_swap.extent.width, this->_swap.extent.height});
    }

    /// \brief add a command to set the viewport and scissor to the whole window
    /// \param cmdBuf   the command buffer
    /// \param oglView  if `true` then use the **OpenGL** convention where
    ///                 Y = 0 maps to the bottom of the screen and Y increases
    ///                 going up.
    ///
    /// **Vulkan** follows the Direct3D convention of using a right-handed NDC
    /// space, which means that Y = 0 maps to the top of the screen, instead
    /// of the bottom (as in OpenGL).  pass `true` as the second argument to
    /// use the **OpenGL** convention.
    void _setViewportCmd (vk::CommandBuffer cmdBuf, bool oglView = false)
    {
        if (oglView) {
            /* NOTE: we negate the height and set the Y origin to ht because Vulkan's
             * viewport coordinates are from top-left down, instead of from
             * bottom-left up. See
             * https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport
             */
            this->_setViewportCmd(cmdBuf,
                0, this->_swap.extent.height,
                this->_swap.extent.width, -(int32_t)this->_swap.extent.height);
        } else {
            this->_setViewportCmd(cmdBuf,
                0, 0,
                this->_swap.extent.width, this->_swap.extent.height);
        }
    }

    /// \brief add a viewport command to the command buffer; this also sets the
    ///        scissor rectangle.
    /// \param cmdBuf   the command buffer
    /// \param x    specifies the Y coordinate of the upper-left corner
    /// \param y    specifies the X coordinate of the upper-left corner
    /// \param wid  specifies the width of the viewport
    /// \param ht   specifies the height of the viewport
    ///
    /// To use the **OpenGL** convention of the Y axis pointing up, specify the
    /// **OpenGL** Y coordinate and a negative height.
    void _setViewportCmd (vk::CommandBuffer cmdBuf,
        int32_t x, int32_t y,
        int32_t wid, int32_t ht);

public:

    /// the width of the window
    int width () const { return this->_swap.extent.width; }

    /// the height of the window
    int height () const { return this->_swap.extent.height; }

};

} // namespace cs237

#endif // !_CS237_WINDOW_HPP_
