#ifndef SETTINGS_H
#define SETTINGS_H

#include <3ds.h>

enum ControlLayout {
    LAYOUT_CLASSIC = 0,
    LAYOUT_MODERN,
    LAYOUT_ONE_HANDED,
    LAYOUT_COUNT
};

struct Config {
    ControlLayout layout;
    int zoomSpeed;   // 1 = Slow, 2 = Medium, 3 = Fast
    int panSpeed;    // 1 = Slow, 2 = Medium, 3 = Fast
    int scrollSpeed; // 1 = Slow, 2 = Medium, 3 = Fast
};

class V2Settings {
public:
    V2Settings();
    void load();
    void save();

    ControlLayout getLayout() const { return config.layout; }
    void setLayout(ControlLayout l) { config.layout = l; }

    int getZoomSpeed() const { return config.zoomSpeed; }
    void setZoomSpeed(int s) { config.zoomSpeed = s; }

    int getPanSpeed() const { return config.panSpeed; }
    void setPanSpeed(int s) { config.panSpeed = s; }

    int getScrollSpeed() const { return config.scrollSpeed; }
    void setScrollSpeed(int s) { config.scrollSpeed = s; }

    const char* getSpeedName(int s);

private:
    Config config;
    const char* configPath = "sdmc:/3ds/V2NES/settings.bin";
};

#endif
