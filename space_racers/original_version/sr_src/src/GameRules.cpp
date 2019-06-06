///functions for vehicle manager which define how game is updated///
///for a specific set of rules///
#include <Game/VehicleManager.h>

using namespace Game;

namespace
{
	struct vehicleSort
	{
		 bool operator () (const Vehicle* lhv, const Vehicle* rhv)
		{
			return lhv->GetCurrentDistance() > rhv->GetCurrentDistance();
		}
	};
}

void VehicleManager::m_UpdateSplitScreen(float dt, tmx::MapLoader& map)
{
	//update quadtree with entire map as cars may be anywhere
	map.UpdateQuadTree(sf::FloatRect(sf::Vector2f(), static_cast<sf::Vector2f>(map.GetMapSize())));

	//so we can stash for sorting race position
	std::vector<Vehicle*> vehicles; //TODO ought to rename this as it appears ambiguous with m_vehicles

	//update each vehicle
	for(auto&& vehicle : m_vehicles)
	{
		vehicle->Update(dt, map.QueryQuadTree(vehicle->GetSprite().getGlobalBounds()));

		if(m_gameData.CurrentRoundState == Racing) //we do this because input is also disabled during respawn
		{
			//enable input if count in finished
			vehicle->EnableInput(m_gameData.RaceStarted);

			//collide with other cars
			for(auto&& otherVehicle : m_vehicles)
			{
				if(vehicle == otherVehicle) continue; //skip testing ourself
				vehicle->Impact(dt, *otherVehicle);
			}
		}
		//store a pointer to sort race positions
		vehicles.push_back(vehicle.get());

		//update global data used by HUD
		m_gameData.Players[vehicle->Ident & UID]->CurrentDistance = vehicle->GetLapDistance();
	}

	//update each view for each vehicle
	for(auto i = 0u; i < m_vehicles.size(); ++i)
	{
		m_UpdateCamera(dt, m_vehicles[i]->GetCentre(), i);
	}

	//sort the cars by distance
	std::sort(vehicles.begin(), vehicles.end(), vehicleSort());
		//and update their position
	for(sf::Uint8 i = 0; i < vehicles.size(); ++i)
	{
		vehicles[i]->SetRacePosition(i + 1);
	}


	//update round status
	switch(m_gameData.CurrentRoundState)
	{
	default: break;
	case Racing:
		if(m_raceOver) break;
		{
			sf::Uint8 lap = vehicles[0]->GetCurrentLap();

			m_gameData.CurrentLap = lap;
			if(m_gameData.CurrentLap > m_gameData.MaxLaps)
			{
				//end the race. 
				m_StartCelebrating(vehicles[0]->Ident & UID);

				//Kill other cars
				for(auto i = 1u; i < vehicles.size(); ++i)
				{
					vehicles[i]->Kill(Eliminated);
				}
			}
		}
		break;
	case RoundEnd:
		//end race on time out
		if(m_roundTimer.getElapsedTime().asSeconds() > m_roundEndDuration
			&& !m_raceOver)
		{
			m_gameData.CurrentRoundState = RaceWon;
			m_audioManager.PlayEffect(Audio::RaceWin);
			m_audioManager.StopMusic();

			//award points
			m_gameData.Players[vehicles[0]->Ident & UID]->RaceWins++;
			for(auto&& p : m_vehicles)
			{
				m_gameData.Players[p->Ident & UID]->Score += ((m_vehicles.size() - p->GetRacePosition()) * 20u);
			}
		}
		break;
	case RaceWon:
		
		break;
	}
}

