/*! \file g-buffer.hxx
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _G_BUFFER_HXX_
#define _G_BUFFER_HXX_

#include "cs237.hxx"

class GBuffer {
  public:

  //! the different types of buffers in the GBuffer; used to index the
  //! _bufs[] array.
    enum Type {
        COORD_BUF = 0,          //!< index for world-space-coordinates (plus the
                                //!< specular sharpness) buffer
        DIFFUSE_BUF,            //!< index for diffuse color buffer
        SPECULAR_BUF,           //!< index for specular color buffer; note that we
                                //!< store the sharpness exponent as the w component
                                //!< of the coord buffer
        EMISSIVE_BUF,           //!< index for emissive light buffer
        NORM_BUF,               //!< index for world-space-normals buffer
        DEPTH_BUF,              //!< index for depth/stencil buffer
        FINAL_BUF,              //!< index for final image buffer
        NUM_BUFS                //!< number of distinct buffers
    };

  //! the number of geometry buffers (i.e., buffers excluding DEPTH_BUF and FINAL_BUF)
    const static int NUM_GBUFS = DEPTH_BUF;

  //! the frame-buffer color attachment for the final buffer
    const static int FINAL_ATTACHMENT = GL_COLOR_ATTACHMENT0 + NUM_GBUFS;

  //! constructor
  //! \param wid width of buffers
  //! \param ht height of buffers
    GBuffer (uint32_t wid, uint32_t ht);

    ~GBuffer ();

  //! Resize the GBuffer in response to a change in the window size
  //! \param wid width of buffers
  //! \param ht height of buffers
    void Resize (uint32_t wid, uint32_t ht);

  //! the current width of the buffer
    int Width () const { return this->_wid; }

  //! the current height of the buffer
    int Height () const { return this->_ht; }

  //! clear the contents of the g-buffer
  //
    void Clear ();

  //! \brief setup the g-buffer for the geometry pass
  //!
    void BindForGeomPass ();

  //! \brief setup the g-buffer for the lighting passes
  //!
    void BindForLightingPass ();

  //! \brief setup the g-buffer for the final pass
  //!
    void BindForFinalPass ();

  //! \brief binds a g-buffer component to the provided texture unit
  //! \param texUnit the binding texture unit
  //! \param id the ID of the buffer to be read
    void BindTexForReading (GLenum texUnit, Type id);

  private:
    GLuint              _fbo;           //<! the frame-buffer object
    uint32_t            _wid;           //!< the width of the buffer
    uint32_t            _ht;            //!< the height of the buffer
    cs237::texture2D    *_bufs[NUM_BUFS]; //!< the geometry, depth, and final buffers

  //! private helper method for allocating and initializing the _buf[] array.
  //! This method is used by both the constructor and the Resize() method.
    void _InitBuffers ();

};

#endif //! _G_BUFFER_HXX_
