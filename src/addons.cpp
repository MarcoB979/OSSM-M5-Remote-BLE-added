// addons.cpp — Addon binding management (Eject Cumpump, Fist-IT, etc.)
// Each addon can be assigned to the left or right double-click slot.

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>

#include "config.h"
#include "language.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "colors.h"


uint8_t CUM_Address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// ---------------------------------------------------------------------------
// Module-local state
// ---------------------------------------------------------------------------

static int  g_addon_slots[ADDON_DEFINITIONS_COUNT] = {ADDON_SLOT_NONE};
static bool g_addons_loaded_from_nvs = false;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const char* addonSlotLabel(int slot)
{
  switch (slot) {
    case ADDON_SLOT_LEFT:  return T_ADDONS_SLOT_LEFT;
    case ADDON_SLOT_RIGHT: return T_ADDONS_SLOT_RIGHT;
    default:               return T_ADDONS_SLOT_OFF;
  }
}

static void loadAddonBindingsIfNeeded()
{
  if (g_addons_loaded_from_nvs) {
    return;
  }

  Preferences pref;
  pref.begin("m5-ctnr", true);
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    char key[24];
    snprintf(key, sizeof(key), "AddonSlot%u", (unsigned)i);
    int slot = pref.getInt(key, ADDON_SLOT_NONE);
    if (slot != ADDON_SLOT_LEFT && slot != ADDON_SLOT_RIGHT) {
      slot = ADDON_SLOT_NONE;
    }
    g_addon_slots[i] = slot;
  }
  pref.end();

  // Enforce unique left/right bindings after loading potentially stale settings.
  int leftOwner  = -1;
  int rightOwner = -1;
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    if (g_addon_slots[i] == ADDON_SLOT_LEFT) {
      if (leftOwner >= 0) {
        g_addon_slots[i] = ADDON_SLOT_NONE;
      } else {
        leftOwner = (int)i;
      }
    } else if (g_addon_slots[i] == ADDON_SLOT_RIGHT) {
      if (rightOwner >= 0) {
        g_addon_slots[i] = ADDON_SLOT_NONE;
      } else {
        rightOwner = (int)i;
      }
    }
  }

  g_addons_loaded_from_nvs = true;
}

static void saveAddonBindings()
{
  Preferences pref;
  pref.begin("m5-ctnr", false);
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    char key[24];
    snprintf(key, sizeof(key), "AddonSlot%u", (unsigned)i);
    pref.putInt(key, g_addon_slots[i]);
  }
  pref.end();
}

static void refreshAddonsUi()
{
  if (ui_AddonsItem0Text != nullptr && ADDON_DEFINITIONS_COUNT > 0) {
    char line[96];
    snprintf(line, sizeof(line), "%s  [%s]",
             ADDON_DEFINITIONS[0].displayName, addonSlotLabel(g_addon_slots[0]));
    lv_label_set_text(ui_AddonsItem0Text, line);
  }
  if (ui_AddonsItem1Text != nullptr && ADDON_DEFINITIONS_COUNT > 1) {
    char line[96];
    snprintf(line, sizeof(line), "%s  [%s]",
             ADDON_DEFINITIONS[1].displayName, addonSlotLabel(g_addon_slots[1]));
    lv_label_set_text(ui_AddonsItem1Text, line);
  }
  if (ui_AddonsItem2Text != nullptr && ADDON_DEFINITIONS_COUNT > 2) {
    // Color Schemes is a direct launcher — show without slot info
    if (strcmp(ADDON_DEFINITIONS[2].id, "color_schemes") == 0) {
      lv_label_set_text(ui_AddonsItem2Text, ADDON_DEFINITIONS[2].displayName);
    } else {
      char line[96];
      snprintf(line, sizeof(line), "%s  [%s]",
               ADDON_DEFINITIONS[2].displayName, addonSlotLabel(g_addon_slots[2]));
      lv_label_set_text(ui_AddonsItem2Text, line);
    }
  }
}

static void cycleAddonSelection(size_t addonIndex)
{
  if (addonIndex >= ADDON_DEFINITIONS_COUNT) {
    return;
  }

  int current = g_addon_slots[addonIndex];
  int next = ADDON_SLOT_NONE;
  if (current == ADDON_SLOT_NONE) {
    next = ADDON_SLOT_LEFT;
  } else if (current == ADDON_SLOT_LEFT) {
    next = ADDON_SLOT_RIGHT;
  } else {
    next = ADDON_SLOT_NONE;
  }

  // Enforce uniqueness: clear the other addon bound to the same slot.
  if (next == ADDON_SLOT_LEFT || next == ADDON_SLOT_RIGHT) {
    for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
      if (i != addonIndex && g_addon_slots[i] == next) {
        g_addon_slots[i] = ADDON_SLOT_NONE;
      }
    }
  }

  g_addon_slots[addonIndex] = next;
  saveAddonBindings();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool triggerAddonForSlot(int slot)
{
  loadAddonBindingsIfNeeded();
  int addonIndex = -1;
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    if (g_addon_slots[i] == slot) {
      addonIndex = (int)i;
      break;
    }
  }

  if (addonIndex < 0) {
    return false;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "eject_cumpump") == 0) {
    _ui_screen_change(ui_EJECTSettings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "fist_it") == 0) {
    _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "color_schemes") == 0) {
    _ui_screen_change(ui_Colors, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  return false;
}

extern "C" void addonsScreenLoaded(void)
{
  loadAddonBindingsIfNeeded();
  refreshAddonsUi();
}

extern "C" void addonsSelectIndex(int index)
{
  loadAddonBindingsIfNeeded();
  if (index < 0 || (size_t)index >= ADDON_DEFINITIONS_COUNT) {
    return;
  }
  // Color Schemes addon launches directly — no slot cycling
  if (strcmp(ADDON_DEFINITIONS[index].id, "color_schemes") == 0) {
    _ui_screen_change(ui_Colors, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return;
  }
  cycleAddonSelection((size_t)index);
  refreshAddonsUi();
}