void VehicleManager::m_UpdateTimeTrial(float dt, tmx::MapLoader& map)
{
	//must update quad tree first
	map.UpdateQuadTree(Helpers::Rectangles::GetViewRect(m_screenData.RaceViews[0]));
	
	for(auto&& vehicle : m_vehicles)
	{
		vehicle->Update(dt, map.QueryQuadTree(vehicle->GetSprite().getGlobalBounds()));

		if(m_gameData.CurrentRoundState == Racing) //we do this because input is also disabled during respawn
		{
			//enable input if count in finished
			vehicle->EnableInput(m_gameData.RaceStarted);
		}
		//update global data used by HUD
		m_gameData.Players[vehicle->Ident & UID]->CurrentDistance = vehicle->GetLapDistance();
	}

	//start lap timer when count in finished
	if(m_gameData.RaceStarted != m_prevRaceStarted) m_lapTimer.restart();
	m_prevRaceStarted = m_gameData.RaceStarted;

	//only check time of first player, as any other cars that exist will be network ghosts
	m_UpdateCamera(dt, m_vehicles[0]->GetSprite().getPosition());

	switch (m_gameData.CurrentRoundState)
	{
	case Racing:
		{
			if(m_raceOver) break;
			//update data for HUD
			sf::Uint8 lap = m_vehicles[0]->GetCurrentLap();
			m_gameData.CurrentLapTime = (m_gameData.RaceStarted) ? m_lapTimer.getElapsedTime().asSeconds() : 1000.f;

			if(lap > m_gameData.CurrentLap)
			{
				//we crossed the lap line
				m_lapTimer.restart();
				if(m_gameData.CurrentLapTime < m_gameData.BestLapTime)
					m_gameData.BestLapTime = m_gameData.CurrentLapTime;
			}
		
			m_gameData.CurrentLap = lap;
			if(m_gameData.CurrentLap > m_gameData.MaxLaps)
			{
				//end the race. TODO: wait for all cars to finish in a network game
				m_StartCelebrating(m_vehicles[0]->Ident & UID);
			}
		}
		break;
	case RoundEnd:
		//end race on time out
		if(m_roundTimer.getElapsedTime().asSeconds() > m_roundEndDuration
			&& !m_raceOver)
		{
			m_gameData.CurrentRoundState = RaceWon;
			m_audioManager.PlayEffect(Audio::RaceWin);
			m_audioManager.StopMusic();
		}
		break;
	case RaceWon:
		
		break;
	default:
		break;
	}
}

