#ifndef _STK_RESEARCH
#define _STK_RESEARCH

#include <vector>

#include <SFGUI/Window.hpp>

namespace st3{
  namespace research{

    /*! struct representing the research level of a player */
    struct data{
      /*! the available fields of research */
      enum fields{
	r_population = 0,
	r_industry,
	r_ship,
	r_num
      };
    
      /*! levels of the fields of research */
      std::vector<float> field;

      /*! default constructor */
      data();
    };
    
    class gui : public sfg::Window{
      data research_level;

    public:
      typedef std::shared_ptr<base> Ptr;
      typedef std::shared_ptr<const base> PtrConst;

      choice::c_researc response;
      bool accept;
      bool done;

      static Ptr Create(choice::c_research c, data r) override;
	
    protected:
      s_research(choice::c_research c, data r);
    };
  };
};
#endif
