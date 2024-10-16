/*! \file app.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _APP_HPP_
#define _APP_HPP_

#include "cs237/cs237.hpp"
#include "scene.hpp"

//! The Project 1 Application class
class Proj1 : public cs237::Application {
public:
    Proj1 (std::vector<std::string> const &args);
    ~Proj1 ();

    //! run the application
    void run () override;

    //! access function for the scene
    const Scene *scene () const { return &this->_scene; }

protected:
    Scene _scene;               //!< the scene to be rendered

};

#endif // !_APP_HPP_

