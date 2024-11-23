/*! \file view.cxx
 *
 * \brief This file defines the viewer operations.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hxx"
#include "view.hxx"
#include "axes.hxx"

/* clip planes in world coordinates */
#define NEAR_Z          0.5f
#define FAR_Z           1000.0f
#define FOVY            65.0f   /* field-of-view in Y-dimension */

#define MIN_DIST        5.0f
#define MAX_DIST        200.0f

/* View initialization.
 */
View::View (Scene const &scene, GLFWwindow *win)
{
    this->sceneName     = scene.Name();

  // link the window and the view together
    glfwSetWindowUserPointer (win, this);
    this->win = win;

  /* view info */
    this->wid           = scene.Width();
    this->ht            = scene.Height();
    this->isVis         = GL_TRUE;
    this->shouldExit    = false;
    this->needsRedraw   = true;

  /* framebuffer size */
    CS237_CHECK (glfwGetFramebufferSize (win, &this->fbWid, &this->fbHt) );

  /* initialize the camera */
    this->camPos        = scene.CameraPos();
    this->camAt         = scene.CameraLookAt();
    this->camUp         = scene.CameraUp();
    this->fov           = scene.HorizontalFOV();

  /* initialzize the current camera transform state */
    this->camRot        = cs237::mat4f(1.0f);
    this->camOffset     = 0;
    float dist          = distance(this->camPos, this->camAt);
    this->minOffset     = cs237::min(0.0f, MIN_DIST - dist);
    this->maxOffset     = cs237::max(0.0f, MAX_DIST - dist);

  /* initialize the axes */
    this->axes          = new Axes(5.0f);
    this->drawAxes      = false;

  /* initialize the light direction line */
    this->lightDir      = new Line(-scene.LightDir(), cs237::color3f(1,1,0), 10.0f);
    this->drawLightDir  = false;

  /* initialize the rendering state */
    this->mode          = WIREFRAME;
    this->enableSSAO    = false;

  // initialize the spot lights
    {
        int i = 0;
        this->lights.resize(scene.NumLights());
        for (auto it = scene.beginLights();  it != scene.endLights();  it++, i++) {
            this->lights[i].name        = it->name;
            this->lights[i].pos         = it->pos;
            this->lights[i].dir         = it->dir;
            this->lights[i].cutoff      = it->cutoff;
            this->lights[i].exponent    = it->exponent;
            this->lights[i].intensity   = it->intensity;
            for (int j = 0;  j < 3;  j++)
                this->lights[i].atten[j] = it->atten[j];
            this->lights[i].Init();
        }
        std::cerr << i << " lights" << std::endl;
    }

  /* convert the scene objects to instances that will serve as templates for the objects in the scene */
    std::vector<GObject> templates;
    templates.reserve (scene.NumModels());
    {
        int i = 0;
        std::map<std::string, cs237::texture2D *> texMap;
        for (auto it = scene.beginModels();  it != scene.endModels();  it++) {
            const OBJ::Model *model = (*it);
            GObject gobj;
            gobj.reserve (model->NumGroups());
            for (auto git = model->beginGroups();  git != model->endGroups();  git++) {
                MeshInfo *mesh = new MeshInfo (scene, model, *git);
                gobj.push_back(mesh);
            }
            templates.push_back(gobj);
        }
        std::cerr << i << " models" << std::endl;
    }

  /* initialize the object instances in the view */
    {
        int i = 0;
        this->objects.resize (scene.NumObjects());
        for (auto it = scene.beginObjs();  it != scene.endObjs();  it++, i++) {
            this->objects[i].meshes = templates[it->model];
            this->objects[i].toWorld = it->toWorld;
            this->objects[i].normToWorld = it->toWorld.normalMatrix();
          // the mapping for normals from world space to object space is (M^{-1})^{T},
          // where M is the mapping for tangent vectors, which is = toWorld^{-1}.
          // Therefore toWorld^{T} is the mapping from world space to object space.
            this->objects[i].normFromWorld = cs237::mat3x3f(it->toWorld).transpose();
            this->objects[i].color = it->color;
          // compute the world-space bounding box around the object
            cs237::AABBf modelBB = scene.Model(it->model)->BBox();
            this->objects[i].bbox.clear();
            for (int j = 0;   j < 8;  j++) {
                cs237::vec3f pt =
                    cs237::vec3f(it->toWorld * cs237::vec4f(modelBB.corner(j), 1));
                this->objects[i].bbox.addPt(pt);
            }
        }
        std::cerr << i << " objects" << std::endl;
    }

  // initialize the ground (if present)
    if (scene.Ground() != nullptr) {
        this->ground = new Ground(scene.Ground());
        std::cerr << "ground bbox: " << this->ground->BBox() << std::endl;
    }
    else {
        this->ground = nullptr;
    }

}

