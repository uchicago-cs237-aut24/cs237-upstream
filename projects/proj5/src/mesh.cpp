/*! \file mesh.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 5
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237/cs237.hpp"
#include "shader-uniforms.hpp"
#include "vertex.hpp"
#include "mesh.hpp"
#include <array>
#include <vector>

Mesh::Mesh (Proj5 *app, OBJ::Model const *model, int grpId)
: device(app->device()),
  vBuf(nullptr), iBuf(nullptr), prim(vk::PrimitiveTopology::eTriangleList), aabb(),
  albedoSrc(MtlPropertySrc::eNone), albedoTexture(),
  emissiveSrc(MtlPropertySrc::eNone), emissiveTexture(),
  specularSrc(MtlPropertySrc::eNone), specularTexture(),
  nMap()
{
    auto grp = model->group(grpId);

    if (grp.norms == nullptr) {
        ERROR("missing normals in model mesh");
    }
    if (grp.txtCoords == nullptr) {
         ERROR("missing texture coordinates in model mesh");
    }
    if ((grp.nVerts == 0) || (grp.nIndices == 0)) {
         ERROR("empty group");
    }

    this->vBuf = new cs237::VertexBuffer<Vertex>(app, grp.nVerts);
    this->iBuf = new cs237::IndexBuffer<uint32_t>(app, grp.nIndices);

    // vertex buffer initialization; first convert struct of arrays to array of structs
    // at the same time, we update the bounding box
    std::vector<Vertex> verts(grp.nVerts);
    for (int i = 0;  i < grp.nVerts;  ++i) {
        this->aabb.addPt(grp.verts[i]);
        verts[i].pos = grp.verts[i];
        verts[i].norm = grp.norms[i];
        verts[i].txtCoord = grp.txtCoords[i];
    }

    // compute tangent vectors; we do this by summing the tangent and bitangent
    // vectors for each occurrence of a vertex in a triangle and then normalizing
    // the result.
    std::vector<glm::vec3> tan(grp.nVerts);
    std::vector<glm::vec3> bitan(grp.nVerts);
    uint32_t nTris = grp.nIndices / 3;
    assert (nTris * 3 == grp.nIndices);
    for (int tri = 0;  tri < nTris;  tri++) {
        // get the indices for the triangle
        uint32_t i1 = grp.indices[3*tri + 0];
        uint32_t i2 = grp.indices[3*tri + 1];
        uint32_t i3 = grp.indices[3*tri + 2];
        // get the vertices for the triangle
        glm::vec3 v1 = grp.verts[i1];
        glm::vec3 v2 = grp.verts[i2];
        glm::vec3 v3 = grp.verts[i3];
        // get the texture coordinates for the triangle
        glm::vec2 vt1 = grp.txtCoords[i1];
        glm::vec2 vt2 = grp.txtCoords[i2];
        glm::vec2 vt3 = grp.txtCoords[i3];
        // the sides of the triangle as a 3x2 matrix
        glm::mat3x2 Q = glm::mat3x2(
            v2.x - v1.x, v3.x - v1.x,   // column one
            v2.y - v1.y, v3.y - v1.y,   // column two
            v2.z - v1.z, v3.z - v1.z);  // column three
        // the sides in tangent space as a 2x2 matrix
        glm::mat2x2 ST = glm::mat2x2 (
            vt2.x - vt1.x, vt3.x - vt1.x,       // first column
            vt2.y - vt1.y, vt3.y - vt1.y);      // second column
        // Q = ST * [T B]^T, so multiply Q by ST^{-1}
        glm::mat3x2 TB = glm::inverse(ST) * Q;
        // extract rows T and B
        glm::vec3 t = glm::vec3(TB[0][0], TB[1][0], TB[2][0]);
        glm::vec3 b = glm::vec3(TB[0][1], TB[1][1], TB[2][1]);
        // add to vector sums
        tan[i1] += t;
        tan[i2] += t;
        tan[i3] += t;
        bitan[i1] += b;
        bitan[i2] += b;
        bitan[i3] += b;
    }
    // compute extended tangents for vertices
    for (int i = 0;  i < grp.nVerts;  i++) {
        glm::vec3 n = grp.norms[i];
        glm::vec3 t = glm::normalize(tan[i]);
        /* NOTE: we only care about the direction of bitan[i], so we do not normalize */
        // orthogonalize the tangent and normal
        t = glm::normalize(t - n * dot(n, t));
        float w = (glm::dot(glm::cross(n, t), bitan[i]) < 0.0f ? -1.0f : 1.0f);
        verts[i].tan = glm::vec4(t, w);
    }

    // copy data
    this->vBuf->copyTo(verts);

    // index buffer initialization
    this->iBuf->copyTo(vk::ArrayProxy<uint32_t>(grp.nIndices, grp.indices));

    // get the material for the group
    OBJ::Material mtl = model->material(grp.material);

    // sampler-creation info
    cs237::Application::SamplerInfo samplerInfo(
        vk::Filter::eLinear,  /* magnification filter */
        vk::Filter::eLinear,  /* minification filter */
        vk::SamplerMipmapMode::eLinear,  /* mipmap mode */
        vk::SamplerAddressMode::eClampToEdge,  /* addressing mode for U coordinates */
        vk::SamplerAddressMode::eClampToEdge,  /* addressing mode for V coordinates */
        vk::BorderColor::eFloatOpaqueBlack);  /* border color */

    // initialize the albedo info
    if (mtl.diffuseC == 0) {
        this->emissiveSrc = MtlPropertySrc::eNone;
        this->albedoColor = glm::vec3(1.0, 1.0, 1.0); /* default for wire-frame */
    } else {
        if ((mtl.diffuseC & OBJ::UniformComponent) != 0) {
            this->albedoSrc = MtlPropertySrc::eConstant;
            this->albedoColor = mtl.diffuse;
        } else {
            this->albedoColor = glm::vec3(1.0, 1.0, 1.0); /* default for wire-frame */
        }
        if ((mtl.diffuseC & OBJ::MapComponent) != 0) {
            this->albedoSrc = MtlPropertySrc::eTexture;
            this->albedoTexture.define(app, mtl.diffuseMap);
        }
    }

    // initialize the emissive info (if present)
    if (mtl.emissiveC == 0) {
        this->emissiveSrc = MtlPropertySrc::eNone;
    } else {
        if ((mtl.emissiveC & OBJ::UniformComponent) != 0) {
            this->emissiveSrc = MtlPropertySrc::eConstant;
            this->emissiveColor = mtl.emissive;
        }
        if ((mtl.emissiveC & OBJ::MapComponent) != 0) {
            this->emissiveSrc = MtlPropertySrc::eTexture;
            this->emissiveTexture.define(app, mtl.emissiveMap);
        }
    }

    // initialize the specular info (if present)
    if (mtl.specularC == 0) {
        this->specularSrc = MtlPropertySrc::eNone;
    } else {
        if ((mtl.specularC & OBJ::UniformComponent) != 0) {
            this->specularSrc = MtlPropertySrc::eConstant;
            this->specularColor = glm::vec4(mtl.specular, mtl.shininess);
        }
        if ((mtl.specularC & OBJ::MapComponent) != 0) {
            this->specularSrc = MtlPropertySrc::eTexture;
            this->specularTexture.define(app, mtl.specularMap);
        }
    }

    // initialize the normal map (if present)
    if (mtl.normalMap != "") {
        this->nMap.define(app, mtl.normalMap);
    }

    // create and initialize the UBO
    this->initUBO (app);
}

