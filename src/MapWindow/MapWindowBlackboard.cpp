// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindowBlackboard.hpp"
#include "FLARM/Friends.hpp"
#include "Tracking/JETProvider/JETProvider.hpp"
#include "util/StringCompare.hxx"
#include "LogFile.hpp"

void
MapWindowBlackboard::ReadComputerSettings(const ComputerSettings &settings) noexcept
{
  computer_settings = settings;
}

void
MapWindowBlackboard::ReadMapSettings(const MapSettings &settings) noexcept
{
  settings_map = settings;
}

[[gnu::pure]]
static bool
IsFriend(FlarmId id) noexcept
{
  return FlarmFriends::GetFriendColor(id) != FlarmColor::NONE;
}

static void
UpdateFadingTraffic(bool fade_traffic,
                    std::map<FlarmId, FlarmTraffic> &dest,
                    const TrafficList &old_list, const TrafficList &new_list,
                    TimeStamp now) noexcept
{
  if (!fade_traffic) {
    dest.clear();
    return;
  }

  if (new_list.modified.Modified(old_list.modified)||true) {
    /* first add all items from the old list */
    for (const auto &traffic : old_list.list)
      if (traffic.location_available)
        dest.try_emplace(traffic.id, traffic);

    /* now remove all items that are in the new list; now only items
       remain that have disappeared */
    for (const auto &traffic : new_list.list)
      if (auto i = dest.find(traffic.id); i != dest.end())
        dest.erase(i);
  }

  /* remove all items that havn't been seen again for too long */
  std::erase_if(dest, [now](const auto &i){
    /* friends expire after 10 minutes, all others after one minute */
    const auto max_age = IsFriend(i.first)
      ? std::chrono::minutes{10}
      : std::chrono::minutes{1};

    return i.second.valid.IsOlderThan(now, max_age);
  });
}

static void
UpdateClimbPositionTraffic(bool fade_traffic,
                           std::map<FlarmId, FlarmTraffic> &dest,
                           const TrafficList &old_list,
                           const TrafficList &new_list,
                           TimeStamp now) noexcept
{
  if (!fade_traffic) {
    dest.clear();
    return;
  }

  constexpr auto ONE_MINUTE = std::chrono::minutes{1};

  for (const auto &new_traffic : new_list.list) {
    if (!new_traffic.location_available)
      continue;

    const FlarmTraffic *old_traffic = old_list.FindTraffic(new_traffic.id);

    GeoPoint position_1min_ago;
    bool have_position_1min = false;

    if (old_traffic != nullptr && old_traffic->location_available && old_traffic->valid.IsValid()) {
      if (old_traffic->valid.IsOlderThan(now, ONE_MINUTE)) {
        position_1min_ago = old_traffic->location;
        have_position_1min = true;
      } else if (old_traffic->location_1min_ago_available) {
        position_1min_ago = old_traffic->location_1min_ago;
        have_position_1min = true;
      }
    }

    bool is_stationary = false;
    if (have_position_1min) {
      auto distance = new_traffic.location.Distance(position_1min_ago);
      is_stationary = distance < 200;
    }

    if (is_stationary && new_traffic.climb_rate_avg2min > 0) {
      if (auto it = dest.find(new_traffic.id); it != dest.end()) {
        it->second.location = new_traffic.location;
        it->second.valid = new_traffic.valid;
      } else {
        FlarmTraffic t = new_traffic;
        t.climb_position = new_traffic.location;
        t.climb_position_time = now;
        t.climb_position_valid = true;
        dest.try_emplace(new_traffic.id, t);
      }
    } else if (auto it = dest.find(new_traffic.id); it != dest.end()) {
      it->second.location = new_traffic.location;
      it->second.valid = new_traffic.valid;
    }
  }

  std::erase_if(dest, [now](const auto &i){
    return i.second.valid.IsOlderThan(now, std::chrono::minutes{10});
  });
}

void
MapWindowBlackboard::ReadBlackboard(const MoreData &nmea_info,
				    const DerivedInfo &derived_info) noexcept
{
  UpdateFadingTraffic(settings_map.fade_traffic,
                      fading_flarm_traffic, gps_info.flarm.traffic,
                      nmea_info.flarm.traffic,
                      nmea_info.clock);

  UpdateClimbPositionTraffic(settings_map.fade_traffic,
                             climb_position_traffic,
                             gps_info.flarm.traffic,
                             nmea_info.flarm.traffic,
                             nmea_info.clock);

  gps_info = nmea_info;
  calculated_info = derived_info;
}

