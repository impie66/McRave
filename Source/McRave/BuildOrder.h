#pragma once
#include <BWAPI.h>
#include <sstream>
#include <set>

namespace McRave::BuildOrder {
    class Item
    {
        int actualCount, reserveCount;
    public:
        Item() {};

        Item(int actual, int reserve = -1) {
            actualCount = actual, reserveCount = (reserve == -1 ? actual : reserve);
        }

        int const getReserveCount() { return reserveCount; }
        int const getActualCount() { return actualCount; }
    };

    // Need a namespace to share variables among the various files used
    namespace All {
        inline std::map <BWAPI::UnitType, Item> itemQueue;
        inline bool getOpening = true;
        inline bool getTech = false;
        inline bool wallNat = false;
        inline bool wallMain = false;
        inline bool scout = false;
        inline bool productionSat = false;
        inline bool techSat = false;
        inline bool fastExpand = false;
        inline bool proxy = false;
        inline bool hideTech = false;
        inline bool delayFirstTech = false;
        inline bool playPassive = false;
        inline bool rush = false;
        inline bool cutWorkers = false; // TODO: Use unlocking
        inline bool lockedTransition = false;
        inline bool gasTrick = false;
        inline bool bookSupply = false;
        inline bool transitionReady = false;

        inline int gasLimit = INT_MAX;
        inline int zealotLimit = INT_MAX;
        inline int dragoonLimit = INT_MAX;
        inline int lingLimit = INT_MAX;
        inline int droneLimit = INT_MAX;
        inline int startCount = 0;
        inline int s = 0;

        inline std::string currentBuild = "";
        inline std::string currentOpener = "";
        inline std::string currentTransition = "";

        inline BWAPI::UpgradeType firstUpgrade = BWAPI::UpgradeTypes::None;
        inline BWAPI::TechType firstTech = BWAPI::TechTypes::None;
        inline BWAPI::UnitType firstUnit = BWAPI::UnitTypes::None;
        inline BWAPI::UnitType techUnit = BWAPI::UnitTypes::None;
        inline BWAPI::UnitType desiredDetection = BWAPI::UnitTypes::None;
        inline std::set <BWAPI::UnitType> techList;
        inline std::set <BWAPI::UnitType> unlockedType;
    }

    namespace Protoss {

        void opener();
        void tech();
        void situational();
        void unlocks();
        void island();

        void PvP1GateCore();
        void PvP2Gate();

        void PvTGateNexus();
        void PvTNexusGate();
        void PvT1GateCore();
        void PvT2Gate();

        void PvZFFE();
        void PvZ1GateCore();
        void PvZ2Gate();
    }

    namespace Terran {
        void opener();
        void tech();
        void situational();
        void unlocks();
        //void island();

        void RaxFact();
    }

    namespace Zerg {

        void opener();
        void tech();
        void situational();
        void unlocks();
        //void island();

        void PoolHatch();
        void HatchPool();
        void PoolLair();
    }

    int buildCount(BWAPI::UnitType);
    bool firstReady();
    bool unlockReady(BWAPI::UnitType);

    void onFrame();
    void getNewTech();
    void checkNewTech();
    void checkAllTech();
    void checkExoticTech();
    bool shouldAddProduction();
    bool shouldAddGas();
    bool shouldExpand();
    bool techComplete();
    bool isAlmostComplete(BWAPI::UnitType);

    std::map<BWAPI::UnitType, Item>& getItemQueue();
    BWAPI::UnitType getTechUnit();
    BWAPI::UnitType getFirstUnit();
    BWAPI::UpgradeType getFirstUpgrade();
    BWAPI::TechType getFirstTech();
    std::set <BWAPI::UnitType>& getTechList();
    std::set <BWAPI::UnitType>& getUnlockedList();
    int gasWorkerLimit();
    bool isWorkerCut();
    bool isUnitUnlocked(BWAPI::UnitType unit);
    bool isTechUnit(BWAPI::UnitType unit);
    bool isOpener();
    bool isFastExpand();
    bool shouldScout();
    bool isWallNat();
    bool isWallMain();
    bool isProxy();
    bool isHideTech();
    bool isPlayPassive();
    bool isRush();
    bool isGasTrick();
    std::string getCurrentBuild();
    std::string getCurrentOpener();
    std::string getCurrentTransition();

    void setLearnedBuild(std::string, std::string, std::string);
}