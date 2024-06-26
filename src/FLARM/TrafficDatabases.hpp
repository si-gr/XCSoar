// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ColorDatabase.hpp"
#include "NameDatabase.hpp"
#include "FlarmNetDatabase.hpp"

struct TrafficDatabases {
  FlarmColorDatabase flarm_colors;

  FlarmNameDatabase flarm_names;

  FlarmNetDatabase flarm_net;

  /**
   * FlarmId of the glider to track.  Check FlarmId::IsDefined()
   * before using this attribute.
   *
   * This is a copy of TeamCodeSettings::team_flarm_id.
   */
  FlarmId team_flarm_id;

  TrafficDatabases()
    :team_flarm_id(FlarmId::Undefined()) {}

  [[gnu::pure]]
  FlarmColor GetColor(FlarmId id) const
  {
    FlarmColor color = flarm_colors.Get(id);
    if (color == FlarmColor::NONE && id == team_flarm_id)
      /* if no color found but target is teammate, use green */
      color = FlarmColor::GREEN;

    return color;
  }

  [[gnu::pure]]
  const TCHAR *FindNameById(FlarmId id) const;

  [[gnu::pure]] gcc_nonnull_all
  FlarmId FindIdByName(const TCHAR *name) const;

  /**
   * Look up all records with the specified name.
   *
   * @param max the maximum size of the given buffer
   * @return the number of items copied to the given buffer
   */
  gcc_nonnull_all
  unsigned FindIdsByName(const TCHAR *name,
                         FlarmId *buffer, unsigned max) const;
};
