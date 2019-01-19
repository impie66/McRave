#pragma once
#include <BWAPI.h>
#include <set>

namespace McRave
{
    class PlayerInfo
    {
        std::set <BWAPI::UpgradeType> playerUpgrades;
        std::set <BWAPI::TechType> playerTechs;
        std::map <BWAPI::Unit, UnitInfo> units;

        BWAPI::Player thisPlayer;
        BWAPI::Race startRace, currentRace;
        BWAPI::TilePosition startLocation;
        PlayerState pState;
        std::string build;

        bool alive;
        double airToAir, airToGround, groundToAir, groundToGround;
        double groundStrength;
        int supply;
    public:
        PlayerInfo() {
            thisPlayer = nullptr;
            currentRace = BWAPI::Races::None;
            startRace = BWAPI::Races::None;
            alive = true;
            pState = PlayerState::None;
            build = "Unknown";

            airToAir = 0.0;
            airToGround = 0.0;
            groundToAir = 0.0;
            groundToGround = 0.0;
            supply = 0;
        }

        bool operator== (PlayerInfo& comparePlayer) {
            return thisPlayer = comparePlayer.thisPlayer;
        }

        void update()
        {
            // Store any upgrades this player has
            for (auto &upgrade : BWAPI::UpgradeTypes::allUpgradeTypes()) {
                if (thisPlayer->getUpgradeLevel(upgrade) > 0)
                    playerUpgrades.insert(upgrade);
            }

            // Store any tech this player has
            for (auto &tech : BWAPI::TechTypes::allTechTypes()) {
                if (thisPlayer->hasResearched(tech))
                    playerTechs.insert(tech);
            }

            // Set current allied status
            if (thisPlayer->isEnemy(BWAPI::Broodwar->self()))
                pState = PlayerState::Enemy;
            else if (thisPlayer->isAlly(BWAPI::Broodwar->self()))
                pState = PlayerState::Ally;
            else if (thisPlayer == Broodwar->self())
                pState = PlayerState::Self;
            else
                pState = PlayerState::None;

            currentRace = alive ? thisPlayer->getRace() : BWAPI::Races::None;
        }

        BWAPI::TilePosition getStartingLocation() { return startLocation; }
        BWAPI::Race getCurrentRace() { return currentRace; }
        BWAPI::Race getStartRace() { return startRace; }
        BWAPI::Player player() { return thisPlayer; }
        PlayerState getPlayerState() { return pState; }
        std::map <BWAPI::Unit, UnitInfo>& getUnits() { return units; }
        std::string getBuild() { return build; }

        bool isAlive() { return alive; }
        bool isEnemy() { return pState == PlayerState::Enemy; }
        bool isAlly() { return pState == PlayerState::Ally; }
        bool isSelf() { return pState == PlayerState::Self; }
        bool isNeutral() { return pState == PlayerState::Neutral; }
        int getSupply() { return supply; }

        void setCurrentRace(BWAPI::Race newRace) { currentRace = newRace; }
        void setStartRace(BWAPI::Race newRace) { startRace = newRace; }
        void setPlayer(BWAPI::Player newPlayer) { thisPlayer = newPlayer; }
        void setAlive(bool newState) { alive = newState; }
        void setBuild(std::string newBuild) { build = newBuild; }

        bool hasUpgrade(BWAPI::UpgradeType upgrade) {
            if (playerUpgrades.find(upgrade) != playerUpgrades.end())
                return true;
            return false;
        }
        bool hasTech(BWAPI::TechType tech) {
            if (playerTechs.find(tech) != playerTechs.end())
                return true;
            return false;
        }
    };
}