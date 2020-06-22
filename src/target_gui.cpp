#include "target_gui.h"

#include <iostream>

#include "command.h"
#include "fleet.h"
#include "utility.h"

using namespace std;
using namespace st3;

const target_gui::option_t target_gui::option_cancel("cancel", "");
const target_gui::option_t target_gui::option_add_waypoint("", fleet_action::go_to);

const point target_gui::option_size = point(200, 30);

target_gui::option_t::option_t(combid k, string v) : key(k), option(v) {}

st3::target_gui::target_gui(point p, list<target_gui::option_t> opts, list<combid> sel, window_t *w) : selected_option("", ""),
                                                                                                       selected_entities(sel),
                                                                                                       position(p) {
  if (opts.empty()) throw classified_error("attempting to build target gui without options!");

  options.insert(options.begin(), opts.begin(), opts.end());
  window = w;
  highlight_index = -1;
  done = false;
  bounds = sf::FloatRect(p.x, p.y, option_size.x, opts.size() * option_size.y);
}

bool st3::target_gui::handle_event(sf::Event e) {
  point p;
  int i;

  switch (e.type) {
    case sf::Event::MouseButtonReleased:
      p = window->mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
      i = compute_index(p);
      if (i > -1) {
        selected_option = options[i];
        done = true;
        return true;
      }
      break;
    case sf::Event::MouseButtonPressed:
      p = window->mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
      return bounds.contains(p);
    case sf::Event::MouseMoved:
      p = window->mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));

      highlight_index = compute_index(p);

      cout << "target gui: mouse move: " << p << ", root = " << point(bounds.left, bounds.top) << ", index = " << highlight_index << endl;
      break;
    case sf::Event::KeyPressed:
      switch (e.key.code) {
        case sf::Keyboard::Return:
          selected_option = options[0];
          done = true;
          return true;
      }
  }

  return false;
}

int st3::target_gui::compute_index(point p) {
  point scale = graphics::inverse_scale(*window);
  sf::FloatRect scaled_bounds(bounds.left, bounds.top,
                              bounds.width * scale.x,
                              bounds.height * scale.y);

  if (scaled_bounds.contains(p)) {
    return (p.y - scaled_bounds.top) / (option_size.y * scale.y);
  } else {
    return -1;
  }
}

void st3::target_gui::draw() {
  vector<sf::Text> text(options.size());
  float max_width = 0;
  sf::RectangleShape r;
  point scale = graphics::inverse_scale(*window);
  point boxdims(option_size.x * scale.x, option_size.y * scale.y);

  for (int i = 0; i < options.size(); i++) {
    text[i].setFont(graphics::default_font);
    text[i].setCharacterSize(0.5 * option_size.y);
    text[i].setScale(scale);
    text[i].setFillColor(sf::Color(10, 20, 30));
    text[i].setString(options[i].key + " - " + options[i].option);
    text[i].setPosition(bounds.left, bounds.top + i * boxdims.y);
    max_width = fmax(max_width, text[i].getLocalBounds().width);
  }

  r.setSize(point(fmax(boxdims.x, max_width * scale.x), boxdims.y));
  r.setOutlineColor(sf::Color(200, 200, 200));
  r.setOutlineThickness(2 * scale.x);

  for (int i = 0; i < options.size(); i++) {
    r.setPosition(bounds.left, bounds.top + i * boxdims.y);
    if (i == highlight_index) {
      r.setFillColor(sf::Color(150, 150, 150));
    } else {
      r.setFillColor(sf::Color(150, 150, 150, 150));
    }

    window->draw(r);
    window->draw(text[i]);
  }
}
