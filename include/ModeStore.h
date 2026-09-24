#pragma once
#include <Preferences.h>

class ModeStore {
public:
    bool begin() {
        ready_ = preferences_.begin("fnc-power", false);
        return ready_;
    }
    bool disabled() { return ready_ && preferences_.getBool("disabled", false); }
    bool save(bool disabled) {
        return ready_ && preferences_.putBool("disabled", disabled) == sizeof(bool);
    }
private:
    Preferences preferences_;
    bool ready_ = false;
};
