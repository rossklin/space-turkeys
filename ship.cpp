#include "ship.h"

using namespace st3;

ship::class_t ship::class_scout = "scout";
ship::class_t ship::class_fighter = "fighter";

std::vector<ship::class_t> ship::classes({
    ship::class_scout,
      ship::class_fighter});