/* BindFramebuffer:
 *
 * initialize the framebuffer back to the screen.
 */
void View::BindFramebuffer()
{
    CS237_CHECK (glBindFramebuffer (GL_FRAMEBUFFER, 0) );
    CS237_CHECK (glViewport (0, 0, this->fbWid, this->fbHt) );

}

/* rotate the camera around the look-at point by the given angle (in degrees)
 */
void View::RotateLeft (float angle)
{
    this->camRot = cs237::rotateY(-angle) * this->camRot;
}

/* rotate the camera up by the given angle (in degrees)
 */
void View::RotateUp (float angle)
{
    this->camRot = cs237::rotateX(angle) * this->camRot;
}

/* move the camera towards the look-at point by the given distance
 */
void View::Move (float dist)
{
    if (dist < 0.0f) {  // move away from camera
        this->camOffset = cs237::max(this->minOffset, this->camOffset - dist);
    }
    else {  // move toward camera
        this->camOffset = cs237::min(this->maxOffset, this->camOffset - dist);
    }
}

/* InitProjMatrix:
 *
 * initialize the projection matrix based on the view state.
 */
void View::InitProjMatrix ()
{
  // compute vertical field of view as per 5.3.1 of M3D
    float eInv = tan (cs237::radians(this->fov) * 0.5f);
    float a = float(this->ht) / float(this->wid);
    float beta = cs237::degrees(2.0 * atan (eInv * a));

    this->projMat = cs237::perspective (
        beta,
        1.0f / a,
        NEAR_Z,
        FAR_Z);

}

/* InitViewMatrix:
 *
 * initialize the model-view matrix based on the view state.
 */
void View::InitViewMatrix ()
{
    cs237::mat4f mvMat = cs237::lookAt (this->camPos, this->camAt, this->camUp);
  // apply rotation followed by translation
    this->viewMat =
        mvMat * cs237::translate(cs237::vec3f(0.0f, 0.0f, this->camOffset)) * this->camRot;

}

/* InitGBuffer:
 */
void View::InitGBuffer()
{
    this->gbuffer = new GBuffer (this->fbWid, this->fbHt);
}

/* InitRenderers:
 */
void View::InitRenderers (Scene const &scene)
{
    this->renderers[WIREFRAME].push_back (new WireframeRenderer());
    this->renderers[TEXTURING].push_back (
        new TexturingRenderer (
            scene.LightDir(), scene.LightIntensity(), scene.AmbientLight()));

  // initialize renderers for deferred rendering
    this->geomRenderer = new GeomRenderer (this->gbuffer);
    this->spotlightRenderer = new SpotlightRenderer (this->gbuffer);
    this->dirLightRenderer = new DirLightRenderer (
        scene.LightDir(), scene.LightIntensity(), this->gbuffer);
    this->finalRenderer = new FinalRenderer (this->gbuffer, scene.AmbientLight());
    this->ssaoRenderer = new SSAORenderer (this->gbuffer, scene.AmbientLight());
}

/* Render:
 *
 * render the scene using forward rendering
 */
