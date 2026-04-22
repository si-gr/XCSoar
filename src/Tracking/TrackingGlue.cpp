// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingGlue.hpp"
#include <chrono>
#include "Tracking/TrackingSettings.hpp"
#include "NMEA/MoreData.hpp"
#include "LogFile.hpp"
#include "util/Macros.hpp"
#include "Interface.hpp"
#include "util/StringCompare.hxx"

static void
UpdateJETProviderTracking(JETProvider::Data *jet_provider_data,
                          const JETProviderSettings &settings) noexcept
{
  if (jet_provider_data == nullptr || jet_provider_data->traffics.empty()) {
    return;
  }

  auto now = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  const std::lock_guard lock{jet_provider_data->historic_circling_mutex};
  
  for (auto &traffic : jet_provider_data->traffics) {
    if (traffic->name == nullptr || StringIsEmpty(traffic->name))
      continue;

    std::string traffic_name{traffic->name};

    if (traffic->is_circling) {
      JETProvider::Data::Traffic new_historic;
      new_historic.name = traffic->name;
      new_historic.location = traffic->location;
      new_historic.epoch = traffic->epoch;
      new_historic.vspeed = traffic->vspeed;
      new_historic.track = traffic->track;
      new_historic.altitude = traffic->altitude;
      new_historic.speed = traffic->speed;
      new_historic.type = traffic->type;
      new_historic.is_circling = true;
      std::string historic_key = traffic_name + "_" + std::to_string(traffic->epoch);
      jet_provider_data->historic_circling_traffic.try_emplace(historic_key, new_historic);
      //LogFormat("Added historic circling traffic %s epoch %u", traffic_name.c_str(), traffic->epoch);
    }
  }

  const unsigned historic_traffic_age_seconds = settings.radar.historic_traffic_age_seconds;
  
  for (auto it = jet_provider_data->historic_circling_traffic.begin(); 
       it != jet_provider_data->historic_circling_traffic.end();) {
    auto age = now - static_cast<int64_t>(it->second.epoch);
    if (age > historic_traffic_age_seconds) {
      it = jet_provider_data->historic_circling_traffic.erase(it);
    } else {
      ++it;
    }
  }
}

TrackingGlue::TrackingGlue(EventLoop &event_loop,
                           CurlGlobal &curl) noexcept
  :skylines(event_loop, this),
   livetrack24(curl),
   jet_provider(curl, this)
{
}

void
TrackingGlue::SetSettings(const TrackingSettings &_settings)
{
  skylines.SetSettings(_settings.skylines);
  livetrack24.SetSettings(_settings.livetrack24);
  settings = _settings;
}

void
TrackingGlue::OnTimer(const MoreData &basic, const DerivedInfo &calculated)
{
  try {
    skylines.Tick(basic, calculated);
  } catch (...) {
    LogError(std::current_exception(), "SkyLines error");
  }

  jet_provider.OnTimer(basic, calculated);

  livetrack24.OnTimer(basic, calculated);
}

void
TrackingGlue::OnTraffic(uint32_t pilot_id, unsigned time_of_day_ms,
                        const GeoPoint &location, int altitude)
{
  bool user_known;

  {
    const std::lock_guard lock{skylines_data.mutex};
    const SkyLinesTracking::Data::Traffic traffic(SkyLinesTracking::Data::Time{time_of_day_ms},
                                                  location, altitude);
    skylines_data.traffic[pilot_id] = traffic;

    user_known = skylines_data.IsUserKnown(pilot_id);
  }

  if (!user_known)
    /* we don't know this user's name yet - try to find it out by
       asking the server */
    skylines.RequestUserName(pilot_id);
}

void TrackingGlue::OnJETTraffic(std::vector<std::unique_ptr<JETProvider::Data::Traffic>> &traffics, bool success)
{
  // Protect shared state with mutex
  const std::lock_guard lock{jet_provider_data.mutex};

  // 1) Remove old traffic items older than 10 minutes (epoch < now - 600)
  auto now = std::chrono::system_clock::now();
  const uint32_t now_s = static_cast<uint32_t>(
    std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());

  for (auto it = jet_provider_data.traffics.begin(); it != jet_provider_data.traffics.end();) {
    if ((*it)->epoch < now_s - 600) {
      it = jet_provider_data.traffics.erase(it);
    } else {
      ++it;
    }
  }

  // 2) If we have new traffics, append them to the remaining list
  if (success) {
    for (auto&& traffic : traffics) {
      for (auto it = jet_provider_data.traffics.begin(); it != jet_provider_data.traffics.end();) {
        // compare name strings
        if ((*it)->name && traffic->name && strcmp((*it)->name, traffic->name) == 0) {
          it = jet_provider_data.traffics.erase(it);
        } else {
          ++it;
        }
      }
      //LogFormat("OnJETTraffic name of traffic to add: %s ", traffic->name); 
      jet_provider_data.traffics.push_back(std::move(traffic));
    }
    
  }

  jet_provider_data.success = success;
  jet_provider_data.last_traffic_count = static_cast<uint32_t>(jet_provider_data.traffics.size());

  UpdateJETProviderTracking(&jet_provider_data, settings.jet_provider_setting);

  //LogFormat("OnJETTraffic size:%d success:%d replaced counter:%d", (int) jet_provider_data.traffics.size(), success, removeCounter);
}

void
TrackingGlue::OnUserName(uint32_t user_id, const char *name)
{
  const std::lock_guard lock{skylines_data.mutex};
  skylines_data.user_names[user_id] = name;
}

void
TrackingGlue::OnWave(unsigned time_of_day_ms,
                     const GeoPoint &a, const GeoPoint &b)
{
  const std::lock_guard lock{skylines_data.mutex};

  /* garbage collection - hard-coded upper limit */
  auto n = skylines_data.waves.size();
  while (n-- >= 64)
    skylines_data.waves.pop_front();

  // TODO: replace existing item?
  skylines_data.waves.emplace_back(SkyLinesTracking::Data::Time{time_of_day_ms},
                                   a, b);
}

void
TrackingGlue::OnThermal([[maybe_unused]] unsigned time_of_day_ms,
                        const AGeoPoint &bottom, const AGeoPoint &top,
                        double lift)
{
  const std::lock_guard lock{skylines_data.mutex};

  /* garbage collection - hard-coded upper limit */
  auto n = skylines_data.thermals.size();
  while (n-- >= 64)
    skylines_data.thermals.pop_front();

  // TODO: replace existing item?
  skylines_data.thermals.emplace_back(bottom, top, lift);
}

void
TrackingGlue::OnSkyLinesError(std::exception_ptr e)
{
  LogError(e, "SkyLines error");
}

void
TrackingGlue::OnJETProviderError(std::exception_ptr e)
{
  LogError(e, "JETProvider error");
}
