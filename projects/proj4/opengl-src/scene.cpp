/*! \file scene.cxx
 *
 * Code to load a scene description.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "json.hxx"
#include "scene.hxx"
#include <map>
#include <functional>
#include <iostream>
#include <unistd.h>

/* helper functions to make extracting values from the JSON easier */

//! load a vec3f from a JSON object.
//! \return false if okay, true if there is an error.
bool LoadVec3 (JSON::Object const *jv, cs237::vec3f &vec)
{
    if (jv == nullptr) {
        return true;
    }

    const JSON::Number *x = jv->fieldAsNumber("x");
    const JSON::Number *y = jv->fieldAsNumber("y");
    const JSON::Number *z = jv->fieldAsNumber("z");
    if ((x == nullptr) || (y == nullptr) || (z == nullptr)) {
        return true;
    }

    vec = cs237::vec3f (
        static_cast<float>(x->realVal()),
        static_cast<float>(y->realVal()),
        static_cast<float>(z->realVal()));

    return false;
}

//! load a color3f from a JSON object.
//! \return false if okay, true if there is an error.
bool LoadColor (JSON::Object const *jv, cs237::color3f &color)
{
    if (jv == nullptr) {
        return true;
    }

    const JSON::Number *r = jv->fieldAsNumber("r");
    const JSON::Number *g = jv->fieldAsNumber("g");
    const JSON::Number *b = jv->fieldAsNumber("b");
    if ((r == nullptr) || (g == nullptr) || (b == nullptr)) {
        return true;
    }

    color = cs237::color3f (
        static_cast<float>(r->realVal()),
        static_cast<float>(g->realVal()),
        static_cast<float>(b->realVal()));

    return false;
}

