#pragma once

#include <SFML/Graphics.hpp>
#include <list>
#include <map>
#include <memory>
#include <set>

#include "types.hpp"

namespace st3 {
/*! functions for the 2d search tree */
namespace grid {
// typedef combid key_type;                               /*!< key type */
// typedef point value_type;                              /*!< value type */
// typedef std::pair<point, point> bound_type;            /*!< type to hold lower and upper bounds */
// typedef std::pair<key_type, value_type> iterator_type; /*!< type representing an element in the tree */
// struct tree;

// /*! a node tracks an area in 2d space */
// struct node {
//   typedef std::shared_ptr<node> ptr;
//   std::weak_ptr<tree> g;      /*!< pointer to the tree */
//   std::weak_ptr<node> parent; /*!< pointer to parent node */
//   bool is_leaf;               /*!< whether the node has elements */
//   bound_type bounds;          /*!< the lower and upper coordinate bounds */

//   // branch node
//   point split;     /*!< if not leaf: the point where the node splits into four children */
//   ptr children[4]; /*!< if not leaf: the four children: UL, UR, BL, BR */

//   // leaf node
//   hm_t<key_type, value_type> leaves; /*!< if leaf: table of elements in this leaf */

//   /*! build a node from a tree and a parent node
// 	@param g the tree
// 	@param p the parent node
//       */
//   node(tree *g, node *p);

//   /*! disallow copy construction */
//   node(const node &n) = delete;

//   /*! recursively delete children */
//   ~node();

//   /*! if leaf: find a suitable split point and generate the four child nodes */
//   void make_split();

//   /*! insert a key value pair

// 	If the node is a leaf, insert them in the leaves table, else
// 	insert them in a suitable child node.

// 	@param k key
// 	@param v value
//       */
//   void insert(key_type k, value_type v);

//   /*! update the position for a given key
// 	@param k key
// 	@param v value
//       */
//   void move(key_type k, value_type v);

//   /*! find all elements with value within a radius of a point
// 	@param p the point
// 	@param r the radius
// 	@return list of elements x such that l2norm(x.second - p) < r
//       */
//   std::list<iterator_type> search(point p, float r);

//   /*! remove element by key
// 	@param k key
//       */
//   void remove(key_type k);
// };

/*! a tree handles grid nodes */
template <typename V>
struct tree {
  typedef std::shared_ptr<tree> ptr;
  // static ptr create();
  typedef std::pair<int, int> K;

  const float r = 10;

  std::map<K, std::set<V>> index; /*!< table over which node elements are listed in */
  std::map<V, K> reverse_index;

  // int max_leaves;                                    /*!< maximum number of leaves per node before split */
  // node *root;                                        /*!< root node */

  // /*! default constructor */
  // tree();

  void clear() {
    index.clear();
    reverse_index.clear();
  }

  K make_key(point p) {
    return K(p.x / r, p.y / r);
  }

  /*! insert an id-position pair
	@param id id
	@param p position
      */
  void insert(V id, point p) {
    K k = make_key(p);
    index[k].insert(id);
    reverse_index[id] = k;
  }

  /*! update the position for given id
	@param id id
	@param p new position
      */
  void move(V id, point p) {
    remove(id);
    insert(id, p);
  }

  /*! find elements in a radius of a point
	@param p point to search around
	@param r search radius
	@return list of (id, position) pairs x s.t. l2norm(x.second - p) < r
      */
  std::list<std::pair<V, point>> search(point p, float rad) {
    std::list<std::pair<V, point>> res;
    for (float x = p.x - rad; x <= p.x + rad; x += r) {
      for (float y = p.y - rad; y <= p.y + rad; y += r) {
        K k = make_key(point(x, y));
        for (auto id : index[k]) res.push_back({id, {x, y}});
      }
    }

    return res;
  }

  /*! remove an element
	@param id id of element to remove
      */
  void remove(V id) {
    K k = reverse_index[id];
    reverse_index.erase(id);
    index[k].erase(id);
  }
};  // namespace grid
};  // namespace grid
};  // namespace st3
