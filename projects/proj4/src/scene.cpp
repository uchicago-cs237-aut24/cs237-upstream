/*! \file scene.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 4
 *
 * Code to load a scene description.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "json.hpp"
#include "scene.hpp"
#include <map>
#include <functional>
#include <iostream>

/* helper functions to make extracting values from the JSON easier */

/// load a floating-point value from a JSON object
/// \return an optional float; None means the field did not exist or had the wrong
///         type
static std::optional<float> loadFloat (json::Object const *jv, std::string const &field)
{
    const json::Number *f = jv->fieldAsNumber(field);
    if (f == nullptr) {
        return std::optional<float>();
    } else {
        return std::optional<float>(static_cast<float>(f->realVal()));
    }
}

//! load a vec3f from a JSON object.
//! \return false if okay, true if there is an error.
static bool loadVec3 (json::Object const *jv, glm::vec3 &vec)
{
    auto x = loadFloat(jv, "x");
    auto y = loadFloat(jv, "y");
    auto z = loadFloat(jv, "z");

    if (x.has_value() && y.has_value() && z.has_value()) {
        vec = glm::vec3(*x, *y, *z);
        return false;
    } else {
        // error: missing or incorrectly typed field
        return true;
    }
}

//! load a color3f from a JSON object.
//! \return false if okay, true if there is an error.
static bool loadColor (json::Object const *jv, glm::vec3 &color)
{
    auto r = loadFloat(jv, "r");
    auto g = loadFloat(jv, "g");
    auto b = loadFloat(jv, "b");
    if (r.has_value() && g.has_value() && b.has_value()) {
        color = glm::vec3(*r, *g, *b);
        return false;
    } else {
        // error: missing or incorrectly typed field
        return true;
    }
}

//! load a window size from a JSON object.
//! \return false if okay, true if there is an error.
static bool loadSize (json::Object const *jv, int &wid, int &ht)
{
    const json::Integer *w = jv->fieldAsInteger ("wid");
    const json::Integer *h = jv->fieldAsInteger ("ht");

    if ((w == nullptr) || (h == nullptr)) {
        return true;
    }
    else {
        wid = static_cast<int>(w->intVal());
        ht = static_cast<int>(h->intVal());
        return false;
    }

}

//! load a ground size from a JSON object.
//! \return false if okay, true if there is an error.
static bool loadGroundSize (json::Object const *jv, float &wid, float &ht)
{
    if (jv == nullptr) {
        return true;
    }

    const json::Number *w = jv->fieldAsNumber ("wid");
    const json::Number *h = jv->fieldAsNumber ("ht");

    if ((w == nullptr) || (h == nullptr)) {
        return true;
    }
    else {
        wid = static_cast<float>(w->realVal());
        ht = static_cast<float>(h->realVal());
        return false;
    }

}

//! load a float from a JSON object.
//! \return false if okay, true if there is an error.
static bool loadFloat (json::Number const *jv, float &f)
{
    if (jv == nullptr) {
        return true;
    }
    else {
        f = static_cast<float>(jv->realVal());
        return false;
    }
}

/***** class Scene member functions *****/

