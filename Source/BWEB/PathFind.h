#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB::PathFinding 
{
	class Path {
		std::vector<BWAPI::TilePosition> tiles;
		double dist;
	public:
		Path::Path()
		{
			tiles ={};
			dist = 0.0;
		}

		std::vector<BWAPI::TilePosition>& getTiles() { return tiles; }
		double getDistance() { return dist; }
		void createUnitPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position);
		void createWallPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position, bool);

		void createPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position, std::function <bool(const BWAPI::TilePosition)>, std::vector<BWAPI::TilePosition>);
	};
}
