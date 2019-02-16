#pragma once
#include <BWAPI.h>

namespace McRave::Workers {

    void onFrame();
    void removeUnit(const std::shared_ptr<UnitInfo>&);

    int getMineralWorkers();
    int getGasWorkers();
    bool shouldMoveToBuild(UnitInfo&, BWAPI::TilePosition, BWAPI::UnitType);
}
