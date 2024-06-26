// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include <cassert>

/**
 * Simple distance statistics with derived values (speed, incremental speed)
 * Incremental speeds track the short-term variation of distance with time,
 * whereas the overall speed is defined by the distance divided by a time value.
 */
class DistanceStat
{
  friend class DistanceStatComputer;
  friend class IncrementalSpeedComputer;

protected:
  /** Distance (m) of metric */
  double distance;
  /** Speed (m/s) of metric */
  double speed;
  /** Incremental speed (m/s) of metric */
  double speed_incremental;

public:
  void Reset() {
    distance = -1;
    speed = 0;
    speed_incremental = 0;
  }

  bool IsDefined() const {
    return distance >= 0;
  }

  /**
   * Setter for distance value
   *
   * @param d Distance value (m)
   */
  void SetDistance(const double d) {
    distance = d;
  }

  /**
   * Accessor for distance value
   *
   * @return Distance value (m)
   */
  double GetDistance() const {
    assert(IsDefined());

    return distance;
  }

  /**
   * Accessor for speed
   *
   * @return Speed (m/s)
   */
  double GetSpeed() const {
    assert(IsDefined());

    return speed;
  }

  /**
   * Accessor for incremental speed (rate of change of
   * distance over dt, low-pass filtered)
   *
   * @return Speed incremental (m/s)
   */
  double GetSpeedIncremental() const {
    assert(IsDefined());

    return speed_incremental;
  }
};