void View::Render ()
{
  /* clear the screen */
    glClearColor (0.2f, 0.2f, 0.4f, 1.0f);      // clear the surface
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    this->InitViewMatrix();

    if (this->mode == DEFERRED) {
        this->DeferredRender();
    }
    else { // forward rendering
        if (this->drawAxes) {
            this->axes->Draw (this->projMat, this->viewMat);
        }

        if (this->drawLightDir) {
          // draw a line segment that shows the light direction
            this->lightDir->Draw (this->projMat, this->viewMat);
          // draw the light meshes in wireframe mode
            RenderPass *r = this->renderers[WIREFRAME][0];
            r->Enable ();
            for (auto it = this->lights.begin();  it != this->lights.end();  it++) {
                if (it->cone != nullptr) { // in case the mesh hasn't been created yet
                    r->Render (this->projMat, this->viewMat, it->cone);
                }
            }
        }

        Renderer passes = this->renderers[this->mode];
        for (auto it = passes.begin(); it != passes.end(); it++) {
            RenderPass *r = *it;
            r->Enable ();

          // Draw the ground (if present)
            if (this->ground != nullptr) {
                r->Render (this->projMat, this->viewMat, this->ground->Instance());
            }

          /* Draw the objects in the scene */
            for (auto it = this->objects.begin();  it != this->objects.end();  it++) {
                r->Render (this->projMat, this->viewMat, &(*it));
            }
        }
    }

}

/* DeferredRender:
 *
 * render the scene using deferred rendering
 */
void View::DeferredRender ()
{
  // Bind and clear GBuffers
    this->gbuffer->BindForGeomPass();
    this->gbuffer->Clear();

  /*** Geometry Pass ******/
std::cerr << "Geometry Pass:\n";
    this->geomRenderer->Enable();
  // Draw the ground (if present)
    if (this->ground != nullptr) {
        this->geomRenderer->Render (this->projMat, this->viewMat, this->ground->Instance());
    }
  // Draw the objects in the scene
    for (auto it = this->objects.begin();  it != this->objects.end();  it++) {
        this->geomRenderer->Render (this->projMat, this->viewMat, &(*it));
    }

  /***** lighting passes *****/
std::cerr << "Lighting Pass:\n";
    this->gbuffer->BindForLightingPass();
    this->spotlightRenderer->Enable(this->camPos);
    for (auto it = lights.begin();  it != lights.end();  it++) {
        SpotLight light = (*it);
std::cerr << "  Spotlight\n";
        this->spotlightRenderer->Render (this->projMat, this->viewMat, *it);
    }
std::cerr << "  Directional light\n";
    this->dirLightRenderer->Render (this->camPos);

  /***** final render *****/
std::cerr << "Final Pass:" << (this->enableSSAO ? " (with SSAO)" : "") << "\n";
    this->gbuffer->BindForFinalPass();
    this->BindFramebuffer();
    if (this->enableSSAO) {
	this->ssaoRenderer->Render();
    }
    else {
	this->finalRenderer->Render();
    }

}

void View::ScreenShot (std::string const &prefix)
{
    int fwid, fht;

    CS237_CHECK (glfwGetFramebufferSize (this->win, &fwid, &fht) );

  // allocate an image for the screenshot
    cs237::image2d img(fwid, fht, GL_RGB, GL_UNSIGNED_BYTE);

  // get the pixels
    CS237_CHECK (glReadBuffer (GL_FRONT) );
    CS237_CHECK (glReadPixels(0, 0, fwid, fht, GL_RGB, GL_UNSIGNED_BYTE, img.data()) );

  // attach a suffix to the file name based on the current rendering mode
    std::string file;
    switch (this->mode) {
      case WIREFRAME: file = prefix + "-w.png"; break;
      case TEXTURING:
        if (this->drawLightDir) {
	    file = prefix + "-v.png";
	} else {
	    file = prefix + "-t.png";
	}
	break;
      case DEFERRED: file = prefix + "-d.png"; break;
      default: file = prefix + ".png"; break;
    }

    std::cerr << "saving screenshot to " << file << "\n";
    img.write (file.c_str(), true);
}

