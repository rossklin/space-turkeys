#include "utility.cpp"
#include "selector.h"

// instantiate selector cast templates
using namespace st3;
using namespace client;

template ship_selector::ptr utility::guaranteed_cast<ship_selector, entity_selector>(entity_selector::ptr);
template fleet_selector::ptr utility::guaranteed_cast<fleet_selector, entity_selector>(entity_selector::ptr);
template solar_selector::ptr utility::guaranteed_cast<solar_selector, entity_selector>(entity_selector::ptr);
template waypoint_selector::ptr utility::guaranteed_cast<waypoint_selector, entity_selector>(entity_selector::ptr);
