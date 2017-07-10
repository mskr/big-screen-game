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
        viewPersMat = glm::mat4(1);
        speed = 0.5f;

	}


	OuterInfluence::~OuterInfluence()
	{
	}

	void OuterInfluence::Update(double deltaTime)
	{
		this->deltaTime = deltaTime;
		Move();
	}

	//Change Position
	void OuterInfluence::Move() {
        if(mode==PATROL){
            meshComponent->transform(glm::translate(glm::vec3(-10, 0, 0)));
            meshComponent->transform(glm::rotate(glm::radians(speed), glm::vec3(0, 0, 1)));
            meshComponent->transform(glm::translate(glm::vec3(10, 0, 0)));
        }
        else {
 //           meshComponent->transform(glm::translate(posDiff*(speed*(float)deltaTime)));
        }
	}

    

	void OuterInfluence::UpdateSlow(double deltaTime)
	{
        
		//Change Behaviour, Change Target
        if (mode==PATROL || glm::distance(targetPosition, glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]))<5) {
            oldPosition = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
            DecideNextAction();
        }
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
        //mode = (mode + 1) % 3;
        
        
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
        glm::vec4 ndcCoords = glm::vec4(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], 0.0f, 1.0f);
        //            glm::vec4 ndcCoords = glm::vec4(1,1,0,1);
        ndcCoords = viewPersMat * ndcCoords;
        ndcCoords = ndcCoords / ndcCoords.w;
        ndcCoords = glm::vec4(grid->pushNDCinsideGrid(glm::vec2(ndcCoords.x,ndcCoords.y)),ndcCoords.z,ndcCoords.w);
        GridCell* tmp = grid->getCellAt(glm::vec2(ndcCoords.x, ndcCoords.y));
        float distance = 9999.0f;
        GridCell* closestWallCell = nullptr;
        float range = 200.0f;
        glm::vec2 minCoords = grid->pushNDCinsideGrid(glm::vec2(ndcCoords.x - range, ndcCoords.y - range));
        glm::vec2 maxCoords = grid->pushNDCinsideGrid(glm::vec2(ndcCoords.x + range, ndcCoords.y + range));
        GridCell* leftLower = grid->getCellAt(minCoords);
        GridCell* rightUpper = grid->getCellAt(maxCoords);
        grid->forEachCellInRange(leftLower, rightUpper, [&](GridCell* cell) {
            if (
                (cell->getBuildState() == GridCell::BuildState::WALL_BOTTOM ||
                    cell->getBuildState() == GridCell::BuildState::WALL_RIGHT ||
                    cell->getBuildState() == GridCell::BuildState::WALL_LEFT ||
                    cell->getBuildState() == GridCell::BuildState::WALL_TOP) &&
                cell->getDistanceTo(tmp) < distance
                ) {
                distance = cell->getDistanceTo(tmp);
                closestWallCell = cell;
            }
        });
        if (closestWallCell != nullptr) {
            targetPosition = glm::vec3(closestWallCell->getXPosition(), closestWallCell->getYPosition(),0);
            posDiff = targetPosition-glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1],0.0f);
            meshComponent->transform(glm::translate(posDiff));
            closestWallCell->setIsSource(true);
        }
        else {
            mode = PATROL;
        }
		//targetPosition = glm::vec3(0, 0, 4);
	}

	void OuterInfluence::Retreat() {
		//Mark hit cell as source and get back higher and to the edge of the screen
        //grid->getClosestWallCell(glm::vec2(oldPosition))->setIsSource(true);
        
		targetPosition = glm::vec3(500, 0, 0);
		posDiff = targetPosition - oldPosition;
        //meshComponent->transform(glm::translate(posDiff));
    }
}
