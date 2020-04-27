/*
 *
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include <iomanip>
#include <stack>
#include <nori/common.h>
#include <nori/timer.h>
#include <nori/accel.h>
#include <Eigen/Geometry>

NORI_NAMESPACE_BEGIN

void Accel::addMesh(Mesh *mesh) {
    if (m_mesh)
        throw NoriException("Accel: only a single mesh is supported!");
    m_mesh = mesh;
    m_bbox = m_mesh->getBoundingBox();
}

void Accel::build()
{
    if(m_mesh->getTriangleCount() == 0)
        return;

    std::cout << "Constructing BVH for mesh with " << m_mesh->getTriangleCount()
              << " triangles...." << endl;


    Timer bvhBuildTimer;

    /*
        browse through all the triangle faces of the mesh and
        initiate a BvhPrimitiveInfo object
    */
    for(uint32_t idx = 0; idx < m_mesh->getTriangleCount(); ++idx)
    {
        /// during each iteration an BvhPrimitiveInfo is instantiated with the following info
        /// 1. bounding box of the triangle face of the mesh
        /// 2. centroid value of the triangle face
        /// 3. face index of the triangle
        m_primitives.push_back(new BvhPrimitiveInfo(m_mesh->getBoundingBox(idx),
                                                    m_mesh->getCentroid(idx),
                                                    idx));
    }

    /// allocate space for the m_nodes
    m_nodes.resize(2 * m_primitives.size() - 1,nullptr);

    /// call the recursive function to build BVH
    /// left_index = 0
    /// right_index = m_primitives.size()
    /// depth = 0
    uint32_t numNodes = buildRecursive(0,
                                       static_cast<uint32_t>(m_primitives.size()),
                                       0);

    cout << "BVH Node size before cleanup: " << m_nodes.size() << endl;

    auto it = std::remove_if(begin(m_nodes),
                             end(m_nodes),
                             [](BvhNode *node){return node == nullptr;});
    m_nodes.erase(it,end(m_nodes));

    cout << "BVH Node size after cleanup: " << m_nodes.size() << endl;

    cout << "BVH construction completed in " << bvhBuildTimer.elapsedString() << "\n";

}

uint32_t Accel::buildRecursive(uint32_t left_index,
                               uint32_t right_index,
                               uint32_t depth)
{
    /// declare the bounding box that encompass the primitives
    /// between left_index and right_index
    BoundingBox3f worldBBox;

    /// generate the bounding box between left_index and
    /// right_index
    for(uint32_t i = left_index; i < right_index;++i)
    {
        worldBBox.expandBy(m_primitives[i]->m_bbox);
    }

    /// get the number of primitives between
    /// left_index and right_index
    uint32_t numPrimitives = right_index - left_index;

    /// instantiate a BvhNode with the expanded
    /// bounding box
    BvhNode *node = new BvhNode(worldBBox);

    ///assign  the node into the m_nodes list
    m_nodes[depth] = node;

    if(numPrimitives <= m_maxPrimitivesPerNode)
    {
        node->makeLeaf(left_index,numPrimitives);
    }
    else
    {
        /*
         * it is the interior node, thus it is
         * qualified to be split into the two nodes
         * - left child node and right child node
         * */
        node->m_numPrimPerNode = numPrimitives;

        /// get the largest axis of the bounding box
        /// between left_index and right_index
        uint32_t largestAxis = static_cast<uint32_t>(worldBBox.getLargestAxis());

        if(worldBBox.max(largestAxis) - worldBBox.min(largestAxis) < Epsilon)
        {
            node->makeLeaf(left_index,numPrimitives);
            return depth;
        }


        uint32_t split_index = static_cast<uint32_t>((left_index + right_index) * 0.5f);

        switch (m_splitMethod)
        {
            case SPLIT_MIDDLE:
            {
                float pMid = 0.5f * (worldBBox.min(largestAxis) + worldBBox.max(largestAxis));

                CompareToMid mid(pMid,largestAxis);

                std::vector<BvhPrimitiveInfo*>::iterator bound = std::partition(m_primitives.begin() + left_index,
                                                                                m_primitives.begin() + right_index,
                                                                                mid);
                split_index = static_cast<uint32_t>(bound - m_primitives.begin());

                if(split_index != left_index &&
                   split_index != right_index)
                    break;
            }
            case SPLIT_EQUAL_POINTS:
            {
                ComparePrimitives comp;
                comp.m_sortingDim = largestAxis;

                split_index = static_cast<uint32_t>((left_index + right_index) * 0.5f);

                std::nth_element(m_primitives.begin() + left_index,
                                 m_primitives.begin() + split_index,
                                 m_primitives.begin() + right_index,
                                 comp);
                break;
            }
            default:
                break;
        }

        static std::stack<uint32_t> parentStack;

        parentStack.push(depth);

        /// build the left side of the tree recursively
        depth = buildRecursive(left_index,split_index,depth + 1);

        uint32_t p = parentStack.top();

        parentStack.pop();

        /// initiate the node as the right child of the current node
        m_nodes[p]->makeNode(depth + 1, largestAxis);

        /// build the right side of the tree recursively
        depth = buildRecursive(split_index,right_index, depth + 1);
    }

    return depth;
}

