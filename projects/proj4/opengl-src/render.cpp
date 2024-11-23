/*! \file render.cxx
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "render.hxx"
#include "view.hxx"
#include <map>

/* The path to the shaders; this is usually set from the compiler command-line.
 * but it defaults to a path relative to the build directory.
 */
#ifndef SHADER_DIR
#  define SHADER_DIR "../shaders/"
#endif

//! for specifying component sources; this definition need to agree with the
//! corresponding definitions in the fragment shaders.
enum {
    NoComponent = 0,
    UniformComponent = 1,
    SamplerComponent = 2
};

//! Load a shader from the file system and compile and link it.  We keep
//! a cache of shaders that have already been loaded to allow two renderers
//! to share the same shader program without compiling it twice.
//
static cs237::ShaderProgram *LoadShader (std::string const & shaderPrefix, bool hasGeom = false)
{
    static std::map<std::string, cs237::ShaderProgram *> Shaders;

    auto it = Shaders.find(shaderPrefix);
    if (it == Shaders.end()) {
      // load, compile, and link the shader program
        cs237::VertexShader vsh((shaderPrefix + ".vsh").c_str());
        cs237::FragmentShader fsh((shaderPrefix + ".fsh").c_str());
        cs237::ShaderProgram *shader;
        if (hasGeom) {
            cs237::GeometryShader gsh((shaderPrefix + ".gsh").c_str());
            shader = new cs237::ShaderProgram (vsh, gsh, fsh);
        }
        else {
            shader = new cs237::ShaderProgram (vsh, fsh);
        }
        if (shader == nullptr) {
            std::cerr << "Cannot build " << shaderPrefix << std::endl;
            exit (1);
        }
        Shaders.insert (std::pair<std::string, cs237::ShaderProgram *>(shaderPrefix, shader));
        return shader;
    }
    else {
        return it->second;
    }

}

/***** virtual base-class RenderPass member functions *****/

RenderPass::RenderPass (cs237::ShaderProgram *sh)
    : _shader(sh)
{ }

RenderPass::~RenderPass ()
{ }

/***** class WireframeRenderer member functions *****/

WireframeRenderer::WireframeRenderer ()
    : RenderPass (LoadShader (SHADER_DIR "wireframe")),
        _mvpLoc(this->_shader->UniformLocation ("mvpMat")),
        _colorLoc(this->_shader->UniformLocation ("color"))
{ }

WireframeRenderer::~WireframeRenderer ()
{ }

void WireframeRenderer::Enable ()
{
    this->_shader->Use();
    CS237_CHECK( glDisable (GL_CULL_FACE) );
    CS237_CHECK( glEnable (GL_DEPTH_TEST) );
    CS237_CHECK( glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) );
}

void WireframeRenderer::Render (cs237::mat4f const &projMat, cs237::mat4f const &viewMat, const Instance *obj)
{
  // set model-view-projection matrix
    cs237::mat4f mvpMat = projMat * viewMat * obj->toWorld;
    cs237::setUniform (this->_mvpLoc, mvpMat);

  // set object color
    cs237::setUniform (this->_colorLoc, cs237::color4f(obj->color));

  // draw
    for (auto it = obj->meshes.begin(); it != obj->meshes.end();  it++) {
        (*it)->Draw (false, false, false);
    }

}

/***** class TexturingRenderer member functions *****/

TexturingRenderer::TexturingRenderer (
    cs237::vec3f const &dir,
    cs237::color3f const &i,
    cs237::color3f const &amb)
    : RenderPass(LoadShader (SHADER_DIR "texture")),
        _lightDir(-dir), _lightI(i), _ambI(amb),
        _mvpLoc(this->_shader->UniformLocation ("mvpMat")),
        _normMatLoc(this->_shader->UniformLocation ("normMat")),
        _lightDirLoc(this->_shader->UniformLocation ("lightDir")),
        _lightILoc(this->_shader->UniformLocation ("lightIntensity")),
        _ambILoc(this->_shader->UniformLocation ("ambIntensity")),
        _hasDiffuseMapLoc(this->_shader->UniformLocation ("hasDiffuseMap")),
        _diffuseCLoc(this->_shader->UniformLocation ("diffuseC")),
        _diffuseMapLoc(this->_shader->UniformLocation ("diffuseMap")),
        _emissiveSrcLoc(this->_shader->UniformLocation ("emissiveSrc")),
        _emissiveCLoc(this->_shader->UniformLocation ("emissiveC")),
        _emissiveMapLoc(this->_shader->UniformLocation ("emissiveMap"))
{
    this->_shader->Use();

  // set lighting uniforms
    cs237::setUniform (this->_lightDirLoc, this->_lightDir);
    cs237::setUniform (this->_lightILoc, this->_lightI);
    cs237::setUniform (this->_ambILoc, this->_ambI);

}

TexturingRenderer::~TexturingRenderer ()
{
}

void TexturingRenderer::Enable ()
{
    this->_shader->Use();
    CS237_CHECK( glEnable (GL_DEPTH_TEST) );
    CS237_CHECK( glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) );
    CS237_CHECK( glEnable (GL_CULL_FACE) );
    CS237_CHECK( glCullFace (GL_BACK) );

}

void TexturingRenderer::Render (
    cs237::mat4f const &projMat,
    cs237::mat4f const &viewMat,
    const Instance *obj)
{
  // set model-view-projection matrix
    cs237::mat4f mvpMat = projMat * viewMat * obj->toWorld;
    cs237::setUniform (this->_mvpLoc, mvpMat);

  // set the normal matrix
    cs237::setUniform (this->_normMatLoc, obj->toWorld.normalMatrix());

  // set the diffuse and emissive texture units
    cs237::setUniform (this->_diffuseMapLoc, 0);
    cs237::setUniform (this->_emissiveMapLoc, 1);

  // render each mesh in the object instance
    for (auto it = obj->meshes.begin(); it != obj->meshes.end();  it++) {
        MeshInfo *mesh = *it;

      // set up the diffuse color
        if (mesh->hasDiffuseMap()) {
            mesh->BindDiffuseMap (GL_TEXTURE0);
            cs237::setUniform (this->_hasDiffuseMapLoc, GL_TRUE);
        }
        else {
            cs237::setUniform (this->_hasDiffuseMapLoc, GL_FALSE);
            cs237::setUniform (this->_diffuseCLoc, mesh->DiffuseColor());
        }

      // set up the emissive color (if any)
        if (mesh->hasEmissiveMap()) {
            mesh->BindEmissiveMap (GL_TEXTURE1);
            cs237::setUniform (this->_emissiveSrcLoc, SamplerComponent);
        }
        else if (mesh->hasEmissive()) {
            cs237::setUniform (this->_emissiveSrcLoc, UniformComponent);
            cs237::setUniform (this->_emissiveCLoc, mesh->EmissiveColor());
        }
        else {
            cs237::setUniform (this->_emissiveSrcLoc, NoComponent);
        }

      // draw the mesh with normals and texture coordinates enabled, but not
      // normal-mapping tangents
        mesh->Draw (true, true, false);
    }

}
