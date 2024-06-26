// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskOptTarget.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"
#include "util/Clamp.hpp"

double
TaskOptTarget::f(const double p) noexcept
{
  // set task targets
  SetTarget(p);

  res = tm.glide_solution(aircraft);

  return res.time_elapsed.count();
}

bool
TaskOptTarget::valid(const double tp)
{
  f(tp);
  return res.IsOk();
}

double
TaskOptTarget::search(const double tp)
{
  if (tp_current.IsTargetLocked()) {
    // can't move, don't bother
    return -1;
  }
  if (iso.IsValid()) {
    tm.target_save();
    const auto t = find_min(tp);
    if (!valid(t)) {
      // invalid, so restore old value
      tm.target_restore();
      return -1;
    } else {
      return t;
    }
  } else {
    return -1;
  }
}

void
TaskOptTarget::SetTarget(const double p)
{
  const GeoPoint loc = iso.Parametric(Clamp(p, xmin, xmax));
  tp_current.SetTarget(loc);
  tp_start.ScanDistanceRemaining(aircraft.location);
}
