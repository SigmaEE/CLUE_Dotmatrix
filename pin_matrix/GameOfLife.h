#pragma once

#include "Screen.h"
#include "Messages.h"

class GameOfLife {

public:
  GameOfLife(Screen*);
  
  void init();

  void init(GameOfLifeConfigMessage*);

  void increment_config();

  void tick();

  void terminate();

  bool is_done() const;

private:
  Screen* m_screen;
  bool m_is_done;
  uint16_t m_time_between_ticks;
  uint32_t m_timestamp_at_last_tick;
  uint8_t m_current_config;

  bool check_is_done() const;

  void set_config();

  static const uint16_t DefaultFrameTimeMs;
  static const uint8_t NumberOfConfigs;
};