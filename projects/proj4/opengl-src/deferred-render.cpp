/*! \file deferred-render.cxx
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "deferred-render.hxx"
#include "view.hxx"
#include <map>

//! for specifying component sources; this definition need to agree with the
//! corresponding definitions in the fragment shaders.
enum {
    NoComponent = 0,
    UniformComponent = 1,
    SamplerComponent = 2
};

/* The path to the shaders; this is usually set from the compiler command-line.
 * but it defaults to a path relative to the build directory.
 */
#ifndef SHADER_DIR
#  define SHADER_DIR "../shaders/"
#endif

//! \brief Load, compile, and link a shader program.
//! \param vertShader the path to the vertex shader
//! \param fragShader the path to the fragment shader
//! \param geomShader the path to the geometry shader ("" for no geometry pass)
//! \return the shader program
//
static cs237::ShaderProgram *LoadShader (
    std::string const &vertShader,
    std::string const &fragShader)
{
  /// cache of previously loaded shaders
    static std::map<std::string, cs237::VertexShader *> VShaders;
    static std::map<std::string, cs237::FragmentShader *> FShaders;
    static std::map<std::string, cs237::GeometryShader *> GShaders;

    cs237::VertexShader *vsh;
    cs237::FragmentShader *fsh;

    auto vit = VShaders.find(vertShader);
    if (vit == VShaders.end()) {
        vsh = new cs237::VertexShader (vertShader.c_str());
        VShaders.insert (
            std::pair<std::string, cs237::VertexShader *>(vertShader, vsh));
    }
    else {
        vsh = vit->second;
    }

    auto fit = FShaders.find(fragShader);
    if (fit == FShaders.end()) {
        fsh = new cs237::FragmentShader (fragShader.c_str());
        FShaders.insert (
            std::pair<std::string, cs237::FragmentShader *>(fragShader, fsh));
    }
    else {
        fsh = fit->second;
    }

    cs237::ShaderProgram *shader = new cs237::ShaderProgram (*vsh, *fsh);

    if (shader == nullptr) {
        std::cerr << "Cannot build shader" << std::endl;
        exit (1);
    }

    return shader;

}

/***** virtual base-class DeferredPass member functions *****/

DeferredPass::DeferredPass (cs237::ShaderProgram *sh, GBuffer *gb)
    : _shader(sh), _gbuffer(gb)
{ }

DeferredPass::~DeferredPass ()
{ }

/***** class GeomRenderer member functions *****/

/* texture units for uniform samplers */
enum {
    DiffuseMapUnit = 0,
    EmissiveMapUnit = 1,
    SpecularMapUnit = 2,
    NormalMapUnit = 3
};

GeomRenderer::GeomRenderer (GBuffer *gbuffer)
  : DeferredPass (
        LoadShader (SHADER_DIR "geo-pass.vsh", SHADER_DIR "geo-pass.fsh"),
        gbuffer),
    _mvpMatLoc(this->_shader->UniformLocation ("mvpMat")),
    _modelMatLoc(this->_shader->UniformLocation ("modelMat")),
    _normMatLoc(this->_shader->UniformLocation ("normMat")),
    _hasDiffuseMapLoc(this->_shader->UniformLocation ("hasDiffuseMap")),
    _diffuseCLoc(this->_shader->UniformLocation ("diffuseC")),
    _diffuseMapLoc(this->_shader->UniformLocation ("diffuseMap")),
    _emissiveSrcLoc(this->_shader->UniformLocation ("emissiveSrc")),
    _emissiveCLoc(this->_shader->UniformLocation ("emissiveC")),
    _emissiveMapLoc(this->_shader->UniformLocation ("emissiveMap")),
    _specularSrcLoc(this->_shader->UniformLocation ("specularSrc")),
    _specularCLoc(this->_shader->UniformLocation ("specularC")),
    _specularMapLoc(this->_shader->UniformLocation ("specularMap")),
    _sharpnessLoc(this->_shader->UniformLocation ("sharpness")),
    _hasNormalMapLoc(this->_shader->UniformLocation ("hasNormalMap")),
    _normalMapLoc(this->_shader->UniformLocation ("normalMap"))
{ }

