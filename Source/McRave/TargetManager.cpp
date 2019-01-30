#include "McRave.h"

using namespace std;
using namespace BWAPI;

namespace McRave::Targets{

    namespace {

        void getBestTarget(UnitInfo& unit)
        {
            UnitInfo* bestTarget = nullptr;
            double closest = DBL_MAX, highest = 0.0;
            auto &unitList = unit.targetsFriendly() ? Units::getUnits(PlayerState::Self) : Units::getUnits(PlayerState::Enemy);
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myStrength = Players::getStrength(PlayerState::Self);

            const auto shouldTarget = [&](UnitInfo& target, bool unitCanAttack, bool targetCanAttack) {

                bool enemyHasGround = enemyStrength.groundToAir > 0.0 || enemyStrength.groundToGround > 0.0;
                bool enemyHasAir = enemyStrength.airToGround > 0.0 || enemyStrength.airToAir > 0.0;
                bool selfHasGround = myStrength.groundToAir > 0.0 || myStrength.groundToGround > 0.0;
                bool selfHasAir = myStrength.airToGround > 0.0 || myStrength.airToAir > 0.0;

                bool targetMatters = (target.getAirDamage() > 0.0 && selfHasAir)
                    || (target.getGroundDamage() > 0.0/* && selfHasGround*/)
                    || (target.getType().isDetector() && (Units::getMyVisible(UnitTypes::Protoss_Dark_Templar) > 0 || Units::getMyVisible(UnitTypes::Protoss_Observer) > 0))
                    || (target.getAirDamage() == 0.0 && target.getGroundDamage() == 0.0)
                    || (target.getType().isWorker())
                    || (!enemyHasGround && !enemyHasAir);

                // Zealot: Don't attack non threatening workers in our territory
                if ((unit.getType() == UnitTypes::Protoss_Zealot && target.getType().isWorker() && !target.isThreatening() && Terrain::isInAllyTerritory(target.getTilePosition()))

                    // If target is an egg, larva, scarab or spell
                    || (target.getType() == UnitTypes::Zerg_Egg || target.getType() == UnitTypes::Zerg_Larva || target.getType() == UnitTypes::Protoss_Scarab || target.getType().isSpell())

                    // If unit can't attack and unit is not a detector
                    || (!unit.getType().isDetector() && unit.getType() != UnitTypes::Terran_Comsat_Station && !unitCanAttack)

                    // If target is stasised
                    || (target.unit()->exists() && target.unit()->isStasised())

                    // Mutalisk: Don't attack useless buildings
                    || (target.getType().isBuilding() && target.getAirDamage() == 0.0 && target.getGroundDamage() == 0.0 && unit.getType() == UnitTypes::Zerg_Mutalisk)

                    // Zealot: Don't attack mines without +2
                    || (target.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)

                    // If target is invisible and can't attack this unit
                    || ((target.isBurrowed() || target.unit()->isCloaked()) && !target.unit()->isDetected() && !targetCanAttack && !unit.getType().isDetector())

                    // TODO: Remove this and improve command checks - If target is under DWEB and my unit is melee 
                    || (!target.getType().isFlyer() && target.unit()->isUnderDisruptionWeb() && unit.getGroundRange() <= 64)

                    // Don't attack units that don't matter
                    || !targetMatters

                    // DT: Don't attack Vultures
                    || (unit.getType() == UnitTypes::Protoss_Dark_Templar && target.getType() == UnitTypes::Terran_Vulture)

                    // Flying units don't attack interceptors
                    || (unit.getType().isFlyer() && target.getType() == UnitTypes::Protoss_Interceptor)

                    // Zealot: Rushing Zealots only attack workers
                    || (unit.getType() == UnitTypes::Protoss_Zealot && BuildOrder::isRush() && !target.getType().isWorker() && Broodwar->getFrameCount() < 10000)

                    // Don't attack enemy spider mines with more than 2 units
                    || (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getUnitsAttacking() >= 2))
                    return false;
                return true;
            };

            const auto checkBest = [&](UnitInfo& target, double thisUnit, double health, double distance) {

                auto priority = (target.getType().isBuilding() && target.getGroundDamage() == 0.0 && target.getAirDamage() == 0.0) ? target.getPriority() / 2.0 : target.getPriority();

                // Detector targeting
                if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == UnitTypes::Terran_Comsat_Station) {
                    if (target.isBurrowed() || target.unit()->isCloaked())
                        thisUnit = (priority * health) / distance;
                }

                // Cluster targeting for AoE units
                else if (unit.getType() == UnitTypes::Protoss_High_Templar || unit.getType() == UnitTypes::Protoss_Arbiter) {
                    if (!target.getType().isBuilding() && target.getType() != UnitTypes::Terran_Vulture_Spider_Mine) {

                        double eGrid = Grids::getEGroundCluster(target.getWalkPosition()) + Grids::getEAirCluster(target.getWalkPosition());
                        double aGrid = Grids::getAGroundCluster(target.getWalkPosition()) + Grids::getAAirCluster(target.getWalkPosition());
                        double score = eGrid;

                        if (eGrid > aGrid)
                            thisUnit = (priority * score) / distance;
                    }
                }

                // Proximity targeting
                else if (unit.getType() == UnitTypes::Protoss_Reaver) {
                    if (target.getType().isBuilding() && target.getGroundDamage() == 0.0 && target.getAirDamage() == 0.0)
                        thisUnit = 0.1 / distance;
                    else
                        thisUnit = health / distance;
                }

                // Priority targeting
                else
                    thisUnit = (priority * health) / distance;

                // If this target is more important to target, set as current target
                if (thisUnit > highest) {
                    highest = thisUnit;
                    bestTarget = &target;
                }
            };

