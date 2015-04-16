#ifndef _STK_GRIDTREE
#define _STK_GRIDTREE

#include <list>
#include <SFML/Graphics.hpp>
#include "types.h"

namespace st3{
  namespace grid{
    typedef idtype key_type;
    typedef point value_type;
    typedef std::pair<point, point> bound_type;
    typedef std::pair<key_type, value_type> iterator_type;
    struct tree;

    struct node{
      tree *g;
      node *parent;
      bool is_leaf;
      bound_type bounds;

      // branch node
      point split;
      node *children[4]; // UL, UR, BL, BR

      // leaf node
      hm_t<key_type, value_type> leaves;

      node(tree *g, node *p);
      node(const node &n) = delete;
      ~node();
      void make_split();
      void insert(key_type k, value_type v); // return leaf node
      void move(key_type k, value_type v);
      std::list<iterator_type> search(point p, float r);
      void remove(key_type id);
    };

    struct tree{
      hm_t<key_type, node*> index;
      int max_leaves;
      node *root;

      tree();
      ~tree();
      tree(const tree &g) = delete;
      void insert(key_type id, value_type p);
      void move(key_type id, value_type p);
      std::list<iterator_type> search(value_type p, float r);
      void remove(key_type id);
    };
  };
};

#endif
