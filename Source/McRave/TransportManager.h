#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "TransportInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TransportManager
	{
		map <Unit, UnitInfo> myTransports;
		map <WalkPosition, int> recentExplorations;
	public:
		void onFrame();
		void updateTransports();
		void updateInformation(UnitInfo&);
		void updateCargo(UnitInfo&);
		void updateDecision(UnitInfo&);
		void updateMovement(UnitInfo&);
		void removeUnit(Unit);
		void storeUnit(Unit);
	};
}

typedef Singleton<McRave::TransportManager> TransportSingleton;
