/*! \file mesh.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 5
 *
 * \author John Reppy
 */

/* CMSC23700 Project 5 sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MESH_HPP_
#define _MESH_HPP_

#include "cs237/cs237.hpp"
#include "obj.hpp"
#include "app.hpp"
#include "shader-uniforms.hpp"
#include "vertex.hpp"

/// source of information about a material component
/// (also see the constants in shader-uniforms.hpp)
enum class MtlPropertySrc {
    eNone = 0,  ///< no color component of that type
    eConstant,  ///< constant component
    eTexture    ///< texture component
};

/// A structure that collects together the data for representing
/// a material property by a texture map
//
struct TextureProperty {
    cs237::Texture2D *txt;      /// The texture
    vk::Sampler sampler;        /// The sampler for sampling the map

    /// default constructor
    TextureProperty () : txt(nullptr), sampler(VK_NULL_HANDLE) { }

    /// is this property defined?
    bool isDefined () const { return (this->txt != nullptr); }

    void define (Proj5 *app, cs237::Image2D *img);

    void define (Proj5 *app, std::string const &name)
    {
        this->define(app, app->scene()->textureByName(name));
    }

    vk::DescriptorImageInfo imageInfo ()
    {
        return vk::DescriptorImageInfo(
            this->sampler,
            this->txt->view(),
            vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    void destroy (vk::Device dev)
    {
        if (this->txt != nullptr) {
            dev.destroySampler(this->sampler);
            delete this->txt;
        }
    }
};

/// the information needed to render a mesh
struct Mesh {
    vk::Device device;                  ///< the Vulkan device
    cs237::VertexBuffer<Vertex> *vBuf;  ///< vertex-array for this mesh
    cs237::IndexBuffer<uint32_t> *iBuf; ///< the index array
    vk::PrimitiveTopology prim;         ///< the primitive type for rendering the mesh
    cs237::AABBf_t aabb;                ///< model-space axis-aligned bounding box
                                        ///  for the mesh

    /* albedo */
    MtlPropertySrc albedoSrc;          ///< source of albedo
    glm::vec3 albedoColor;              ///< constant albedo (when albedoSrc == eConstant)
    TextureProperty albedoTexture;     ///< albedo texture (when albedoSrc == eTexture)
    /* emissive color */
    MtlPropertySrc emissiveSrc;        ///< source of emissive color
    glm::vec3 emissiveColor;            ///< constant emissive
                                        ///  (when emissiveSrc == eConstant)
    TextureProperty emissiveTexture;   ///< emissive texture (when emissiveSrc == eTexture)
    /* specular color */
    MtlPropertySrc specularSrc;        ///< source of specular color
    glm::vec4 specularColor;            ///< constant specular color
                                        ///  (when specularSrc == eConstant)
    TextureProperty specularTexture;   ///< specular texture (when specularSrc == eTexture)
    /* normal map */
    TextureProperty nMap;              ///< normal-map information (when present)

    vk::DescriptorSet descSet;          ///< the descriptor set for the material
                                        ///  UBO and samplers
    MaterialUBO *ubo;                   ///< material-properties UBO

    /// create a Mesh object by allocating and loading buffers for it.
    /// \param app    the owning app
    /// \param model  the `Model` that contains the mesh data
    /// \param grpId  the index of the group in the model
    Mesh (Proj5 *app, OBJ::Model const *model, int grpId);

    /// create a Mesh object by triangulating a height field
    /// \param app    the owning app
    /// \param hf     the height-field
    Mesh (Proj5 *app, HeightField const *hf);

    /// Mesh destuctor
    ~Mesh ();

    /// return the number of indices in the mesh
    uint32_t nIndices() const { return this->iBuf->nIndices(); }

    /// return the number of samplers required for this mesh
    int nSamplers () const
    {
        int n = 0;
        if (this->albedoSrc == MtlPropertySrc::eTexture) { ++n; }
        if (this->emissiveSrc == MtlPropertySrc::eTexture) { ++n; }
        if (this->specularSrc == MtlPropertySrc::eTexture) { ++n; }
        if (this->nMap.txt != nullptr) { ++n; }
        return n;
    }

    /// initialize the UBO for this mesh
    void initUBO (Proj5 *app);

    /// record commands in the command buffer to draw the mesh using
    /// `vkCmdDrawIndexed`.
    void draw (vk::CommandBuffer cmdBuf);

    /// binding indices for mesh uniforms
    static constexpr uint32_t kUBOBind = 0;
    static constexpr uint32_t kAlbedoBind = 1;
    static constexpr uint32_t kEmissiveBind = 2;
    static constexpr uint32_t kSpecularBind = 3;
    static constexpr uint32_t kNormalBind = 4;

};

/***** class MeshFactory *****/

/// a factory class for allocating and initializing mesh objects
class MeshFactory {
public:

    /// constructor for the factory
    /// \param app      the owing application
    /// \param nMeshes  the number of meshes in the scene
    MeshFactory (Proj5 *app, int nMeshes);

    /// destructor
    ~MeshFactory ();

    /// create a Mesh object by allocating and loading buffers for it.
    /// \param model  the `Model` that contains the mesh data
    /// \param grpId  the index of the group in the model
    Mesh *alloc (OBJ::Model const *model, int grpId)
    {
        auto mesh = new Mesh (this->_app, model, grpId);
        this->_allocDS (mesh);
        return mesh;
    }

    /// create a Mesh object by triangulating a height field
    /// \param hf     the height-field
    Mesh *alloc (HeightField const *hf)
    {
        auto mesh = new Mesh (this->_app, hf);
        this->_allocDS (mesh);
        return mesh;
    }

    /// the descriptor layout for mesh material descriptor sets
    vk::DescriptorSetLayout materialLayout () const { return this->_layout; }

private:
    Proj5 *_app;                        ///< the owning application
    vk::DescriptorPool _dsPool;         ///< the pool for allocating the per-mesh
                                        ///  descriptor sets

    /// the descriptor-set layout for the per-mesh sampler descriptor sets.
    vk::DescriptorSetLayout _layout;

    /// allocate the descriptor set for the mesh
    void _allocDS (Mesh *mesh);

};

#endif // !_MESH_HPP_