Mesh::~Mesh ()
{
    this->albedoTexture.destroy(this->device);
    this->emissiveTexture.destroy(this->device);
    this->specularTexture.destroy(this->device);
    this->nMap.destroy(this->device);

    delete this->ubo;

    delete this->vBuf;
    delete this->iBuf;

}

void Mesh::initUBO (Proj5 *app)
{
    MaterialUB ub;

    ub.albedoSrc = static_cast<int>(this->albedoSrc);
    ub.albedo = this->albedoColor;
    ub.emissiveSrc = static_cast<int>(this->emissiveSrc);
    ub.emissive = this->emissiveColor;
    ub.specularSrc = static_cast<int>(this->specularSrc);
    ub.specular = this->specularColor;
    ub.hasNormalMap = (this->nMap.isDefined() ? VK_TRUE : VK_FALSE);

    this->ubo = new cs237::UniformBuffer<MaterialUB>(app, ub);

}

void Mesh::draw (vk::CommandBuffer cmdBuf)
{
    cmdBuf.bindVertexBuffers(0, this->vBuf->vkBuffer(), {0});
    cmdBuf.bindIndexBuffer(this->iBuf->vkBuffer(), 0, vk::IndexType::eUint32);

    cmdBuf.drawIndexed(static_cast<uint32_t>(this->nIndices()), 1, 0, 0, 0);

}

/***** TextureProperty methods *****/

void TextureProperty::define (Proj5 *app, cs237::Image2D *img)
{
    assert (img != nullptr && "undefined image for texture property");

    this->txt = new cs237::Texture2D(app, img);

    cs237::Application::SamplerInfo samplerInfo(
        vk::Filter::eLinear,  /* magnification filter */
        vk::Filter::eLinear,  /* minification filter */
        vk::SamplerMipmapMode::eLinear,  /* mipmap mode */
        vk::SamplerAddressMode::eClampToEdge,  /* addressing mode for U coordinates */
        vk::SamplerAddressMode::eClampToEdge,  /* addressing mode for V coordinates */
        vk::BorderColor::eFloatOpaqueBlack);  /* border color */

    this->sampler = app->createSampler (samplerInfo);

}

/***** MeshFactory methods *****/

