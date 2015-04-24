#include "research.h"
#include "solar.h"
#include "utility.h"

void st3::solar_choice::normalize(){
  utility::normalize_vector(p);
  utility::normalize_vector(do_research);
  utility::normalize_vector(industry);
  utility::normalize_vector(resource);
  utility::normalize_vector(industry_fleet);
}

st3::solar_choice::solar_choice(){

}

float st3::solar::resource_constraint(std::vector<float> r){
  return 1;
}

void st3::solar::add_ship(ship s)
{

}

st3::solar::solar()
{

}
