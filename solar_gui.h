#ifndef _STK_SOLARGUI
#define _STK_SOLARGUI

#include <string>
#include <vector>
#include <functional>

#include "types.h"
#include "solar.h"
#include "research.h"

#include <TGUI/TGUI.hpp>

namespace st3{
  /*! blocking gui to generate a solar choice */
  class solar_gui : public tgui::Gui{
    /*! names of available choice templates */
    static std::vector<std::string> template_name;

    /*! function type to compute time derivatives of subsectors in control group */
    typedef std::function<float(int, float)> incfunc;
    struct control_group;

    /*! struct combining subsector label, slider for input and feedback label */
    struct control{
      /*! label for subsector */
      tgui::Label::Ptr label;

      /*! slider for assigning workers to subsector */
      tgui::Slider::Ptr slider;

      /*! label for feedback: resulting increment per time */
      tgui::Label::Ptr feedback;

      /*! default constructor: assign the gui to tgui compontents */
      control(tgui::Gui &g);

      /*! set component dimensions and values
	@param c control group for callback
	@param id subsector id
	@param conf file path of tgui conf file
	@param pos absolute pixel position
	@param dims pixel dimensions
	@param name subsector name
	@param base_value current subsector level value, or -1 if N/A
	@param value value to set worker assignment slider to
	@param cap maximum for worker assignment slider
	@param deriv value to set per time increment feedback to */
      void setup(solar_gui::control_group *c,
		 int id,
		 std::string conf, 
		 point pos, 
		 point dims, 
		 std::string name, 
		 float base_value,
		 float value, 
		 float cap, 
		 float deriv);

      /*! update per time increment feedback
	@param deriv new feedback value */
      void update(float deriv);
    };

    /*! structure handling input for a work sector */
    struct control_group{
      /*! reference to the main gui for relating industry/agriculture caps */
      solar_gui *sg;

      /*! sector id */
      int id;

      /*! function to compute per time increments */
      incfunc incs;

      /*! maximum number of workers in this sector */
      float cap;

      /*! sector label */
      tgui::Label::Ptr label;

      /*! controls for subsectors */
      std::vector<control*> controls;

      /*! default constructor
	@param g reference to parent solar gui
	@param id sector id
	@param conf file location of tgui config file
	@param pos absolute pixel position
	@param label_dims dimension to use for labels
	@param names names of subsectors
	@param values current level values of subsectors
	@param proportions proportions to distribute workers between subsectors
	@param total total number of workers assigned to sector
	@param incs function to compute per time increments
	@param cap maximum number of workers to assign to sector
      */
      control_group(solar_gui *g, 
		    int id,
		    std::string conf,
		    std::string title,
		    point pos,
		    point label_dims,
		    std::vector<std::string> names,
		    std::vector<float> values,
		    std::vector<float> proportions,
		    float total,
		    incfunc incs,
		    float cap);

      /*! default destructor: deletes sub controls */
      ~control_group();

      /*! function to update control values
	@param tot total number of assigned workers
	@param p proprtions to disrtibute workers between subsectors
      */
      void load_controls(float tot, std::vector<float> p);

      /*! callback function to handle updating the value of a control
	@param c tgui::Callback identifier
      */
      void update_value(tgui::Callback const &c);

      /*! compute the sum of values over the controls
	@return the sum
      */
      float get_sum();
    };

    /*! reference to the window */
    sf::RenderWindow &window;

    /*! controls for sectors */
    std::vector<control_group*> controls;

    /*! label to show population info */
    tgui::Label::Ptr header_population;

    /*! template selector */
    tgui::ComboBox::Ptr tsel;

    /*! indicate to client_game when the gui is done */
    bool done;

    /*! indicate to client_game whether the choice was accepted */
    bool accept;

    /*! data about the solar */
    solar::solar s;

    /*! current research level */
    research r;

    /*! time per round used to compute sector increments */
    float round_time;

  public:
    /*! choice to be read by client_game */
    solar::choice_t c;

    /*! default constructor
      @param w main window
      @param s solar data
      @param c choice initializer
      @param r current research level
      @param rt timer per round
     */
    solar_gui(sf::RenderWindow &w, solar::solar s, solar::choice_t c, research r, float rt);

    /*! destructor: deletes sector controls */
    ~solar_gui();

    /*! set the controls to match a choice
      @param c choice to set controls for
    */
    void load_controls(solar::choice_t c);

    /*! callback for buttons done/cancel
      @param a whether done was pressed
    */
    void callback_done(bool a);

    /*! callback for template selector */
    void callback_template();

    /*! run the solar gui 
     @return whether the choice was accepted
    */
    bool run();

    /*! build a choice object from the control states
      @return the choice 
    */
    solar::choice_t evaluate();

    /*! compute the number of workers available for a sector
      @param id sector id
      @return number of workers
    */
    float get_control_cap(int id);

    /*! update the population header label to match controls */
    void update_popsum();

    /*! load a choice template
      @param s name of template
    */
    void load_template(std::string s);

    /*! compute the number of workers s.t. the population increment is a factor p between the minimun and the maximum
      @param p population growth priority
      @return allowed number of workers
    */
    float compute_workers(float p);

    /*! compute the number of workers s.t. the population increment is at least 0 but if possible a factor p between the minimun and the maximum
      @param p population growth priority
      @return allowed number of workers
    */
    float compute_workers_nostarve(float p);
  };
};
#endif