bool Scene::load (std::string const &path)
{
    if (this->_loaded) {
        std::cerr << "Scene is already loaded" << std::endl;
        return true;
    }
    this->_loaded = true;

    std::string sceneDir = path + "/";

    // load the scene description file
    json::Value *root = json::parseFile(sceneDir + "scene.json");

    // check for errors
    if (root == nullptr) {
        std::cerr << "Unable to load scene \"" << path << "\"\n";
        return true;
    } else if (! root->isObject()) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; root is not an object" << std::endl;
        return true;
    }
    const json::Object *rootObj = root->asObject();

    // load the camera info
    const json::Object *cam = rootObj->fieldAsObject ("camera");
    if ((cam == nullptr)
    ||  loadSize (cam->fieldAsObject ("size"), this->_wid, this->_ht)
    ||  loadFloat (cam->fieldAsNumber ("fov"), this->_fov)
    ||  loadVec3 (cam->fieldAsObject ("pos"), this->_camPos)
    ||  loadVec3 (cam->fieldAsObject ("look-at"), this->_camAt)
    ||  loadVec3 (cam->fieldAsObject ("up"), this->_camUp)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad camera" << std::endl;
        return true;
    }

    // load the lighting information
    const json::Object *lighting = rootObj->fieldAsObject ("lighting");
    if ((lighting == nullptr)
    ||  loadVec3 (lighting->fieldAsObject ("direction"), this->_lightDir)
    ||  loadColor (lighting->fieldAsObject ("intensity"), this->_lightI)
    ||  loadColor (lighting->fieldAsObject ("ambient"), this->_ambI)
    ||  loadFloat (lighting->fieldAsNumber ("shadow"), this->_shadowFactor)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad lighting\n";
        return true;
    }
    // make sure that the light direction is a unit vector
    this->_lightDir = glm::normalize(this->_lightDir);
    // make sure that color values are in 0..1 range
    this->_lightI = glm::clamp(this->_lightI, 0.0f, 1.0f);
    this->_ambI = glm::clamp(this->_ambI, 0.0f, 1.0f);
    // get the array of spot lights; we allow at most 4 lights
    json::Array const *lights = lighting->fieldAsArray("lights");
    if ((lights == nullptr) || (lights->length() == 0)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad lights array\n";
        return true;
    }
    // allocate space for the lights in the scene
    this->_spotLights.resize(lights->length());
    for (int i = 0;  i < lights->length();  i++) {
        json::Object const *light = (*lights)[i]->asObject();
        if (loadVec3 (light->fieldAsObject("pos"), this->_spotLights[i].pos)
        ||  loadVec3 (light->fieldAsObject("direction"), this->_spotLights[i].dir)
        ||  loadFloat (light->fieldAsNumber("cutoff"), this->_spotLights[i].cutoff)
        ||  loadFloat (light->fieldAsNumber("exponent"), this->_spotLights[i].exponent)
        ||  loadColor (light->fieldAsObject("intensity"), this->_spotLights[i].intensity)) {
            std::cerr << "Invalid scene description in \"" << path
                << "\"; bad lighting\n";
            return true;
        }
        // get attenuation coefficients
        json::Array const *aten = light->fieldAsArray("attenuation");
        if ((aten == nullptr)
        ||  (aten->length() != 3)
        ||  loadFloat((*aten)[0]->asNumber(), this->_spotLights[i].k0)
        ||  loadFloat((*aten)[1]->asNumber(), this->_spotLights[i].k1)
        ||  loadFloat((*aten)[2]->asNumber(), this->_spotLights[i].k2)) {
            std::cerr << "Invalid scene description in \"" << path
                << "\"; bad attenuation array\n";
            return true;
        }
        // normalize the light's direction vector
        this->_spotLights[i].dir = glm::normalize(this->_spotLights[i].dir);
        // make sure that the light intensity is in 0..1 range
        this->_spotLights[i].intensity = glm::clamp(this->_spotLights[i].intensity, 0.0f, 1.0f);
    }

    // get the object array from the JSON tree and check that it is non-empty
    json::Array const *objs = rootObj->fieldAsArray("objects");
    if ((objs == nullptr) || (objs->length() == 0)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; missing objects array\n";
        return true;
    }

    // allocate space for the objects in the scene
    this->_objs.resize(objs->length());

    // we use a map to keep track of which models have already been loaded
    std::map<std::string, int> objMap;
    std::map<std::string, int>::iterator it;

    // load the objects in the scene
    int numModels = 0;
    for (int i = 0;  i < objs->length();  i++) {
        json::Object const *object = (*objs)[i]->asObject();
        if (object == nullptr) {
            std::cerr << "Expected array of JSON objects for field 'objects' in \""
                << path << "\"\n";
            return true;
        }
        json::String const *file = object->fieldAsString("file");
        json::Object const *frame = object->fieldAsObject("frame");
        glm::vec3 pos, xAxis, yAxis, zAxis;
        if ((file == nullptr) || (frame == nullptr)
        ||  loadVec3 (object->fieldAsObject("pos"), pos)
        ||  loadVec3 (frame->fieldAsObject("x-axis"), xAxis)
        ||  loadVec3 (frame->fieldAsObject("y-axis"), yAxis)
        ||  loadVec3 (frame->fieldAsObject("z-axis"), zAxis)
        ||  loadColor (object->fieldAsObject("color"), this->_objs[i].color)) {
            std::cerr << "Invalid objects description in \"" << path << "\"\n";
            return true;
        }
        // have we already loaded this model?
        it = objMap.find(file->value());
        int modelId;
        if (it != objMap.end()) {
            modelId = it->second;
        }
        else {
            // load the model from the file sytem and add it to the map
            modelId = numModels++;
            OBJ::Model *model = new OBJ::Model (sceneDir + file->value());
            this->_models.push_back(model);
            objMap.insert (std::pair<std::string, int> (file->value(), modelId));
        }
        this->_objs[i].model = modelId;
        // set the object-space to world-space transform
        this->_objs[i].toWorld = glm::mat4 (
            glm::vec4 (xAxis, 0.0f),
            glm::vec4 (yAxis, 0.0f),
            glm::vec4 (zAxis, 0.0f),
            glm::vec4 (pos, 1.0f));
    }

    // load the texture images used by the materials in the models
    for (auto modIt = this->_models.begin();  modIt != this->_models.end();  modIt++) {
        const OBJ::Model *model = *modIt;
        for (auto grpIt = model->beginGroups();  grpIt != model->endGroups();  grpIt++) {
            const OBJ::Material *mat = &model->material((*grpIt).material);
//            this->_loadTexture (sceneDir, mat->ambientMap);
            this->_loadTexture (sceneDir, mat->emissiveMap);
            this->_loadTexture (sceneDir, mat->diffuseMap);
            this->_loadTexture (sceneDir, mat->specularMap);
            this->_loadTexture (sceneDir, mat->normalMap, true);
        }
    }

    // load the ground information (if present)
    const json::Object *ground = rootObj->fieldAsObject ("ground");
    if (ground != nullptr) {
        float wid, ht;
        float vScale;
        glm::vec3 color;
        json::Object const *plane = ground->fieldAsObject("plane");
        json::String const *hf = ground->fieldAsString("height-field");
        json::String const *cmap = ground->fieldAsString("color-map");
        json::String const *nmap = ground->fieldAsString("normal-map");
        if ((plane == nullptr) || (hf == nullptr) || (cmap == nullptr)
        ||  loadGroundSize (ground->fieldAsObject("size"), wid, ht)
        ||  loadFloat (ground->fieldAsNumber ("v-scale"), vScale)
        ||  loadColor (ground->fieldAsObject("color"), color)) {
            std::cerr << "Invalid ground description in \"" << path << "\"\n";
            return true;
        }
        // extract the normal-distance representation of the ground plane
        auto nx = loadFloat(plane, "nx");
        auto ny = loadFloat(plane, "ny");
        auto nz = loadFloat(plane, "nz");
        auto d = loadFloat(plane, "d");
        if (!nx.has_value() || !ny.has_value() || !nz.has_value() || !d.has_value()) {
            std::cerr << "Invalid ground plane in \"" << path << "\"\n";
            return true;
        }
        this->_groundPlane = cs237::Planef_t(glm::vec3(*nx, *ny, *nz), *d);
        // load the color-map texture
        this->_loadTexture (sceneDir, cmap->value());
        cs237::Image2D *cmapImg = this->textureByName (cmap->value());
        // load the optional normal-map texture
        cs237::Image2D *nmapImg;
        if (nmap != nullptr) {
            this->_loadTexture (sceneDir, nmap->value(), true);
            nmapImg = this->textureByName (nmap->value());
        } else {
            nmapImg = nullptr;
        }
        this->_hf = new HeightField (
            sceneDir + hf->value(), wid, ht, vScale, color,
            cmapImg, nmapImg);
    }

    if ((ground == nullptr) && (objs->length() == 0)) {
        std::cerr << "Invalid empty scene description in \"" << path << "\"\n";
        return true;
    }

    // free up the space used by the JSON object
    delete root;

    return false;
}

void Scene::_loadTexture (std::string path, std::string name, bool nMap)
{
    if (name.empty()) {
        return;
    }
    // have we already loaded this texture?
    if (this->_texs.find(name) != this->_texs.end()) {
        return;
    }
    // load the image data;
    cs237::Image2D *img;
    if (nMap) {
        // normal data should not be sRGB encoded!
        img = new cs237::DataImage2D(path + name);
    } else {
        img = new cs237::Image2D(path + name);
    }
    // add to _texs map
    this->_texs.insert (std::pair<std::string, cs237::Image2D *>(name, img));

}

cs237::Image2D *Scene::textureByName (std::string name) const
{
    if (! name.empty()) {
        auto it = this->_texs.find(name);
        if (it != this->_texs.end()) {
            return it->second;
        }
    }
    return nullptr;

}

Scene::Scene ()
    : _loaded(false), _wid(0), _ht(0), _fov(0),
      _camPos(), _camAt(), _camUp(), _hf(nullptr),
      _models(), _objs(), _spotLights(), _texs()
{ }

Scene::~Scene ()
{
    if (this->_hf != nullptr) { delete this->_hf; }
    for (auto it : this->_models) {
        delete it;
    }
}