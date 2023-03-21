#pragma once

#include "Screen.h"
#include "Messages.h"

class GameOfLife {

public:
  GameOfLife(Screen*);
  
  void init(GameOfLifeConfigMessage*);

  void tick();

  bool is_done() const;

private:
  Screen* m_screen;
};