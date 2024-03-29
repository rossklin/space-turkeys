#pragma once

#include "graphics.hpp"
#include "rsg/src/RskTypes.hpp"

namespace st3 {
/*! base class for graphical representation of game objects */
class fixed_star {
  sf::Color color; /*!< greatest distance from entities at which clicks are handled */
  float radius;

 public:
  point position;

  /** create a star at a point
       * @param p the point
       */
  fixed_star(point p);

  /*! draw the fixed star
	@param w the window
      */
  void draw(RSG::WindowPtr w);

  bool operator==(const fixed_star &rhs);
};
};  // namespace st3
