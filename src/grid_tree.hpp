#pragma once

#include <SFML/Graphics.hpp>
#include <list>
#include <memory>

#include "types.hpp"

namespace st3 {
/*! functions for the 2d search tree */
namespace grid {
typedef combid key_type;                               /*!< key type */
typedef point value_type;                              /*!< value type */
typedef std::pair<point, point> bound_type;            /*!< type to hold lower and upper bounds */
typedef std::pair<key_type, value_type> iterator_type; /*!< type representing an element in the tree */
struct tree;

/*! a node tracks an area in 2d space */
struct node {
  tree *g;           /*!< pointer to the tree */
  node *parent;      /*!< pointer to parent node */
  bool is_leaf;      /*!< whether the node has elements */
  bound_type bounds; /*!< the lower and upper coordinate bounds */

  // branch node
  point split;       /*!< if not leaf: the point where the node splits into four children */
  node *children[4]; /*!< if not leaf: the four children: UL, UR, BL, BR */

  // leaf node
  hm_t<key_type, value_type> leaves; /*!< if leaf: table of elements in this leaf */

  /*! build a node from a tree and a parent node
	@param g the tree
	@param p the parent node
      */
  node(tree *g, node *p);

  /*! disallow copy construction */
  node(const node &n) = delete;

  /*! recursively delete children */
  ~node();

  /*! if leaf: find a suitable split point and generate the four child nodes */
  void make_split();

  /*! insert a key value pair
	
	If the node is a leaf, insert them in the leaves table, else
	insert them in a suitable child node.

	@param k key
	@param v value
      */
  void insert(key_type k, value_type v);

  /*! update the position for a given key
	@param k key
	@param v value
      */
  void move(key_type k, value_type v);

  /*! find all elements with value within a radius of a point
	@param p the point
	@param r the radius
	@return list of elements x such that l2norm(x.second - p) < r
      */
  std::list<iterator_type> search(point p, float r);

  /*! remove element by key
	@param k key
      */
  void remove(key_type k);
};

/*! a tree handles grid nodes */
struct tree {
  typedef std::unique_ptr<tree> ptr;
  static ptr create();

  hm_t<key_type, node *> index; /*!< table over which node elements are listed in */
  int max_leaves;               /*!< maximum number of leaves per node before split */
  node *root;                   /*!< root node */

  /*! default constructor */
  tree();

  /*! destructor: deletes nodes */
  ~tree();

  /*! disallow copy construction */
  tree(const tree &g) = delete;

  void clear();

  /*! insert an id-position pair
	@param id id
	@param p position
      */
  void insert(key_type id, value_type p);

  /*! update the position for given id
	@param id id
	@param p new position
      */
  void move(key_type id, value_type p);

  /*! find elements in a radius of a point
	@param p point to search around
	@param r search radius
	@return list of (id, position) pairs x s.t. l2norm(x.second - p) < r
      */
  std::list<iterator_type> search(value_type p, float r);

  /*! remove an element
	@param id id of element to remove
      */
  void remove(key_type id);
};
};  // namespace grid
};  // namespace st3