            for (auto &t : unitList) {
                UnitInfo &target = *t;

                // Valid check;
                if (!target.unit()
                    || !target.getWalkPosition().isValid()
                    || !unit.getWalkPosition().isValid()
                    || (target.getType().isBuilding() && !target.isThreatening() && target.getGroundDamage() == 0.0 && Terrain::isInAllyTerritory(target.getTilePosition()) && Broodwar->getFrameCount() < 10000))
                    continue;

                bool targetCanAttack = ((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.getGroundDamage() > 0.0) || (!unit.getType().isFlyer() && target.getType() == UnitTypes::Terran_Vulture_Spider_Mine));
                bool unitCanAttack = ((target.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!target.getType().isFlyer() && unit.getGroundDamage() > 0.0) || (unit.getType() == UnitTypes::Protoss_Carrier));

                // HACK: Check for a flying building
                if (target.unit()->exists() && target.unit()->isFlying() && unit.getAirDamage() <= 0.0)
                    unitCanAttack = false;

                double allyRange = target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
                double airDist = unit.getPosition().getDistance(target.getPosition());
                double widths = unit.getType().tileWidth() * 16.0 + target.getType().tileWidth() * 16.0;
                double distance = widths + max(allyRange, airDist);
                double health = targetCanAttack ? 1.0 + (0.2*(1.0 - unit.getPercentTotal())) : 1.0;
                double thisUnit = 0.0;

                // Set sim position
                if ((unitCanAttack || targetCanAttack) && distance < closest) {
                    unit.setSimPosition(target.getPosition());
                    closest = distance;
                }

                if (unit.isLightAir() && !Stations::getMyStations().empty()) {
                    auto myStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
                    distance = target.getPosition().getDistance(myStation);
                }

                // If should target, check if it's best
                if (shouldTarget(target, unitCanAttack, targetCanAttack))
                    checkBest(target, thisUnit, health, distance);
            }

            if (bestTarget)
                unit.setTarget(*bestTarget);

            // If unit is close, increment it
            if (bestTarget && Util::unitInRange(unit))
                bestTarget->incrementBeingAttackedCount();
        }

        void getEngagePosition(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setEngagePosition(Positions::None);
                return;
            }

            int distance = (int)unit.getDistance(unit.getTarget());
            int range = unit.getTarget().getType().isFlyer() ? (int)unit.getAirRange() : (int)unit.getGroundRange();
            int leftover = distance - range;
            Position direction = (unit.getPosition() - unit.getTarget().getPosition()) * leftover / distance;

            if (distance > range)
                unit.setEngagePosition(unit.getPosition() - direction);
            else
                unit.setEngagePosition(unit.getPosition());
        }

        void getPathToTarget(UnitInfo& unit)
        {
            // Don't want a path if we're not fighting - waste of CPU
            if (unit.getRole() != Role::Combat)
                return;

            // If no target, no distance/path available
            if (!unit.hasTarget()) {
                BWEB::PathFinding::Path newPath;
                unit.setEngDist(0.0);
                unit.setPath(newPath);
                return;
            }

            const auto shouldCreatePath =
                (unit.getPath().getTiles().empty() || !unit.samePath());														    // If both units have the same tile

            const auto canCreatePath =
                (!unit.hasTransport()																							    // If unit has no transport				
                    && unit.getTilePosition().isValid() && unit.getTarget().getTilePosition().isValid()								// If both units have valid tiles
                    && !BWEB::Map::isUsed(unit.getTarget().getTilePosition())														// Doesn't overlap buildings
                    && !BWEB::Map::isUsed(unit.getTilePosition())
                    && !unit.getType().isFlyer() && !unit.getTarget().getType().isFlyer()											// Doesn't include flyers
                    && unit.getPosition().getDistance(unit.getTarget().getPosition()) < SIM_RADIUS									// Isn't too far from engaging
                    && BWEB::Map::isWalkable(unit.getTilePosition()) && BWEB::Map::isWalkable(unit.getTarget().getTilePosition()));	// Walkable tiles  

                                                                                                                                    // Set distance as estimate when targeting a building/flying unit or far away
            if (unit.getTarget().getType().isBuilding() || unit.getTarget().getType().isFlyer() || unit.getPosition().getDistance(unit.getTarget().getPosition()) >= SIM_RADIUS || unit.getTilePosition() == unit.getTarget().getTilePosition()) {
                BWEB::PathFinding::Path newPath;
                unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));
                unit.setPath(newPath);
                return;
            }

            // If should create path, grab one from BWEB
            if (shouldCreatePath && canCreatePath) {
                BWEB::PathFinding::Path newPath;
                newPath.createUnitPath(unit.getPosition(), unit.getTarget().getPosition());
                unit.setPath(newPath);
            }
            Visuals::displayPath(unit.getPath().getTiles());

            // Measure distance minus range
            if (canCreatePath) {
                if (unit.getPath().isReachable()) {
                    double range = unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
                    unit.setEngDist(max(0.0, unit.getPath().getDistance() - range));
                }
                // If unreachable
                else if (!unit.getPath().isReachable()) {
                    // HACK: Estimate until we can fix our pathing
                    auto dist = unit.getPosition().getDistance(unit.getEngagePosition());
                    unit.setEngDist(dist);
                    //auto dist = DBL_MAX;
                    //unit.setEngDist(dist);
                    //unit.circleRed();
                }
            }
            // Otherwise approximate and double
            else {
                auto dist = unit.getPosition().getDistance(unit.getEngagePosition());
                unit.setEngDist(dist);
            }
        }
    }

    void getTarget(UnitInfo& unit)
    {
        getBestTarget(unit);
        getEngagePosition(unit);
        getPathToTarget(unit);
    }
}