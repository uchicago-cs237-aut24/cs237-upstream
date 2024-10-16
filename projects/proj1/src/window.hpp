/*! \file window.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

#include "cs237/cs237.hpp"
#include "app.hpp"
#include "instance.hpp"
#include "render-modes.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "shader-uniforms.hpp"

/// The Project 1 Window class
class Proj1Window : public cs237::Window {
public:
    Proj1Window (Proj1 *app);

    ~Proj1Window () override;

    void draw () override;

    /// reshape the window
    void reshape (int wid, int ht) override;

    /// handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:
    vk::RenderPass _renderPass;                 ///< the shared render pass for drawing
    RenderMode _mode;                           ///< the current rendering mode
    Renderer *_renderer[kNumRenderModes];       ///< the renderers for the various modes

    // support for uniform buffers

    /// a tuple of the information needed to support the
    /// per-frame uniform-buffer object
    struct UBOInfo {
        bool valid;                     ///< true when the contents of the UBO is
                                        ///  valid (i.e., equal to the _ubCache)
        SceneUBO_t *ubo;                ///< the Vulkan uniform buffer object
        vk::DescriptorSet descSet;      ///< the descriptor set for access the UBO

        UBOInfo () : valid(false), ubo(nullptr), descSet() { }
        UBOInfo (SceneUBO_t *u, vk::DescriptorSet ds)
          : valid(false), ubo(u), descSet(ds)
        { }

    };

    vk::DescriptorPool _descPool;               ///< the descriptor pool
    vk::DescriptorSetLayout _dsLayout;          ///< layout of descriptor set

    // scene data
    std::vector<Mesh *> _meshes;                ///< the meshes in the scene
    std::vector<Instance *> _objs;              ///< the objects to render

    // Current camera state
    glm::vec3 _camPos;                          ///< camera position in world space
    glm::vec3 _camAt;                           ///< camera look-at point in world space
    glm::vec3 _camUp;                           ///< camera up vector in world space
    SceneUB _ubCache;                           ///< cache of scene uniform data; this
                                                ///  cache gets updated when the camera
                                                ///  state changes

    /// extend the generic frame-data structure with project-specific data
    struct FrameData : public Window::FrameData {
        UBOInfo uboInfo;

        FrameData (Window *w);
        virtual ~FrameData () override;
    };

    /// get the uniform-buffer info for the i'th frame
    UBOInfo *_uboInfo (int i)
    {
        assert ((0 <= i) && (i < cs237::kMaxFrames) && "bad frame index");
        return &(reinterpret_cast<FrameData *>(this->_frames[i])->uboInfo);
    }

    /// get the uniform-buffer info for the current frame
    UBOInfo *_uboInfo () { return this->_uboInfo (this->_curFrameIdx); }

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `Proj1FrameData` struct.
    Window::FrameData *_allocFrameData (Window *w) override;

    /// allocate and initialize the meshes and drawables
    void _initMeshes (const Scene *scene);

    /// initialize the `_renderPass` field
    void _initRenderPass ();
    /// initialize the renderers for the various render modes
    void _initRenderers ();
    /// initialize descriptor pool and UBOs
    void _initDescriptorPools ();

    /** HINT: you will need to define initialization functions
     ** to initialize the rendering structures, uniforms, etc.
     */

    /// specialized version of `_init` method.  This initializes the UB cache
    /// and projection matrix
    virtual void _init () override;

    /// record the rendering commands
    void _recordCommandBuffer ();

    /// get the scene being rendered
    const Scene *_scene () const
    {
        return reinterpret_cast<Proj1 *>(this->_app)->scene();
    }

}; // Proj1Window

#endif // !_WINDOW_HPP_
