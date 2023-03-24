#pragma once

#include "Screen.h"
#include "Messages.h"

class GameOfLife {

public:
  GameOfLife(Screen*);
  
  void init(GameOfLifeConfigMessage*);

  void tick();

  void terminate();

  bool is_done() const;

private:
  Screen* m_screen;
  bool m_is_done;
  uint16_t m_time_between_ticks;
  uint32_t m_timestamp_at_last_tick;

  bool check_is_done() const;
};