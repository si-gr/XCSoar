// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Markers.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "LogFile.hpp"
#include "LocalPath.hpp"

void
MarkLocation(const GeoPoint &loc, const BrokenDateTime &time)
try {
  assert(time.IsPlausible());

  FileOutputStream file(LocalPath(_T("xcsoar-marks.txt")),
                        FileOutputStream::Mode::APPEND_OR_CREATE);
  BufferedOutputStream os(file);
  os.Format("%02u.%02u.%04u\t%02u:%02u:%02u\tLon:%f\tLat:%f\n",
            time.day, time.month, time.year,
            time.hour, time.minute, time.second,
            (double)loc.longitude.Degrees(),
            (double)loc.latitude.Degrees());
  os.Flush();
  file.Commit();
} catch (...) {
  LogError(std::current_exception());
}
