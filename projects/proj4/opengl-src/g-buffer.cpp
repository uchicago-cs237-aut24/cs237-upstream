/*! \file g-buffer.cxx
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "g-buffer.hxx"

//! A table of buffer-texture properties indexed by buffer
static struct {
    GLenum ifmt;        //!< the internal texture format
    GLenum fmt;         //!< format argument to glTexImage2D
    GLenum ty;          //!< type argument to glTexImage2D
    GLenum attach;      //!< the attachment tag
} BufferProps[GBuffer::NUM_BUFS] = {
        { GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0 },        // COORD_BUF
        { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT1 },    // DIFFUSE_BUF
        { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT2 },    // SPECULAR_BUF
        { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT3 },    // EMISSIVE_BUF
        { GL_RGB32F, GL_RGB, GL_FLOAT, GL_COLOR_ATTACHMENT4 },          // NORM_BUF
        { GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL,                       // DEPTH_BUF
          GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
          GL_DEPTH_STENCIL_ATTACHMENT },
        { GL_RGB32F, GL_RGB, GL_FLOAT, GBuffer::FINAL_ATTACHMENT }      // FINAL_BUF
    };

GBuffer::GBuffer (uint32_t wid, uint32_t ht)
    : _fbo(0), _wid(wid), _ht(ht)
{
    CS237_CHECK( glGenFramebuffers(1, &this->_fbo) );

  // allocate and attach the buffers
    this->_InitBuffers ();
}

GBuffer::~GBuffer ()
{
  // delete textures
    for (int i = 0;  i < NUM_BUFS;  i++) {
        if (this->_bufs[i] != nullptr) {
            delete this->_bufs[i];
        }
    }
}

void GBuffer::Resize (uint32_t wid, uint32_t ht)
{
  // avoid a resize if things haven't actually changed
    if ((this->_wid == wid) && (this->_ht == ht)) {
        return;
    }

    this->_wid = wid;
    this->_ht = ht;

  // first we delete the old buffers
    for (int i = 0;  i < NUM_BUFS;  i++) {
        if (this->_bufs[i] != nullptr) {
            delete this->_bufs[i];
        }
    }

  // allocate and attach fresh buffers
    this->_InitBuffers ();

}

void GBuffer::_InitBuffers ()
{
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, this->_fbo) );

  // for each buffer in the G-buffer, create a new texture and attach it to the
  // frame-buffer object
std::cerr << "_InitBuffers:\n";
    for (uint32_t i = 0 ; i < NUM_BUFS; i++) {
        this->_bufs[i] = new cs237::texture2D (
            GL_TEXTURE_2D, BufferProps[i].ifmt, this->_wid, this->_ht,
            BufferProps[i].fmt, BufferProps[i].ty);
std::cerr << "buffer " << i << ": texture = " << this->_bufs[i]->Id() << "\n";
        this->_bufs[i]->Parameter (GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        this->_bufs[i]->Parameter (GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        this->_bufs[i]->Parameter (GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        this->_bufs[i]->Parameter (GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CS237_CHECK(
            glFramebufferTexture(
                GL_FRAMEBUFFER,
                BufferProps[i].attach,
                this->_bufs[i]->Id(), 0) );
    }

    GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "G Buffer error, status = " << status << std::endl;
        exit (EXIT_FAILURE);
    }

  // restore default FBO
    CS237_CHECK( glBindTexture (GL_TEXTURE_2D, 0) );
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
}

void GBuffer::Clear ()
{
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, this->_fbo) );

    GLfloat zerosf[4] = { 0, 0, 0, 0 };
    GLuint zerosui[4] = { 0, 0, 0, 0 };
    for (int i = 0;  i < NUM_GBUFS;  i++) {
        if (BufferProps[i].ty == GL_FLOAT) {
            CS237_CHECK( glClearBufferfv (GL_COLOR, i, zerosf) );
        } else {
            CS237_CHECK( glClearBufferuiv (GL_COLOR, i, zerosui) );
        }
    }

  // clear depth buffer
    CS237_CHECK( glDepthMask (GL_TRUE) );
    CS237_CHECK( glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0f, 0) );

}

void GBuffer::BindForGeomPass ()
{
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, this->_fbo) );

    GLenum attachments[NUM_GBUFS];

    for (int i = 0;  i < NUM_GBUFS;  i++) {
        attachments[i] = BufferProps[i].attach;
    }

    CS237_CHECK( glDrawBuffers(NUM_GBUFS, attachments) );

    CS237_CHECK( glViewport(0, 0, this->_wid, this->_ht) );

}

void GBuffer::BindForLightingPass ()
{
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, this->_fbo) );

    glDrawBuffer (FINAL_ATTACHMENT);
    glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT);

  // clear the final buffer
//    GLfloat zeros[4] = { 0, 0, 0, 0 };
//    CS237_CHECK( glClearBufferfv (GL_COLOR, FINAL_BUF, zeros) );

}

void GBuffer::BindForFinalPass ()
{
    CS237_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
    CS237_CHECK( glDrawBuffer(GL_BACK) );
}

void GBuffer::BindTexForReading (GLenum texUnit, GBuffer::Type id)
{
    CS237_CHECK( glActiveTexture (texUnit) );
    this->_bufs[id]->Bind();
}