MeshFactory::MeshFactory (Proj5 *app, int nMeshes)
: _app(app)
{
    // allocate the descriptor pool; there is one UBO and at most 4 samplers per mesh
    int nUBOs = nMeshes;
    int nSamplers = 4 * nMeshes;
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, nUBOs),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, nSamplers)
        };
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        nMeshes, /* max sets; one per mesh */
        poolSizes); /* pool sizes */

    this->_dsPool = app->device().createDescriptorPool(poolInfo);

    // create the layout for the material descriptor sets
    // 1 UBO + up to 4 samplers for a mesh
    std::array<vk::DescriptorSetLayoutBinding, 5> layoutBindings = {
        vk::DescriptorSetLayoutBinding(
            0, /* binding */
            vk::DescriptorType::eUniformBuffer, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr) /* samplers */
    };
    for (int i = 1;  i < 5;  ++i) {
        layoutBindings[i] = vk::DescriptorSetLayoutBinding(
            i, /* binding */
            vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
            1, /* descriptor count */
            vk::ShaderStageFlagBits::eFragment, /* stages */
            nullptr); /* samplers */
    }

    // we allow undefined samplers, since not every object has every kind of
    // texture.  To enable this mode, we have to pass the address of a
    // DescriptorSetLayoutBindingFlagsCreateInfo struct as the pNext field
    // of the DescriptorSetLayoutCreateInfo struct
    std::array<vk::DescriptorBindingFlags,5> bindingFlags = {
            vk::DescriptorBindingFlags{},
            vk::DescriptorBindingFlagBits::ePartiallyBound,
            vk::DescriptorBindingFlagBits::ePartiallyBound,
            vk::DescriptorBindingFlagBits::ePartiallyBound,
            vk::DescriptorBindingFlagBits::ePartiallyBound
        };
    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagInfo(bindingFlags);
    vk::DescriptorSetLayoutCreateInfo layoutInfo(
        {}, /* flags */
        layoutBindings,
        &bindingFlagInfo); /* bindings */
    this->_layout = app->device().createDescriptorSetLayout(layoutInfo);

}

MeshFactory::~MeshFactory ()
{
    this->_app->device().destroyDescriptorSetLayout(this->_layout);
    this->_app->device().destroyDescriptorPool(this->_dsPool);

}

void MeshFactory::_allocDS (Mesh *mesh)
{
    vk::DescriptorSetAllocateInfo allocInfo(this->_dsPool, this->_layout);
    mesh->descSet = (this->_app->device().allocateDescriptorSets(allocInfo))[0];

    auto uboInfo = mesh->ubo->descInfo();
    std::vector<vk::WriteDescriptorSet> descWrites = {
            vk::WriteDescriptorSet(
                mesh->descSet, /* descriptor set */
                Mesh::kUBOBind, /* binding */
                0, /* array element */
                vk::DescriptorType::eUniformBuffer, /* descriptor type */
                nullptr, /* image info */
                uboInfo, /* buffer info */
                nullptr) /* texel buffer view */
        };

    vk::DescriptorImageInfo albedoInfo;
    if (mesh->albedoSrc == MtlPropertySrc::eTexture) {
        albedoInfo = mesh->albedoTexture.imageInfo();
        descWrites.push_back(
            vk::WriteDescriptorSet(
                mesh->descSet,
                Mesh::kAlbedoBind, /* binding */
                0, /* array element */
                vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
                albedoInfo,
                nullptr, /* buffer info */
                nullptr)); /* texel buffer view */
    }

    vk::DescriptorImageInfo emissiveInfo;
    if (mesh->emissiveSrc == MtlPropertySrc::eTexture) {
        emissiveInfo = mesh->emissiveTexture.imageInfo();
        descWrites.push_back(
            vk::WriteDescriptorSet(
                mesh->descSet,
                Mesh::kEmissiveBind, /* binding */
                0, /* array element */
                vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
                emissiveInfo,
                nullptr, /* buffer info */
                nullptr)); /* texel buffer view */
    }

    vk::DescriptorImageInfo specularInfo;
    if (mesh->specularSrc == MtlPropertySrc::eTexture) {
        specularInfo = mesh->specularTexture.imageInfo();
        descWrites.push_back(
            vk::WriteDescriptorSet(
                mesh->descSet,
                Mesh::kSpecularBind, /* binding */
                0, /* array element */
                vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
                specularInfo,
                nullptr, /* buffer info */
                nullptr)); /* texel buffer view */
    }

    vk::DescriptorImageInfo normalInfo;
    if (mesh->nMap.isDefined()) {
        normalInfo = mesh->nMap.imageInfo();
        descWrites.push_back(
            vk::WriteDescriptorSet(
                mesh->descSet,
                Mesh::kNormalBind, /* binding */
                0, /* array element */
                vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
                normalInfo,
                nullptr, /* buffer info */
                nullptr)); /* texel buffer view */
    }

    this->_app->device().updateDescriptorSets (descWrites, nullptr);

}
