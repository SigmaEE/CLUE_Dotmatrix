#pragma once

#include "Screen.h"

class GameOfLife {

public: 
  enum class starting_configuration {
    SEED_1
  };

  GameOfLife(Screen*, starting_configuration);
  
  void init();

  void tick();

  bool is_done() const;

private:
  Screen* m_screen;
  starting_configuration m_starting_config;
};