GeomRenderer::~GeomRenderer ()
{ }

void GeomRenderer::Enable ()
{
    assert(this->_gbuffer != nullptr);

    this->_shader->Use();

    CS237_CHECK( glDepthMask (GL_TRUE) );
    CS237_CHECK( glEnable (GL_DEPTH_TEST));
    CS237_CHECK( glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) );
    CS237_CHECK( glEnable (GL_CULL_FACE));
    CS237_CHECK( glCullFace (GL_BACK));

}

void GeomRenderer::Render (
    cs237::mat4f const &projMat,
    cs237::mat4f const &viewMat,
    const Instance *obj)
{
  // set model-view-projection matrix
    cs237::mat4f mvpMat = projMat * viewMat * obj->toWorld;

    cs237::setUniform (this->_mvpMatLoc, mvpMat);

  // set the normal matrix
    cs237::setUniform (this->_normMatLoc, obj->toWorld.normalMatrix());

  // set the model matrix.
    cs237::setUniform (this->_modelMatLoc, obj->toWorld);

  // set the texture units
    cs237::setUniform (this->_diffuseMapLoc, DiffuseMapUnit);
    cs237::setUniform (this->_emissiveMapLoc, EmissiveMapUnit);
    cs237::setUniform (this->_specularMapLoc, SpecularMapUnit);
    cs237::setUniform (this->_normalMapLoc, NormalMapUnit);

    for (auto it = obj->meshes.begin(); it != obj->meshes.end();  it++) {
        MeshInfo *mesh = *it;
      // set up the diffuse color
        if (mesh->hasDiffuseMap()) {
            mesh->BindDiffuseMap (GL_TEXTURE0 + DiffuseMapUnit);
            cs237::setUniform (this->_hasDiffuseMapLoc, GL_TRUE);
        }
        else {
            cs237::setUniform (this->_hasDiffuseMapLoc, GL_FALSE);
            cs237::setUniform (this->_diffuseCLoc, mesh->DiffuseColor());
        }
      // set up the emissive color (if any)
        if (mesh->hasEmissive ()) {
            cs237::setUniform (this->_emissiveSrcLoc, UniformComponent);
            cs237::setUniform (this->_emissiveCLoc, mesh->EmissiveColor());
        }
        else if (mesh->EmissiveMap() != nullptr) {
            mesh->BindEmissiveMap (GL_TEXTURE0 + EmissiveMapUnit);
            cs237::setUniform (this->_emissiveSrcLoc, SamplerComponent);
        }
        else {
            cs237::setUniform (this->_emissiveSrcLoc, NoComponent);
        }
      // set up the specular color (if any)
        if (mesh->hasSpecular ()) {
            cs237::setUniform (this->_specularSrcLoc, UniformComponent);
            cs237::setUniform (this->_specularCLoc, mesh->SpecularColor());
            cs237::setUniform (this->_sharpnessLoc, mesh->Sharpness());
        }
        else if (mesh->SpecularMap() != nullptr) {
            mesh->BindEmissiveMap (GL_TEXTURE0 + SpecularMapUnit);
            cs237::setUniform (this->_specularSrcLoc, SamplerComponent);
            cs237::setUniform (this->_sharpnessLoc, mesh->Sharpness());
        }
        else {
            cs237::setUniform (this->_specularSrcLoc, NoComponent);
        }
      // set up the normal map (if present)
        if (mesh->hasNormalMap()) {
            mesh->BindNormalMap (GL_TEXTURE0 + NormalMapUnit);
            cs237::setUniform (this->_hasNormalMapLoc, GL_TRUE);
        }
        else {
            cs237::setUniform (this->_hasNormalMapLoc, GL_FALSE);
        }
      // draw the mesh with normals and texture coordinates enabled, but not normal-mapping tangents
        mesh->Draw (true, true, false);
    }

}


/***** class LightVolRenderer member functions *****/

