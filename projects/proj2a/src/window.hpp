/*! \file window.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 2
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
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
class Proj2Window : public cs237::Window {
public:
    Proj2Window (Proj2 *app);

    ~Proj2Window () override;

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
    vk::DescriptorPool _descPool;               ///< the descriptor pool for uniforms
    vk::DescriptorSetLayout _descSetLayout;     ///< layout of the descriptor set for
                                                ///  the uniform buffers (set 0)

    // scene data
    std::vector<Mesh *> _meshes;                ///< the meshes in the scene
    std::vector<Instance *> _objs;              ///< the objects to render

    // Current camera state
    glm::vec3 _camPos;                          ///< camera position in world space
    glm::vec3 _camAt;                           ///< camera look-at point in world space
    glm::vec3 _camUp;                           ///< camera up vector in world space
    SceneUB _sceneUBCache;                      ///< cache of uniform data; this
                                                ///  cache gets updated when the camera
                                                ///  state or the viewport changes

    /// extend the generic frame-data structure with project-specific data
    struct FrameData : public Window::FrameData {
        SceneUBO *ubo;                  ///< uniform buffer object for scene
                                        ///  data (transforms and lighting)
        bool valid;                     ///< true when the contents of the UBO is
                                        ///  valid (i.e., equal to the _sceneUBCache)
        vk::DescriptorSet ds;           ///< the descriptor set for access to the UBO

        void refresh ()
        {
            this->ubo->copyTo(reinterpret_cast<Proj2Window *>(this->win)->_sceneUBCache);
            this->valid = true;
        }

        FrameData (Window *w);
        virtual ~FrameData () override;
    };

    /// allocate and initialize the meshes and drawables
    void _initMeshes (const Scene *scene);

    /// initialize the `_renderPass` field
    void _initRenderPass ();
    /// initialize the descriptor pool and layouts for the uniform buffers
    void _initDescriptorPools ();
    /// initialize the renderers for the various render modes
    void _initRenderers ();

    /// update the camera and viewport-dependent contents of the scene-data cache
    void _updateUBCache ();

    /// invalidate the per-frame scene-data UBOs and update the scene-data cache
    void _invalidateSceneUBOs ();

    /// record the rendering commands
    void _recordCommandBuffer (Proj2Window::FrameData *frame);

    /// get the scene being rendered
    const Scene *_scene () const
    {
        return reinterpret_cast<Proj2 *>(this->_app)->scene();
    }

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `FrameData` struct.
    cs237::Window::FrameData *_allocFrameData (cs237::Window *w) override;

}; // Proj2Window

#endif // !_WINDOW_HPP_
