#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

V2Settings::V2Settings() {
    config.layout = LAYOUT_CLASSIC;
    config.zoomSpeed = 2;
    config.panSpeed = 2;
    config.scrollSpeed = 2;
}

void V2Settings::load() {
    FILE* f = fopen(configPath, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (size == sizeof(Config)) {
            fread(&config, sizeof(Config), 1, f);
        }
        // If size doesn't match (old format with theme/accent), fall through to defaults
        fclose(f);
    }
}

void V2Settings::save() {
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/V2NES", 0777);
    mkdir("sdmc:/V2NES", 0777);
    mkdir("sdmc:/V2NES/maps", 0777);
    mkdir("sdmc:/V2NES/walkthroughs", 0777);

    FILE* f = fopen(configPath, "wb");
    if (f) {
        fwrite(&config, sizeof(Config), 1, f);
        fclose(f);
    }
}

const char* V2Settings::getSpeedName(int s) {
    switch (s) {
        case 1:  return "Slow";
        case 2:  return "Medium";
        case 3:  return "Fast";
        default: return "Medium";
    }
}
