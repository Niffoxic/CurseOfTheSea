// Created by Niffoxic (Harsh Dubey)
#include <engine/engine.h>

int main()
{
    cots::engine engine;
    if (not engine.init())
        return 1;
    return engine.execute();
}
