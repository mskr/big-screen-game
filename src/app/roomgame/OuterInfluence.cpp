#include "OuterInfluence.h"
#include <iostream>
namespace roomgame {
	const int PATROL = 0;
	const int ATTACK = 1;
	const int RETREAT = 2;
    const int ATTACK_CHANCE_BASE = 0;
    const float BASE_SPEED = 0.5f;
    const float ROT_SPEED_MULTIPLIER = 50.0f;

	OuterInfluence::OuterInfluence()
	{
		grid = nullptr;
		mode = 0;
		actionStatus = 0;
		oldPosition = glm::vec3(0, 0, 0);
		targetPosition = glm::vec3(2, 0, 0);
		posDiff = glm::vec3(0);
        viewPersMat = glm::mat4(1);
        speed = BASE_SPEED;
        attackChance = ATTACK_CHANCE_BASE;
        attackChanceGrowth = 1;
        unsigned seed1 = (unsigned) std::chrono::system_clock::now().time_since_epoch().count();
        rndGenerator = std::default_random_engine(seed1);
        distributor100 = std::uniform_int_distribution<int>(0, 100);
	}



	OuterInfluence::~OuterInfluence()
	{
	}

	void OuterInfluence::calcPositions(bool init = false) {
		for (int i = 0; i < 5; i++) {
            float tmpI = i + 0.1f;
            glm::mat4 transPat = glm::translate(glm::vec3(sinf(tmpI*(float)glfwGetTime()), cosf(tmpI*(float)glfwGetTime())*0.5f, cosf(tmpI*(float)glfwGetTime())*0.5f));
            glm::mat4 transAtt = glm::translate(glm::vec3(cosf(tmpI*(float)glfwGetTime())*0.05f, sinf(tmpI*(float)glfwGetTime())*0.1, cosf(tmpI*(float)glfwGetTime())*0.05f));
            glm::mat4 translation = glm::mix(transPat, transAtt, movementType);
            if (i<meshComponent->influencePositions_.size()) {
                meshComponent->influencePositions_[i] = meshComponent->model_matrix_*translation;
            }
            else {
				meshComponent->influencePositions_.push_back(meshComponent->model_matrix_*translation);
            }
		}
        if (mode == ATTACK) {
            movementType = min(movementType + ChangeSpeed*(float)deltaTime, 1);
            speed = min(speed + ChangeSpeed*(float)deltaTime, 1.3f);
        }
        else if (mode == RETREAT) {
            movementType = max(movementType - ChangeSpeed*(float)deltaTime, 0);
            speed = max(speed - ChangeSpeed*10*(float)deltaTime, 0.5f);
        }
	}

	void OuterInfluence::Update(double deltaTime)
	{
		this->deltaTime = deltaTime;
		Move();
		calcPositions();
        if (mode == ATTACK) {
            Attack();
        }
        else if (mode == RETREAT) {
            Retreat();
        }
	}

	//Change Position
	void OuterInfluence::Move() {
        if(mode==PATROL){
            meshComponent->transform(glm::translate(glm::vec3(-10, 0, 0)));
            meshComponent->transform(glm::rotate(glm::radians(speed*ROT_SPEED_MULTIPLIER*(float)deltaTime), glm::vec3(0, 0, 1)));
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
		case RETREAT:
			break;
		}

	}

	void OuterInfluence::Patrol() {
		//Move around in a circle until the attack decision (chance gets higher over time)
        int randNumber = distributor100(rndGenerator);
        //std::cout << randNumber << "/" << attackChance << std::endl;
        if (randNumber > 100-attackChance) {
            attackChance = ATTACK_CHANCE_BASE;
            oldPosition = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
			mode = ATTACK;
            //std::cout << "Changed mode to attack" << std::endl;
            ChooseTarget();
        }
        else {
            attackChance += attackChanceGrowth;
        }
	}

    void OuterInfluence::ChooseTarget() {
        //Set the closest cell with wall build state as moveTarget (move towards it until collison)
        glm::vec4 ndcCoords = glm::vec4(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], 0.0f, 1.0f);
        ndcCoords = viewPersMat * ndcCoords;
        ndcCoords = ndcCoords / ndcCoords.w;
        ndcCoords = glm::vec4(grid->pushNDCinsideGrid(glm::vec2(ndcCoords.x, ndcCoords.y)), ndcCoords.z, ndcCoords.w);
        GridCell* tmp = grid->getCellAt(glm::vec2(ndcCoords.x, ndcCoords.y));
        if (tmp == nullptr) {
            std::cout << "nullptr when choosing target" << std::endl;
            return;
        }
        float cellDistance = 9999.0f;
        GridCell* closestWallCell = nullptr;
        GridCell* leftLower = grid->getCellAt(0,0);
        GridCell* rightUpper = grid->getCellAt(grid->getNumRows()-1,grid->getNumColumns()-1);
        grid->forEachCellInRange(leftLower, rightUpper, [&](GridCell* cell) {
            if ((cell->getBuildState() & GridCell::WALL) != 0 && cell->getDistanceTo(tmp) < cellDistance && (cell->getBuildState() & GridCell::TEMPORARY) == 0) {
                cellDistance = cell->getDistanceTo(tmp);
                closestWallCell = cell;
            }
        });
        //closestWallCell = grid->getCellAt(0, 0);
        if (closestWallCell != nullptr) {
            targetPosition = glm::vec3(closestWallCell->getXPosition(), closestWallCell->getYPosition(), 0);
            targetPosition += grid->getTranslation();
            posDiff = targetPosition - glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], 0.0f);
            distance = glm::length(posDiff);
            targetCell = closestWallCell;
        }
        else {
            mode = PATROL;
            //std::cout << "Changed mode to patrol" << std::endl;
        }
    }

	void OuterInfluence::Attack() {
        //If the target Cell is reached, change to retreat mode and mark the cell as source
        glm::vec3 currentPos = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
        float dist = glm::distance(oldPosition, currentPos);
        //std::cout << "posdiff: " << posDiff.x << "," << posDiff.y << "," << posDiff.z << ",  " << dist << "/" << distance << std::endl;
        if (dist>distance) {
            targetPosition = oldPosition;
            oldPosition = currentPos;
            posDiff = targetPosition - currentPos;
            distance = glm::length(posDiff);
            mode = RETREAT;
            grid->buildAt(targetCell->getCol(),targetCell->getRow(),GridCell::SOURCE,InteractiveGrid::BuildMode::Additive);
            //grid->replaceRoompieceWith(targetCell->getCol(), targetCell->getRow(), GridCell::INSIDE_ROOM);
            //std::cout << "Changed mode to retreat" << std::endl;
        }
	}

	void OuterInfluence::Retreat() {
		//If back at original position, change to patrol mode
        glm::vec3 currentPos = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
        float dist = glm::distance(oldPosition, currentPos);
        //std::cout << "posdiff: " << posDiff.x << "," << posDiff.y << "," << posDiff.z << ",  " << dist << "/" << distance << std::endl;
        if (dist>distance) {
            mode = PATROL;
            //std::cout << "Changed mode to patrol" << std::endl;
        }
    }
}
