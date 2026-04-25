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
#include "Eject.h"
#include "FistIT.h"
#include "x-toys.h"
#include "styles.h"
#include "icons.h"
#include "main.h"


uint8_t CUM_Address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// ---------------------------------------------------------------------------
// Module-local state
// ---------------------------------------------------------------------------

static int  g_addon_slots[ADDON_DEFINITIONS_COUNT] = {ADDON_SLOT_NONE};
static bool g_addons_loaded_from_nvs = false;
static lv_obj_t* g_addon_icon_eject = nullptr;
static lv_obj_t* g_addon_icon_fistit = nullptr;

static constexpr int EJECT_ICON_W = 20;
static constexpr int EJECT_ICON_H = 21;
static constexpr int FIST_ICON_W = 18;
static constexpr int FIST_ICON_H = 23;
static uint8_t g_icon_eject_buf[LV_CANVAS_BUF_SIZE(EJECT_ICON_W, EJECT_ICON_H, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
static uint8_t g_icon_fist_buf[LV_CANVAS_BUF_SIZE(FIST_ICON_W, FIST_ICON_H, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
static bool g_icon_eject_ready = false;
static bool g_icon_fist_ready = false;

static const char* const EJECT_ICON_MASK[EJECT_ICON_H] = {
  "....................",
  "...#.....###.....#..",
  "..###..#####....###.",
  ".####..######..####.",
  "..###...####...###..",
  "....#.....#.....#...",
  "....................",
  "........#####.......",
  "......##..#..##.....",
  ".....##.......##....",
  ".....##.......##....",
  "......###...###.....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##########....",
  "........#####.......",
  "....................",
};

static const char* const FIST_ICON_MASK[FIST_ICON_H] = {
  "..........#####...",
  ".....##.##.##..#..",
  "..##...#...#...##.",
  "##..#...#...#..##.",
  "##...##..##..##.##",
  ".##.............##",
  ".##............##.",
  ".###.##........##.",
  "..#######......##.",
  ".###....##.....##.",
  "##.##....##...##..",
  "##..##....##..##..",
  "##...##....##.##..",
  "##....##....#.##..",
  "##.....##.....##..",
  "##......##....##..",
  "##.......##..###..",
  "##........######..",
  ".##.........###...",
  "..##........##....",
  "...##.......##....",
  "....#########.....",
  "..................",
};

int addonsGetEjectIconWidth()
{
  return EJECT_ICON_W;
}

int addonsGetEjectIconHeight()
{
  return EJECT_ICON_H;
}

const char* const* addonsGetEjectIconMask()
{
  return EJECT_ICON_MASK;
}

int addonsGetFistIconWidth()
{
  return FIST_ICON_W;
}

int addonsGetFistIconHeight()
{
  return FIST_ICON_H;
}

const char* const* addonsGetFistIconMask()
{
  return FIST_ICON_MASK;
}

static lv_obj_t* createIconBase(lv_obj_t* parent, int width, int height)
{
  if (parent == nullptr) {
    return nullptr;
  }

  lv_obj_t* icon = lv_canvas_create(parent);
  lv_obj_remove_style_all(icon);
  lv_obj_set_size(icon, width, height);
  lv_obj_set_align(icon, LV_ALIGN_LEFT_MID);
  lv_obj_set_x(icon, 8);
  lv_obj_set_y(icon, 0);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(icon, LV_OBJ_FLAG_ADV_HITTEST);
  return icon;
}

// Icon rendering now delegated to src/icons.cpp

static lv_obj_t* createEjectIcon(lv_obj_t* parent)
{
  lv_obj_t* icon = createIconBase(parent, EJECT_ICON_W, EJECT_ICON_H);
  if (icon == nullptr) {
    return nullptr;
  }

  icons_render_mask_canvas(icon, g_icon_eject_buf, g_icon_eject_ready, EJECT_ICON_MASK, EJECT_ICON_W, EJECT_ICON_H,
                           getActiveBackgroundColor(), getActiveTextPrimaryColor());
  return icon;
}

static lv_obj_t* createFistIcon(lv_obj_t* parent)
{
  lv_obj_t* icon = createIconBase(parent, FIST_ICON_W, FIST_ICON_H);
  if (icon == nullptr) {
    return nullptr;
  }

  icons_render_mask_canvas(icon, g_icon_fist_buf, g_icon_fist_ready, FIST_ICON_MASK, FIST_ICON_W, FIST_ICON_H,
                           getActiveBackgroundColor(), getActiveTextPrimaryColor());
  return icon;
}

static void ensureAddonRowIcons(void)
{
  if (ui_AddonsItem0 != nullptr && g_addon_icon_eject == nullptr) {
    g_addon_icon_eject = createEjectIcon(ui_AddonsItem0);
  }
  if (ui_AddonsItem1 != nullptr && g_addon_icon_fistit == nullptr) {
    g_addon_icon_fistit = createFistIcon(ui_AddonsItem1);
  }

  if (ui_AddonsItem0Text != nullptr) {
    lv_obj_set_align(ui_AddonsItem0Text, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(ui_AddonsItem0Text, EJECT_ICON_W + 14);
    lv_obj_set_y(ui_AddonsItem0Text, 0);
  }
  if (ui_AddonsItem1Text != nullptr) {
    lv_obj_set_align(ui_AddonsItem1Text, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(ui_AddonsItem1Text, FIST_ICON_W + 14);
    lv_obj_set_y(ui_AddonsItem1Text, 0);
  }
}

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

static bool isAddonAssigned(const char *addonId)
{
  if (addonId == nullptr) {
    return false;
  }

  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    if (strcmp(ADDON_DEFINITIONS[i].id, addonId) == 0) {
      return g_addon_slots[i] != ADDON_SLOT_NONE;
    }
  }
  return false;
}

static void syncAddonActivationFromBindings()
{
  EjectSetAddonEnabled(isAddonAssigned("eject_cumpump"));
  FistITSetAddonEnabled(isAddonAssigned("fist_it"));
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
  syncAddonActivationFromBindings();
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

static int findAddonIndexForSlot(int slot)
{
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    if (g_addon_slots[i] == slot) {
      return (int)i;
    }
  }
  return -1;
}

extern "C" void refreshHomeAddonButtonLabels(void)
{
  loadAddonBindingsIfNeeded();

  if (ui_HomeButtonLText != nullptr) {
    lv_label_set_text(ui_HomeButtonLText, T_MENU);
  }

  if (ui_HomeButtonRText != nullptr) {
    lv_label_set_text(ui_HomeButtonRText, T_SCREEN_PATTERN);
  }
}

static void refreshAddonsUi()
{
  ensureAddonRowIcons();

  if (ui_AddonsItem0Text != nullptr && ADDON_DEFINITIONS_COUNT > 0) {
    char line[96];
    snprintf(line, sizeof(line), "%s  [%s]",
             ADDON_DEFINITIONS[0].displayName, addonSlotLabel(g_addon_slots[0]));
    lv_label_set_text(ui_AddonsItem0Text, line);
    // Ensure option 1 uses slider1 style mapping
    if (ui_AddonsItem0 != nullptr) {
      int slotIdx = 0;
      lv_obj_add_style(ui_AddonsItem0, &style_slider_indicator[slotIdx % 4], LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_style(ui_AddonsItem0, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
      if (strcmp(ADDON_DEFINITIONS[0].id, "x_toys") == 0 && Ossm_paired) {
        // Mark this addon visually disabled when OSSM is connected via ESP-NOW
        lv_obj_add_style(ui_AddonsItem0, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_add_state(ui_AddonsItem0, LV_STATE_DISABLED);
      }
    }
  }

  if (ui_AddonsItem1Text != nullptr && ADDON_DEFINITIONS_COUNT > 1) {
    char line[96];
    snprintf(line, sizeof(line), "%s  [%s]",
             ADDON_DEFINITIONS[1].displayName, addonSlotLabel(g_addon_slots[1]));
    lv_label_set_text(ui_AddonsItem1Text, line);
    // Ensure option 2 uses slider2 style mapping
    if (ui_AddonsItem1 != nullptr) {
      int slotIdx = 1;
      lv_obj_add_style(ui_AddonsItem1, &style_slider_indicator[slotIdx % 4], LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_style(ui_AddonsItem1, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
      if (strcmp(ADDON_DEFINITIONS[1].id, "x_toys") == 0 && Ossm_paired) {
        lv_obj_add_style(ui_AddonsItem1, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_add_state(ui_AddonsItem1, LV_STATE_DISABLED);
      }
    }
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
    // Ensure option 3 uses slider3 style mapping
    if (ui_AddonsItem2 != nullptr) {
      int slotIdx = 2;
      lv_obj_add_style(ui_AddonsItem2, &style_slider_indicator[slotIdx % 4], LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_style(ui_AddonsItem2, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
      if (strcmp(ADDON_DEFINITIONS[2].id, "x_toys") == 0 && Ossm_paired) {
        lv_obj_add_style(ui_AddonsItem2, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_add_state(ui_AddonsItem2, LV_STATE_DISABLED);
      }
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
  syncAddonActivationFromBindings();
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
    FistITPrepareScreen();
    _ui_screen_change(FistITGetScreen(), LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "color_schemes") == 0) {
    _ui_screen_change(ui_Colors, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "x_toys") == 0) {
    if (Ossm_paired) {
      // Do not launch X-toys while OSSM is connected via ESP-NOW
      return false;
    }
    XtoysPrepareScreen();
    _ui_screen_change(XtoysGetScreen(), LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "streaming_mode") == 0) {
    // Launch the Streaming screen
    _ui_screen_change(ui_Streaming, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  return false;
}

// Trigger addon by its index in the ADDON_DEFINITIONS array
// If assigned to LEFT or RIGHT slot, launch it directly
// Otherwise, cycle its assignment like the right button does
void triggerAddonByIndex(int index)
{
  loadAddonBindingsIfNeeded();
  if (index < 0 || (size_t)index >= ADDON_DEFINITIONS_COUNT) {
    return;
  }
  
  int slot = g_addon_slots[index];
  if (slot == ADDON_SLOT_LEFT || slot == ADDON_SLOT_RIGHT) {
    // Addon is assigned to a slot, launch it
    triggerAddonForSlot(slot);
  } else {
    // Addon not assigned, cycle its selection (like right button does)
    cycleAddonSelection(index);
    refreshHomeAddonButtonLabels();
  }
}

extern "C" void addonsScreenLoaded(void)
{
  loadAddonBindingsIfNeeded();
  refreshAddonsUi();
  refreshHomeAddonButtonLabels();
}

extern "C" void addonsInitFromStorage(void)
{
  loadAddonBindingsIfNeeded();
  refreshHomeAddonButtonLabels();
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
  refreshHomeAddonButtonLabels();
}
