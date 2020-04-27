#pragma once

#include <Eigen/Geometry>
#include <nori/bbox.h>
#include <nori/common.h>

NORI_NAMESPACE_BEGIN

struct BvhPrimitiveInfo {
    BvhPrimitiveInfo(const BoundingBox3f &bbox,
                     const Point3f &centroid,
                     const uint32_t &faceIndex):
        m_bbox(bbox),
        m_centroid(centroid),
        m_faceIndex(faceIndex)
    {}

    BoundingBox3f m_bbox; /// bounding box of the triangular face
    Point3f m_centroid;   /// centroid value of the triangular face
    uint32_t m_faceIndex; /// index of the triangular face in the mesh
};


struct CompareToMid
{
    float m_centroid;
    uint32_t m_axis;

    CompareToMid(float d, uint32_t a)
        : m_centroid(d),
          m_axis(a)
    {}

    bool operator()(BvhPrimitiveInfo *a)
    {
        return a->m_centroid[m_axis] < m_centroid;
    }
};


struct ComparePrimitives
{
    uint32_t m_sortingDim;

    bool operator()(BvhPrimitiveInfo *a, BvhPrimitiveInfo *b)
    {
        return a->m_centroid[m_sortingDim] < b->m_centroid[m_sortingDim];
    }
};

NORI_NAMESPACE_END
