/*! \file shader.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_SHADER_HPP_
#define _CS237_SHADER_HPP_

#ifndef _CS237_HPP_
#error "cs237-shader.hpp should not be included directly"
#endif

namespace cs237 {

/// A wrapper class for loading a pipeline of pre-compiled shaders from
/// the file system.
//
class Shaders {
  public:

    /*! \brief load a pre-compiled shader program from the file system.
     *  \param device   the logical device that will run the shaders
     *  \param stem     the base name of the shader files
     *  \param stages   a bitmask spcifying the shader stages that form the program.
     *                  The flags that we support are: eVertex, eTessellationControl,
     *                  eTessellationEvaluation, eFragment, and eCompute.
     *
     * This version of the constructor assumes that the shader programs all
     * have the same base filename and only differ in their file extensions.
     */
    Shaders (
        vk::Device device,
        std::string const &stem,
        vk::ShaderStageFlags stages);

    /* \brief load a pre-compiled shader program from the file system.
     *  \param device   the logical device that will run the shaders
     *  \param files    a vector of the shader file names; the order of the file
     *                  names should be a subset of: vertex, tessellation control,
     *                  tessellation evaluation, fragment, and compute.
     *  \param stages   a bitmask spcifying the shader stages that form the program.
     *                  The number of set bits should be equal to the number
     *                  of elements in the `files` vector.
     */
    Shaders (
        vk::Device device,
        std::vector<std::string> const &files,
        vk::ShaderStageFlags stages);

    ~Shaders ();

    /// return the number of shader stages in the pipeline
    int numStages () const { return this->_stages.size(); }

    /// return a pointer to the array of stage create infos
    std::vector<vk::PipelineShaderStageCreateInfo> const &stages () const
    {
        return this->_stages;
    }

  private:
    vk::Device _device;
    std::vector<vk::PipelineShaderStageCreateInfo> _stages;

}; // Shaders

} /* namespace cs237 */

#endif /* !_CS237_SHADER_HPP_ */
