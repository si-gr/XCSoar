// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonLabel.hpp"
#include "MenuBar.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"
#include "util/StringBuilder.hxx"
#include "util/StringStrip.hxx"
#include "util/CharUtil.hxx"
#include "util/Macros.hpp"

#include <algorithm>

/**
 * @return false if there is at least one ASCII letter in the string
 */
[[gnu::pure]]
static bool
LacksAlphaASCII(const TCHAR *s)
{
  for (; *s != 0; ++s)
    if (IsAlphaASCII(*s))
      return false;

  return true;
}

/**
 * Translate a portion of the source string.
 *
 * @return the translated string or nullptr if the buffer is too small
 */
[[gnu::pure]]
static const TCHAR *
GetTextN(const TCHAR *src, const TCHAR *src_end,
         TCHAR *buffer, size_t buffer_size)
{
  if (src == src_end)
    /* gettext("") returns the PO header, and thus we need to exclude
       this special case */
    return _T("");

  const size_t src_length = src_end - src;
  if (src_length >= buffer_size)
    /* buffer too small */
    return nullptr;

  /* copy to buffer, because gettext() expects a null-terminated
     string */
  *std::copy(src, src_end, buffer) = _T('\0');

  return gettext(buffer);
}

ButtonLabel::Expanded
ButtonLabel::Expand(const TCHAR *text, TCHAR *buffer, size_t size)
{
  Expanded expanded;
  const TCHAR *dollar;

  if (text == nullptr || *text == _T('\0') || *text == _T(' ')) {
    expanded.visible = false;
    return expanded;
  } else if ((dollar = StringFind(text, '$')) == nullptr) {
    /* no macro, we can just translate the text */
    expanded.visible = true;
    expanded.enabled = true;
    const TCHAR *nl = StringFind(text, '\n');
    if (nl != nullptr && LacksAlphaASCII(nl + 1)) {
      /* Quick hack for skipping the translation for second line of a two line
         label with only digits and punctuation in the second line, e.g.
         for menu labels like "Config\n2/3" */

      /* copy the text up to the '\n' to a new buffer and translate it */
      TCHAR translatable[256];
      const TCHAR *translated = GetTextN(text, nl, translatable,
                                         ARRAY_SIZE(translatable));
      if (translated == nullptr) {
        /* buffer too small: keep it untranslated */
        expanded.text = text;
        return expanded;
      }

      /* concatenate the translated text and the part starting with '\n' */
      try {
        expanded.text = BuildString(buffer, size, translated, nl);
      } catch (BasicStringBuilder<TCHAR>::Overflow) {
        expanded.text = gettext(text);
      }
    } else
      expanded.text = gettext(text);
    return expanded;
  } else {
    const TCHAR *macros = dollar;
    /* backtrack until the first non-whitespace character, because we
       don't want to translate whitespace between the text and the
       macro */
    macros = StripRight(text, macros);

    TCHAR s[100];
    expanded.enabled = !ExpandMacros(text, s, ARRAY_SIZE(s));
    if (s[0] == _T('\0') || s[0] == _T(' ')) {
      expanded.visible = false;
      return expanded;
    }

    /* copy the text (without trailing whitespace) to a new buffer and
       translate it */
    TCHAR translatable[256];
    const TCHAR *translated = GetTextN(text, macros, translatable,
                                       ARRAY_SIZE(translatable));
    if (translated == nullptr) {
      /* buffer too small: fail */
      // TODO: find a more clever fallback
      expanded.visible = false;
      return expanded;
    }

    /* concatenate the translated text and the macro output */
    expanded.visible = true;
    expanded.text = BuildString(buffer, size, translated, s + (macros - text));
    return expanded;
  }
}
