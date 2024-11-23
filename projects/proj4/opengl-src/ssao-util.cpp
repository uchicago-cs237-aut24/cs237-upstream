/*! \file ssao-util.cxx
 *
 * Support code for Scree-Space Ambient Occlusion
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "ssao-util.hxx"
#include <random>

static std::default_random_engine Generator;
static std::uniform_real_distribution<float> Dist_m1_1(-1.0f, 1.0f);
static std::uniform_real_distribution<float> Dist_0_1(0.0f, 1.0f);

// generate a random direction vector
//
cs237::vec3f RandomDir ()
{
    cs237::vec3f v;
    do {
	v[0] = Dist_m1_1(Generator);
	v[1] = Dist_m1_1(Generator);
	v[2] = Dist_m1_1(Generator);
    } while (dot(v, v) > 1.0);
    v.normalize();

    return v;
}

// uniform block layout for random directions
// NOTE: this struct must match the layout in ssao.fsh
//
struct RandomDirUB {
    cs237::vec3f	dirs[SSAO_MAX_NUM_POINTS];
    float               radii[256];
};

// setup the buffer of random direction vectors and random radii
//
GLuint InitRandomBuffer ()
{
    RandomDirUB		data;

    for (int i = 0;  i < SSAO_MAX_NUM_POINTS;  i++) {
	data.dirs[i] = RandomDir();
    }
    for (int i = 0;  i < 256;  i++) {
        data.radii[i] = (Dist_0_1(Generator) + 3.0f) * 0.1f;
    }

  // allocate the buffer and copy the random data to it
    GLuint bufId;
    CS237_CHECK( glGenBuffers (1, &bufId) );
    CS237_CHECK( glBindBuffer (GL_UNIFORM_BUFFER, bufId) );
    CS237_CHECK(
	    glBufferData (
		GL_UNIFORM_BUFFER,
		sizeof(RandomDirUB),
		&data,
		GL_STATIC_DRAW)
	);
    CS237_CHECK( glBindBuffer(GL_UNIFORM_BUFFER, 0) );

    return bufId;

}
