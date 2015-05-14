/*
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

#if !defined(__NORI_KDTREE_H)
#define __NORI_KDTREE_H

#include <nori/bbox.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Simple kd-tree node data structure for use with \ref PointKDTree.
 *
 * This class is an example of how one might write a space-efficient kd-tree
 * node that is compatible with the \ref PointKDTree class. The implementation
 * supports associating a custom data record with each node and works up to
 * 16 dimensions.
 *
 * \tparam _PointType Underlying point data type (e.g. \ref TPoint3<float>)
 * \tparam _DataRecord Custom storage that should be associated with each
 *  tree node
 *
 * \ingroup libcore
 * \sa PointKDTree
 * \sa LeftBalancedKDNode
 */
template <typename _PointType, typename _DataRecord> struct GenericKDTreeNode {
    typedef _PointType                       PointType;
    typedef _DataRecord                      DataRecord;
    typedef uint32_t                         IndexType;
    typedef typename PointType::Scalar       Scalar;

    enum {
        ELeafFlag  =  0x10,
        EAxisMask  =  0x0F
    };

    PointType position;
    IndexType right;
    DataRecord data;
    uint8_t flags;

    /// Initialize a KD-tree node
    GenericKDTreeNode() : position((Scalar) 0),
        right(0), data(), flags(0) { }
    /// Initialize a KD-tree node with the given data record
    GenericKDTreeNode(const PointType &position, const DataRecord &data)
        : position(position), right(0), data(data), flags(0) {}

    /// Given the current node's index, return the index of the right child
    IndexType getRightIndex(IndexType self) const { return right; }
    /// Given the current node's index, set the right child index
    void setRightIndex(IndexType self, IndexType value) { right = value; }

    /// Given the current node's index, return the index of the left child
    IndexType getLeftIndex(IndexType self) const { return self + 1; }
    /// Given the current node's index, set the left child index
    void setLeftIndex(IndexType self, IndexType value) {
        if (value != self+1)
            throw NoriException("GenericKDTreeNode::setLeftIndex(): Internal error!");
    }

    /// Check whether this is a leaf node
    bool isLeaf() const { return flags & (uint8_t) ELeafFlag; }
    /// Specify whether this is a leaf node
    void setLeaf(bool value) {
        if (value)
            flags |= (uint8_t) ELeafFlag;
        else
            flags &= (uint8_t) ~ELeafFlag;
    }

    /// Return the split axis associated with this node
    uint16_t getAxis() const { return flags & (uint8_t) EAxisMask; }
    /// Set the split flags associated with this node
    void setAxis(uint8_t axis) { flags = (flags & (uint8_t) ~EAxisMask) | axis; }

    /// Return the position associated with this node
    const PointType &getPosition() const { return position; }
    /// Set the position associated with this node
    void setPosition(const PointType &value) { position = value; }

    /// Return the data record associated with this node
    DataRecord &getData() { return data; }
    /// Return the data record associated with this node (const version)
    const DataRecord &getData() const { return data; }
    /// Set the data record associated with this node
    void setData(const DataRecord &val) { data = val; }
};

/* Forward declaration; the implementation is at the end of this file */
template <typename DataType, typename IndexType> void permute_inplace(
        DataType *data, std::vector<IndexType> &perm);

/**
 * \brief Generic multi-dimensional kd-tree data structure for point data
 *
 * This class organizes point data in a hierarchical manner so various
 * types of queries can be performed efficiently. It supports several
 * heuristics for building ``good'' trees, and it is oblivious to the
 * data layout of the nodes themselves.
 *
 * \tparam _NodeType Underlying node data structure. See \ref GenericKDTreeNode as
 * an example for the required public interface
 *
 * \ingroup libcore
 * \see GenericKDTreeNode
 */