LightVolRenderer::LightVolRenderer (cs237::ShaderProgram *sh, GBuffer *gbuffer)
  : DeferredPass (sh, gbuffer),
    _screenSizeLoc(this->_shader->UniformLocation ("screenSize")),
    _gbCoordLoc(this->_shader->UniformLocation ("gbCoordBuf")),
    _gbDiffuseLoc(this->_shader->UniformLocation ("gbDiffuseBuf")),
    _gbSpecularLoc(this->_shader->UniformLocation ("gbSpecularBuf")),
    _gbNormalLoc(this->_shader->UniformLocation ("gbNormalBuf")),
    _cameraPosLoc(this->_shader->UniformLocation ("cameraPos"))
{
  // set the locations of the sampler uniforms
    this->_shader->Use();
    cs237::setUniform (this->_gbCoordLoc, GBuffer::COORD_BUF);
    cs237::setUniform (this->_gbDiffuseLoc, GBuffer::DIFFUSE_BUF);
    cs237::setUniform (this->_gbSpecularLoc, GBuffer::SPECULAR_BUF);
    cs237::setUniform (this->_gbNormalLoc, GBuffer::NORM_BUF);
/* if we add shadow mapping, then we will need the DEPTH_BUF */
}

LightVolRenderer::~LightVolRenderer ()
{ }

void LightVolRenderer::BindTextures ()
{
    GBuffer::Type bufs[] = {
            GBuffer::COORD_BUF, GBuffer::DIFFUSE_BUF, GBuffer::SPECULAR_BUF,
            GBuffer::NORM_BUF
        };

    for (int i = 0;  i < sizeof(bufs)/sizeof(GBuffer::Type);  i++) {
        this->_gbuffer->BindTexForReading (GL_TEXTURE0 + bufs[i], bufs[i]);
    }

}

/***** class StencilRenderer member functions *****/

StencilRenderer::StencilRenderer (GBuffer * gbuffer)
  : DeferredPass (
        LoadShader (SHADER_DIR "obj-to-clip.vsh", SHADER_DIR "stencil-pass.fsh"),
        gbuffer),
    _mvpLoc(this->_shader->UniformLocation ("mvpMat"))
{ }

StencilRenderer::~StencilRenderer ()
{ }

void StencilRenderer::Render (cs237::mat4f const &mvpMat, SpotLight const &light)
{
    this->_shader->Use();
    assert(_gbuffer);

  // disable the draw buffers
    CS237_CHECK( glDrawBuffer (GL_NONE) );

  // Enable depth-testing, but disable writing to the depth buffer
    CS237_CHECK( glEnable (GL_DEPTH_TEST) );
    CS237_CHECK( glDepthMask (GL_FALSE) );

  // We want to disable culling the faces because the stencil test needs to test on both
  // back and front faces.
    CS237_CHECK( glDisable(GL_CULL_FACE) );

  // Clear the stencil buffer
    CS237_CHECK( glClear(GL_STENCIL_BUFFER_BIT) );

  // We need the stencil test to be enabled but we want it
  // to succeed always. Only the depth test matters.
    CS237_CHECK( glStencilFunc (GL_ALWAYS, 0, 0xff) );
    CS237_CHECK( glStencilOpSeparate (GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP) );
    CS237_CHECK( glStencilOpSeparate (GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP) );

    cs237::setUniform (this->_mvpLoc, mvpMat);
    Instance *obj = light.cone;
    for (auto it = obj->meshes.begin(); it != obj->meshes.end();  it++) {
        MeshInfo *mesh = *it;
      // draw the mesh with normals, texture coordinates and tangents disabled
        mesh->Draw (false, false, false);
    }

}


/***** class SpotlightRenderer member functions *****/

SpotlightRenderer::SpotlightRenderer (GBuffer *gbuffer)
  : LightVolRenderer (
        LoadShader (SHADER_DIR "obj-to-clip.vsh", SHADER_DIR "spot-light.fsh"),
        gbuffer),
    _stencilRenderer (gbuffer),
    _mvpLoc(this->_shader->UniformLocation ("mvpMat"))
{
  // get locations of spot-light properties
    this->_spotlightLocs.position = this->_shader->UniformLocation("light.position");
    this->_spotlightLocs.constant = this->_shader->UniformLocation("light.constant");
    this->_spotlightLocs.linear = this->_shader->UniformLocation("light.linear");
    this->_spotlightLocs.quadratic = this->_shader->UniformLocation("light.quadratic");
    this->_spotlightLocs.direction = this->_shader->UniformLocation("light.direction");
    this->_spotlightLocs.intensity = this->_shader->UniformLocation("light.intensity");
    this->_spotlightLocs.cutoff = this->_shader->UniformLocation("light.cutoff");
    this->_spotlightLocs.exponent = this->_shader->UniformLocation("light.exponent");

}

