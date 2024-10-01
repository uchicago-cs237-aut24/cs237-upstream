/*! \file shader.cpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237/cs237.hpp"
#include <fstream>

namespace cs237 {

// read the contents of a pre-compiled shader file
static std::vector<char> _readFile (std::string const &name)
{
    std::ifstream file(name, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        ERROR("unable to open shader file \"" + name + "\"!");
    }

    // determine the file size; note that the current position is at the *end* of the file.
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    // move back to the beginning and read the contents
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;

}

struct Stage {
    Stage (vk::Device device, std::string const &file, vk::ShaderStageFlagBits k);

    vk::PipelineShaderStageCreateInfo StageInfo ();

    vk::ShaderStageFlagBits kind;
    vk::ShaderModule module;
};

struct ShaderInfo {
    std::string suffix;
    vk::ShaderStageFlagBits bit;
};

static std::array<ShaderInfo,5> _shaderInfo = {
    ShaderInfo{ ".vert.spv", vk::ShaderStageFlagBits::eVertex },
/* geometry shaders are not supported by the MoltenVK driver
    { ".geom.spv", vk::ShaderStageFlagBits::eGeometry },
*/
    ShaderInfo{ ".tesc.spv", vk::ShaderStageFlagBits::eTessellationControl },
    ShaderInfo{ ".tese.spv", vk::ShaderStageFlagBits::eTessellationEvaluation },
    ShaderInfo{ ".frag.spv", vk::ShaderStageFlagBits::eFragment },
    ShaderInfo{ ".comp.spv", vk::ShaderStageFlagBits::eCompute },
};

Stage::Stage (vk::Device dev, std::string const &name, vk::ShaderStageFlagBits k)
    : kind(k)
{
    // get the code for the shader
    auto code = _readFile (name);

    // create the shader module
    vk::ShaderModuleCreateInfo moduleInfo(
        {},
        code.size(),
        reinterpret_cast<const uint32_t*>(code.data()));

    this->module = dev.createShaderModule(moduleInfo);

}

vk::PipelineShaderStageCreateInfo Stage::StageInfo ()
{
    // info to specify the pipeline stage
    vk::PipelineShaderStageCreateInfo stageInfo(
        {},
        this->kind,
        this->module,
        "main",
        nullptr);

    return stageInfo;
}

Shaders::Shaders (vk::Device device, std::string const &stem, vk::ShaderStageFlags stages)
  : _device(device)
{
    std::vector<Stage> stageVec;

    for (int i = 0;  i < _shaderInfo.size();  ++i) {
        if (stages & _shaderInfo[i].bit) {
            std::string name = stem + _shaderInfo[i].suffix;
            stageVec.push_back(Stage(device, name, _shaderInfo[i].bit));
        }
    }

    this->_stages.reserve(stageVec.size());
    for (auto stage : stageVec) {
        this->_stages.push_back(stage.StageInfo());
    }

}

Shaders::Shaders (
    vk::Device device,
    std::vector<std::string> const &files,
    vk::ShaderStageFlags stages)
  : _device(device)
{
    std::vector<Stage> stageVec;
    for (int i = 0, j = 0;  i < _shaderInfo.size();  ++i) {
        if (stages & _shaderInfo[i].bit) {
            if (j < files.size()) {
                stageVec.push_back(Stage(device, files[j++], _shaderInfo[i].bit));
            } else {
                ERROR("more shader source files than shader stages");
            }
        }
    }

    if (files.size() != stageVec.size()) {
        ERROR("mismatch in number of files/stages");
    }

    this->_stages.reserve(stageVec.size());
    for (auto stage : stageVec) {
        this->_stages.push_back(stage.StageInfo());
    }

}

Shaders::~Shaders ()
{
    for (auto stage : this->_stages) {
        this->_device.destroyShaderModule(stage.module);
    }
}

} // namespace cs237
