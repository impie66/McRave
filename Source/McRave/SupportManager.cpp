#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Support {

    namespace {

        void updateDestination(UnitInfo& unit)
        {
            // Detectors want to stay close to their target
            if (unit.hasTarget())
                unit.setDestination(unit.getTarget().getPosition());

            // Overlords move towards the closest stations for now
            else if (unit.getType() == UnitTypes::Zerg_Overlord)
                unit.setDestination(Stations::getClosestStation(PlayerState::Self, unit.getPosition()));

            // Find the highest combat cluster that doesn't overlap a current support action of this UnitType
            else {
                auto highestCluster = 0.0;

                for (auto &[score, position] : Combat::getCombatClusters()) {
                    if (score > highestCluster && !Command::overlapsActions(unit.unit(), position, unit.getType(), PlayerState::Self, 64)) {
                        highestCluster = score;
                        unit.setDestination(position);
                    }
                }
            }

            Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Purple);
        }

        void updateDecision(UnitInfo& unit)
        {
            auto scoreBest = 0.0;
            auto posBest = Positions::Invalid;
            auto start = unit.getWalkPosition();
            auto building = Broodwar->self()->getRace().getResourceDepot();

            // If this unit is a scanner sweep, add the action and return
            if (unit.getType() == UnitTypes::Spell_Scanner_Sweep) {
                Command::addAction(unit.unit(), unit.getPosition(), UnitTypes::Spell_Scanner_Sweep, PlayerState::Self);
                return;
            }

            // TODO: Overlord scouting, need to use something different to spread overlords
            // Disabled
            if (unit.getType() == UnitTypes::Zerg_Overlord && !Terrain::getEnemyStartingPosition().isValid())
                posBest = BWEB::Map::getMainPosition();//Terrain::closestUnexploredStart();

            // Check if any expansions need detection on them - currently only looks at next
            else if (unit.getType().isDetector() && com(unit.getType()) >= 2 && BuildOrder::buildCount(building) > vis(building) && !Command::overlapsActions(unit.unit(), (Position)Buildings::getCurrentExpansion(), unit.getType(), PlayerState::Self, 320))
                posBest = Position(Buildings::getCurrentExpansion());

            // Arbiters cast stasis on a target		
            else if (unit.getType() == UnitTypes::Protoss_Arbiter && unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) < SIM_RADIUS && unit.getTarget().unit()->exists() && unit.unit()->getEnergy() >= TechTypes::Stasis_Field.energyCost() && !Command::overlapsActions(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm, PlayerState::Self, 96)) {
                if ((Grids::getEGroundCluster(unit.getTarget().getWalkPosition()) + Grids::getEAirCluster(unit.getTarget().getWalkPosition())) > STASIS_LIMIT) {
                    unit.unit()->useTech(TechTypes::Stasis_Field, unit.getTarget().unit());
                    Command::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Stasis_Field, PlayerState::Self);
                    return;
                }
            }

            else {
                Command::escort(unit);
                //for (int x = start.x - 20; x <= start.x + 20; x++) {
                //    for (int y = start.y - 20; y <= start.y + 20; y++) {
                //        WalkPosition w(x, y);
                //        Position p(w);

                //        // If not valid, too close, in danger or overlaps existing commands
                //        if (!w.isValid()
                //            || p.getDistance(unit.getPosition()) <= 64
                //            || Command::isInDanger(unit, p)
                //            || Command::overlapsActions(unit.unit(), p, unit.getType(), PlayerState::Self, 96))
                //            continue;

                //        auto threat = max(MIN_THREAT, Grids::getEAirThreat(w));
                //        auto dist = unit.hasTarget() ? (p.getDistance(unit.getDestination())) + p.getDistance(unit.getTarget().getPosition()) : (p.getDistance(unit.getDestination()));

                //        // Try to keep the unit alive if it's cloaked inside detection
                //        if (unit.unit()->isCloaked()) {
                //            if (!Command::overlapsDetection(unit.unit(), p, PlayerState::Enemy) || unit.getPercentShield() > 0.8)
                //                threat = threat / 2.0f;
                //            if (unit.getPercentShield() <= LOW_SHIELD_PERCENT_LIMIT && threat > MIN_THREAT && Command::overlapsDetection(unit.unit(), p, PlayerState::Enemy))
                //                continue;
                //        }

                //        // Score this move
                //        auto score = 1.0 / (threat * dist);

                //        if (score > scoreBest) {
                //            scoreBest = score;
                //            posBest = p;
                //        }
                //    }
                //}
            }

            //// Move and update commands
            //if (posBest.isValid()) {
            //    unit.setEngagePosition(posBest);
            //    unit.command(UnitCommandTypes::Move, posBest, true);
            //    Broodwar->drawLineMap(unit.getPosition(), posBest, Colors::Green);
            //}
        }
    }

    void onFrame()
    {
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            UnitInfo &unit = *u;
            if (unit.getRole() == Role::Support) {
                updateDestination(unit);
                updateDecision(unit);
            }
        }
    }


}