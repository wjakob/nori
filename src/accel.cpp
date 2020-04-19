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

#include <stack>
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
    Timer bvhBuildTimer;

    /* Nothing to do here for now */
    for(uint32_t idx = 0; idx < m_mesh->getTriangleCount(); ++idx)
    {
        m_primitives.push_back(new BvhPrimitiveInfo(m_mesh->getBoundingBox(idx),
                                                    m_mesh->getCentroid(idx),
                                                    idx));
    }

    m_nodes.resize(2 * m_primitives.size(),nullptr);

    uint32_t numNodes = buildRecursive(0,m_primitives.size(),0);

    auto it = std::remove_if(begin(m_nodes),
                             end(m_nodes),
                             [](BvhNode *node){return node == nullptr;});
    m_nodes.erase(it,end(m_nodes));


    cout << "BVH construction completed in " << bvhBuildTimer.elapsedString() << endl;
}

uint32_t Accel::buildRecursive(uint32_t left_index,
                               uint32_t right_index,
                               uint32_t depth)
{
    BoundingBox3f worldBBox;

    /// generate the bounding box between left_index and
    /// right_index
    for(uint32_t i = left_index; i < right_index;++i)
    {
        worldBBox.expandBy(m_primitives[i]->m_bbox);
    }

    uint32_t numPrimitives = right_index - left_index;

    BvhNode *node = new BvhNode(worldBBox);
    m_nodes[depth] = node;

    if(numPrimitives <= m_primitivesPerNode)
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

        uint32_t largestAxis =(uint32_t)worldBBox.getLargestAxis();

        uint32_t splitIndex = (uint32_t) ((left_index + right_index) * 0.5f);

        switch (m_splitMethod) {
            case SPLIT_MIDDLE:
            {
                float pMid = 0.5f * (worldBBox.min[largestAxis] + worldBBox.max[largestAxis]);

                CompareToMid mid(pMid,largestAxis);

                std::vector<BvhPrimitiveInfo*>::iterator bound = std::partition(m_primitives.begin() + left_index,
                                                                                m_primitives.begin() + right_index,mid);
                splitIndex = bound - m_primitives.begin();

                if(splitIndex != left_index &&
                   splitIndex != right_index)
                    break;
            }
            case SPLIT_EQUAL_POINTS:
            {
                ComparePrimitives comp;
                comp.m_sortingDim = largestAxis;

                splitIndex = (uint32_t) ((left_index + right_index) * 0.5f);

                std::nth_element(m_primitives.begin() + left_index,
                                 m_primitives.begin() + splitIndex,
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
        depth = buildRecursive(left_index,splitIndex,depth + 1);

        uint32_t p = parentStack.top();

        parentStack.pop();

        m_nodes[p]->makeNode(depth + 1, largestAxis);

        depth = buildRecursive(splitIndex,right_index, depth + 1);
    }

    return depth;
}

bool Accel::rayIntersect(const Ray3f &ray_, Intersection &its, bool shadowRay) const {

    Ray3f ray(ray_); /// Make a copy of the ray (we will need to update its '.maxt' value)

    bool foundIntersection = false;  // Was an intersection found so far?

    float minT = ray.mint;
    float maxT = ray.maxt;

    std::multimap<float,uint32_t> check;
    std::multimap<float,uint32_t>::iterator it;

    uint32_t nodeIndex = 0;

    /// get the root node - is the very first node in the node list
    /// it is confirmed during the construction of the node
    BvhNode *node = m_nodes[nodeIndex];

    /// check if the ray intersects the root bounding box
    if(node->m_bbox.rayIntersect(ray,minT,maxT))
    {
        /// the root node bounding box is intersected and
        /// therefore we need to insert it to the map for
        /// further processing
        check.insert(std::pair<float,uint32_t>(minT,nodeIndex));

        while(check.size() > 0)
        {
            it = check.begin();

            /// check for the leaf node
            if(m_nodes[(*it).second]->m_leaf)
            {
               uint32_t  firstPrimitiveOffset = m_nodes[(*it).second]->m_index;

               uint32_t f = (uint32_t) -1;      // Triangle index of the closest intersection

               for(uint32_t i = 0; i < m_nodes[(*it).second]->m_numPrimPerNode;++i)
               {
                   float u,v,t;
                   if(m_mesh->rayIntersect(firstPrimitiveOffset + i,ray,u,v,t))
                   {
                       if(shadowRay)
                           return true;

                       ray.maxt = its.t = t;
                       its.uv = Point2f(u,v);
                       its.mesh = m_mesh;
                       f = firstPrimitiveOffset + i;
                       foundIntersection = true;
                   }
               }

               if(foundIntersection)
               {
                   /* At this point, we now know that there is an intersection,
                      and we know the triangle index of the closest such intersection.

                      The following computes a number of additional properties which
                      characterize the intersection (normals, texture coordinates, etc..)
                   */

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

               return foundIntersection;
            }
            else
            {
                /// dealing with the interior node

                /// get the index of the right child of the current node
                nodeIndex = m_nodes[(*it).second]->m_index;

                /// get the right child
                BvhNode *rightChild = m_nodes[nodeIndex];

                if(rightChild->m_bbox.rayIntersect(ray,minT,maxT))
                    check.insert(std::pair<float,uint32_t>(minT,nodeIndex));

                /// left child is by default located next to the parent node in the node list - m_nodes
                nodeIndex = (*it).second + 1;

                /// get the left child
                BvhNode *leftChild = m_nodes[nodeIndex];

                if(leftChild->m_bbox.rayIntersect(ray,minT,maxT))
                    check.insert(std::pair<float,uint32_t>(minT,nodeIndex));
            }

            check.erase(it);
        }
    }

    return foundIntersection;
}

NORI_NAMESPACE_END

