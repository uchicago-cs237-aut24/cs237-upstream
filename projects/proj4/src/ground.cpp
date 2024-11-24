/*! \file ground.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 4.  This file implements the
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

Mesh::Mesh (Proj4 *app, HeightField const *hf)
: device(app->device()),
  vBuf(nullptr), iBuf(nullptr), prim(vk::PrimitiveTopology::eTriangleList), aabb(),
  albedoSrc(MtlPropertySrc::eNone), albedoTexture(),
  emissiveSrc(MtlPropertySrc::eNone), emissiveTexture(),
  specularSrc(MtlPropertySrc::eNone), specularTexture(),
  nMap()
{
    uint32_t nr = hf->numRows();
    uint32_t nc = hf->numCols();
    uint32_t nVerts = hf->numVerts();
    assert ((nr >= 2) && (nc >= 2));
    uint32_t nTris = hf->numTris();

    // since we are not using triangle strips, the number of indices will be 3*nTris
    Vertex *verts = new Vertex[nVerts];
    uint32_t *indices = new uint32_t[3*nTris];

    /***** vertex positions *****/

    for (int r = 0;  r < nr;  r++) {
	for (int c = 0;  c < nc;  c++) {
            auto pos = hf->posAt(r, c);
	    verts[hf->indexOf(r, c)].pos = pos;
            this->aabb.addPt(pos);
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

    // compute the extended tangent vectors for normal-mapping mode
    for (int r = 0;  r < nr;  r++) {
	for (int c = 0;  c < nc;  c++) {
	    int idx = hf->indexOf(r, c);
	    glm::vec3 T(1.0f, 0.0f, 0.0f);
	    glm::vec3 B(0.0f, 0.0f, 1.0f);
	    glm::vec3 N = verts[idx].norm;
	    // orthogonalize using Gram-Schmidt
	    T = normalize (T - dot(T, N) * N);
	    B = normalize (B - dot(B, N) * N - dot(B, T) * T);
	    float Tw = dot(B, cross(N, T));
	    verts[idx].tan = glm::vec4(T[0], T[1], T[2], (Tw < 0.0f) ? -1 : 1);
	}
    }

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

    this->albedoColor = hf->color();
    if (hf->colorMap() != nullptr) {
        this->albedoSrc = MtlPropertySrc::eTexture;
        this->albedoTexture.define(app, hf->colorMap());
    }
    this->emissiveSrc = MtlPropertySrc::eNone;
    this->specularSrc = MtlPropertySrc::eNone;
    if (hf->normalMap() != nullptr) {
        this->nMap.define(app, hf->normalMap());
    }

    this->initUBO(app);
}
