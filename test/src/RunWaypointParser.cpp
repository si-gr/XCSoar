// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Waypoint/WaypointReader.hpp"
#include "Waypoint/Factory.hpp"
#include "Waypoint/Waypoints.hpp"
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <tchar.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH\n");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  Waypoints way_points;

  ConsoleOperationEnvironment operation;
  ReadWaypointFile(path, way_points,
                   WaypointFactory(WaypointOrigin::NONE),
                   operation);

  way_points.Optimise();
  printf("Size %d\n", way_points.size());

  way_points.VisitNamePrefix(_T(""), [](const auto &p){
    const auto &wp = *p;
    _ftprintf(stdout, _T("%s, %f, %f, %.0fm\n"), wp.name.c_str(),
              (double)wp.location.latitude.Degrees(),
              (double)wp.location.longitude.Degrees(),
              (double)wp.elevation);
  });

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
