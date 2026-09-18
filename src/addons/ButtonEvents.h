#pragma once

// Shared addon button-event snapshot, consumed by each addon's HandleScreen().
// Contains only the three events addon screens react to; core screens read the
// full flag set from ButtonHandlers.h directly.

struct ButtonEvents {
    bool leftShort;
    bool mxShort;
    bool rightShort;
};
