// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class TaskPoint;
struct AircraftState;

/**
 * Class used to control advancement through an OrderedTask
 */
class TaskAdvance
{
protected:
  /** arm state */
  bool armed = false;

  /** need to arm */
  bool request_armed = false;

public:

  /**
   * Enumeration of states the task advance mechanism can be in
   */
  enum State {
    /** Advance is manual (user must manually adjust) */
    MANUAL = 0,
    /** Advance is auto (no user input required) */
    AUTO,
    /** Armed for start */
    START_ARMED,
    /** Not yet armed for start (user must arm when ready) */
    START_DISARMED,
    /** Armed for a turn */
    TURN_ARMED,
    /** Not yet armed for turn (user must arm when ready) */
    TURN_DISARMED
  };

  /**
   * Resets as if never flown
   */
  virtual void Reset();

  /**
   * Set arming trigger
   *
   * @param do_armed True to arm trigger, false to clear
   */
  void SetArmed(const bool do_armed);

  /**
   * Accessor for arm state
   *
   * @return True if armed
   */
  bool IsArmed() const {
    return armed;
  }

  /**
   * Accessor for arm request state
   *
   * @return True if arm requested
   */
  bool NeedToArm() const  {
    return request_armed;
  }

  /**
   * Toggle arm state
   *
   * @return Arm state after toggle
   */
  bool ToggleArmed();

  /** 
   * Retrieve current advance state
   * 
   * @return Advance state
   */
  virtual State GetState() const = 0;

  /**
   * Determine whether all conditions are satisfied for a turnpoint
   * to auto-advance based on condition of the turnpoint, transition
   * characteristics and advance mode.
   *
   * @param tp The task point to check for satisfaction
   * @param state current aircraft state
   * @param x_enter whether this step transitioned enter to this tp
   * @param x_exit whether this step transitioned exit to this tp
   *
   * @return true if this tp is ready to advance
   */
  virtual bool CheckReadyToAdvance(const TaskPoint &tp,
                                   const AircraftState &state,
                                   const bool x_enter, const bool x_exit) = 0;

protected:
  /** 
   * Update state after external change to the arm state
   */
  virtual void UpdateState() = 0;

  /**
   * Determine whether, according to OZ entry, an AAT OZ is ready to advance
   *
   * @param has_entered True if the aircraft has entered the OZ
   * @param close_to_target If true, will advance when aircraft is close to target
   *
   * @return True if ready to advance
   */
  virtual bool IsAATStateReady(const bool has_entered,
                               const bool close_to_target) const;

  /**
   * Determine whether state is satisfied for a turnpoint
   *
   * @param tp The task point to check for satisfaction
   * @param state current aircraft state
   * @param x_enter whether this step transitioned enter to this tp
   * @param x_exit whether this step transitioned exit to this tp
   *
   * @return true if this tp is ready to advance
   */
  [[gnu::pure]]
  bool IsStateReady(const TaskPoint &tp,
                    const AircraftState &state,
                    const bool x_enter,
                    const bool x_exit) const;
};
