#ifndef _STK_FIXED_STAR
#define _STK_FIXED_STAR

#include "graphics.h"

namespace st3{
  namespace client{
    /*! base class for graphical representation of game objects */
    class fixed_star{
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
      void draw(st3::window_t &w);
    };
  };
};
#endif