void VehicleManager::m_UpdateLocalMulti(float dt, tmx::MapLoader& map)
{
	//so we can stash for sorting race position
	std::vector<Vehicle*> vehicles;
	sf::Uint8 lastPlace, elimCount = 0;
	sf::Uint8 deadCount = 0;

	map.UpdateQuadTree(Helpers::Rectangles::GetViewRect(m_screenData.RaceViews[0]));

	for(auto&& vehicle : m_vehicles)
	{
		vehicle->Update(dt, map.QueryQuadTree(vehicle->GetSprite().getGlobalBounds()));

		if(m_gameData.CurrentRoundState == Racing)
		{
			//enable input if count in finished
			vehicle->EnableInput(m_gameData.RaceStarted);
			
			//collide with other cars
			for(auto&& otherVehicle : m_vehicles)
			{
				if(vehicle == otherVehicle) continue; //skip testing ourself
				vehicle->Impact(dt, *otherVehicle);
			}

			//eliminate stragglers
			if(vehicle->GetRacePosition() != 1u 
				&& !m_playerBounds.contains(vehicle->GetCentre())
				&& vehicle->GetState() == Alive//don't eliminate falling cars
				&& m_resetTimer.getElapsedTime().asSeconds() > m_resetTime) //allow time to re-centre if all cars are killed
			{
					vehicle->Kill(Eliminated);
					//m_audioManager.PlayEffect(Audio::Eliminated, m_screenData.RaceView.getCenter());
			}
		}
		vehicles.push_back(vehicle.get());
		//update shared data for HUD
		m_gameData.Players[vehicle->Ident & UID]->CurrentDistance = vehicle->GetLapDistance();
		m_gameData.Players[vehicle->Ident & UID]->Active = (vehicle->GetState() != Eliminated);

		//count eliminated vehicles incase we get stuck when all 4 are eliminated
		State state = vehicle->GetState();
		if(state == Eliminated)
			elimCount++;
		else if (state == Dead || state == Dying)
			deadCount++;

	}

	//sort the cars by distance
	std::sort(vehicles.begin(), vehicles.end(), vehicleSort());
	//and update their position
	for(sf::Uint8 i = 0; i < vehicles.size(); ++i)
	{
		vehicles[i]->SetRacePosition(i + 1);
		if(vehicles[i]->GetState() != Eliminated)
			lastPlace = i;
	}

	//reset cars if all eliminated
	if(elimCount == m_vehicles.size())
	{
		m_ResetAll(vehicles[0]);
	}
	//give cars time to respawn if everyone is dead but not yet eliminated
	if(deadCount + elimCount == m_vehicles.size())
		m_resetTimer.restart();


	switch(m_gameData.CurrentRoundState)
	{
	default:
	case Racing:
		if(m_raceOver) break;
		//set sudden death if on last lap
		if(vehicles[0]->GetCurrentLap() == m_gameData.MaxLaps
			&& !m_gameData.SuddenDeath)
		{
			//---sudden death---//			
			m_gameData.SuddenDeath = true;							
			//if(!m_altRules)//put everyone on 1 point to win
			//{
			//	for(auto player : m_gameData.Players)
			//	{
			//		player->RoundWins = m_gameData.RoundsToWin - 1;
			//	}
			//}			
		}

		//end race if leader more than max laps, or tried to cheat by going backwards
		if(m_gameData.SuddenDeath /*&& m_altRules*/) //in 2 player mode
		{
			sf::Uint8 currentLap = vehicles[0]->GetCurrentLap();
			if(currentLap > m_gameData.MaxLaps ||
				currentLap < m_gameData.MaxLaps - 1)
			{
				//find who has highest score
				sf::Uint8 currentId = 0u, currentHigh = 0u;
				for(auto i = 0u; i < m_gameData.Players.size(); ++i)
				{
					if(m_gameData.Players[i]->RoundWins > currentHigh)
					{
						currentHigh = m_gameData.Players[i]->RoundWins;
						currentId = i;
					}
					//if we have potential draw then tie break by distance
					if(m_gameData.Players[i]->RoundWins == currentHigh)
					{
						if(m_vehicles[i]->GetCurrentDistance() > m_vehicles[currentId]->GetCurrentDistance())
							currentId = i;
					}
				}

				//and kill others
				for(auto i = 0u; i < m_vehicles.size(); ++i)
				{
					if(i != currentId)
						m_vehicles[i]->Kill(Eliminated);
				}

				//and set to race end
				m_StartCelebrating(currentId);
			}
		}

		//end round if lastCar is first car
		if(lastPlace == 0 && m_resetTimer.getElapsedTime().asSeconds() > m_resetTime)
		{
			if(vehicles[lastPlace]->GetState() == Alive)
			{
				m_StartCelebrating(vehicles[lastPlace]->Ident & UID);
			}
		}

		//update global data with current lap
		m_gameData.CurrentLap = vehicles[0]->GetCurrentLap();

		{
			//moves view if leader is moving out of bounds 
			sf::Vector2f dest = (m_raceOver)? m_cameraDestination : vehicles[0]->GetCentre();
			m_UpdateCamera(dt, dest);
		}
		break;

	case RoundEnd:
		{
			//move camera to round winner	
			m_UpdateCamera(dt, m_cameraDestination);
		}
		//check if we've timed out
		if(m_roundTimer.getElapsedTime().asSeconds() > m_roundEndDuration)
		{		
			//update score
			sf::Uint8 ident = vehicles[0]->Ident & UID;
			m_gameData.Players[ident]->RoundWins++;

			//if alt rules subtract from other player
			if(m_altRules)
			{
				//vehicles[1]->Ident & UID;
				m_gameData.Players[vehicles[1]->Ident & UID]->RoundWins--;
			}

			//change state
			if(m_gameData.Players[ident]->RoundWins >= m_gameData.RoundsToWin
				|| (m_gameData.SuddenDeath && m_gameData.CurrentLap >= m_gameData.MaxLaps + 1))
			{
				m_gameData.CurrentRoundState = RaceWon;

				//add score based on number of rounds won
				for(auto& p : m_gameData.Players)
				{
					p->Score += p->RoundWins * 10u;
				}

				//and notch up a win
				m_gameData.Players[ident]->RaceWins++;
				m_audioManager.PlayEffect(Audio::RaceWin);
				m_audioManager.StopMusic();
			}
			else
			{	
				m_ResetAll(vehicles[0]);
				m_starSystem.Stop();
			}
		}
		break;
	case RoundReset:
		//move view back to node position and set round to race
		//if no one has won
		{
			if(m_UpdateCamera(dt, m_resetTarget)) m_gameData.CurrentRoundState = Racing;
		}
		break;
	case RaceWon:

		break;
	}

	//always update player bounds
	m_SetPlayerBounds(m_screenData.RaceViews[0].getCenter());
}
