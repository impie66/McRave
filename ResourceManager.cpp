#include "McRave.h"

void ResourceTrackerClass::update()
{
	clock_t myClock;
	double duration = 0.0;
	myClock = clock();	

	for (auto &r : Broodwar->neutral()->getUnits())
	{
		if (r && r->exists())
		{
			if (Grids().getBaseGrid(r->getTilePosition()) == 1)
			{
				if (r->getType().isMineralField() && r->getInitialResources() > 0 && myMinerals.find(r) == myMinerals.end())
				{
					storeMineral(r);
				}

				if (myGas.find(r) == myGas.end() && r->getType() == UnitTypes::Resource_Vespene_Geyser)
				{
					storeGas(r);
				}				
			}	
			else if (Grids().getBaseGrid(r->getTilePosition()) == 0)
			{
				removeResource(r);
			}
			if (r->getInitialResources() == 0 && r->getDistance(Terrain().getPlayerStartingPosition()) < 2560)
			{
				storeBoulder(r);
			}
		}
	}

	// Assume saturated so check happens
	minSat = true;
	for (auto &m : myMinerals)
	{
		if (m.first->exists())
		{
			m.second.setRemainingResources(m.first->getResources());				
		}
		if (minSat && m.second.getGathererCount() < 2)
		{
			minSat = false;
		}		
	}

	// Assume saturated again
	gasSat = true;
	for (auto &g : myGas)
	{
		if (g.first->exists())
		{
			g.second.setUnitType(g.first->getType());
			g.second.setRemainingResources(g.first->getResources());			
		}
		if (g.second.getGathererCount() < 3 && g.second.getType() != UnitTypes::Resource_Vespene_Geyser)
		{
			gasNeeded = 3 - g.second.getGathererCount();
			gasSat = false;
			break;
		}
	}

	duration = 1000.0 * (clock() - myClock) / (double)CLOCKS_PER_SEC;
	//Broodwar->drawTextScreen(200, 50, "Resource Manager: %d ms", duration);
}

void ResourceTrackerClass::storeMineral(Unit resource)
{	
	myMinerals[resource].setGathererCount(0);
	myMinerals[resource].setRemainingResources(resource->getResources());
	myMinerals[resource].setUnit(resource);
	myMinerals[resource].setClosestBase(resource->getClosestUnit(Filter::IsAlly && Filter::IsResourceDepot));
	myMinerals[resource].setUnitType(resource->getType());
	myMinerals[resource].setPosition(resource->getPosition());
	myMinerals[resource].setWalkPosition(Util().getWalkPosition(resource));
	myMinerals[resource].setTilePosition(resource->getTilePosition());
	return;
}

void ResourceTrackerClass::storeGas(Unit resource)
{
	myGas[resource].setGathererCount(0);
	myGas[resource].setRemainingResources(resource->getResources());
	myGas[resource].setUnit(resource);
	myGas[resource].setClosestBase(resource->getClosestUnit(Filter::IsAlly && Filter::IsResourceDepot));
	myGas[resource].setUnitType(resource->getType());
	myGas[resource].setPosition(resource->getPosition());
	myGas[resource].setWalkPosition(Util().getWalkPosition(resource));
	myGas[resource].setTilePosition(resource->getTilePosition());
	return;
}

void ResourceTrackerClass::storeBoulder(Unit resource)
{
	myBoulders[resource].setGathererCount(0);
	myBoulders[resource].setRemainingResources(resource->getResources());
	myBoulders[resource].setUnit(resource);
	myBoulders[resource].setClosestBase(resource->getClosestUnit(Filter::IsAlly && Filter::IsResourceDepot));
	myBoulders[resource].setUnitType(resource->getType());
	myBoulders[resource].setPosition(resource->getPosition());
	myBoulders[resource].setWalkPosition(Util().getWalkPosition(resource));
	myBoulders[resource].setTilePosition(resource->getTilePosition());
	return;
}

void ResourceTrackerClass::removeResource(Unit resource)
{
	// Remove dead resources
	if (myMinerals.find(resource) != myMinerals.end())
	{
		myMinerals.erase(resource);		
	}
	else if (myGas.find(resource) != myGas.end())
	{
		myGas.erase(resource);
	}
	else if (myBoulders.find(resource) != myBoulders.end())
	{
		myBoulders.erase(resource);
	}

	// Any workers that targeted that resource now have no target
	for (auto &worker : Workers().getMyWorkers())
	{
		if (worker.second.getTarget() == resource)
		{
			worker.second.setTarget(nullptr);
		}
	}
}
