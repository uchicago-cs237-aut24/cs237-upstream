/*! \file main.c
 *
 * \brief this file contains the main function and the GLFW callbacks.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hxx"
#include "scene.hxx"
#include "view.hxx"

//! parameters for the view controls
#define ROTATE_ANGLE    3.0f            //!< angle of rotation in degrees
#define ZOOM_DIST       0.1f            //! distance to move camera

/***** Callback functions *****/

static void Error (int err, const char *msg)
{
    std::cerr << "[GLFW ERROR " << err << "] " << msg << std::endl;
}

/*! \brief Run the simulation and then redraw the animation.
 */
void Display (GLFWwindow *win)
{
    View *view = (View *)glfwGetWindowUserPointer(win);

  /* avoid drawing when we are invisible */
    if (! view->isVis)
        return;

    view->Render ();
    glfwSwapBuffers (win);

    view->needsRedraw = false;

} /* end of Display */

/*! \brief Window resize callback.
 *  \param wid the new width of the window.
 *  \param ht the new height of the window.
 */
void Reshape (GLFWwindow *win, int wid, int ht)
{
    View *view = (View *)glfwGetWindowUserPointer(win);

std::cerr << "Reshape(_, " << wid << ", " << ht << ")\n";

    view->wid = wid;
    view->ht = ht;

  /* reset the viewport
   * We use the framebuffer size instead of the window size, because it
   * is different on Apple's retina displays.
   */
    glfwGetFramebufferSize (win, &view->fbWid, &view->fbHt);
    glViewport(0, 0, view->fbWid, view->fbHt);

  /* recompute the projection matrix */
    view->InitProjMatrix ();

  /* resize the G-buffer */
    view->gbuffer->Resize(view->fbWid, view->fbHt);

} /* end of Reshape */

/*! \brief Keyboard-event callback.
 *  \param win The window receiving the event
 *  \param key key token
 *  \param scancode unique scancode for physical key (ignored)
 *  \param action PRESS, REPEAT, or RELEASE
 *  \param mods modifier bits
 */
void Key (GLFWwindow *win, int key, int scancode, int action, int mods)
{
  // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE) || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {
        View *view = (View *)glfwGetWindowUserPointer(win);

        switch (key) {
          case GLFW_KEY_A:  // 'a' or 'A' ==> toggle axes mode
            view->drawAxes = !view->drawAxes;
            view->needsRedraw = true;
            break;
          case GLFW_KEY_G:  // 'g' or 'G' ==> switch to deferred rendering mode
            view->needsRedraw = (view->mode != DEFERRED);
            view->mode = DEFERRED;
            break;
          case GLFW_KEY_L:  // 'l' or 'L' ==> toggle drawing of the light-direction
            view->drawLightDir = !view->drawLightDir;
            view->needsRedraw = true;
            break;
          case GLFW_KEY_O:  // 'o' or 'O' ==> toggle SSOA
            if (view->mode == DEFERRED) {
		view->enableSSAO = !view->enableSSAO;
		view->needsRedraw = true;
                std::cerr << "SSAO is " << (view->enableSSAO ? "on\n" : "off\n");
	    }
	    break;
          case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
            glfwSetWindowShouldClose (win, true);
            break;
	  case GLFW_KEY_S:  // 's' or 'S' ==> take a screenshot
            view->ScreenShot (view->sceneName);
	    break;
          case GLFW_KEY_T:  // 't' or 'T' ==> switch to texturing mode
            view->needsRedraw = (view->mode != TEXTURING);
            view->mode = TEXTURING;
            break;
          case GLFW_KEY_W:  // 'w' or 'W' ==> switch to wireframe mode
            view->needsRedraw = (view->mode != WIREFRAME);
            view->mode = WIREFRAME;
            break;
          case GLFW_KEY_LEFT:
          /* rotate camera to the left */
            view->RotateLeft (ROTATE_ANGLE);
            view->needsRedraw = true;
            break;
          case GLFW_KEY_RIGHT:
          /* rotate camera to the right */
            view->RotateLeft (-ROTATE_ANGLE);
            view->needsRedraw = true;
            break;
          case GLFW_KEY_UP:
          /* rotate camera up */
            view->RotateUp (ROTATE_ANGLE);
            view->needsRedraw = true;
            break;
          case GLFW_KEY_DOWN:
          /* rotate camera down */
            view->RotateUp (-ROTATE_ANGLE);
            view->needsRedraw = true;
            break;
          case GLFW_KEY_KP_ADD:
          /* zoom camera toward look-at point */
            view->Move (ZOOM_DIST);
            view->needsRedraw = true;
          case GLFW_KEY_EQUAL: // shifted is '+'
            if (mods == GLFW_MOD_SHIFT) {
              /* zoom camera toward look-at point */
                view->Move (ZOOM_DIST);
                view->needsRedraw = true;
            }
            break;
          case GLFW_KEY_KP_SUBTRACT:
          /* zoom camera away from look-at point */
            view->Move (-ZOOM_DIST);
            view->needsRedraw = true;
            break;
          case GLFW_KEY_MINUS:
            if (mods == 0) {
              /* zoom camera away from look-at point */
                view->Move (-ZOOM_DIST);
                view->needsRedraw = true;
            }
            break;
          default: // ignore all other keys
            return;
        }
    }

} /* end of Key */

