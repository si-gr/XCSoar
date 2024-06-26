// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"
#include "util/UTF8.hpp"
#include "util/TruncateString.hpp"
#include "util/Macros.hpp"

#ifdef _UNICODE
#include "util/Macros.hpp"

#include <stringapiset.h>
#endif

bool
ProfileMap::Get(const char *key, TCHAR *value, size_t max_size) const noexcept
{
  const char *src = Get(key);
  if (src == nullptr) {
    value[0] = _T('\0');
    return false;
  }

#ifdef _UNICODE
  int result = MultiByteToWideChar(CP_UTF8, 0, src, -1,
                                   value, max_size);
  return result > 0;
#else
  if (!ValidateUTF8(src))
    return false;

  CopyTruncateString(value, max_size, src);
  return true;
#endif
}

#ifdef _UNICODE

void
ProfileMap::Set(const char *key, const TCHAR *value) noexcept
{
  char buffer[MAX_PATH];
  int length = WideCharToMultiByte(CP_UTF8, 0, value, -1,
                                   buffer, ARRAY_SIZE(buffer),
                                   nullptr, nullptr);
  if (length <= 0)
    return;

  Set(key, buffer);
}

#endif
