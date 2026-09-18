#include "ToyProfiles.h"

#include <Preferences.h>

namespace {

constexpr const char* NS = "toys";
constexpr const char* KEY_COUNT = "count";

String keyFor(int i, const char* field) {
    return String(field) + "_" + String(i);
}

}  // namespace

void toyProfilesInit() {
    // Nothing to do; profiles are read from NVS lazily.
}

int toyProfilesCount() {
    Preferences prefs;
    prefs.begin(NS, true);
    const int count = prefs.getInt(KEY_COUNT, 0);
    prefs.end();
    return count;
}

bool toyProfilesGet(int index, ToyProfile& out) {
    if (index < 0 || index >= toyProfilesCount()) return false;
    Preferences prefs;
    prefs.begin(NS, true);
    out.prefix = prefs.getString(keyFor(index, "name").c_str(), "");
    out.type = (ToyFeatureType)prefs.getInt(keyFor(index, "type").c_str(), 0);
    out.minValue = prefs.getInt(keyFor(index, "min").c_str(), 0);
    out.maxValue = prefs.getInt(keyFor(index, "max").c_str(), 20);
    out.commandTemplate = prefs.getString(keyFor(index, "tpl").c_str(), "Vibrate:%d;");
    prefs.end();
    return out.prefix.length() > 0;
}

int toyProfilesMatch(const char* name) {
    if (!name || !*name) return -1;
    const String n = name;
    const int count = toyProfilesCount();
    for (int i = 0; i < count; ++i) {
        ToyProfile p;
        if (!toyProfilesGet(i, p)) continue;
        if (n.startsWith(p.prefix)) return i;
    }
    return -1;
}

bool toyProfilesAdd(const ToyProfile& p) {
    String prefix = p.prefix;
    prefix.trim();
    if (prefix.length() == 0) return false;

    const int count = toyProfilesCount();
    int idx = -1;
    for (int i = 0; i < count; ++i) {
        ToyProfile e;
        if (toyProfilesGet(i, e) && e.prefix.equalsIgnoreCase(prefix)) {
            idx = i;
            break;
        }
    }
    if (idx < 0) idx = count;

    Preferences prefs;
    prefs.begin(NS, false);
    if (idx == count) prefs.putInt(KEY_COUNT, count + 1);
    prefs.putString(keyFor(idx, "name").c_str(), prefix);
    prefs.putInt(keyFor(idx, "type").c_str(), (int)p.type);
    prefs.putInt(keyFor(idx, "min").c_str(), p.minValue);
    prefs.putInt(keyFor(idx, "max").c_str(), p.maxValue);
    prefs.putString(keyFor(idx, "tpl").c_str(), p.commandTemplate);
    prefs.end();
    return true;
}

bool toyProfilesRemove(int index) {
    const int count = toyProfilesCount();
    if (index < 0 || index >= count) return false;

    Preferences prefs;
    prefs.begin(NS, false);
    for (int i = index; i < count - 1; ++i) {
        prefs.putString(keyFor(i, "name").c_str(), prefs.getString(keyFor(i + 1, "name").c_str(), ""));
        prefs.putInt(keyFor(i, "type").c_str(), prefs.getInt(keyFor(i + 1, "type").c_str(), 0));
        prefs.putInt(keyFor(i, "min").c_str(), prefs.getInt(keyFor(i + 1, "min").c_str(), 0));
        prefs.putInt(keyFor(i, "max").c_str(), prefs.getInt(keyFor(i + 1, "max").c_str(), 20));
        prefs.putString(keyFor(i, "tpl").c_str(), prefs.getString(keyFor(i + 1, "tpl").c_str(), ""));
    }
    const int last = count - 1;
    prefs.remove(keyFor(last, "name").c_str());
    prefs.remove(keyFor(last, "type").c_str());
    prefs.remove(keyFor(last, "min").c_str());
    prefs.remove(keyFor(last, "max").c_str());
    prefs.remove(keyFor(last, "tpl").c_str());
    prefs.putInt(KEY_COUNT, count - 1);
    prefs.end();
    return true;
}