//! load a window size from a JSON object.
//! \return false if okay, true if there is an error.
bool LoadSize (JSON::Object const *jv, int &wid, int &ht)
{
    if (jv == nullptr) {
        return true;
    }

    const JSON::Integer *w = jv->fieldAsInteger ("wid");
    const JSON::Integer *h = jv->fieldAsInteger ("ht");

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
bool LoadSize (JSON::Object const *jv, float &wid, float &ht)
{
    if (jv == nullptr) {
        return true;
    }

    const JSON::Number *w = jv->fieldAsNumber ("wid");
    const JSON::Number *h = jv->fieldAsNumber ("ht");

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
bool LoadFloat (JSON::Number const *jv, float &f)
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

bool Scene::Load (std::string const &path)
{
    std::string sceneDir = path;

    if (this->_loaded) {
        std::cerr << "Scene is already loaded\n";
        return true;
    }
    if (sceneDir.empty()) {
        std::cerr << "Unexpected empty path for scene\n";
        return true;
    }

  // strip trailing '/' if present
    if (sceneDir.back() == '/') {
        sceneDir.pop_back();
    }

    if (access(sceneDir.c_str(), F_OK) != 0) {
        std::cerr << "Scene \"" << path << "\" does not exist or is inaccessable\n";
        return true;
    }

  // remove directory part of sceneDir to get the scene name
    {
        size_t ix = sceneDir.find_last_of("/");
        if (ix == std::string::npos) {
            this->name = sceneDir;
        } else {
            this->name = sceneDir.substr(ix+1);
        }
    }

  // prefix for accessing contents of the scene
    sceneDir = sceneDir + "/";

  // load the scene description file
    JSON::Value *root = JSON::ParseFile(sceneDir + "scene.json");

  // check for errors
    if (root == nullptr) {
        std::cerr << "Unable to load scene \"" << path << "\"" << std::endl;
        return true;
    } else if (! root->isObject()) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; root is not an object" << std::endl;
        return true;
    }
    const JSON::Object *rootObj = root->asObject();

  // load the camera info
    const JSON::Object *cam = rootObj->fieldAsObject ("camera");
    if ((cam == nullptr)
    ||  LoadSize (cam->fieldAsObject ("size"), this->_wid, this->_ht)
    ||  LoadFloat (cam->fieldAsNumber ("fov"), this->_fov)
    ||  LoadVec3 (cam->fieldAsObject ("pos"), this->_camPos)
    ||  LoadVec3 (cam->fieldAsObject ("look-at"), this->_camAt)
    ||  LoadVec3 (cam->fieldAsObject ("up"), this->_camUp)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad camera" << std::endl;
        return true;
    }

  // load the lighting information
    const JSON::Object *lighting = rootObj->fieldAsObject ("lighting");
    if ((lighting == nullptr)
    ||  LoadVec3 (lighting->fieldAsObject ("direction"), this->_lightDir)
    ||  LoadColor (lighting->fieldAsObject ("intensity"), this->_lightI)
    ||  LoadColor (lighting->fieldAsObject ("ambient"), this->_ambI)
    ||  LoadFloat (lighting->fieldAsNumber ("shadow"), this->_shadowFactor)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad lighting" << std::endl;
        return true;
    }
  //! make sure that the light direction is a unit vector
    this->_lightDir.normalize();
  //! make sure that color values are in 0..1 range
    this->_lightI.clamp();
    this->_ambI.clamp();

  // get the spot-light array from the JSON tree
    JSON::Array const *lights = lighting->fieldAsArray("lights");
    if (lights == nullptr) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad lights array" << std::endl;
        return true;
    }

    if (lights->length() != 0) {
        this->_lights.resize(lights->length());
        for (int i = 0;  i < lights->length();  i++) {
            JSON::Object const *light = (*lights)[i]->asObject();
            if (light == nullptr) {
                std::cerr << "Expected array of JSON objects for field 'lights' in \""
                    << path << "\"" << std::endl;
                return true;
            }
            JSON::String const *name = light->fieldAsString("name");
            JSON::Object const *atten = light->fieldAsObject("attenuation");
            if ((name == nullptr)
            ||  LoadVec3 (light->fieldAsObject("pos"), this->_lights[i].pos)
            ||  LoadVec3 (light->fieldAsObject("direction"), this->_lights[i].dir)
            ||  LoadFloat (light->fieldAsNumber("cutoff"), this->_lights[i].cutoff)
            ||  LoadFloat (light->fieldAsNumber("exponent"), this->_lights[i].exponent)
            ||  LoadColor (light->fieldAsObject("intensity"), this->_lights[i].intensity)
            ||  LoadFloat (atten->fieldAsNumber("constant"), this->_lights[i].atten[0])
            ||  LoadFloat (atten->fieldAsNumber("linear"), this->_lights[i].atten[1])
            ||  LoadFloat (atten->fieldAsNumber("quadratic"), this->_lights[i].atten[2])) {
                std::cerr << "Invalid lights description in \"" << path << "\"" << std::endl;
                return true;
            }
            this->_lights[i].name = name->value();
            this->_lights[i].dir.normalize();
            this->_lights[i].intensity.clamp();
          // check attenuation values to ensure that there is some attenuation
            if ((this->_lights[i].atten[0] < 0.0f)
            ||  (this->_lights[i].atten[1] < 0.0f)
            ||  (this->_lights[i].atten[2] < 0.0f)
            || ((this->_lights[i].atten[2] < 0.001f) && (this->_lights[i].atten[1] <= 1.0f))) {
                std::cerr << "Invalid attenuation factors for light "
                    << this->_lights[i].name << std::endl;
                return true;
            }
        }
    }

  // get the object array from the JSON tree and check that it is non-empty
    JSON::Array const *objs = rootObj->fieldAsArray("objects");
    if (objs == nullptr) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad objects array" << std::endl;
        return true;
    }

    if (objs->length() != 0) {
      // allocate space for the objects in the scene
        this->_objs.resize(objs->length());

      // we use a map to keep track of which models have already been loaded
        std::map<std::string, int> objMap;
        std::map<std::string, int>::iterator it;

      // load the objects in the scene
        int numModels = 0;
        for (int i = 0;  i < objs->length();  i++) {
            JSON::Object const *object = (*objs)[i]->asObject();
            if (object == nullptr) {
                std::cerr << "Expected array of JSON objects for field 'objects' in \""
                    << path << "\"" << std::endl;
                return true;
            }
            JSON::String const *file = object->fieldAsString("file");
            JSON::Object const *frame = object->fieldAsObject("frame");
            cs237::vec3f pos, xAxis, yAxis, zAxis;
            if ((file == nullptr) || (frame == nullptr)
            ||  LoadVec3 (object->fieldAsObject("pos"), pos)
            ||  LoadVec3 (frame->fieldAsObject("x-axis"), xAxis)
            ||  LoadVec3 (frame->fieldAsObject("y-axis"), yAxis)
            ||  LoadVec3 (frame->fieldAsObject("z-axis"), zAxis)
            ||  LoadColor (object->fieldAsObject("color"), this->_objs[i].color)) {
                std::cerr << "Invalid objects description in \"" << path << "\"" << std::endl;
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
            this->_objs[i].toWorld = cs237::mat4f (
                cs237::vec4f (xAxis, 0.0f),
                cs237::vec4f (yAxis, 0.0f),
                cs237::vec4f (zAxis, 0.0f),
                cs237::vec4f (pos, 1.0f));
        }

      // load the texture images used by the materials in the models
        for (auto modIt = this->_models.begin();  modIt != this->_models.end();  modIt++) {
            const OBJ::Model *model = *modIt;
            for (auto grpIt = model->beginGroups();  grpIt != model->endGroups();  grpIt++) {
                const OBJ::Material *mat = &model->Material((*grpIt).material);
                /* we ignore the ambient map */
                this->_LoadTexture (sceneDir, mat->emissiveMap, true);
                this->_LoadTexture (sceneDir, mat->diffuseMap, true);
                this->_LoadTexture (sceneDir, mat->specularMap, true);
                this->_LoadTexture (sceneDir, mat->normalMap, false);
            }
        }
    }

  // load the ground information (if present)
    const JSON::Object *ground = rootObj->fieldAsObject ("ground");
    if (ground != nullptr) {
        float wid, ht;
        float vScale;
        cs237::color3f color;
        JSON::String const *hf = ground->fieldAsString("height-field");
        JSON::String const *cmap = ground->fieldAsString("color-map");
        JSON::String const *nmap = ground->fieldAsString("normal-map");
        if ((hf == nullptr)
        ||  LoadSize (ground->fieldAsObject("size"), wid, ht)
        ||  LoadFloat (ground->fieldAsNumber ("v-scale"), vScale)
        ||  LoadColor (ground->fieldAsObject("color"), color)) {
            std::cerr << "Invalid ground description in \"" << path << "\"" << std::endl;
            return true;
        }
        cs237::texture2D *cmapTxt = nullptr;
        if (cmap != nullptr) {
          // load the color-map texture
            this->_LoadTexture (sceneDir, cmap->value());
            cmapTxt = this->LoadTexture2D (cmap->value());
        }
        cs237::texture2D *nmapTxt = nullptr;
        if (nmap != nullptr) {
          // load the normal-map texture
            this->_LoadTexture (sceneDir, nmap->value());
            nmapTxt = this->LoadTexture2D (nmap->value());
        }
        this->_hf = new HeightField (sceneDir + hf->value(), wid, ht, vScale, color, cmapTxt, nmapTxt);
    }
    else {
        this->_hf = nullptr;
    }

    if ((ground == nullptr) && (objs->length() == 0)) {
        std::cerr << "Invalid empty scene description in \"" << path << "\"" << std::endl;
        return true;
    }

  // free up the space used by the JSON object
    delete root;

    this->_loaded = true;

    return false;
}

