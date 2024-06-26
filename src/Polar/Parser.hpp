// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

struct PolarShape;
struct PolarInfo;

bool
ParsePolarShape(PolarShape &shape, const char *s);

bool
ParsePolar(PolarInfo &polar, const char *s);

void
FormatPolarShape(const PolarShape &shape, char *buffer, size_t max_size);

void
FormatPolar(const PolarInfo &polar, char *buffer, size_t max_size,
              bool include_v_no=false);
