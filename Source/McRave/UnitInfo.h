#pragma once
#include <BWAPI.h>
#include "..\BWEB\BWEB.h"
#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class ResourceInfo;

	class UnitInfo {
		double percentHealth, percentShield, percentTotal, groundRange, airRange, groundDamage, airDamage, speed;						// StarCraft stats
		double visibleGroundStrength, visibleAirStrength, maxGroundStrength, maxAirStrength, priority;		// McRave stats
		double engageDist;
		double simValue, simBonus;
		int localStrategy, globalStrategy, lastAttackFrame, lastVisibleFrame, shields, health, minStopFrame;
		int killCount, frameCreated;
		int lastMoveFrame;
		int resourceHeldFrames;
		int remainingTrainFrame;
		int energy;

		bool burrowed;
		bool engage, retreat;

		Unit thisUnit;
		UnitType unitType, buildingType;
		Player player;

		McRave::Role role;
		McRave::TransportState tState;
		McRave::CombatState cState;

		UnitInfo* target;
		UnitInfo* transport;
		ResourceInfo* resource;
		set<UnitInfo*> assignedCargo;

		Position position, engagePosition, destination, simPosition, lastPos;
		WalkPosition walkPosition, lastWalk;
		TilePosition tilePosition, buildPosition, lastTile;

		BWEB::Path targetPath;
		void updateTarget();
		void updateStuckCheck();
	public:
		UnitInfo();

		// General use
		void updateUnit();
		void createDummy(UnitType);
		double getDistance(UnitInfo unit) { return position.getDistance(unit.getPosition()); }

		// Roles
		McRave::Role getRole() { return role; }
		void setRole(McRave::Role newRole) { role = newRole; }

		// Simulation
		void setSimValue(double newValue) { simValue = newValue; }
		double getSimValue() { return simValue; }

		// Engage/Retreat
		void resetForces() { engage = false, retreat = false; }
		void setRetreat() { retreat = true; }
		void setEngage() { engage = true; }
		bool shouldRetreat() { return retreat; }
		bool shouldEngage() { return engage; }

		// Assigned Target
		bool hasTarget() { return target != nullptr; }
		UnitInfo &getTarget() { return *target; }
		void setTarget(UnitInfo * unit) { target = unit; }

		// Assigned Transport
		bool hasTransport() { return transport != nullptr; }
		UnitInfo &getTransport() { return *transport; }
		void setTransport(UnitInfo * unit) { transport = unit; }

		// Assigned Resource
		bool hasResource() { return resource != nullptr; }
		ResourceInfo &getResource() { return *resource; }
		void setResource(ResourceInfo * unit) { resource = unit; }

		// Assigned Cargo
		set<UnitInfo*>& getAssignedCargo() { return assignedCargo; }

		// States
		McRave::TransportState getTransportState() { return tState; }
		void setTransportState(McRave::TransportState newState) { tState = newState; }
		McRave::CombatState getCombatState() { return cState; }
		void setCombatState(McRave::CombatState newState) { cState = newState; }

		// Holding resource
		int framesHoldingResource() { return resourceHeldFrames; }

		// Engagement distances
		double getEngDist() { return engageDist; }
		void setEngDist(double newValue) { engageDist = newValue; }

		// Last positions
		Position getLastPosition() { return lastPos; }
		WalkPosition getLastWalk() { return lastWalk; }
		TilePosition getLastTile() { return lastTile; }
		bool sameTile() { return lastTile == tilePosition; }
		void setLastPositions();

		// Target path
		void setTargetPath(BWEB::Path& newPath) { targetPath = newPath; }
		BWEB::Path& getTargetPath() { return targetPath; }
		bool samePath() {
			return (targetPath.getTiles().front() == target->getTilePosition() && targetPath.getTiles().back() == tilePosition);
		}

		// Attack frame
		bool hasAttackedRecently() {
			return (Broodwar->getFrameCount() - lastAttackFrame < 50);
		}

		// Training frame
		void setRemainingTrainFrame(int newFrame) { remainingTrainFrame = newFrame; }
		int getRemainingTrainFrames() { return remainingTrainFrame; }

		// Stuck
		bool isStuck() {
			return (Broodwar->getFrameCount() - lastMoveFrame > 50);
		}

		// Creation frame
		void setCreationFrame() { frameCreated = Broodwar->getFrameCount(); }
		int getFrameCreated() { return frameCreated; }
		
		// Starcraft Stats
		double getPercentHealth()			{ return percentHealth; }				// Returns the units health percentage
		double getPercentShield()			{ return percentShield; }
		double getPercentTotal()			{ return percentTotal; }
		double getVisibleGroundStrength()	{ return visibleGroundStrength; }		// Returns the units visible ground strength		
		double getMaxGroundStrength()		{ return maxGroundStrength; }			// Returns the units max ground strength		
		double getVisibleAirStrength()		{ return visibleAirStrength; }			// Returns the units visible air strength		
		double getMaxAirStrength()			{ return maxAirStrength; }				// Returns the units max air strength
		double getGroundRange()				{ return groundRange; }					// Returns the units ground range including upgrades		
		double getAirRange()				{ return airRange; }					// Returns the units air range including upgrades				
		double getGroundDamage()			{ return groundDamage; }				// Returns the units ground damage (not including most upgrades)		
		double getAirDamage()				{ return airDamage; }					// Returns the units air damage (not including most upgrades)		
		double getSpeed()					{ return speed; }						// Returns the units movement speed in pixels per frame including upgrades	
		int getShields()					{ return shields; }
		int getHealth()						{ return health; }		
		int getLastAttackFrame()			{ return lastAttackFrame; }				// Returns the frame on which isStartingAttack was last true		
		int getMinStopFrame()				{ return minStopFrame; }				// Returns the minimum number of frames that the unit needs after a shot before another command can be issued		
		int getLastVisibleFrame()			{ return lastVisibleFrame; }			// Returns the last frame since this unit was visible
		int getEnergy()						{ return energy; }

		// McRave Stats
		double getPriority()				{ return priority; }					// Returns the units priority for targeting purposes based on strength (not including value)
		int getLocalStrategy()				{ return localStrategy; }				// Returns the units local strategy		
		int getGlobalStrategy()				{ return globalStrategy; }				// Returns the units global strategy		
		double getSimBonus()				{ return simBonus; }

		// Kill count
		int getKillCount()					{ return killCount; }
		void setKillCount(int newValue)		{ killCount = newValue; }

		// Debug circles
		void circleGreen()								{ Broodwar->drawCircleMap(position, unitType.width(), Colors::Green); }		
		void circleYellow()								{ Broodwar->drawCircleMap(position, unitType.width(), Colors::Yellow); }
		void circleRed()								{ Broodwar->drawCircleMap(position, unitType.width(), Colors::Red); }

		bool isBurrowed()								{ return burrowed; }
		Position getSimPosition()						{ return simPosition; }
		Unit unit()										{ return thisUnit; }		
		UnitType getType()								{ return unitType; }
		UnitType getBuildingType()						{ return buildingType; }
		Player getPlayer()								{ return player; }

		Position getPosition()							{ return position; }
		Position getEngagePosition()					{ return engagePosition; }
		Position getDestination()						{ return destination; }
		WalkPosition getWalkPosition()					{ return walkPosition; }
		TilePosition getTilePosition()					{ return tilePosition; }
		TilePosition getBuildPosition()					{ return buildPosition; }

		void setSimBonus(double newValue)				{ simBonus = newValue; }
		void setLocalStrategy(int newValue)				{ localStrategy = newValue; }
		void setGlobalStrategy(int newValue)			{ globalStrategy = newValue; }
		void setLastAttackFrame(int newValue)			{ lastAttackFrame = newValue; }

		void setUnit(Unit newUnit)						{ thisUnit = newUnit; }		
		void setType(UnitType newType)					{ unitType = newType; }
		void setBuildingType(UnitType newType)			{ buildingType = newType; }
		void setPlayer(Player newPlayer)				{ player = newPlayer; }

		void setSimPosition(Position newPosition)		{ simPosition = newPosition; }
		void setPosition(Position newPosition)			{ position = newPosition; }
		void setEngagePosition(Position newPosition)	{ engagePosition = newPosition; }
		void setDestination(Position newPosition)		{ destination = newPosition; }
		void setWalkPosition(WalkPosition newPosition)	{ walkPosition = newPosition; }
		void setTilePosition(TilePosition newPosition)	{ tilePosition = newPosition; }
		void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }
	};
}