SpotlightRenderer::~SpotlightRenderer ()
{ }

void SpotlightRenderer::Enable (cs237::vec3f const &camPos)
{
    this->_shader->Use();

  // set the uniforms
    cs237::setUniform (
        this->_screenSizeLoc,
        cs237::vec2f(this->_gbuffer->Width(), this->_gbuffer->Height()));
    cs237::setUniform (this->_cameraPosLoc, camPos);

}

void SpotlightRenderer::Render (
    cs237::mat4f const &projMat,
    cs237::mat4f const &viewMat,
    SpotLight const &light)
{
  // set up the model-view matrix for the spot-light
    cs237::mat4f mvpMat = projMat * viewMat * light.cone->toWorld;

  // we need the stencil test for both the stencil and lighting passes
    CS237_CHECK( glEnable(GL_STENCIL_TEST) );

  // disable writing to the depth buffer
    CS237_CHECK( glDepthMask (GL_FALSE) );

  // first we render the stencil
    this->_stencilRenderer.Render (mvpMat, light);

  // switch to the light-volume shader
    this->_shader->Use();

  // Setup the G-buffer for the light-pass
    CS237_CHECK( glDrawBuffer(GBuffer::FINAL_ATTACHMENT) );

  // Make sure depth-testing is not enabled
    CS237_CHECK( glDisable (GL_DEPTH_TEST) );

  // Only perform the fragment shader on pixels that have a stencil value of more than 0.
    CS237_CHECK( glEnable(GL_STENCIL_TEST) );
    CS237_CHECK( glStencilFunc (GL_NOTEQUAL, 0, 0xFF) );
    CS237_CHECK( glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP) );

  // Turn on blending for the light-pass
    CS237_CHECK( glEnable (GL_BLEND) );
    CS237_CHECK( glBlendEquation (GL_FUNC_ADD) );
    CS237_CHECK( glBlendFunc (GL_ONE, GL_ONE) );

  // Cull the front-face. The camera may be inside the light volume and if
  // we do back face culling as we normally do we will not see the light
  // until we exit its volume.
    CS237_CHECK( glEnable (GL_CULL_FACE) );
    CS237_CHECK( glCullFace (GL_FRONT) );

  // bind the G-buffer textures
    this->BindTextures ();

  // set the spot-light's uniforms
    cs237::setUniform(this->_spotlightLocs.position, light.pos);
    cs237::setUniform(this->_spotlightLocs.constant, light.atten[0]);
    cs237::setUniform(this->_spotlightLocs.linear, light.atten[1]);
    cs237::setUniform(this->_spotlightLocs.quadratic, light.atten[2]);
    cs237::setUniform(this->_spotlightLocs.direction, light.dir);
    cs237::setUniform(this->_spotlightLocs.cutoff, light.cosCutoff);
    cs237::setUniform(this->_spotlightLocs.intensity, light.intensity);
    cs237::setUniform(this->_spotlightLocs.exponent, light.exponent);

  // set up the model-view matrix for the spot-light
    cs237::setUniform (this->_mvpLoc, mvpMat);

  // render the spot-light volume.
    Instance *obj = light.cone;
    for (auto it = obj->meshes.begin(); it != obj->meshes.end();  it++) {
        MeshInfo *mesh = *it;
      // draw the mesh with normals, texture coordinates and tangents disabled
        mesh->Draw (false, false, false);
    }

  // cleanup
    CS237_CHECK( glDisable (GL_BLEND) );
    CS237_CHECK( glDisable (GL_STENCIL_TEST) );

}

/***** class DirLightRenderer member functions *****/

