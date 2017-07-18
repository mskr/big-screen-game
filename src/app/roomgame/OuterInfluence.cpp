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

	void OuterInfluence::calcPositions() {
		std::vector<glm::mat4> positions;
		for (int i = 0; i < 5; i++) {
			glm::mat4 translation = glm::translate(glm::vec3(sinf(i*(float)glfwGetTime()), 0, 0));
			if (mode == ATTACK) {
				translation *= 0.1f;
			}
			positions.push_back(meshComponent->model_matrix_*translation);
		}
	}

	void OuterInfluence::Update(double deltaTime)
	{
		this->deltaTime = deltaTime;
		Move();
		calcPositions();
	}

	//Change Position
	void OuterInfluence::Move() {
        if(mode==PATROL){
            meshComponent->transform(glm::translate(glm::vec3(-10, 0, 0)));
            meshComponent->transform(glm::rotate(glm::radians(speed), glm::vec3(0, 0, 1)));
            meshComponent->transform(glm::translate(glm::vec3(10, 0, 0)));
        }
        else{
            meshComponent->transform(glm::inverse(meshComponent->model_matrix_) * glm::translate((posDiff)*(speed*(float)deltaTime)) * meshComponent->model_matrix_);
        }
	}

    

	void OuterInfluence::UpdateSlow(double deltaTime)
	{
        
		//Change Behaviour, Change Target
        DecideNextAction();
        

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

	}

	void OuterInfluence::Patrol() {
		//Move around randomly (a bit above ground)
		std::srand((unsigned int)std::time(0));
        int randNumber = rand() % 100;
        if (randNumber > 80) {
            oldPosition = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
			mode = ATTACK;
			ChooseTarget();
        }
	}

    void OuterInfluence::ChooseTarget() {
        //Set the closest cell with wall build state as moveTarget (move down and towards it until collison)
        //targetPosition = glm::vec3(grid->getClosestWallCell(glm::vec2(oldPosition))->getPosition(),-3);
        //posDiff = targetPosition - oldPosition;
        glm::vec4 ndcCoords = glm::vec4(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], 0.0f, 1.0f);
        //            glm::vec4 ndcCoords = glm::vec4(1,1,0,1);
        ndcCoords = viewPersMat * ndcCoords;
        ndcCoords = ndcCoords / ndcCoords.w;
        ndcCoords = glm::vec4(grid->pushNDCinsideGrid(glm::vec2(ndcCoords.x, ndcCoords.y)), ndcCoords.z, ndcCoords.w);
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
        //closestWallCell = grid->getCellAt(63, 63);
        if (closestWallCell != nullptr) {
            targetPosition = glm::vec3(closestWallCell->getXPosition(), closestWallCell->getYPosition(), 0);
            targetPosition += grid->getTranslation();
			//glm::mat4 worldMat = meshComponent->model_matrix_
            posDiff = targetPosition - glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], 0.0f);
            //meshComponent->transform(glm::translate(posDiff*10.0f));
            //meshComponent->transform(glm::translate(glm::vec3(-10,0,0)));
            closestWallCell->setIsSource(true);
        }
        else {
            mode = PATROL;
        }
    }

	void OuterInfluence::Attack() {
        if (glm::distance(targetPosition, glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]))<0.5f) {
            glm::vec3 currentPos = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
            targetPosition = oldPosition;
            posDiff = targetPosition - currentPos;
            //meshComponent->transform(glm::translate(posDiff*10.0f));
            mode = RETREAT;
        }
	}

	void OuterInfluence::Retreat() {
		//Mark hit cell as source and get back higher and to the edge of the screen
        //grid->getClosestWallCell(glm::vec2(oldPosition))->setIsSource(true);
        
        //meshComponent->transform(glm::translate(posDiff));
        if (glm::distance(targetPosition, glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2])) < 0.5f) {
            mode = PATROL;
        }
    }
}
