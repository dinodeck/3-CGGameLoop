#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings
{
    std::string name;
    int width;
    int height;
    std::string manifestPath;
    std::string mainScript;
    std::string onUpdate;

    Settings() :
        name("CGGameLoop"),
        width(640),
        height(360),
        manifestPath("manifest.lua"),
        mainScript("main.lua"),
        onUpdate("update()") {}
};

#endif