template <typename _NodeType> class PointKDTree {
public:
    typedef _NodeType                        NodeType;
    typedef typename NodeType::PointType     PointType;
    typedef typename NodeType::IndexType     IndexType;
    typedef typename PointType::Scalar       Scalar;
    typedef typename PointType::VectorType   VectorType;
    typedef TBoundingBox<PointType>          BoundingBoxType;

    enum {
        Dimension = VectorType::RowsAtCompileTime
    };

    /// Supported tree construction heuristics
    enum Heuristic {
        /// Create a balanced tree by splitting along the median
        Balanced = 0,

        /**
         * \brief Use the sliding midpoint tree construction rule. This
         * ensures that cells do not become overly elongated.
         */
        SlidingMidpoint
    };

    /// Result data type for k-nn queries
    struct SearchResult {
        float distSquared;
        IndexType index;

        SearchResult() {}

        SearchResult(float distSquared, IndexType index)
            : distSquared(distSquared), index(index) { }

        std::string toString() const {
            std::ostringstream oss;
            oss << "SearchResult[distance=" << std::sqrt(distSquared)
                << ", index=" << index << "]";
            return oss.str();
        }

        bool operator==(const SearchResult &r) const {
            return distSquared == r.distSquared &&
                index == r.index;
        }
    };

public:
    /**
     * \brief Create an empty KD-tree that can hold the specified
     * number of points
     */
    PointKDTree(size_t nodes = 0, Heuristic heuristic = SlidingMidpoint)
        : m_nodes(nodes), m_heuristic(heuristic), m_depth(0) { }

    // =============================================================
    //! @{ \name \c stl::vector-like interface
    // =============================================================
    /// Clear the kd-tree array
    void clear() { m_nodes.clear(); m_bbox.reset(); }
    /// Resize the kd-tree array
    void resize(size_t size) { m_nodes.resize(size); }
    /// Reserve a certain amount of memory for the kd-tree array
    void reserve(size_t size) { m_nodes.reserve(size); }
    /// Return the size of the kd-tree
    size_t size() const { return m_nodes.size(); }
    /// Return the capacity of the kd-tree
    size_t capacity() const { return m_nodes.capacity(); }
    /// Append a kd-tree node to the node array
    void push_back(const NodeType &node) {
        m_nodes.push_back(node);
        m_bbox.expandBy(node.getPosition());
    }
    /// Return one of the KD-tree nodes by index
    NodeType &operator[](size_t idx) { return m_nodes[idx]; }
    /// Return one of the KD-tree nodes by index (const version)
    const NodeType &operator[](size_t idx) const { return m_nodes[idx]; }
    //! @}
    // =============================================================

    /// Set the BoundingBox of the underlying point data
    void setBoundingBox(const BoundingBoxType &bbox) { m_bbox = bbox; }
    /// Return the BoundingBox of the underlying point data
    const BoundingBoxType &getBoundingBox() const { return m_bbox; }
    /// Return the depth of the constructed KD-tree
    size_t getDepth() const { return m_depth; }
    /// Set the depth of the constructed KD-tree (be careful with this)
    void setDepth(size_t depth) { m_depth = depth; }

    /**
     * \brief Construct the KD-tree hierarchy
     *
     * When only adding nodes using the \ref push_back() function, the
     * bounding box is already computed, hence \c true can be passed
     * to this function to avoid an unnecessary recomputation.
     */
    void build(bool recomputeBoundingBox = false) {
        if (m_nodes.size() == 0) {
            std::cerr << "KDTree::build(): kd-tree is empty!" << endl;
            return;
        }

        cout << "Building a " << Dimension << "-dimensional kd-tree over "
             << m_nodes.size() << " data points ("
             << memString(m_nodes.size() * sizeof(NodeType)).c_str() << ") .. ";
        cout.flush();

        if (recomputeBoundingBox) {
            m_bbox.reset();
            for (size_t i=0; i<m_nodes.size(); ++i)
                m_bbox.expandBy(m_nodes[i].getPosition());
        }

        /* Instead of shuffling around the node data itself, only modify
           an indirection table initially. Once the tree construction
           is done, this table will contain a indirection that can then
           be applied to the data in one pass */
        std::vector<IndexType> indirection(m_nodes.size());
        for (size_t i=0; i<m_nodes.size(); ++i)
            indirection[i] = (IndexType) i;

        m_depth = 0;
        build(1, indirection.begin(), indirection.begin(), indirection.end());
        permute_inplace(&m_nodes[0], indirection);

        cout << "done." << endl;
    }

    /**
     * \brief Run a search query
     *
     * \param p Search position
     * \param results Index list of search results
     * \param searchRadius  Search radius
     */
    void search(const PointType &p, float searchRadius, std::vector<IndexType> &results) const {
        if (m_nodes.size() == 0)
            return;

        IndexType *stack = (IndexType *) alloca((m_depth+1) * sizeof(IndexType));
        IndexType index = 0, stackPos = 1, found = 0;
        float distSquared = searchRadius*searchRadius;
        stack[0] = 0;
        results.clear();

        while (stackPos > 0) {
            const NodeType &node = m_nodes[index];
            IndexType nextIndex;

            /* Recurse on inner nodes */
            if (!node.isLeaf()) {
                float distToPlane = p[node.getAxis()]
                    - node.getPosition()[node.getAxis()];

                bool searchBoth = distToPlane*distToPlane <= distSquared;

                if (distToPlane > 0) {
                    /* The search query is located on the right side of the split.
                       Search this side first. */
                    if (hasRightChild(index)) {
                        if (searchBoth)
                            stack[stackPos++] = node.getLeftIndex(index);
                        nextIndex = node.getRightIndex(index);
                    } else if (searchBoth) {
                        nextIndex = node.getLeftIndex(index);
                    } else {
                        nextIndex = stack[--stackPos];
                    }
                } else {
                    /* The search query is located on the left side of the split.
                       Search this side first. */
                    if (searchBoth && hasRightChild(index))
                        stack[stackPos++] = node.getRightIndex(index);

                    nextIndex = node.getLeftIndex(index);
                }
            } else {
                nextIndex = stack[--stackPos];
            }

            /* Check if the current point is within the query's search radius */
            const float pointDistSquared = (node.getPosition() - p).squaredNorm();

            if (pointDistSquared < distSquared) {
                ++found;
                results.push_back(index);
            }

            index = nextIndex;
        }
    }

    /**
     * \brief Run a k-nearest-neighbor search query
     *
     * \param p Search position
     * \param sqrSearchRadius
     *      Specifies the squared maximum search radius. This parameter can be used
     *      to restrict the k-nn query to a subset of the data -- it that is not
     *      desired, simply set it to positive infinity. After the query
     *      finishes, the parameter value will correspond to the (generally lower)
     *      maximum query radius that was necessary to ensure that the number of
     *      results did not exceed \c k.
     * \param k Maximum number of search results
     * \param results Target array for search results. Must
     *      contain storage for at least \c k+1 entries!
     *      (one extra entry is needed for shuffling data around)
     * \return The number of search results (equal to \c k or less)
     */
    size_t nnSearch(const PointType &p, float &_sqrSearchRadius,
            size_t k, SearchResult *results) const {
        if (m_nodes.size() == 0)
            return 0;

        IndexType *stack = (IndexType *) alloca((m_depth+1) * sizeof(IndexType));
        IndexType index = 0, stackPos = 1;
        float sqrSearchRadius = _sqrSearchRadius;
        size_t resultCount = 0;
        bool isHeap = false;
        stack[0] = 0;

        while (stackPos > 0) {
            const NodeType &node = m_nodes[index];
            IndexType nextIndex;

            /* Recurse on inner nodes */
            if (!node.isLeaf()) {
                float distToPlane = p[node.getAxis()] - node.getPosition()[node.getAxis()];

                bool searchBoth = distToPlane*distToPlane <= sqrSearchRadius;

                if (distToPlane > 0) {
                    /* The search query is located on the right side of the split.
                       Search this side first. */
                    if (hasRightChild(index)) {
                        if (searchBoth)
                            stack[stackPos++] = node.getLeftIndex(index);
                        nextIndex = node.getRightIndex(index);
                    } else if (searchBoth) {
                        nextIndex = node.getLeftIndex(index);
                    } else {
                        nextIndex = stack[--stackPos];
                    }
                } else {
                    /* The search query is located on the left side of the split.
                       Search this side first. */
                    if (searchBoth && hasRightChild(index))
                        stack[stackPos++] = node.getRightIndex(index);

                    nextIndex = node.getLeftIndex(index);
                }
            } else {
                nextIndex = stack[--stackPos];
            }

            /* Check if the current point is within the query's search radius */
            const float pointDistSquared = (node.getPosition() - p).squaredNorm();

            if (pointDistSquared < sqrSearchRadius) {
                /* Switch to a max-heap when the available search
                   result space is exhausted */
                if (resultCount < k) {
                    /* There is still room, just add the point to
                       the search result list */
                    results[resultCount++] = SearchResult(pointDistSquared, index);
                } else {
                    auto comparator = [](SearchResult &a, SearchResult &b) -> bool {
                        return a.distSquared < b.distSquared;
                    };

                    if (!isHeap) {
                        /* Establish the max-heap property */
                        std::make_heap(results, results + resultCount, comparator);
                        isHeap = true;
                    }
                    SearchResult *end = results + resultCount + 1;

                    /* Add the new point, remove the one that is farthest away */
                    results[resultCount] = SearchResult(pointDistSquared, index);
                    std::push_heap(results, end, comparator);
                    std::pop_heap(results, end, comparator);

                    /* Reduce the search radius accordingly */
                    sqrSearchRadius = results[0].distSquared;
                }
            }
            index = nextIndex;
        }
        _sqrSearchRadius = sqrSearchRadius;
        return resultCount;
    }

    /**
     * \brief Run a k-nearest-neighbor search query without any
     * search radius threshold
     *
     * \param p Search position
     * \param k Maximum number of search results
     * \param results
     *      Target array for search results. Must contain
     *      storage for at least \c k+1 entries! (one
     *      extra entry is needed for shuffling data around)
     * \return The number of used traversal steps
     */

    size_t nnSearch(const PointType &p, size_t k,
            SearchResult *results) const {
        float searchRadiusSqr = std::numeric_limits<float>::infinity();
        return nnSearch(p, searchRadiusSqr, k, results);
    }

protected:
    /// Return whether or not the inner node of the specified index has a right child node.
    bool hasRightChild(IndexType index) const {
        return m_nodes[index].getRightIndex(index) != 0;
    }

    /// Tree construction routine
    void build(size_t depth,
              typename std::vector<IndexType>::iterator base,
              typename std::vector<IndexType>::iterator rangeStart,
              typename std::vector<IndexType>::iterator rangeEnd) {
        if (rangeEnd <= rangeStart)
            throw NoriException("Internal error!");

        m_depth = std::max(depth, m_depth);

        IndexType count = (IndexType) (rangeEnd-rangeStart);

        if (count == 1) {
            /* Create a leaf node */
            m_nodes[*rangeStart].setLeaf(true);
            return;
        }

        int axis = 0;
        typename std::vector<IndexType>::iterator split;

        switch (m_heuristic) {
            case Balanced: {
                    /* Build a balanced tree */
                    split = rangeStart + count/2;
                    axis = m_bbox.getLargestAxis();
                };
                break;

            case SlidingMidpoint: {
                    /* Sliding midpoint rule: find a split that is close to the spatial median */
                    axis = m_bbox.getLargestAxis();

                    Scalar midpoint = (Scalar) 0.5f
                        * (m_bbox.max[axis]+m_bbox.min[axis]);

                    size_t nLT = std::count_if(rangeStart, rangeEnd,
                        [&](IndexType i) {
                            return m_nodes[i].getPosition()[axis] <= midpoint;
                        }
                    );

                    /* Re-adjust the split to pass through a nearby point */
                    split = rangeStart + nLT;

                    if (split == rangeStart)
                        ++split;
                    else if (split == rangeEnd)
                        --split;
                };
                break;
        }

        std::nth_element(rangeStart, split, rangeEnd,
            [&](IndexType i1, IndexType i2) {
                return m_nodes[i1].getPosition()[axis] < m_nodes[i2].getPosition()[axis];
            }
        );

        NodeType &splitNode = m_nodes[*split];
        splitNode.setAxis(axis);
        splitNode.setLeaf(false);

        if (split+1 != rangeEnd)
            splitNode.setRightIndex((IndexType) (rangeStart - base),
                    (IndexType) (split + 1 - base));
        else
            splitNode.setRightIndex((IndexType) (rangeStart - base), 0);

        splitNode.setLeftIndex((IndexType) (rangeStart - base),
                (IndexType) (rangeStart + 1 - base));
        std::iter_swap(rangeStart, split);

        /* Recursively build the children */
        Scalar temp = m_bbox.max[axis],
            splitPos = splitNode.getPosition()[axis];
        m_bbox.max[axis] = splitPos;
        build(depth+1, base, rangeStart+1, split+1);
        m_bbox.max[axis] = temp;

        if (split+1 != rangeEnd) {
            temp = m_bbox.min[axis];
            m_bbox.min[axis] = splitPos;
            build(depth+1, base, split+1, rangeEnd);
            m_bbox.min[axis] = temp;
        }
    }
protected:
    std::vector<NodeType> m_nodes;
    BoundingBoxType m_bbox;
    Heuristic m_heuristic;
    size_t m_depth;
};