void Scene::_LoadTexture (std::string path, std::string name, bool genMipmaps)
{
    if (name.empty()) {
        return;
    }
  // have we already loaded this texture?
    if (this->_texs.find(name) != this->_texs.end()) {
        return;
    }
  // load the image data;
    cs237::image2d *img = new cs237::image2d(path + name);
    if (img == nullptr) {
        std::cerr << "Unable to find texture-image file \"" << path+name << "\"" << std::endl;
        exit (1);
    }
    cs237::texture2D *txt = new cs237::texture2D(GL_TEXTURE_2D, img);
    txt->Parameter (GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (genMipmaps) {
        CS237_CHECK( glGenerateMipmap(GL_TEXTURE_2D) );
        txt->Parameter (GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
        txt->Parameter (GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
  // add to _texs map
    this->_texs.insert (std::pair<std::string, cs237::texture2D *>(name, txt));
  // free image
    delete img;
}

cs237::texture2D *Scene::LoadTexture2D (std::string const &name) const
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
      _camPos(), _camAt(), _camUp(),
      _models(), _objs()
{ }

Scene::~Scene ()
{
    for (auto it = this->_models.begin();  it != this->_models.end();  it++) {
        delete *it;
    }
    for (auto it = this->_texs.begin();  it != this->_texs.end();  it++) {
        delete it->second;
    }
}
