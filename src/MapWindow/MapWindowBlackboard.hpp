// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BaseBlackboard.hpp"
#include "Blackboard/ComputerSettingsBlackboard.hpp"
#include "Blackboard/MapSettingsBlackboard.hpp"
#include "thread/Debug.hpp"
#include "UIState.hpp"
#include "Tracking/JETProvider/JETProvider.hpp"

#include <map>
#include <string>

/**
 * Blackboard used by map window: provides read-only access to local
 * copies of data required by map window
 * 
 */
class MapWindowBlackboard:
  public BaseBlackboard,
  public ComputerSettingsBlackboard,
  public MapSettingsBlackboard
{
  UIState ui_state;

  /**
   * FLARM traffic that has disappeared, but will remain on the map
   * (greyed out) for some time.
   */
  std::map<FlarmId, FlarmTraffic> fading_flarm_traffic;

  /**
   * FLARM traffic at climb position - remains visible for 10 minutes
   * at the position where climb was detected.
   */
  std::map<FlarmId, FlarmTraffic> climb_position_traffic;

  /**
   * JETProvider traffic tracking for movement and climb position.
   * Uses traffic name as key.
   */
  std::map<std::string, JETProvider::Data::Traffic> jet_provider_traffic_1min_ago;

  /**
   * JETProvider traffic at climb position - remains visible for 10 minutes
   * at the position where climb was detected.
   */
  std::map<std::string, JETProvider::Data::Traffic> jet_provider_climb_position_traffic;

  /**
   * JETProvider historic circling positions - remains visible for 5 minutes
   * at the position where circling was detected, rendered at 25% size.
   */
  std::map<std::string, JETProvider::Data::Traffic> jet_provider_historic_circling_traffic;

protected:
  MapWindowBlackboard() noexcept {
    /* this needs to be initialised because ReadBlackboard() uses the
       previous FLARM traffic list */
    gps_info.Reset();
  }

  [[gnu::const]]
  const MoreData &Basic() const noexcept {
    assert(InDrawThread());

    return BaseBlackboard::Basic();
  }

  [[gnu::const]]
  const DerivedInfo &Calculated() const noexcept {
    assert(InDrawThread());

    return BaseBlackboard::Calculated();
  }

  [[gnu::const]]
  const auto &GetFadingFlarmTraffic() const noexcept {
    return fading_flarm_traffic;
  }

  [[gnu::const]]
  const auto &GetClimbPositionTraffic() const noexcept {
    return climb_position_traffic;
  }

  [[gnu::const]]
  const auto &GetJETProviderTraffic1MinAgo() const noexcept {
    return jet_provider_traffic_1min_ago;
  }

  auto &GetJETProviderTraffic1MinAgoForUpdate() noexcept {
    return jet_provider_traffic_1min_ago;
  }

  [[gnu::const]]
  const auto &GetJETProviderHistoricCirclingTraffic() const noexcept {
    return jet_provider_historic_circling_traffic;
  }

  auto &GetJETProviderHistoricCirclingTrafficForUpdate() noexcept {
    return jet_provider_historic_circling_traffic;
  }

  void UpdateJETProviderTracking(const JETProvider::Data *jet_provider_data) noexcept;

  [[gnu::const]]
  const ComputerSettings &GetComputerSettings() const noexcept {
    assert(InDrawThread());

    return ComputerSettingsBlackboard::GetComputerSettings();
  }

  [[gnu::const]]
  const MapSettings &GetMapSettings() const noexcept {
    assert(InDrawThread());

    return settings_map;
  }

  [[gnu::const]]
  const UIState &GetUIState() const noexcept {
    assert(InDrawThread());

    return ui_state;
  }

  void ReadBlackboard(const MoreData &nmea_info,
                      const DerivedInfo &derived_info) noexcept;
  void ReadComputerSettings(const ComputerSettings &settings) noexcept;
  void ReadMapSettings(const MapSettings &settings) noexcept;

  void ReadUIState(const UIState &new_value) noexcept {
    ui_state = new_value;
  }
};