DirLightRenderer::DirLightRenderer (
    cs237::vec3f const &dir,
    cs237::color3f const &i,
    GBuffer *gbuffer)
  : LightVolRenderer (
        LoadShader (SHADER_DIR "screen-quad.vsh", SHADER_DIR "dir-light.fsh"),
        gbuffer)
{
  // allocate VAO for drawing screen quad
    CS237_CHECK( glGenVertexArrays (1, &(this->_vaoId)) );

  // set lighting uniforms
    this->_shader->Use();
    cs237::setUniform (this->_shader->UniformLocation("light.direction"), dir);
    cs237::setUniform (this->_shader->UniformLocation("light.intensity"), i);

}

void DirLightRenderer::Render (cs237::vec3f const &camPos)
{
    assert(this->_gbuffer != nullptr);

    CS237_CHECK( glDrawBuffer (GBuffer::FINAL_ATTACHMENT) );

  // Make sure depth-testing is not enabled
    CS237_CHECK( glDepthMask(GL_FALSE) );
    CS237_CHECK( glDisable(GL_DEPTH_TEST) );

  // Make sure that stencil testing has been disabled
    CS237_CHECK( glDisable (GL_STENCIL_TEST) );

  // Turn on blending for the light-pass
    CS237_CHECK( glEnable(GL_BLEND) );
    CS237_CHECK( glBlendEquation(GL_FUNC_ADD) );
    CS237_CHECK( glBlendFunc(GL_ONE, GL_ONE) );

  // Disable culling
    CS237_CHECK( glDisable (GL_CULL_FACE) );

    this->_shader->Use();

  // set g-buffer textures
    this->BindTextures ();

  // set uniforms
    cs237::setUniform (
        this->_screenSizeLoc,
        cs237::vec2f(this->_gbuffer->Width(), this->_gbuffer->Height()));
    cs237::setUniform (this->_cameraPosLoc, camPos);

  // render the screen quad as a fan using vertex IDs
    CS237_CHECK( glBindVertexArray (this->_vaoId) );
    CS237_CHECK( glDrawArrays (GL_TRIANGLES, 0, 6) );

}

DirLightRenderer::~DirLightRenderer ()
{ }


/***** class FinalRenderer member functions *****/

FinalRenderer::FinalRenderer (GBuffer *gbuffer, cs237::color3f const &ambI)
  : DeferredPass(
        LoadShader (SHADER_DIR "screen-quad.vsh", SHADER_DIR "final.fsh"),
        gbuffer),
    _screenSizeLoc(this->_shader->UniformLocation ("screenSize")),
    _gbDiffuseLoc(this->_shader->UniformLocation ("gbDiffuseBuf")),
    _gbEmissiveLoc(this->_shader->UniformLocation ("gbEmissiveBuf")),
    _gbFinalLoc(this->_shader->UniformLocation ("gbFinalBuf"))
{
  // allocate VAO for drawing screen quad
    CS237_CHECK( glGenVertexArrays (1, &(this->_vaoId)) );

  // set lighting uniforms
    this->_shader->Use();
    cs237::setUniform (this->_shader->UniformLocation("ambient"), ambI);
}

FinalRenderer::~FinalRenderer ()
{ }

void FinalRenderer::Render ()
{
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );

    CS237_CHECK( glDrawBuffer (GL_BACK) );
    CS237_CHECK( glClear (GL_COLOR_BUFFER_BIT) );

  // enable
    this->_shader->Use();

    CS237_CHECK( glDisable (GL_BLEND) );
    CS237_CHECK( glDisable (GL_DEPTH_TEST) );
    CS237_CHECK( glDisable (GL_STENCIL_TEST) );
    CS237_CHECK( glDisable (GL_CULL_FACE) );

  // set up textures for final rendering pass (ambient, emissive, and final buffers)
    _gbuffer->BindTexForReading (GL_TEXTURE0, GBuffer::DIFFUSE_BUF);
    cs237::setUniform (this->_gbDiffuseLoc, 0);
    _gbuffer->BindTexForReading (GL_TEXTURE1, GBuffer::EMISSIVE_BUF);
    cs237::setUniform (this->_gbEmissiveLoc, 1);
    _gbuffer->BindTexForReading (GL_TEXTURE2, GBuffer::FINAL_BUF);
    cs237::setUniform (this->_gbFinalLoc, 2);

  // set uniforms
    cs237::setUniform (
        this->_screenSizeLoc,
        cs237::vec2f(this->_gbuffer->Width(), this->_gbuffer->Height()));

  // render the screen quad using vertex IDs
    CS237_CHECK( glBindVertexArray (this->_vaoId) );
    CS237_CHECK( glDrawArrays (GL_TRIANGLES, 0, 6) );
    CS237_CHECK( glBindVertexArray (0) );
}