/**
 * \brief Apply an arbitrary permutation to an array in linear time
 *
 * This algorithm is based on Donald Knuth's book
 * "The Art of Computer Programming, Volume 3: Sorting and Searching"
 * (1st edition, section 5.2, page 595)
 *
 * Given a permutation and an array of values, it applies the permutation
 * in linear time without requiring additional memory. This is based on
 * the fact that each permutation can be decomposed into a disjoint set
 * of permutations, which can then be applied individually.
 *
 * \param data
 *     Pointer to the data that should be permuted
 * \param perm
 *     Input permutation vector having the same size as \c data. After
 *     the function terminates, this vector will be set to the
 *     identity permutation.
 */
template <typename DataType, typename IndexType> void permute_inplace(
        DataType *data, std::vector<IndexType> &perm) {
    for (size_t i=0; i<perm.size(); i++) {
        if (perm[i] != i) {
            /* The start of a new cycle has been found. Save
               the value at this position, since it will be
               overwritten */
            IndexType j = (IndexType) i;
            DataType curval = data[i];

            do {
                /* Shuffle backwards */
                IndexType k = perm[j];
                data[j] = data[k];

                /* Also fix the permutations on the way */
                perm[j] = j;
                j = k;

                /* Until the end of the cycle has been found */
            } while (perm[j] != i);

            /* Fix the final position with the saved value */
            data[j] = curval;
            perm[j] = j;
        }
    }
}

NORI_NAMESPACE_END

#endif /* __NORI_KDTREE_H */
