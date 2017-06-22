#include "OuterInfluence.h"
namespace roomgame {
	const int PATROL = 0;
	const int ATTACK = 1;
	const int RETREAT = 2;

	OuterInfluence::OuterInfluence()
	{
		mode = 0;
		actionStatus = 0;
		position = glm::vec3(0, 0, 0);
		targetPosition = glm::vec3(2, 0, 0);
	}


	OuterInfluence::~OuterInfluence()
	{
	}

	void OuterInfluence::SetGridPointer(RoomInteractiveGrid* newGrid) {
		grid = newGrid;
	}

	void OuterInfluence::Update(double deltaTime)
	{
		actionStatus += 0.01*deltaTime;
		Move();
	}

	//Change Position
	void OuterInfluence::Move() {
		position = position*(1 - actionStatus) + targetPosition*actionStatus;
	}

	void OuterInfluence::UpdateSlow(double deltaTime)
	{
		if (actionStatus > 1) {
			actionStatus = 0;
			DecideNextAction();
		}
		//Change Behaviour, Change Target
	}

	void OuterInfluence::DecideNextAction() {
		switch (mode) {
		case PATROL:
			Patrol();
			break;
		case ATTACK:
			Attack();
			break;
		case RETREAT:
			Retreat();
			break;
		}
		mode = (mode + 1) % 3;
	}

	void OuterInfluence::Patrol() {
		//Move around randomly (a bit above ground)
		std::srand(std::time(0));
		targetPosition = glm::vec3(rand()%40, rand() % 40, 1);
	}

	void OuterInfluence::Attack() {
		//Set the closest cell with wall build state as moveTarget (move down and towards it until collison)
		targetPosition = glm::vec3(0, 0, 4);
	}

	void OuterInfluence::Retreat() {
		//Mark hit cell as source and get back higher and to the edge of the screen
		targetPosition = glm::vec3(30, 30, 1);
	}
}