/***** class SSAORenderer member functions *****/

#include "ssao-util.hxx"

SSAORenderer::SSAORenderer (GBuffer *gbuffer, cs237::color3f const &ambI)
  : DeferredPass(
        LoadShader (SHADER_DIR "screen-quad.vsh", SHADER_DIR "ssao.fsh"),
        gbuffer),
    _randVecsUB(InitRandomBuffer()),
    _screenSizeLoc(this->_shader->UniformLocation ("screenSize")),
    _gbDiffuseLoc(this->_shader->UniformLocation ("gbDiffuseBuf")),
    _gbEmissiveLoc(this->_shader->UniformLocation ("gbEmissiveBuf")),
    _gbNormalLoc(this->_shader->UniformLocation ("gbNormalBuf")),
    _gbDepthLoc(this->_shader->UniformLocation ("gbDepthBuf")),
    _gbFinalLoc(this->_shader->UniformLocation ("gbFinalBuf")),
    _randVecsUBIdx(this->_shader->UniformBlockIndex ("RandomDirUB"))
{
  // allocate VAO for drawing screen quad
    CS237_CHECK( glGenVertexArrays (1, &(this->_vaoId)) );

  // set lighting uniforms
    this->_shader->Use();
    cs237::setUniform (this->_shader->UniformLocation("ambient"), ambI);

  // set random vectors uniform
    CS237_CHECK( glBindBufferBase (GL_UNIFORM_BUFFER, 0, this->_randVecsUB) );
    CS237_CHECK( glUniformBlockBinding (this->_shader->Id(), this->_randVecsUBIdx, 0) );

}

SSAORenderer::~SSAORenderer ()
{ }

void SSAORenderer::Render ()
{
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );

    CS237_CHECK( glDrawBuffer (GL_BACK) );
    CS237_CHECK( glClear (GL_COLOR_BUFFER_BIT) );

  // enable
    this->_shader->Use();

    CS237_CHECK( glDisable (GL_BLEND) );
    CS237_CHECK( glDisable (GL_DEPTH_TEST) );
    CS237_CHECK( glDisable (GL_STENCIL_TEST) );
    CS237_CHECK( glDisable (GL_CULL_FACE) );

  // set up textures for final rendering pass (ambient, emissive, and final buffers)
    _gbuffer->BindTexForReading (GL_TEXTURE0, GBuffer::DIFFUSE_BUF);
    cs237::setUniform (this->_gbDiffuseLoc, 0);
    _gbuffer->BindTexForReading (GL_TEXTURE1, GBuffer::EMISSIVE_BUF);
    cs237::setUniform (this->_gbEmissiveLoc, 1);
    _gbuffer->BindTexForReading (GL_TEXTURE2, GBuffer::FINAL_BUF);
    cs237::setUniform (this->_gbFinalLoc, 2);
    _gbuffer->BindTexForReading (GL_TEXTURE3, GBuffer::NORM_BUF);
    cs237::setUniform (this->_gbNormalLoc, 3);
    _gbuffer->BindTexForReading (GL_TEXTURE4, GBuffer::DEPTH_BUF);
    cs237::setUniform (this->_gbDepthLoc, 4);

  // set uniforms
    cs237::setUniform (
        this->_screenSizeLoc,
        cs237::vec2f(this->_gbuffer->Width(), this->_gbuffer->Height()));

  // render the screen quad using vertex IDs
    CS237_CHECK( glBindVertexArray (this->_vaoId) );
    CS237_CHECK( glDrawArrays (GL_TRIANGLES, 0, 6) );
    CS237_CHECK( glBindVertexArray (0) );
}
