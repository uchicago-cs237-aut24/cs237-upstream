/*! \file app.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 4
 *
 * The main application class for Project 4.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "app.hpp"
#include "window.hpp"
#include <filesystem>
#include <cstdlib>
#include <unistd.h>

namespace std_fs = std::filesystem;

#ifdef PROJ4_SOURCE_ROOT
//!< the absolute path to the directory containing the scenes
const std::string kDataDir = PROJ4_SOURCE_ROOT "/scenes/";
#else
# error PROJ4_SOURCE_ROOT not defined
#endif

static void usage (int sts)
{
    std::cerr << "usage: proj4 [options] <scene>\n";
    exit (sts);
}

Proj4::Proj4 (std::vector<std::string> const &args)
  : cs237::Application (args, "CS237 Project 4")
{
    // the last argument is the name of the scene directory that we should render
    if (args.size() < 2) {
        usage(EXIT_FAILURE);
    }
    std_fs::path scenePath = args.back();
    if (! scenePath.is_absolute()) {
        // assume relative to the data directory
        scenePath = kDataDir + std::string(scenePath);
    }

    // verify that the scene path exists
    auto sts = std_fs::status(scenePath);
    if (sts.type() != std_fs::file_type::directory) {
        std::cerr << "proj1: scene '" << std::string(scenePath)
            << "' is not a directory or does not exist\n";
        exit(EXIT_FAILURE);
    }

    // load the scene
    if (this->_scene.load(scenePath)) {
        std::cerr << "proj1: cannot load scene from '" << scenePath << "'\n";
        exit(EXIT_FAILURE);
    }

    /** HINT: create the descriptor pool and layout for the per-mesh descriptor sets */
}

Proj4::~Proj4 ()
{
    this->_device.destroyDescriptorSetLayout(this->_meshDSLayout);
    this->_device.destroyDescriptorPool(this->_meshDSPool);
}

void Proj4::run ()
{
    // create the application window
    Proj4Window *win = new Proj4Window (this);

    // complete the lab-specific window initialization
    win->initialize ();

    // wait until the window is closed
    while(! win->windowShouldClose()) {
        glfwPollEvents();
        win->draw ();
    }

    // wait until any in-flight rendering is complete
    vkDeviceWaitIdle(this->_device);

    // cleanup
    delete win;
}

vk::DescriptorSet Proj4::allocMeshDS ()
{
    // create the descriptor set
    vk::DescriptorSetAllocateInfo allocInfo(this->_meshDSPool, this->_meshDSLayout);

    return(this->_device.allocateDescriptorSets(allocInfo))[0];

}

void Proj4::controlsHelpMessage ()
{
    // print information about the keyboard commands
/** HINT: add any additional commands that you support to this message */
    std::cout
        << "# Project 4 User Interface\n"
        << "#   Rendering mode controls\n"
        << "#     'd' for deferred rendering of scene\n"
        << "#     't' for forward rendering of scene with textures and basic lighting\n"
        << "#     'w' for wire-frame rendering of scene\n"
        << "#   Lighting controls in deferred mode\n"
        << "#     'l' to toggle the direct light\n"
        << "#     'p' to toggle the spot lights\n"
        << "#     'e' to toggle emissive lighting\n"
        << "#     's' to toggle shadows (extra credit)\n"
        << "#   Other controls\n"
        << "#     'h' to display this message\n"
        << "#     left and right arrow keys to rotate view\n"
        << "#     'q' to quit\n";

}
