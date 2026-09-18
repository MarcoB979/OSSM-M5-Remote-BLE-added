# languages/

Per-language UI strings for the M5 Remote.

## Files

- `defaultlanguage.h` — the **master list**. Every `(key, English)` pair lives here
  exactly once, as `LANG_STRINGS(X)`. This single list drives the `TextId` enum
  and is the English/fallback table.
- `xx.h` (e.g. `de.h`) — an additional language. Defines `LANG_XX(X)` with the
  **same keys in the same order**, using `NULL` for strings not yet translated
  (those fall back to English at runtime).

## Adding a language (e.g. German)

1. Create `de.h`:

   ```c
   #pragma once
   #define LANG_DE(X) \
       X(HEADER, "M5 Fernbedienung") \
       X(OK,     NULL)            /* not translated yet -> falls back to English */
   ```

   Keys must match `defaultlanguage.h` and stay in the same order.

2. In `../language.cpp`:
   - `#include "languages/de.h"`
   - build the table:

     ```c
     #define LANG_DE_ENTRY(key, text) text,
     static const char* const languageDe[Tk_COUNT] = { LANG_DE(LANG_DE_ENTRY) };
     #undef LANG_DE_ENTRY
     ```

   - register it in `languageTables[]`.

3. In `../language.h`: extend `LanguageId` with `LANGUAGE_DE`.

4. In `../screens/SettingsScreen.cpp`: add `{ LANGUAGE_DE, "Deutsch" }` to
   `s_selectable_languages` (the Settings → Language list).

## How switching works

The active language is an index selected at runtime by `languageSet()` and
persisted by `languageStore()` (NVS namespace `"m5-ctnr"`, key `"Language"`).
`languageInit()` loads it at boot, before the UI is built. No reflash is needed
to switch languages — only a restart to re-render the labels.
