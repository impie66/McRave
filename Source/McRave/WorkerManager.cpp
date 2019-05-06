#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Workers {

    namespace {

        int minWorkers = 0;
        int gasWorkers = 0;
        int boulderWorkers = 0;

        bool closeToResource(UnitInfo& worker)
        {
            auto close = BWEB::Map::getGroundDistance(worker.getResource().getPosition(), worker.getPosition()) <= 128.0;
            auto sameArea = mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition());
            return close || sameArea;
        }

        bool misc(UnitInfo& worker)
        {
            // If worker is potentially stuck, try to find a manner pylon
            // TODO: Use workers target? Check if it's actually targeting pylon?
            if (worker.framesHoldingResource() >= 100 || worker.framesHoldingResource() <= -200) {
                auto &pylon = Util::getClosestUnit(worker.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getType() == UnitTypes::Protoss_Pylon;
                });
                if (pylon && pylon->unit() && pylon->unit()->exists() && pylon->getPosition().getDistance(worker.getPosition()) < 128.0) {
                    if (worker.unit()->getLastCommand().getTarget() != pylon->unit())
                        worker.unit()->attack(pylon->unit());
                    return true;
                }
            }
            return false;
        }

        bool transport(UnitInfo& worker)
        {
            // Workers that have a transport coming to pick them up should not do anything other than returning cargo
            if (worker.hasTransport() && !worker.unit()->isCarryingMinerals() && !worker.unit()->isCarryingGas()) {
                if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
                    worker.unit()->move(worker.getTransport().getPosition());
                return true;
            }
            return false;
        }

        bool build(UnitInfo& worker)
        {
            Position center = Position(worker.getBuildPosition()) + Position(worker.getBuildingType().tileWidth() * 16, worker.getBuildingType().tileHeight() * 16);
            Position topLeft = Position(worker.getBuildPosition());
            Position botRight = topLeft + Position(worker.getBuildingType().tileWidth() * 32, worker.getBuildingType().tileHeight() * 32);

            // Can't execute build if we have no building
            if (!worker.getBuildingType().isValid() || !worker.getBuildPosition().isValid())
                return false;

            // 1) Attack any enemies inside the build area
            if (worker.hasTarget() && Util::rectangleIntersect(topLeft, botRight, worker.getTarget().getPosition())) {
                if (Command::attack(worker)) {}
                else if (Command::move(worker)) {}
                return true;
            }

            // 2) Cancel any buildings we don't need
            else if ((BuildOrder::buildCount(worker.getBuildingType()) <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings::isBuildable(worker.getBuildingType(), worker.getBuildPosition()))) {
                worker.setBuildingType(UnitTypes::None);
                worker.setBuildPosition(TilePositions::Invalid);
                worker.unit()->stop();
                return true;
            }

            // 3) Move to build
            else if (shouldMoveToBuild(worker, worker.getBuildPosition(), worker.getBuildingType())) {

                // TODO: Generate a path and check for threat
                worker.setDestination(center);

                if (worker.getPosition().getDistance(center) > 32.0 + (96.0 * (double)worker.getBuildingType().isRefinery())) {
                    BWEB::Path newPath;
                    newPath.createUnitPath(worker.getPosition(), center);
                    worker.setPath(newPath);
                    Command::move(worker);
                }
                else if (worker.unit()->getOrder() != Orders::PlaceBuilding || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Build)
                    worker.unit()->build(worker.getBuildingType(), worker.getBuildPosition());
                return true;
            }
            return false;
        }

        bool clearNeutral(UnitInfo& worker)
        {
            auto resourceDepot = Broodwar->self()->getRace().getResourceDepot();
            if (vis(resourceDepot) < 2
                || (BuildOrder::buildCount(resourceDepot) == vis(resourceDepot) && BuildOrder::isOpener())
                || worker.unit()->isCarryingMinerals()
                || worker.unit()->isCarryingGas()
                || boulderWorkers != 0)
                return false;

            // Find boulders to clear
            for (auto &b : Resources::getMyBoulders()) {
                ResourceInfo &boulder = *b;
                if (!boulder.unit() || !boulder.unit()->exists())
                    continue;
                if ((worker.getPosition().getDistance(boulder.getPosition()) <= 320.0 && boulder.getGathererCount() == 0) || (worker.unit()->isGatheringMinerals() && worker.unit()->getOrderTarget() == boulder.unit())) {
                    if (worker.unit()->getOrderTarget() != boulder.unit())
                        worker.unit()->gather(boulder.unit());
                    boulderWorkers = 1;
                    return true;
                }
            }
            return false;
        }

        bool gather(UnitInfo& worker)
        {
            auto resourceExists = worker.hasResource() && worker.getResource().unit() && worker.getResource().unit()->exists();

            // Mine the closest mineral field
            const auto mineRandom =[&]() {
                auto closest = worker.unit()->getClosestUnit(Filter::IsMineralField);
                if (closest && closest->exists() && worker.unit()->getLastCommand().getTarget() != closest)
                    worker.unit()->gather(closest);
            };

            // Check if we need to re-issue a gather command
            const auto shouldIssueGather =[&]() {
                if (worker.hasResource() && worker.getResource().unit()->exists() && (worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas() || worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather) && worker.unit()->getTarget() != worker.getResource().unit())
                    return true;
                if (!worker.hasResource() && (worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather))
                    return true;
                return false;
            };

            // Can't gather if we are carrying a resource
            if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals())
                return false;

            // If worker has a resource and it's mineable
            if (worker.hasResource() && worker.getResource().getResourceState() == ResourceState::Mineable) {
                auto resourceCentroid = worker.getResource().getStation() ? worker.getResource().getStation()->getResourceCentroid() : Positions::Invalid;
                worker.setDestination(resourceCentroid);

                // 1) If it's close or same area, don't need a path, set to empty	
                if (closeToResource(worker)) {
                    BWEB::Path emptyPath;
                    worker.setPath(emptyPath);
                }

                // 2) If it's far, generate a path
                else if (worker.getLastTile() != worker.getTilePosition() && resourceCentroid.isValid()) {
                    BWEB::Path newPath;
                    newPath.createUnitPath(worker.getPosition(), resourceCentroid);
                    worker.setPath(newPath);
                }

                Visuals::displayPath(worker.getPath().getTiles());

                // 3) If we are under a threat, try to get away from the threat
                //if (worker.getPath().getTiles().empty() && Grids::getEGroundThreat(worker.getWalkPosition()) > 0.0) {
                    //Command::kite(w);
                    //worker.circleOrange();
                    //return true;
                //}

                // 4) If no threat on path, mine it
                /*else*/ if (!Util::hasThreatOnPath(worker, worker.getPath())) {
                    if (shouldIssueGather())
                        worker.unit()->gather(worker.getResource().unit());
                    else if (!resourceExists)
                        Command::move(worker);
                    return true;
                }
            }

            // Mine something else while waiting
            mineRandom();
            return false;
        }

        bool returnCargo(UnitInfo& worker)
        {
            // Can't return cargo if we aren't carrying a resource
            if (!worker.unit()->isCarryingGas() && !worker.unit()->isCarryingMinerals())
                return false;

            auto checkPath = (worker.hasResource() && worker.getPosition().getDistance(worker.getResource().getPosition()) > 320.0) || (!worker.hasResource() && !Terrain::isInAllyTerritory(worker.getTilePosition()));
            if (checkPath) {
                // TODO: Create a path to the closest station and check if it's safe
            }

            // TODO: Check if we have a building to place first?
            if (worker.unit()->getOrder() != Orders::ReturnMinerals && worker.unit()->getOrder() != Orders::ReturnGas && worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
                worker.unit()->returnCargo();
            return true;
        }

        void updateAssignment(UnitInfo& worker)
        {
            // Check the status of the worker and the assigned resource
            const auto injured = worker.unit()->getHitPoints() + worker.unit()->getShields() < worker.getType().maxHitPoints() + worker.getType().maxShields();
            const auto threatened = (worker.hasResource() && !closeToResource(worker) && Util::hasThreatOnPath(worker, worker.getPath())) || (worker.hasResource() && closeToResource(worker) && int(worker.getTargetedBy().size()) > 0);
            const auto excessAssigned = worker.hasResource() && !injured && !threatened && worker.getResource().getGathererCount() >= 3 + int(worker.getResource().getType().isRefinery());

            // Check if worker needs a re-assignment
            const auto needGas = !Resources::isGasSaturated() && (gasWorkers < BuildOrder::gasWorkerLimit() || !BuildOrder::isOpener());
            const auto needMinerals = !Resources::isMinSaturated() && (gasWorkers > BuildOrder::gasWorkerLimit() || !BuildOrder::isOpener());
            const auto needNewAssignment = !worker.hasResource() || needGas || needMinerals || threatened || excessAssigned;

            auto closestStation = BWEB::Stations::getClosestStation(worker.getTilePosition());
            auto distBest = (injured || threatened) ? 0.0 : DBL_MAX;
            auto oldResource = worker.hasResource() ? worker.getResource().shared_from_this() : nullptr;
            vector<const BWEB::Station *> safeStations;

            const auto resourceReady = [&](ResourceInfo& resource, int i) {
                if (!resource.unit()
                    || resource.getType() == UnitTypes::Resource_Vespene_Geyser
                    || (resource.unit()->exists() && !resource.unit()->isCompleted())
                    || resource.getGathererCount() >= i
                    || resource.getResourceState() == ResourceState::None)
                    return false;
                return true;
            };

            // Return if we dont need an assignment
            if (!needNewAssignment)
                return;

            // 1) Find safe stations to mine resources from
            if (closestStation) {
                for (auto &s : Stations::getMyStations()) {
                    auto station = s.second;
                    auto path = Stations::pathStationToStation(closestStation, station);

                    // Store station if it's safe
                    if ((path && !Util::hasThreatOnPath(worker, *path)) || worker.getPosition().getDistance(station->getResourceCentroid()) < 128.0)
                        safeStations.push_back(station);
                }
            }

            // 2) Check if we need gas workers
            if (needGas || !worker.hasResource()) {
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = *r;
                    if (!resourceReady(resource, 3))
                        continue;
                    if (!resource.getStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                        continue;

                    auto dist = resource.getPosition().getDistance(worker.getPosition());
                    if ((dist < distBest && !injured) || (dist > distBest && injured)) {
                        worker.setResource(r.get());
                        distBest = dist;
                    }
                }
            }

            // 3) Check if we need mineral workers
            if (needMinerals || !worker.hasResource()) {
                for (int i = 1; i <= 2; i++) {
                    for (auto &r : Resources::getMyMinerals()) {

                        auto &resource = *r;
                        if (!resourceReady(resource, i))
                            continue;
                        if (!resource.getStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                            continue;

                        double dist = resource.getPosition().getDistance(worker.getPosition());
                        if ((dist < distBest && !injured && !threatened) || (dist > distBest && (injured || threatened))) {
                            worker.setResource(r.get());
                            distBest = dist;
                        }
                    }
                    if (worker.hasResource())
                        break;
                }

            }

            // 4) Assign resource
            if (worker.hasResource()) {

                // Remove current assignemt
                if (oldResource) {
                    oldResource->getType().isMineralField() ? minWorkers-- : gasWorkers--;
                    oldResource->removeTargetedBy(worker.weak_from_this());
                }

                // Add next assignment
                worker.getResource().getType().isMineralField() ? minWorkers++ : gasWorkers++;
                worker.getResource().addTargetedBy(worker.weak_from_this());

                BWEB::Path emptyPath;
                worker.setPath(emptyPath);

                // HACK: Update saturation checks
                Resources::onFrame();
            }
        }

        constexpr tuple commands{ misc, transport, returnCargo, clearNeutral, build, gather };
        void updateDecision(UnitInfo& worker)
        {
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Transport"),
                make_pair(2, "ReturnCargo"),
                make_pair(3, "ClearNeutral"),
                make_pair(4, "Build"),
                make_pair(5, "Gather")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = worker.getType().isBuilding() ? -16 : worker.getType().width() / 2;
            int i = Util::iterateCommands(commands, worker);
            Broodwar->drawTextMap(worker.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateWorkers()
        {
            // Sort units by distance to destination
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Worker) {
                    updateAssignment(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame() {
        Visuals::startPerfTest();
        boulderWorkers = 0; // HACK: Need a better solution to limit boulder workers
        updateWorkers();
        Visuals::endPerfTest("Workers");
    }

    void removeUnit(UnitInfo& worker) {
        worker.getResource().getType().isRefinery() ? gasWorkers-- : minWorkers--;
        worker.getResource().removeTargetedBy(worker.weak_from_this());
        worker.setResource(nullptr);
    }

    bool shouldMoveToBuild(UnitInfo& worker, TilePosition tile, UnitType type) {
        auto center = Position(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);
        auto mineralIncome = max(0.0, double(Workers::getMineralWorkers() - 1) * 0.045);
        auto gasIncome = max(0.0, double(Workers::getGasWorkers() - 1) * 0.07);
        auto speed = worker.getSpeed();
        auto dist = mapBWEM.GetArea(worker.getTilePosition()) ? BWEB::Map::getGroundDistance(worker.getPosition(), center) : worker.getPosition().getDistance(Position(worker.getBuildPosition()));
        auto time = (dist / speed) + 50.0;
        auto enoughGas = worker.getBuildingType().gasPrice() > 0 ? Broodwar->self()->gas() + int(gasIncome * time) >= worker.getBuildingType().gasPrice() : true;
        auto enoughMins = worker.getBuildingType().mineralPrice() > 0 ? Broodwar->self()->minerals() + int(mineralIncome * time) >= worker.getBuildingType().mineralPrice() : true;

        return enoughGas && enoughMins;
    };

    int getMineralWorkers() { return minWorkers; }
    int getGasWorkers() { return gasWorkers; }
    int getBoulderWorkers() { return boulderWorkers; }
}