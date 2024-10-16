/*! \file scene.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * \author John Reppy
 *
 * This file defines the Scene class, which represents the contents of a scene
 * loaded from disk.
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _SCENE_HPP_
#define _SCENE_HPP_

#include <vector>
#include <string>
#include <map>
#include "cs237/cs237.hpp"
#include "obj.hpp"

//! an instance of a model, which has its own position and color.
struct SceneObj {
    int model;          //!< the ID of the model that defines the object's mesh
    glm::mat4 toWorld;  //!< affine transform from object space to world space
    glm::vec3 color;    //!< the color of the object

    /// return the matrix for converting normal vectors from the object's
    /// coordinate system to the world coordinate system
    glm::mat3 normToWorld () const
    {
        return glm::inverseTranspose(glm::mat3(this->toWorld));
    }

};

//! a scene consisting of an initial camera configuration and some objects
class Scene {
  public:

  // constructor/destructor
    Scene ();
    ~Scene ();

  //! load a scene from the specified path
  //! \param path  the path to the scene directory
  //! \return true if there were any errors loading the scene and false otherwise
    bool load (std::string const &path);

  //! the width of the viewport as specified by the scene
    int width () const { return this->_wid; }

  //! the height of the viewport as specified by the scene
    int height () const { return this->_ht; }

  //! the horizontal field of vew
    float horizontalFOV () const { return this->_fov; }

  //! the initial camera position
    glm::vec3 cameraPos () const { return this->_camPos; }

  //! the initial camera look-at point
    glm::vec3 cameraLookAt () const { return this->_camAt; }

  //! the initial camera up vector
    glm::vec3 cameraUp () const { return this->_camUp; }

  //! the light's unit-length direction vector
    glm::vec3 lightDir () const { return this->_lightDir; }

  //! the directional light's intensity
    glm::vec3 lightIntensity () const { return this->_lightI; }

  //! the ambient light intensity in the scene
    glm::vec3 ambientLight () const { return this->_ambI; }

  //! return the number of objects in the scene
    int numObjects () const { return this->_objs.size(); }

  //! iterator for looping over the objects in the scene
    std::vector<SceneObj>::const_iterator beginObjs () const { return this->_objs.begin(); }

  //! terminator for looping over the objects in the scene
    std::vector<SceneObj>::const_iterator endObjs () const { return this->_objs.end(); }

  //! return the i'th object in the scene
    SceneObj const object (int idx) const { return this->_objs[idx]; }

  //! return the number of distinct models in the scene
    int numModels () const { return this->_models.size(); }

  //! iterator for looping over the models in the scene
    std::vector<OBJ::Model const *>::const_iterator beginModels () const
    {
        return this->_models.begin();
    }

  //! terminator for looping over the models in the scene
    std::vector<OBJ::Model const *>::const_iterator endModels () const
    {
        return this->_models.end();
    }

  //! return the i'th model in the scene
    const OBJ::Model *model (int idx) const { return this->_models[idx]; }

  private:
    bool _loaded;               //!< has the scene been loaded?

    int _wid;                   //!< initial window width
    int _ht;                    //!< initial window height
    float _fov;                 //!< horizontal field of view in degrees
    glm::vec3 _camPos;          //!< camera position
    glm::vec3 _camAt;           //!< the point at which the camera is pointing at
    glm::vec3 _camUp;           //!< the camera's up vector
    glm::vec3 _lightDir;        //!< unit vector that specifies the light's direction
    glm::vec3 _lightI;          //!< the directional light's intensity
    glm::vec3 _ambI;            //!< the ambient light's intensity

    std::vector<OBJ::Model const *> _models;    //!< the OBJ models in the scene
    std::vector<SceneObj> _objs;                //!< the objects in the scene

};

#endif /* !_SCENE_HPP_ */
