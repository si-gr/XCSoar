// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Icon.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/TextInBox.hpp"
#ifdef HAVE_SKYLINES_TRACKING
#include "Tracking/SkyLines/Data.hpp"
#endif
#ifdef HAVE_HTTP
#include "net/client/tim/Glue.hpp"
#include "net/client/tim/Thermal.hpp"
#endif

template<typename T>
static void
DrawThermalSources(Canvas &canvas, const MaskedIcon &icon,
                   const WindowProjection &projection,
                   const T &sources,
                   const double aircraft_altitude,
                   const SpeedVector &wind) noexcept
{
  for (const auto &source : sources) {
    // find height difference
    if (aircraft_altitude < source.ground_height)
      continue;

    // draw thermal at location it would be at the glider's height
    GeoPoint location = wind.IsNonZero()
      ? source.CalculateAdjustedLocation(aircraft_altitude, wind)
      : source.location;

    // draw if it is in the field of view
    if (auto p = projection.GeoToScreenIfVisible(location))
      icon.Draw(canvas, *p);
  }
}

void
MapWindow::DrawThermalEstimate(Canvas &canvas) const noexcept
{
  const MoreData &basic = Basic();
  const DerivedInfo &calculated = Calculated();
  const ThermalLocatorInfo &thermal_locator = calculated.thermal_locator;

  if (render_projection.GetMapScale() > 10000)
    return;

  // draw only at close map scales in non-circling mode

  DrawThermalSources(canvas, look.thermal_source_icon, render_projection,
                     thermal_locator.sources, basic.nav_altitude,
                     calculated.wind_available
                     ? calculated.wind : SpeedVector::Zero());

#ifdef HAVE_SKYLINES_TRACKING
  const auto &cloud_settings = GetComputerSettings().tracking.skylines.cloud;
  if (cloud_settings.show_thermals && skylines_data != nullptr) {
    const std::lock_guard lock{skylines_data->mutex};
    for (auto &i : skylines_data->thermals) {
      // TODO: apply wind drift
      if (auto p = render_projection.GeoToScreenIfVisible(i.bottom_location))
        look.thermal_source_icon.Draw(canvas, *p);
    }
  }
#endif

#ifdef HAVE_HTTP
  if (tim_glue != nullptr && GetComputerSettings().weather.enable_tim) {
    const uint8_t color_wheel[10][3] = {
      {0, 0, 255},
      {0, 113, 255},
      {0, 227, 255},
      {0, 255, 170},
      {0, 255, 57},
      {57, 255, 0},
      {170, 255, 0},
      {255, 227, 0},
      {255, 113, 0},
      {255, 0, 0}};

    const auto lock = tim_glue->Lock();
    for (const auto &i : tim_glue->Get()) {
      if (auto p = render_projection.GeoToScreenIfVisible(i.location)) {
        const auto diff = std::chrono::system_clock::now() - i.time;
        auto color_i = std::min(std::max(int(i.climb_rate / 0.5f), 0), 9);
        auto radius = std::max(10 - int(std::chrono::round<std::chrono::seconds>(diff).count() / 200), 1);

        canvas.SelectHollowBrush();
        canvas.Select(Pen(Layout::Scale(2),
                          Color(color_wheel[color_i][0], color_wheel[color_i][1], color_wheel[color_i][2])));
        canvas.DrawCircle(*p, Layout::Scale(radius));

        char thermal_text[32];
        StringFormat(thermal_text, sizeof(thermal_text), "%.1f", i.climb_rate);
        PixelPoint text_point = *p;
        text_point.y -= Layout::Scale(5);
        text_point.x -= Layout::Scale(5);
        TextInBoxMode mode;
        TextInBox(canvas, thermal_text, text_point, mode, GetClientRect());
      }
    }
  }
#endif
}
