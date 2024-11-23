/*! \file view.hxx
 *
 * \brief type definitions for tracking the view state.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _VIEW_HXX_
#define _VIEW_HXX_

#include "cs237.hxx"
#include "scene.hxx"
#include "instance.hxx"
#include "render.hxx"
#include "mesh.hxx"
#include "ground.hxx"
#include "spot-light.hxx"
#include "g-buffer.hxx"
#include "deferred-render.hxx"

enum RenderMode {
    WIREFRAME = 0,      //!< render scene as a wireframe
    TEXTURING,          //!< render the scene using texture mapping and direct diffuse lighting
    DEFERRED,           //!< render the scene using deferred lighting
    NUM_RENDER_MODES    //!< == to the number of rendering modes
};

/*! \brief The current state of the view */
struct View {
    std::string         sceneName;      //!< the name of the scene (used for screen shots)
    GLFWwindow          *win;           //!< the application window
    bool                shouldExit;     //!< set to true when the application should exit
    bool                needsRedraw;    //!< set to true when the display is out of date
  // Camera state
    cs237::vec3f        camPos;         //!< initial camera position in world space
    cs237::vec3f        camAt;          //!< camera look-at point in world space
    cs237::vec3f        camUp;          //!< initial camera up vector in world space
    float               fov;            //!< horizontal field of view specified by scene
    cs237::mat4f        camRot;         //!< cumlative camera rotation
    float               camOffset;      //!< offset for camera from initial distance to lookAt point.
    float               minOffset;      //!< minimum allowed offset
    float               maxOffset;      //!< maximum allowed offset
  // view info
    cs237::mat4f        viewMat;        //!< the current view matrix
    cs237::mat4f        projMat;        //!< the current projection matrix
    int                 wid, ht;        //!< window dimensions
    int                 fbWid, fbHt;    //!< framebuffer dimensions
    bool                isVis;          //!< true, when the window is visible
  // axes support
    class Axes          *axes;          //!< for drawing the world-space axes
    bool                drawAxes;       //!< draw the axes when true
  // light direction support
    class Line          *lightDir;      //!< for drawing the light direction
    bool                drawLightDir;   //!< draw the light direction when true
  // rendering state
    RenderMode          mode;           //!< the current rendering mode
    Renderer            renderers[NUM_RENDER_MODES];
                                        //!< the array of forward renderers indexed by mode
  // Renderers for deferred rendering
    GeomRenderer        *geomRenderer;
    SpotlightRenderer   *spotlightRenderer;
    DirLightRenderer    *dirLightRenderer;
    FinalRenderer       *finalRenderer;
    SSAORenderer        *ssaoRenderer;
    bool		enableSSAO;     //!< true if SSAO is enabled
  // the g-buffer object for the view
    GBuffer             *gbuffer;
  // scene info
    std::vector<SpotLight> lights;      //!< the spot lights in the scene
    std::vector<MeshInfo> meshes;       //!< the meshes that represent the objects
    std::vector<Instance> objects;      //!< the objects in the scene
    Ground              *ground;        //!< ground mesh; nullptr if not present

  //! constructor for the view
    View (Scene const &scene, GLFWwindow *win);

  //! Bind the default frame buffer to the rendering context and set the viewport
    void BindFramebuffer ();

  //! Make the view's window be the current OpenGL context
    void MakeCurrent () { glfwMakeContextCurrent (this->win); }

  /* rotate the camera around the look-at point by the given angle (in degrees) */
    void RotateLeft (float angle);

  /*! rotate the camera up by the given angle (in degrees) */
    void RotateUp (float angle);

  /*! move the camera towards the look-at point by the given distance */
    void Move (float dist);

  /*! \brief initialize the renderers by loading and compiling their shaders.
   *  Note that this function needs to be called after the current
   *  OpenGL context has been set.
   */
    void InitRenderers (Scene const &scene);

  /*! \brief initialize the projection matrix based on the current camera state. */
    void InitProjMatrix ();

  /*! \brief initialize the view matrix based on the current camera state. */
    void InitViewMatrix ();

  /*! \brief initialize the g-buffer for the view */
    void InitGBuffer();

  /*! \brief render the state of the scene using a forward renderer
   */
    void Render ();

  /*! \brief render the state of the scene using the deferred renderer
   */
    void DeferredRender ();

  /*! \brief dump a screen shot of the window to a file as a PNG image.
   * This function grabs the front buffer.
   */
    void ScreenShot (std::string const &file);

};

#endif /* !_VIEW_HXX_ */