bool Accel::rayIntersect(const Ray3f &ray_, Intersection &its, bool shadowRay) const {

    Ray3f ray(ray_); /// Make a copy of the ray (we will need to update its '.maxt' value)

    its.t = std::numeric_limits<float>::infinity();

    /// store ray's minimum and maximum extent
    float minT = ray.mint;
    float maxT = ray.maxt;

    if(m_nodes.empty() || maxT < minT)
        return false;

    std::multimap<float,uint32_t> check;
    std::multimap<float,uint32_t>::iterator it;

    uint32_t nodeIndex = 0;

    /// get the root node - is the very first node in the node list
    /// it is confirmed during the construction of the node
    BvhNode *node = m_nodes[nodeIndex];

    if(!node->m_bbox.rayIntersect(ray,minT,maxT))
        return false;
    else
    {
        /// the entry will be sorted by the key value in the ascending order
        /// so it means that the earliest ray entry hit time will be at the
        /// beginning of the container
        check.insert(std::pair<float,uint32_t>(minT,nodeIndex));

        while (check.size() > 0) {
            it = check.begin();

            if(m_nodes[(*it).second]->m_leaf)
            {
                uint32_t firstPrimitiveOffset = m_nodes[(*it).second]->m_index;

                for(uint32_t i = 0; i < m_nodes[(*it).second]->m_numPrimPerNode; ++i)
                {
                    float u,v,t;

                    uint32_t f = m_primitives[i + firstPrimitiveOffset]->m_faceIndex;

                    if(m_mesh->rayIntersect(f,ray,u,v,t))
                    {
                        if(shadowRay)
                            return true;
                        if(t < its.t)
                        {
                            ray.maxt = its.t = t;
                            its.uv = Point2f(u,v);
                            its.mesh = m_mesh;


                            /* Find the barycentric coordinates */
                            Vector3f bary;
                            bary << 1-its.uv.sum(), its.uv;

                            /* References to all relevant mesh buffers */
                            const Mesh *mesh   = its.mesh;
                            const MatrixXf &V  = mesh->getVertexPositions();
                            const MatrixXf &N  = mesh->getVertexNormals();
                            const MatrixXf &UV = mesh->getVertexTexCoords();
                            const MatrixXu &F  = mesh->getIndices();

                            /* Vertex indices of the triangle */
                            uint32_t idx0 = F(0, f), idx1 = F(1, f), idx2 = F(2, f);

                            Point3f p0 = V.col(idx0), p1 = V.col(idx1), p2 = V.col(idx2);

                            /* Compute the intersection positon accurately
                               using barycentric coordinates */
                            its.p = bary.x() * p0 + bary.y() * p1 + bary.z() * p2;

                            /* Compute proper texture coordinates if provided by the mesh */
                            if (UV.size() > 0)
                                its.uv = bary.x() * UV.col(idx0) +
                                    bary.y() * UV.col(idx1) +
                                    bary.z() * UV.col(idx2);

                            /* Compute the geometry frame */
                            its.geoFrame = Frame((p1-p0).cross(p2-p0).normalized());

                            if (N.size() > 0) {
                                /* Compute the shading frame. Note that for simplicity,
                                   the current implementation doesn't attempt to provide
                                   tangents that are continuous across the surface. That
                                   means that this code will need to be modified to be able
                                   use anisotropic BRDFs, which need tangent continuity */

                                its.shFrame = Frame(
                                    (bary.x() * N.col(idx0) +
                                     bary.y() * N.col(idx1) +
                                     bary.z() * N.col(idx2)).normalized());
                            } else {
                                its.shFrame = its.geoFrame;
                            }
                        }
                    }
                }
            }
            else {
                /// NON-leaf nodes

                /// get the index of right child of current node
                nodeIndex = m_nodes[(*it).second]->m_index;

                BvhNode *rightChild = m_nodes[nodeIndex];

                if(rightChild->m_bbox.rayIntersect(ray,minT,maxT))
                    check.insert(std::pair<float,uint32_t>(minT,nodeIndex));

                ///get the index of left child of the node
                nodeIndex = (*it).second + 1;

                BvhNode *leftChild = m_nodes[nodeIndex];

                if(leftChild->m_bbox.rayIntersect(ray,minT,maxT))
                    check.insert(std::pair<float,uint32_t>(minT,nodeIndex));
            }

            check.erase(it);
        }
    }

    return  its.t < std::numeric_limits<float>::infinity() ;
}

NORI_NAMESPACE_END

