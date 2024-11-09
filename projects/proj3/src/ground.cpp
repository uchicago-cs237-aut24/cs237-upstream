/*! \file ground.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3.  This file implements the
 * height-field mesh constructor.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "app.hpp"
#include "cs237/cs237.hpp"
#include "mesh.hpp"

// helper function that computes the normal for a triangle; vertices should be in CCW order
//
inline glm::vec3 triNormal (glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    return glm::normalize(glm::cross(v1 - v0, v2 - v0));
}

Mesh::Mesh (Proj3 *app, HeightField const *hf)
{
    uint32_t nr = hf->numRows();
    uint32_t nc = hf->numCols();
    uint32_t nVerts = hf->numVerts();
    assert ((nr >= 2) && (nc >= 2));
    uint32_t nTris = hf->numTris();

    // since we are not using triangle strips, the number of indices will be 3*nTris
    assert (nVerts < 0xffff && "too many vertices");
    Vertex *verts = new Vertex[nVerts];
    uint32_t *indices = new uint32_t[3*nTris];

    /***** vertex positions *****/

    for (int r = 0;  r < nr;  r++) {
	for (int c = 0;  c < nc;  c++) {
	    verts[hf->indexOf(r, c)].pos = hf->posAt(r, c);
	}
    }

    /***** vertex normals *****/

    // first compute the normals for the corner vertices of the height field
    verts[hf->indexOf(0,    0)].norm =
	triNormal(hf->posAt(0,0), hf->posAt(1,0), hf->posAt(0,1));
    verts[hf->indexOf(0,    nc-1)].norm =
	triNormal(hf->posAt(0,nc-1), hf->posAt(0,nc-2), hf->posAt(1,nc-1));
    verts[hf->indexOf(nr-1, 0)].norm =
	triNormal(hf->posAt(nr-1,0), hf->posAt(nr-1,1), hf->posAt(nr-2,1));
    verts[hf->indexOf(nr-1, nc-1)].norm =
	triNormal(hf->posAt(nr-1,nc-1), hf->posAt(nr-2,nc-1), hf->posAt(nr-1,nc-2));
    // then compute the normals for the edge vertices of the height field
    for (int r = 1;  r < nr-1;  r++) {
	verts[hf->indexOf(r, 0)].norm = 0.5f * (
	    triNormal(hf->posAt(r, 0), hf->posAt(r, 1), hf->posAt(r-1, 0)) +
	    triNormal(hf->posAt(r, 0), hf->posAt(r+1, 0), hf->posAt(r, 1)));
	verts[hf->indexOf(r, nc-1)].norm = 0.5f * (
	    triNormal(hf->posAt(r, nc-1), hf->posAt(r-1, nc-1), hf->posAt(r, nc-2)) +
	    triNormal(hf->posAt(r, nc-1), hf->posAt(r, nc-2), hf->posAt(r+1, nc-1)));
    }
    for (int c = 1;  c < nc-1;  c++) {
	verts[hf->indexOf(0,    c)].norm = 0.5f * (
	    triNormal(hf->posAt(0, c), hf->posAt(0, c-1), hf->posAt(1, c)) +
	    triNormal(hf->posAt(0, c), hf->posAt(1, c), hf->posAt(0, c+1)));
	verts[hf->indexOf(nr-1, c)].norm = 0.5f * (
	    triNormal(hf->posAt(nr-1, c), hf->posAt(nr-2, c), hf->posAt(nr-1, c-1)) +
	    triNormal(hf->posAt(nr-1, c), hf->posAt(nr-1, c+1), hf->posAt(nr-2, c)));
    }
    // finally compute the normals for the interior vertices
    for (int r = 1;  r < nr-1;  r++) {
	for (int c = 1;  c < nc-1;  c++) {
	    verts[hf->indexOf(r, c)].norm = 0.25f * (
		triNormal(hf->posAt(r,c), hf->posAt(r-1,c), hf->posAt(r,c-1)) +
		triNormal(hf->posAt(r,c), hf->posAt(r,c+1), hf->posAt(r-1,c)) +
		triNormal(hf->posAt(r,c), hf->posAt(r+1,c), hf->posAt(r,c+1)) +
		triNormal(hf->posAt(r,c), hf->posAt(r,c-1), hf->posAt(r+1,c)));
	}
    }

    /***** texture coordinates *****/

    // we want the following mapping for the height-field corners:
    //	(row,  col)		(x, y)
    //   --------------------------------
    //	(0,    0)		(0, 1)
    //	(0,    nc-1)		(1, 1)
    //	(nr-1, 0)		(0, 0)
    //	(nr-1, nc-1)		(1, 0)
    //
    for (int r = 0;  r < nr;  r++) {
	float y = float(nr - r - 1) / float(nr-1);
	for (int c = 0;  c < nc;  c++) {
	    float x = float(c) / float(nc-1);
	    verts[hf->indexOf(r,c)].txtCoord = glm::vec2(x, y);
	}
    }

    /***** tangent vectors *****/

    /** HINT: compute the extended tangent vectors for normal-mapping mode */

    /***** indices *****/

    int idx = 0;
    for (int r = 1;  r < nr;  r++) {
	for (int c = 1;  c < nc;  c++) {
	    if ((c & 1) == (r & 1)) {
	      // triangulate from upper left to lower right
		indices[idx + 0] = hf->indexOf(r,   c-1);
		indices[idx + 1] = hf->indexOf(r,   c  );
		indices[idx + 2] = hf->indexOf(r-1, c-1);
		indices[idx + 3] = hf->indexOf(r-1, c  );
		indices[idx + 4] = hf->indexOf(r-1, c-1);
		indices[idx + 5] = hf->indexOf(r,   c  );
	    }
	    else {
	      // triangulate from lower left to upper right
		indices[idx + 0] = hf->indexOf(r,   c  );
		indices[idx + 1] = hf->indexOf(r-1, c  );
		indices[idx + 2] = hf->indexOf(r,   c-1);
		indices[idx + 3] = hf->indexOf(r-1, c-1);
		indices[idx + 4] = hf->indexOf(r,   c-1);
		indices[idx + 5] = hf->indexOf(r-1, c  );
	    }
	    idx += 6;
	}
    }

    // create the Vulkan buffer objects
    this->vBuf = new cs237::VertexBuffer<Vertex>(
        app,
        vk::ArrayProxy<Vertex>(nVerts, verts));
    this->iBuf = new cs237::IndexBuffer<uint32_t>(
        app,
        vk::ArrayProxy<uint32_t>(3*nTris, indices));

    // free the temporary arrays
    delete[] verts;
    delete[] indices;

    /** HINT: other initialization, such as color and normal maps, and samplers */

}
