/*! \file window.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3
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
#include "render-info.hpp"
#include "scene.hpp"
#include "shader-uniforms.hpp"

/// The Project 1 Window class
class Proj3Window : public cs237::Window {
public:
    Proj3Window (Proj3 *app);

    ~Proj3Window () override;

    void draw () override;

    /// reshape the window
    void reshape (int wid, int ht) override;

    /// handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:
    RenderMode _mode;                           ///< the current rendering mode

    // scene data
    std::vector<Mesh *> _meshes;                ///< the meshes in the scene
    std::vector<Instance *> _objs;              ///< the objects to render

    // Current camera state
    glm::vec3 _camPos;                          ///< camera position in world space
    glm::vec3 _camAt;                           ///< camera look-at point in world space
    glm::vec3 _camUp;                           ///< camera up vector in world space

    // support for uniform buffers
    vk::DescriptorPool _descPool;               ///< the descriptor pool for uniforms

    // per-scene UBO
    SceneUBO *_sceneUBO;                        ///< per-scene UBO
    vk::DescriptorSet _sceneDS;                 ///< the descriptor set for access
                                                ///  to `_sceneUBO`
    vk::DescriptorSetLayout _sceneLayout;       ///< the layout of the `_sceneDS`

    // per-light UBOs
    std::vector<ShadowUBO *> _shadowUBO;        ///< per-light UBOs that hold shadow
                                                ///  information
    std::vector<vk::DescriptorSet> _shadowDS;   ///< the descriptor sets for access
                                                ///  to the shadow UBOs
    vk::DescriptorSetLayout _shadowLayout;      ///< the layout of the `_shadowDS`
                                                ///  sets

    // per-frame UB cache
    FrameUB _frameUBCache;                      ///< holds cache of per-frame uniforms

    /// extend the generic frame-data structure with project-specific data
    struct FrameData : public Window::FrameData {
        FrameUBO *ubo;                  ///< per-frame UBO
        bool valid;                     ///< true when the contents of the UBO is
                                        ///  valid (i.e., equal to the _sceneUBCache)
        vk::DescriptorSet ds;           ///< the descriptor set for access to the UBO

        void refresh ()
        {
            this->ubo->copyTo(reinterpret_cast<Proj3Window *>(this->win)->_frameUBCache);
            this->valid = true;
        }

        FrameData (Window *w);
        virtual ~FrameData () override;
    };

    /// Rendering information for non-shadow mode
    RenderInfo _noShadowsRInfo;

    /// Rendering information for the ambient-lighting pass when shadows are enabled
    RenderInfo _ambientWShadowsRInfo;
    /// Rendering information for rendering a light's shadow map
    DepthRenderInfo _shadowMapRInfo;
    /// Rendering information for rendering a light when shadows are enabled
    RenderInfo _lightRInfo;

    /// allocate and initialize the meshes and drawables
    void _initMeshes (const Scene *scene);

    /// initialize the rendering information
    void _initRenderInfo ();
    /// initialize the descriptor pool and layouts for the uniform buffers
    void _initDescriptorPools ();

    /// update the per-frame cache
    void _updateFrameUBCache ();

    /// invalidate the per-frame UBOs and update the per-frame cache
    void _invalidateFrameUBOs ();

    /// record the rendering commands
    void _recordCommandBuffer (Proj3Window::FrameData *frame);

    /// get the scene being rendered
    const Scene *_scene () const
    {
        return reinterpret_cast<Proj3 *>(this->_app)->scene();
    }

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `FrameData` struct.
    cs237::Window::FrameData *_allocFrameData (cs237::Window *w) override;

}; // Proj3Window

#endif // !_WINDOW_HPP_
