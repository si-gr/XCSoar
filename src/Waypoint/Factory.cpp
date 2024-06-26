// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Factory.hpp"
#include "Terrain/RasterTerrain.hpp"

bool
WaypointFactory::FallbackElevation(Waypoint &waypoint) const
{
  if (terrain != nullptr) {
    // Load waypoint altitude from terrain
    const auto h = terrain->GetTerrainHeight(waypoint.location);
    if (!h.IsSpecial()) {
      waypoint.elevation = h.GetValue();
      return true;
    }
  }

  return false;
}