/*! \brief Visibility-change callback: sets the visibility state of the view.
 *  \param state the current state of the window; GL_TRUE for iconified, GL_FALSE otherwise.
 */
void Visible (GLFWwindow *win, int state)
{
    View *view = (View *)glfwGetWindowUserPointer(win);

    view->isVis = (state == GL_FALSE);

} /* end of Visible */

/*! \brief the main function for the program
 *  \param argc the count of arguments in \a argv
 *  \param argv the array of command-line arguments.
 */
int main(int argc, const char **argv)
{
    glfwSetErrorCallback (Error);

    glfwInit ();

  // Check the GLFW version
    {
        int major;
        glfwGetVersion (&major, NULL, NULL);
        if (major < 3) {
            std::cerr << "CS237 requires GLFW 3.0 or later" << std::endl;
            exit (EXIT_FAILURE);
        }
    }

    glfwWindowHint (GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    std::string title = std::string("CMSC 23700 Project 4: ") + std::string(argv[1]);
    GLFWwindow *window = glfwCreateWindow(
        1024, 768,
        title.c_str(), NULL, NULL);
    if (window == nullptr)
        exit (EXIT_FAILURE);

    glfwMakeContextCurrent (window);

  // Check the OpenGL version
    {
        GLint major, minor;
        glGetIntegerv (GL_MAJOR_VERSION, &major);
        glGetIntegerv (GL_MINOR_VERSION, &minor);
        if ((major < 4) || ((major == 4) && (minor < 1))) {
            std::cerr << "CS237 requires OpenGL 4.1 or later; found " << major << "." << minor << std::endl;
            exit (EXIT_FAILURE);
        }
    }

  // load the scene file; note that this must happen _after_ the GL context
  // is established, since we are creating textures, etc.
    Scene scene;
    if (argc != 2) {
        std::cerr << "usage: proj4 <scene>" << std::endl;
        return EXIT_FAILURE;
    }
    else if (scene.Load(argv[1])) {
        return EXIT_FAILURE;
    }

  // set the window to the requested initial size
    glfwSetWindowSize(window, scene.Width(), scene.Height());

    View *view = new View(scene, window);

    view->BindFramebuffer ();
    view->InitGBuffer();

  // NOTE: the shaders can ony be initialized after a GL context is in place.
    view->InitRenderers (scene);

    view->InitProjMatrix ();
    view->InitViewMatrix ();

  // set up callbacks
    glfwSetWindowRefreshCallback (window, Display);
    glfwSetWindowSizeCallback (window, Reshape);
    glfwSetWindowIconifyCallback (window, Visible);
    glfwSetKeyCallback (window, Key);

    while (! glfwWindowShouldClose(window)) {
        if (view->needsRedraw) {
            Display (window);
        }
        glfwWaitEvents ();
    }

    glfwTerminate ();

    return EXIT_SUCCESS;

} /* end of main */
