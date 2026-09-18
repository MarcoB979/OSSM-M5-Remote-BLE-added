// language.cpp - language tables, runtime selection and NVS persistence.
//
// English is the default language and lives in languages/defaultlanguage.h.
// Additional languages are separate headers (languages/xx.h) whose tables are
// built here and registered in languageTables[]. Entries left as NULL in a
// language table automatically fall back to English.
//
// ADDING A LANGUAGE:
//   1. Create languages/xx.h with #define LANG_XX(X) ... (same keys, same
//      order as defaultlanguage.h; NULL for untranslated strings).
//   2. #include it below and build its table with a LANG_XX_ENTRY macro.
//   3. Add the table pointer to languageTables[].
//   4. Extend LanguageId in language.h and add a Settings entry that calls
//      languageSet(lang) + languageStore(lang).

#include "language.h"
#include <Preferences.h>

// --- English / fallback table (built from the master list) ---
#define LANG_EN_ENTRY(key, text) text,
static const char* const languageEn[Tk_COUNT] = {
    LANG_STRINGS(LANG_EN_ENTRY)
};
#undef LANG_EN_ENTRY

static_assert(sizeof(languageEn) / sizeof(languageEn[0]) == Tk_COUNT,
              "languageEn[] is out of sync with TextId");

// --- Additional languages (add tables here) ---
// #include "languages/de.h"
// #define LANG_DE_ENTRY(key, text) text,
// static const char* const languageDe[Tk_COUNT] = { LANG_DE(LANG_DE_ENTRY) };
// #undef LANG_DE_ENTRY

// --- Registry: index = LanguageId ---
static const char* const* const languageTables[LANGUAGE_COUNT] = {
    languageEn,
    // languageDe,
};

static LanguageId g_language = LANGUAGE_EN;

extern "C" {

const char* languageGet(TextId id) {
    if (id >= Tk_COUNT) return "";
    const char* text = languageTables[g_language][id];
    return (text != nullptr) ? text : languageEn[id];  // English fallback
}

LanguageId languageGetCurrent(void) {
    return g_language;
}

void languageSet(LanguageId lang) {
    if (lang < LANGUAGE_COUNT) g_language = lang;
}

void languageInit(void) {
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    int stored = prefs.getInt("Language", (int)LANGUAGE_EN);
    prefs.end();
    languageSet((LanguageId)stored);
}

void languageStore(LanguageId lang) {
    if (lang >= LANGUAGE_COUNT) return;
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    prefs.putInt("Language", (int)lang);
    prefs.end();
}

} // extern "C"
