
#pragma once

#include <nori/common.h>
#include <nori/bbox.h>

NORI_NAMESPACE_BEGIN

struct BvhNode
{
    BoundingBox3f m_bbox;
    bool m_leaf;
    uint32_t m_numPrimPerNode;

    /// Leaf node - if leaf node, m_index points to the traingle in the primitive list
    /// Interior node - if interior node, m_index points to the right child in the node list
    uint32_t m_index;
    uint32_t m_splitAxis;


    BvhNode(const BoundingBox3f &bbox)
        : m_bbox(bbox),
          m_leaf(false),
          m_numPrimPerNode(0),
          m_index(-1),
          m_splitAxis(-1)
    {

    }

    void makeLeaf(uint32_t index, uint32_t numObjs)
    {
        m_leaf = true;
        m_index = index;
        m_numPrimPerNode = numObjs;
    }

    void makeNode(uint32_t index, uint32_t axis)
    {
        m_leaf = false;

        /// the coordinate axis along which
        /// the primitives are stored
        m_splitAxis = axis;

        /// index to the right node in the node list
        /// left node is always next to the parent node in the node list
        m_index = index;
    }
};

NORI_NAMESPACE_END
