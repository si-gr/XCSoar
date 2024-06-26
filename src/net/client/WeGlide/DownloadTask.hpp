// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/Task.hxx"

#include <memory>

class OrderedTask;
class CurlGlobal;
struct WeGlideSettings;
struct TaskBehaviour;
class Waypoints;
class ProgressListener;

namespace WeGlide {

/**
 * Download the task declared in WeGlide.  Returns nullptr if no task
 * is declared and throws on error.
 */
Co::Task<std::unique_ptr<OrderedTask>>
DownloadDeclaredTask(CurlGlobal &curl, const WeGlideSettings &settings,
                     const TaskBehaviour &task_behaviour,
                     const Waypoints *waypoints,
                     ProgressListener &progress);

} // namespace WeGlide
