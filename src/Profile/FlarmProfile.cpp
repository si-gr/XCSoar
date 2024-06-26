// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmProfile.hpp"
#include "Map.hpp"
#include "FLARM/ColorDatabase.hpp"

#include <string>

#include <string.h>

static void
LoadColor(const ProfileMap &map,FlarmColorDatabase &db,
          const char *key, FlarmColor color)
{
  const char *ids = map.Get(key);
  if (ids == nullptr)
    return;

  const char *p = ids;
  while (true) {
    char *endptr;
    FlarmId id = FlarmId::Parse(p, &endptr);
    if (id.IsDefined())
      db.Set(id, color);

    p = strchr(endptr, ',');
    if (p == nullptr)
      break;

    ++p;
  }
}

void
Profile::Load(const ProfileMap &map, FlarmColorDatabase &db)
{
  LoadColor(map, db, "FriendsGreen", FlarmColor::GREEN);
  LoadColor(map, db, "FriendsBlue", FlarmColor::BLUE);
  LoadColor(map, db, "FriendsYellow", FlarmColor::YELLOW);
  LoadColor(map, db, "FriendsMagenta", FlarmColor::MAGENTA);
}

void
Profile::Save(ProfileMap &map, const FlarmColorDatabase &db)
{
  std::string ids[4];

  for (const auto &i : db) {
    assert(i.first.IsDefined());
    assert((int)i.second < (int)FlarmColor::COUNT);

    if (i.second == FlarmColor::NONE)
      continue;

    unsigned color_index = (int)i.second - 1;

    char id_buffer[16];
    ids[color_index] += i.first.Format(id_buffer);
    ids[color_index] += ',';
  }

  map.Set("FriendsGreen", ids[0].c_str());
  map.Set("FriendsBlue", ids[1].c_str());
  map.Set("FriendsYellow", ids[2].c_str());
  map.Set("FriendsMagenta", ids[3].c_str());
}
