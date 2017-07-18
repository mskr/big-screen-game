#include "OuterInfluence.h"
#include <iostream>
namespace roomgame {
	const int PATROL = 0;
	const int ATTACK = 1;
	const int RETREAT = 2;
    const int ATTACK_CHANCE_BASE = 0;

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
        attackChance = ATTACK_CHANCE_BASE;
        attackChanceGrowth = 1;
        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        rndGenerator = std::default_random_engine(seed1);
        distributor100 = std::uniform_int_distribution<int>(0, 100);
        positions = std::vector<glm::mat4>();
	}



	OuterInfluence::~OuterInfluence()
	{
	}

	void OuterInfluence::calcPositions(bool init = false) {
		for (int i = 0; i < 5; i++) {
			glm::mat4 translation = glm::translate(glm::vec3(sinf(i*(float)glfwGetTime()), 0, 0));
			if (mode == ATTACK) {
				translation *= 0.1f;
			}
            if (i<positions.size()) {
                positions[i] = meshComponent->model_matrix_*translation;
            }
            else {
                positions.push_back(meshComponent->model_matrix_*translation);
            }
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
			//Attack();
			break;
		case RETREAT:
			//Retreat();
			break;
		}

	}

	void OuterInfluence::Patrol() {
		//Move around randomly (a bit above ground)
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
        //Set the closest cell with wall build state as moveTarget (move down and towards it until collison)
        //targetPosition = glm::vec3(grid->getClosestWallCell(glm::vec2(oldPosition))->getPosition(),-3);
        //posDiff = targetPosition - oldPosition;
        glm::vec4 ndcCoords = glm::vec4(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], 0.0f, 1.0f);
        //            glm::vec4 ndcCoords = glm::vec4(1,1,0,1);
        ndcCoords = viewPersMat * ndcCoords;
        ndcCoords = ndcCoords / ndcCoords.w;
        ndcCoords = glm::vec4(grid->pushNDCinsideGrid(glm::vec2(ndcCoords.x, ndcCoords.y)), ndcCoords.z, ndcCoords.w);
        GridCell* tmp = grid->getCellAt(glm::vec2(ndcCoords.x, ndcCoords.y));
        float cellDistance = 9999.0f;
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
                cell->getDistanceTo(tmp) < cellDistance
                ) {
                cellDistance = cell->getDistanceTo(tmp);
                closestWallCell = cell;
            }
        });
        //closestWallCell = grid->getCellAt(0, 0);
        if (closestWallCell != nullptr) {
            targetPosition = glm::vec3(closestWallCell->getXPosition(), closestWallCell->getYPosition(), 0);
            targetPosition += grid->getTranslation();
			//glm::mat4 worldMat = meshComponent->model_matrix_
            posDiff = targetPosition - glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], 0.0f);
            distance = glm::length(posDiff);
            //meshComponent->transform(glm::translate(posDiff*10.0f));
            //meshComponent->transform(glm::translate(glm::vec3(-10,0,0)));
            closestWallCell->setIsSource(true);
        }
        else {
            mode = PATROL;
            //std::cout << "Changed mode to patrol" << std::endl;
        }
    }

	void OuterInfluence::Attack() {
        glm::vec3 currentPos = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
        float dist = glm::distance(oldPosition, currentPos);
        //std::cout << "posdiff: " << posDiff.x << "," << posDiff.y << "," << posDiff.z << ",  " << dist << "/" << distance << std::endl;
        if (dist>distance) {
            targetPosition = oldPosition;
            oldPosition = currentPos;
            posDiff = targetPosition - currentPos;
            distance = glm::length(posDiff);
            //meshComponent->transform(glm::translate(posDiff*10.0f));
            mode = RETREAT;
            //std::cout << "Changed mode to retreat" << std::endl;
        }
	}

	void OuterInfluence::Retreat() {
		//Mark hit cell as source and get back higher and to the edge of the screen
        //grid->getClosestWallCell(glm::vec2(oldPosition))->setIsSource(true);
        
        //meshComponent->transform(glm::translate(posDiff));
        glm::vec3 currentPos = glm::vec3(meshComponent->model_matrix_[3][0], meshComponent->model_matrix_[3][1], meshComponent->model_matrix_[3][2]);
        float dist = glm::distance(oldPosition, currentPos);
        //std::cout << "posdiff: " << posDiff.x << "," << posDiff.y << "," << posDiff.z << ",  " << dist << "/" << distance << std::endl;
        if (dist>distance) {
            mode = PATROL;
            //std::cout << "Changed mode to patrol" << std::endl;
        }
    }
}
