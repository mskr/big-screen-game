#include "OuterInfluence.h"
namespace roomgame {
	const int PATROL = 0;
	const int ATTACK = 1;
	const int RETREAT = 2;

	OuterInfluence::OuterInfluence()
	{
		grid = nullptr;
		mode = 0;
		actionStatus = 0;
		oldPosition = glm::vec3(0, 0, 0);
		targetPosition = glm::vec3(2, 0, 0);
		posDiff = glm::vec3(0);
	}


	OuterInfluence::~OuterInfluence()
	{
	}

	void OuterInfluence::Update(double deltaTime)
	{
		this->deltaTime = deltaTime;
		actionStatus += 0.1f*(float)deltaTime;
		Move();
        if (mode == ATTACK) {
            GridCell* tmp = grid->getCellAt(glm::vec2(0, 0));
//            if (tmp->getBuildState == tmp->WALL_BOTTOM) {
//                tmp->setIsSource(true);
//            }
        }
	}

	//Change Position
	void OuterInfluence::Move() {
        if(mode==PATROL){
            glm::mat4 movMat = glm::mat4(1);
            movMat = glm::scale(movMat, glm::vec3(0.1, 0.1, 0.1));
            movMat = glm::translate(movMat, glm::vec3(0, 0, -2));
            movMat = glm::rotate(movMat, glm::radians(360 * actionStatus), glm::vec3(0, 0, 1));
            movMat = glm::translate(movMat, glm::vec3(10, 0, 0));
            meshComponent->model_matrix_ = movMat;
        }
        else {
            meshComponent->transform(glm::translate(glm::mat4(1), posDiff*(0.1f*(float)deltaTime)));
        }
	}

	void OuterInfluence::UpdateSlow(double deltaTime)
	{
		if (actionStatus > 1) {
			actionStatus = 0;
			oldPosition = targetPosition;
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
		if (mode < ATTACK) {
			mode = max(((rand() % 6) - 4), 0);
		}
		else {
			mode = (mode + 1) % 3;
		}
	}

	void OuterInfluence::Patrol() {

		//Move around randomly (a bit above ground)
		//std::srand((unsigned int)std::time(0));
		//float randX = ((float)(rand() % 100)) / 10.0f;
		//float randY = ((float)(rand() % 100)) / 10.0f;
		//targetPosition = glm::vec3(randX, randY, -1);
		//posDiff = glm::normalize(targetPosition - oldPosition);
	}

	void OuterInfluence::Attack() {
		//Set the closest cell with wall build state as moveTarget (move down and towards it until collison)
		//targetPosition = glm::vec3(grid->getClosestWallCell(glm::vec2(oldPosition))->getPosition(),-3);
		//posDiff = targetPosition - oldPosition;
        posDiff = (glm::vec3(0) - oldPosition)*2.0f;
		//targetPosition = glm::vec3(0, 0, 4);
	}

	void OuterInfluence::Retreat() {
		//Mark hit cell as source and get back higher and to the edge of the screen
		grid->getClosestWallCell(glm::vec2(oldPosition))->setIsSource(true);
		targetPosition = glm::vec3(0, 0, -1);
		posDiff = targetPosition - oldPosition;
	}
}
