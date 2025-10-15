#include "stdafx.h"
#include "engine.h"

int main(int argc, char* argv[]) {
    engine::startup(argc, argv);
    while (engine::should_run()) {
        engine::run();
    }
    engine::shutdown();
    return 0;
}