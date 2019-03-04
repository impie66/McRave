#include "McRave.h"
#include "BuildOrder.h"
#include <fstream>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;


namespace McRave::Learning {
    namespace {

        std::map <std::string, Build> myBuilds;
        std::stringstream ss;
        std::vector <std::string> buildNames;

        bool isBuildPossible(string build, string opener)
        {
            vector<UnitType> buildings, defenses;
            auto wallOptional = false;
            auto tight = false;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "2Gate" && opener == "Natural") {
                    buildings ={ Protoss_Gateway, Protoss_Gateway, Protoss_Pylon };
                    defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
                }
                else if (build == "GateNexus" || build == "NexusGate") {
                    int count = Util::chokeWidth(BWEB::Map::getNaturalChoke()) / 64;
                    buildings.insert(buildings.end(), count, Protoss_Pylon);
                }
                else {
                    tight = true;
                    buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
                    defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
                    wallOptional = true;
                }
            }

            if (Broodwar->self()->getRace() == Races::Terran) {
                buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };
                tight = true;
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
                defenses.insert(defenses.end(), 6, Zerg_Sunken_Colony);
            }

            if (Broodwar->self()->getRace() == Races::Terran) {
                if (Terrain::findMainWall(buildings, defenses, tight) || wallOptional)
                    return true;
            }
            else {
                if (Terrain::findNaturalWall(buildings, defenses, tight) || wallOptional)
                    return true;
            }
            return false;
        }

        bool isBuildAllowed(Race enemy, string build)
        {
            auto p = enemy == Races::Protoss;
            auto z = enemy == Races::Zerg;
            auto t = enemy == Races::Terran;
            auto r = enemy == Races::Unknown || enemy == Races::Random;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "1GateCore")
                    return true;
                if (build == "2Gate")
                    return true;
                if (build == "NexusGate" || build == "GateNexus")
                    return t;
                if (build == "FFE")
                    return z;
            }

            if (Broodwar->self()->getRace() == Races::Terran && build != "") {
                return true; // For now, test all builds to make sure they work!
            }

            if (Broodwar->self()->getRace() == Races::Zerg && build != "") {
                if (build == "PoolLair")
                    return z;
                if (build == "HatchPool")
                    return t || p;
            }
            return false;
        }

        bool isOpenerAllowed(Race enemy, string build, string opener)
        {
            // Ban certain openers from certain race matchups
            auto p = enemy == Races::Protoss;
            auto z = enemy == Races::Zerg;
            auto t = enemy == Races::Terran;
            auto r = enemy == Races::Unknown || enemy == Races::Random;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "1GateCore") {
                    if (opener == "0Zealot")
                        return t;
                    if (opener == "1Zealot")
                        return true;
                    if (opener == "2Zealot")
                        return p || z || r;
                }

                if (build == "2Gate") {
                    if (opener == "Proxy")
                        return false;
                    if (opener == "Natural")
                        return z /*|| p*/;
                    if (opener == "Main")
                        return true;
                }

                if (build == "FFE") {
                    if (opener == "Gate" || opener == "Nexus" || opener == "Forge")
                        return z;
                }

                if (build == "NexusGate") {
                    if (opener == "Dragoon" || opener == "Zealot")
                        return /*p ||*/ t;
                }
                if (build == "GateNexus") {
                    if (opener == "1Gate" || opener == "2Gate")
                        return t;
                }
            }

            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (build == "PoolLair")
                    if (opener == "9Pool")
                        return z;
                if (build == "HatchPool")
                    if (opener == "12Hatch")
                        return t || p;
            }
            return false;
        }

        bool isTransitionAllowed(Race enemy, string build, string transition)
        {
            // Ban certain transitions from certain race matchups
            auto p = enemy == Races::Protoss;
            auto z = enemy == Races::Zerg;
            auto t = enemy == Races::Terran;
            auto r = enemy == Races::Unknown || enemy == Races::Random;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "1GateCore") {
                    if (transition == "DT")
                        return p || t || z;
                    if (transition == "3GateRobo")
                        return p || r;
                    if (transition == "Corsair")
                        return z;
                    if (transition == "Reaver")
                        return p /*|| t*/ || r;
                    if (transition == "4Gate")
                        return p || t;
                }

                if (build == "2Gate") {
                    if (transition == "DT")
                        return /*p ||*/t;
                    if (transition == "Reaver")
                        return p /*|| t*/ || r;
                    if (transition == "Expand")
                        return p || z;
                    if (transition == "DoubleExpand")
                        return t;
                    if (transition == "4Gate")
                        return z;
                }

                if (build == "FFE") {
                    if (transition == "NeoBisu" || transition == "2Stargate" || transition == "StormRush")
                        return z;
                }

                if (build == "NexusGate") {
                    if (transition == "DoubleExpand" || transition == "ReaverCarrier")
                        return t;
                    if (transition == "Standard")
                        return t || p;
                }

                if (build == "GateNexus") {
                    if (transition == "DoubleExpand" || transition == "Carrier" || transition == "Standard")
                        return t;
                }
            }

            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (build == "PoolLair")
                    if (transition == "1HatchMuta")
                        return z;
                if (build == "HatchPool") {
                    if (transition == "2HatchMuta")
                        return t || p;
                    if (transition == "2HatchHydra")
                        return p;
                    if (transition == "3HatchLing")
                        return t;
                }
            }
            return false;
        }

        void getDefaultBuild()
        {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::vP()) {
                    BuildOrder::setCurrentBuild("1GateCore");
                    BuildOrder::setCurrentOpener("1Zealot");
                    BuildOrder::setCurrentTransition("Reaver");
                }
                else if (Players::vZ()) {
                    BuildOrder::setCurrentBuild("1GateCore");
                    BuildOrder::setCurrentOpener("2Zealot");
                    BuildOrder::setCurrentTransition("Corsair");
                }
                else if (Players::vT()) {
                    BuildOrder::setCurrentBuild("GateNexus");
                    BuildOrder::setCurrentOpener("1Gate");
                    BuildOrder::setCurrentTransition("Standard");
                    isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                }
                else {
                    BuildOrder::setCurrentBuild("2Gate");
                    BuildOrder::setCurrentOpener("Main");
                    BuildOrder::setCurrentTransition("Reaver");
                }
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::vZ()) {
                    BuildOrder::setCurrentBuild("PoolLair");
                    BuildOrder::setCurrentOpener("9Pool");
                    BuildOrder::setCurrentTransition("1HatchMuta");
                }
                else {
                    BuildOrder::setCurrentBuild("HatchPool");
                    BuildOrder::setCurrentOpener("12Hatch");
                    BuildOrder::setCurrentTransition("2HatchMuta");
                }
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
            }
            return;
        }
    }

    void onEnd(bool isWinner)
    {
        // File extension including our race initial;
        const auto mapLearning = false;
        const string dash = "-";
        const string noStats = " 0 0 ";
        const string myRaceChar{ *Broodwar->self()->getRace().c_str() };
        const string enemyRaceChar{ *Broodwar->enemy()->getRace().c_str() };
        const string extension = mapLearning ? myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + Broodwar->mapFileName() + ".txt" : myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + ".txt";

        // Write into the write directory 3 tokens at a time (4 if we detect a dash)
        ofstream config("bwapi-data/write/" + extension);
        string token;
        LearningToken currentToken = LearningToken::Build;

        const auto shouldIncrement = [&](string name) {

            // Depending on token, increment the correct data
            if (currentToken == LearningToken::Transition && name == BuildOrder::getCurrentTransition()) {
                currentToken = LearningToken::None;
                return true;
            }
            if (currentToken == LearningToken::Opener && name == BuildOrder::getCurrentOpener()) {
                currentToken = LearningToken::Transition;
                return true;
            }
            if (currentToken == LearningToken::Build && name == BuildOrder::getCurrentBuild()) {
                currentToken = LearningToken::Opener;
                return true;
            }
            return false;
        };

        int totalWins, totalLoses;
        ss >> totalWins >> totalLoses;
        isWinner ? totalWins++ : totalLoses++;
        config << totalWins << " " << totalLoses << endl;

        // For each token, check if we should increment the wins or losses then shove it into the config file
        while (ss >> token) {
            if (token == dash) {
                int w, l;

                ss >> token >> w >> l;
                shouldIncrement(token) ? (isWinner ? w++ : l++) : 0;
                config << dash << endl;
                config << token << " " << w << " " << l << endl;
            }
            else {
                int w, l;

                ss >> w >> l;
                shouldIncrement(token) ? (isWinner ? w++ : l++) : 0;
                config << token << " " << w << " " << l << endl;
            }
        }
    }

    void onStart()
    {
        if (!Broodwar->enemy()) {
            getDefaultBuild();
            return;
        }

        bool testing = false;
        if (testing) {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                BuildOrder::setCurrentBuild("2Gate");
                BuildOrder::setCurrentOpener("Natural");
                BuildOrder::setCurrentTransition("DT");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                BuildOrder::setCurrentBuild("HatchPool");
                BuildOrder::setCurrentOpener("12Hatch");
                BuildOrder::setCurrentTransition("3HatchLing");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
        }

        // File extension including our race initial;
        const auto mapLearning = false;
        const string dash = "-";
        const string noStats = " 0 0 ";
        const string myRaceChar{ *Broodwar->self()->getRace().c_str() };
        const string enemyRaceChar{ *Broodwar->enemy()->getRace().c_str() };
        const string extension = mapLearning ? myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + Broodwar->mapFileName() + ".txt" : myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + ".txt";

        // Tokens
        string buffer, token;

        // Protoss builds, openers and transitions
        if (Broodwar->self()->getRace() == Races::Protoss) {

            myBuilds["1GateCore"].openers ={ "0Zealot", "1Zealot", "2Zealot" };
            myBuilds["1GateCore"].transitions ={ "3GateRobo", "Robo", "Corsair", "4Gate", "DT" };

            myBuilds["2Gate"].openers ={ "Proxy", "Natural", "Main" };
            myBuilds["2Gate"].transitions ={ "DT", "Robo", "Expand", "DoubleExpand", "4Gate" };

            myBuilds["FFE"].openers ={ "Gate", "Nexus", "Forge" };
            myBuilds["FFE"].transitions ={ "NeoBisu", "2Stargate", "StormRush", "4GateArchon", "CorsairReaver" };

            myBuilds["NexusGate"].openers ={ "Dragoon", "Zealot" };
            myBuilds["NexusGate"].transitions ={ "Standard", "DoubleExpand", "ReaverCarrier" };

            myBuilds["GateNexus"].openers ={ "1Gate", "2Gate" };
            myBuilds["GateNexus"].transitions ={ "Standard", "DoubleExpand", "Carrier" };
        }

        if (Broodwar->self()->getRace() == Races::Zerg) {

            myBuilds["PoolHatch"].openers ={ "9Pool", "12Pool", "13Pool" };
            myBuilds["PoolHatch"].transitions={ "2HatchMuta", "2HatchHydra" };

            myBuilds["HatchPool"].openers ={ "9Hatch" , "10Hatch", "12Hatch" };
            myBuilds["HatchPool"].transitions={ "3HatchLing", "2HatchMuta", "2HatchHydra" };

            myBuilds["PoolLair"].openers ={ "9Pool" };
            myBuilds["PoolLair"].transitions={ "1HatchMuta", "1HatchLurker" };
        }

        if (Broodwar->self()->getRace() == Races::Terran) {
            BuildOrder::setCurrentBuild("RaxFact");
            BuildOrder::setCurrentOpener("10Rax");
            BuildOrder::setCurrentTransition("2FactVulture");
            isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
            return;// Don't know what to do yet

/*
            myBuilds["RaxFact"].openers ={ "8Rax", "10Rax", "12Rax" };
            myBuilds["RaxFact"].transitions ={ "Joyo", "2FactVulture", "1FactExpand", "2PortWraith" };

            myBuilds["2Rax"].openers = { "}*/
        }

        // Winrate tracking
        auto bestBuildWR = -0.01;
        string bestBuild = "";

        auto bestOpenerWR = -0.01;
        string bestOpener = "";

        auto bestTransitionWR = -0.01;
        string bestTransition = "";

        const auto parseLearningFile = [&](ifstream &file) {
            int dashCnt = -1; // Start at -1 so we can index at 0
            while (file >> buffer)
                ss << buffer << " ";

            // Create a copy so we aren't dumping out the information
            stringstream sscopy;
            sscopy << ss.str();

            // Initialize
            string thisBuild = "";
            string thisOpener = "";
            Race enemyRace = Broodwar->enemy()->getRace();
            int totalWins, totalLoses;

            // Store total games played to calculate UCB values
            sscopy >> totalWins >> totalLoses;
            int totalGamesPlayed = totalWins + totalLoses;

            while (!sscopy.eof()) {
                sscopy >> token;

                if (token == "-") {
                    dashCnt++;
                    dashCnt %= 3;
                    continue;
                }

                int w, l, numGames;
                sscopy >> w >> l;
                numGames = w + l;

                double winRate = numGames > 0 ? double(w) / double(numGames) : 0;
                double ucbVal = sqrt(2.0 * log((double)totalGamesPlayed) / numGames);
                double val = winRate + ucbVal;

                if (l == 0)
                    val = DBL_MAX;

                switch (dashCnt) {

                    // Builds
                case 0:
                    if (val > bestBuildWR && isBuildAllowed(enemyRace, token)) {
                        bestBuild = token;
                        bestBuildWR = val;

                        bestOpenerWR = -0.01;
                        bestOpener = "";

                        bestTransitionWR = -0.01;
                        bestTransition = "";
                    }
                    break;

                    // Openers
                case 1:
                    if (val > bestOpenerWR && isOpenerAllowed(enemyRace, bestBuild, token)) {
                        bestOpener = token;
                        bestOpenerWR = val;

                        bestTransitionWR = -0.01;
                        bestTransition = "";
                    }
                    break;

                    // Transitions
                case 2:
                    if (val > bestTransitionWR && isTransitionAllowed(enemyRace, bestBuild, token)) {
                        bestTransition = token;
                        bestTransitionWR = val;
                    }
                    break;
                }
            }
            return;
        };

        // Attempt to read a file from the read directory
        ifstream readFile("bwapi-data/read/" + extension);
        ifstream writeFile("bwapi-data/write/" + extension);
        if (readFile)
            parseLearningFile(readFile);

        // Try to read a file from the write directory
        else if (writeFile)
            parseLearningFile(writeFile);

        // If no file, create one
        else {
            if (!writeFile.good()) {
                ss << noStats;
                for (auto &b : myBuilds) {
                    auto &build = b.second;
                    ss << dash << " ";
                    ss << b.first << noStats;

                    // Load the openers for this build
                    ss << dash << " ";
                    for (auto &opener : build.openers)
                        ss << opener << noStats;


                    // Load the transitions for this build
                    ss << dash << " ";
                    for (auto &transition : build.transitions)
                        ss << transition << noStats;
                }
            }
        }

        // TODO: If the build is possible based on this map (we have wall/island requirements)
        if (isBuildPossible(bestBuild, bestOpener) && isBuildAllowed(Broodwar->enemy()->getRace(), bestBuild)) {
            BuildOrder::setCurrentBuild(bestBuild);
            BuildOrder::setCurrentOpener(bestOpener);
            BuildOrder::setCurrentTransition(bestTransition);
        }
        else
            getDefaultBuild();
    }
}