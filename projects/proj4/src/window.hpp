/*! \file window.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 4
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
#include "render-modes.hpp"
#include "scene.hpp"
#include "shader-uniforms.hpp"
#include "instance.hpp"

/// constants to define the near and far planes of the view frustum
constexpr float kNearZ = 0.5;   // how close to the origin you can get
constexpr float kFarZ = 500.0f;  // distance to far plane
constexpr float kFOV = 90.0f;        ///< field of view angle in degrees
constexpr float kCameraSpeed = 2.0f; ///< camera rotation speed (in degrees)

/// struct to collect pipeline info
struct PipelineInfo {
    vk::PipelineLayout layout;  ///< pipeline layout
    vk::Pipeline pipe;          ///< pipeline

    void destroy (vk::Device device)
    {
        device.destroyPipeline(this->pipe);
        device.destroyPipelineLayout(this->layout);
    }
};

/// The Project 4 Window class
class Proj4Window : public cs237::Window {
public:
    Proj4Window (Proj4 *app);

    ~Proj4Window () override;

    void draw () override;

    /// reshape the window
    void reshape (int wid, int ht) override;

    /// handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:
    RenderFlags _renderFlags;                   ///< rendering controls

    // scene data
    std::vector<Mesh *> _meshes;                ///< the meshes in the scene
    std::vector<Instance *> _objs;              ///< the objects to render

    // Current camera state
    glm::vec3 _camPos;                          ///< camera position in world space
    glm::vec3 _camAt;                           ///< camera look-at point in world space
    glm::vec3 _camUp;                           ///< camera up vector in world space
    float _angle;                               ///< rotation angle for camera
    float _radius;                              ///< distance from look-at point

    glm::mat4 _viewM;                           ///< current view matrix
    glm::mat4 _projM;                           ///< current projection matrix

    vk::DescriptorPool _descPool;               ///< the descriptor pool for uniforms

    // resources for forward rendering modes (eWireFrame and eTexture)
    // some of these are also used in the geometry pass
    LightingUBO *_lightingUBO;                  ///< per-scene lighting information
    vk::DescriptorSetLayout _lightingLayout;    ///< descriptor-set layout for lightingUBO
    vk::DescriptorSet _lightingDS;              ///< descriptor set for lightingUBO

    MeshFactory *_meshFactory;                  ///< a factory object for creating
                                                ///  mesh objects

    /// The render pass for the forward renderers.
    vk::RenderPass _renderPass;

    /// Rendering information for wire-frame-rendering mode
    PipelineInfo _wireFramePipeline;
    /// Rendering information for textured-rendering mode
    PipelineInfo _texturePipeline;

    /** HINT: define resources for deferred rendering */

    /// extend the generic frame-data structure with project-specific data
    struct FrameData : public Window::FrameData {
        /** HINT: add per-fame UBOs etc */
        bool valid;                     ///< true when the contents of the UBO is
                                        ///  valid (i.e., equal to the _sceneUBCache)

        void refresh ()
        {
            /** HINT: update the per-frame UBOs for this frame */
            this->valid = true;
        }

        FrameData (Window *w);
        virtual ~FrameData () override;
    };

    /// allocate and initialize the meshes and drawables
    void _initMeshes (const Scene *scene);

    /// initialize the rendering information for the forward renderers
    void _initForwardRenderInfo ();

    /** HINT: define a method (or methods) initializing render passes,
     ** pipelines, etc for the deferred-rendering mode
     **/

    /// initialize the descriptor pool and layouts for the uniform buffers
    void _initDescriptorPools ();

    /// allocate and initialize the lighting UBO
    void _initLighting ();

    /// record the rendering commands for the forward renderers
    void _recordForwardCommands (Proj4Window::FrameData *frame);

    /** HINT: define a method (or methods) for recording the deferred-rendering-mode
     ** commands into a command buffer.
     **/

    /// get the scene being rendered
    const Scene *_scene () const
    {
        return reinterpret_cast<Proj4 *>(this->_app)->scene();
    }

    /// set the camera position based on the current angle
    void _setCameraPos ()
    {
        float rAngle = glm::radians(this->_angle);
        float camX = this->_radius * sin(rAngle);
        float camZ = this->_radius * cos(rAngle);
        this->_camPos = glm::vec3(camX, this->_camPos.y, camZ);

        // update the UBO cache
        this->_viewM = glm::lookAt(this->_camPos, this->_camAt, this->_camUp);
    }

    /// set the projection matrix based on the current window size
    void _setProjMat ()
    {
        this->_projM = glm::perspectiveFov(
            glm::radians(kFOV),
            float(this->_wid), float(this->_ht),
            kNearZ, kFarZ);
    }

    /// invalidate the per-frame UBOs and update the per-frame cache
    void _invalidateFrameUBOs ();

    /// override the `Window::_allocFrameData` method to allocate our
    /// extended `FrameData` struct.
    cs237::Window::FrameData *_allocFrameData (cs237::Window *w) override;

}; // Proj4Window

#endif // !_WINDOW_HPP_
