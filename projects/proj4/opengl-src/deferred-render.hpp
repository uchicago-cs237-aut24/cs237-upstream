/*! \file deferred-render.hxx
 *
 * Render classes to support deferred rendering
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _DEFERRED_RENDER_HXX_
#define _DEFERRED_RENDER_HXX_

#include "render.hxx"

//! an abstract base class that wraps a deferred-render pass
//
class DeferredPass {
  public:

    virtual ~DeferredPass ();

  protected:
    cs237::ShaderProgram *_shader;      //!< the shader program
    GBuffer *_gbuffer;                  //!< the geometry buffer that is the render target

  //! constructor (protected)
    DeferredPass (cs237::ShaderProgram *sh, GBuffer *gb);

};

//! A renderer to draw the scene using just the G Buffer
//
class GeomRenderer : public DeferredPass {
  public:
    GeomRenderer (GBuffer * gbuffer);
    virtual ~GeomRenderer ();

  //! enable the renderer; this function does initialization that is
  //! common to multiple objects rendered with the same renderer
    void Enable ();

  //! render a mesh using this renderer
  //! \param projMat the projection matrix for the current camera state
  //! \param viewMat the view matrix for the camera
  //! \param mesh the mesh to be rendered
    void Render (
        cs237::mat4f const &projMat,
        cs237::mat4f const &viewMat,
        const Instance *mesh);

  protected:
    int     _mvpMatLoc;                 //!< the model-view-projection matrix uniform
    int     _modelMatLoc;               //!< the model matrix uniform
    int     _normMatLoc;                //!< the normal-vector transform matrix uniform
    int     _hasDiffuseMapLoc;          //!< the location of the diffuse texture-mapping flag
    int     _diffuseCLoc;               //!< the diffuse-color uniform
    int     _diffuseMapLoc;             //!< the diffuse-color-map texture uniform
    int     _emissiveSrcLoc;
    int     _emissiveCLoc;
    int     _emissiveMapLoc;
    int     _specularSrcLoc;
    int     _specularCLoc;
    int     _specularMapLoc;
    int     _sharpnessLoc;
    int     _hasNormalMapLoc;
    int     _normalMapLoc;
};

//! A common base class for the spotlight and directional light renderers
//
class LightVolRenderer : public DeferredPass {
  public:
    virtual ~LightVolRenderer ();

  protected:
    cs237::vec3f _lightDir;             //!< unit vector that specifies the light's direction
    cs237::color3f _lightI;             //!< the directional light's intensity
    cs237::color3f _ambI;               //!< the ambient light's intensity
                                        // Uniform variable locations:
    int _screenSizeLoc;                 //!< the screen size
    int _gbCoordLoc;                    //!< the coodinate buffer location
    int _gbDiffuseLoc;                  //!< the diffuse-color buffer location
    int _gbSpecularLoc;                 //!< the diffuse-color buffer location
    int _gbNormalLoc;                   //!< the normals buffer location
    int _cameraPosLoc;                  //!< the cameraPos location

  //! LightVolRenderer constructor
  //! \param gbuffer pointer to the gbuffer
    LightVolRenderer (cs237::ShaderProgram *sh, GBuffer *gbuffer);

  //! bind textures to the GBuffers
    void BindTextures ();

};

//! A renderer to perform the stencil-pass on a spot-light-volume
//
class StencilRenderer : public DeferredPass {
  public:

    StencilRenderer (GBuffer *gbuffer);
    virtual ~StencilRenderer ();

    void Render (cs237::mat4f const &mpvMat, SpotLight const &light);

  protected:
                                        // Uniform variable locations:
    int         _mvpLoc;                //!< the model-view-projection matrix uniform
};

//! A renderer to draw the light volume for a spotlight
//
class SpotlightRenderer : public LightVolRenderer {
  public:

  //! SpotlightRenderer constructor
  //! \param dir the light's direction vector
  //! \param i the light's intensity
    SpotlightRenderer (GBuffer *gbuffer);
    virtual ~SpotlightRenderer ();

    void Enable (cs237::vec3f const &camPos);

    void Render (
        cs237::mat4f const &projMat,
        cs237::mat4f const &viewMat,
        SpotLight const &light);

  protected:
    void GetSpotLightLocs();

  //!< a struct holding the locations for the members of the spotlight struct
    struct SpotlightLocs {
        int position;
        int intensity;
        int constant;
        int linear;
        int quadratic;
        int direction;
        int cutoff;
        int exponent;
    };

    StencilRenderer _stencilRenderer;   //!< the stencil renderer
                                        // Uniform variable locations:
    int _mvpLoc;                        //!< the model-view-projection matrix uniform
    SpotlightLocs _spotlightLocs;       //!< the locations for the spotlight
};

//! A renderer to draw the directional light volume
//
class DirLightRenderer : public LightVolRenderer {
  public:

  //! DirLightRenderer constructor
  //! \param dir the light's direction vector
  //! \param i the light's intensity
    DirLightRenderer (
        cs237::vec3f const &dir,
        cs237::color3f const &i,
        GBuffer *gbuffer);

    virtual ~DirLightRenderer ();

    void Render (cs237::vec3f const &camPos);

  protected:
    GLuint                      _vaoId;         //!< vertex array object for screen quad

};

//! The final renderer, which draws the screen and merges the lighting
//! information from the lighting passes with the ambient and emissive
//! lighting
//
class FinalRenderer : public DeferredPass {
  public:
    FinalRenderer (GBuffer *gbuffer, cs237::color3f const &ambI);
    virtual ~FinalRenderer ();

  //! render a texture using the screen quad
    void Render ();

  protected:
    GLuint _vaoId;                      //!< vertex array object for screen quad
                                        // Uniform variable locations:
    int _screenSizeLoc;                 //!< the screen size location
    int _gbDiffuseLoc;                  //!< the diffuse-color buffer location
    int _gbEmissiveLoc;                 //!< the emissive-color buffer location
    int _gbFinalLoc;                    //!< the final buffer location
};

//! A version of the final renderer that supports Screen-space Ambient Occlusion.
//
class SSAORenderer : public DeferredPass {
  public:
    SSAORenderer (GBuffer *gbuffer, cs237::color3f const &ambI);
    virtual ~SSAORenderer ();

  //! render a texture using the screen quad
    void Render ();

  protected:
    GLuint _vaoId;                      //!< vertex array object for screen quad
    GLuint _randVecsUB;                 //!< uniform buffer of random directions and
                                        //!< lengths
                                        // Uniform variable locations:
    int _screenSizeLoc;                 //!< the screen size location
    int _gbDiffuseLoc;                  //!< the diffuse-color buffer location
    int _gbEmissiveLoc;                 //!< the emissive-color buffer location
    int _gbNormalLoc;                   //!< the normals-buffer location
    int _gbDepthLoc;                    //!< the depth-buffer location
    int _gbFinalLoc;                    //!< the final buffer location
    int _randVecsUBIdx;                 //!< index of random vectors block
};

#endif // !_DEFERRED_RENDER_HXX_
