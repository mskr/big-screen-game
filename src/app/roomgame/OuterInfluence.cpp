#include "OuterInfluence.h"
#include <iostream>
namespace roomgame {
	const int PATROL = 0;
	const int ATTACK = 1;
	const int RETREAT = 2;
    const int ATTACK_CHANCE_BASE = 0;
    const float BASE_SPEED = 0.5f;
    const float ROT_SPEED_MULTIPLIER = 50.0f;

	OuterInfluence::OuterInfluence(): MeshComponent(nullptr), distance_(0), targetCell_(nullptr), deltaTime_(0)
    {
        Grid = nullptr;
        mode_ = 0;
        actionStatus_ = 0;
        oldPosition_ = glm::vec3(0, 0, 0);
        targetPosition_ = glm::vec3(2, 0, 0);
        posDiff_ = glm::vec3(0);
        ViewPersMat = glm::mat4(1);
        speed_ = BASE_SPEED;
        attackChance_ = ATTACK_CHANCE_BASE;
        attackChanceGrowth_ = 2;
        const auto seed1 = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
        rndGenerator_ = std::default_random_engine(seed1);
        distributor100_ = std::uniform_int_distribution<int>(0, 100);
    }


    OuterInfluence::~OuterInfluence()
	{
	}

	void OuterInfluence::CalcPositions(bool init = false) {
		for (auto i = 0; i < 5; i++) {
		    const auto tmpI = i + 0.1f;
		    const auto transPat = glm::translate(glm::vec3(sinf(tmpI*static_cast<float>(glfwGetTime())), cosf(tmpI*static_cast<float>(glfwGetTime()))*0.5f, cosf(tmpI*static_cast<float>(glfwGetTime()))*0.5f));
		    const auto transAtt = glm::translate(glm::vec3(cosf(tmpI*static_cast<float>(glfwGetTime()))*0.05f, sinf(tmpI*static_cast<float>(glfwGetTime()))*0.1, cosf(tmpI*static_cast<float>(glfwGetTime()))*0.05f));
		    const auto translation = glm::mix(transPat, transAtt, movementType_);
            if (i<MeshComponent->influencePositions_.size()) {
                MeshComponent->influencePositions_[i] = MeshComponent->model_matrix_*translation;
            }
            else {
				MeshComponent->influencePositions_.push_back(MeshComponent->model_matrix_*translation);
            }
		}
        if (mode_ == ATTACK) {
            movementType_ = min(movementType_ + changeSpeed_*(float)deltaTime_, 1);
            speed_ = min(exp(speed_ + changeSpeed_*0.2f*(float)deltaTime_)-1.0f, 0.8f);
        }
        else if (mode_ == RETREAT) {
            movementType_ = max(movementType_ - changeSpeed_*(float)deltaTime_, 0);
            speed_ = min(speed_ + changeSpeed_*0.1f*(float)deltaTime_, 0.4f);
//            speed_ = max(speed_ - changeSpeed_*10*(float)deltaTime_, 0.5f);
        }else
        {
            speed_ = min(speed_ + changeSpeed_*0.1f*(float)deltaTime_, 0.4f);
        }
	}

	void OuterInfluence::Update(double deltaTime)
	{
		this->deltaTime_ = deltaTime;
		Move();
		CalcPositions();
        if (mode_ == ATTACK) {
            Attack();
        }
        else if (mode_ == RETREAT) {
            Retreat();
        }
	}

	//Change Position
	void OuterInfluence::Move() const
	{
        if(mode_==PATROL){
            auto rotateMat = translate(glm::vec3(-30, 0, 0));
            rotateMat *= rotate(glm::radians(speed_*ROT_SPEED_MULTIPLIER*static_cast<float>(deltaTime_)), glm::vec3(0, 0, 1));
            rotateMat *= translate(glm::vec3(30, 0, 0));
            MeshComponent->transform(rotateMat);
        }
        else{
            auto targetMovMat = inverse(MeshComponent->model_matrix_);
            targetMovMat *= translate((posDiff_)*(speed_*static_cast<float>(deltaTime_)));
            targetMovMat *= MeshComponent->model_matrix_;
            MeshComponent->transform(targetMovMat);
        }
	}

	void OuterInfluence::UpdateSlow(double deltaTime)
	{
        //If patrolling, check if Attack should be initiated
        if(mode_==PATROL)
        {
            CheckForPatrolEnd();
        }
    }

	void OuterInfluence::CheckForPatrolEnd() {
	    const auto randNumber = distributor100_(rndGenerator_);
        if (randNumber > 100-attackChance_) {
            attackChance_ = ATTACK_CHANCE_BASE;
            oldPosition_ = glm::vec3(MeshComponent->model_matrix_[3][0], MeshComponent->model_matrix_[3][1], MeshComponent->model_matrix_[3][2]);
			mode_ = ATTACK;
            ChooseTarget();
        }
        else {
            attackChance_ += attackChanceGrowth_;
        }
	}

    void OuterInfluence::ChooseTarget() {
        //Set the closest cell with wall build state as moveTarget (move towards it until collison)
        auto ndcCoords = glm::vec4(MeshComponent->model_matrix_[3][0], MeshComponent->model_matrix_[3][1], 0.0f, 1.0f);
        ndcCoords = ViewPersMat * ndcCoords;
        ndcCoords = ndcCoords / ndcCoords.w;
        ndcCoords = glm::vec4(Grid->pushNDCinsideGrid(glm::vec2(ndcCoords.x, ndcCoords.y)), ndcCoords.z, ndcCoords.w);
        auto tmp = Grid->getCellAt(glm::vec2(ndcCoords.x, ndcCoords.y));
        if (tmp == nullptr) {
            std::cout << "nullptr when choosing cell where Outer Influence is positioned" << std::endl;
            return;
        }
        auto cellDistance = 9999.0f;
        GridCell* closestWallCell = nullptr;
        const auto leftLower = Grid->getCellAt(0,0);
        const auto rightUpper = Grid->getCellAt(Grid->getNumRows()-1,Grid->getNumColumns()-1);
        Grid->forEachCellInRange(leftLower, rightUpper, static_cast<std::function<void(GridCell*)>>([&](GridCell* cell) {
            if ((cell->getBuildState() & GridCell::WALL) != 0 && cell->getDistanceTo(tmp) < cellDistance && (cell->getBuildState() & (GridCell::TEMPORARY|GridCell::SOURCE)) == 0) {
                cellDistance = cell->getDistanceTo(tmp);
                closestWallCell = cell;
            }
        }));
        if (closestWallCell != nullptr) {
            speed_ = 0;
            targetPosition_ = glm::vec3(closestWallCell->getXPosition(), closestWallCell->getYPosition(), 0);
            targetPosition_ += Grid->getTranslation();
            posDiff_ = targetPosition_ - glm::vec3(MeshComponent->model_matrix_[3][0], MeshComponent->model_matrix_[3][1], 0.0f);
            distance_ = glm::length(posDiff_);
            targetCell_ = closestWallCell;
        }
        else {
            mode_ = PATROL;
        }
    }

	void OuterInfluence::Attack() {
        //If the target Cell is reached, change to retreat mode_ and mark the cell as source
	    const auto currentPos = glm::vec3(MeshComponent->model_matrix_[3][0], MeshComponent->model_matrix_[3][1], MeshComponent->model_matrix_[3][2]);
	    const auto dist = glm::distance(oldPosition_, currentPos);
        if (dist>distance_) {
            speed_ = 0;
            targetPosition_ = oldPosition_;
            oldPosition_ = currentPos;
            posDiff_ = targetPosition_ - currentPos;
            distance_ = glm::length(posDiff_);
            mode_ = RETREAT;
            auto bs = targetCell_->getBuildState();
            Grid->buildAt(targetCell_->getCol(), targetCell_->getRow(), GridCell::SOURCE, InteractiveGrid::BuildMode::Additive);
            Grid->buildAt(targetCell_->getCol(), targetCell_->getRow(), GridCell::WALL, InteractiveGrid::BuildMode::RemoveSpecific);
            bs = targetCell_->getBuildState();
            const auto wPos = Grid->getWorldCoordinates(targetCell_->getPosition());
            MeshComponent->sourcePositions_.push_back(wPos);
        }
	}

	void OuterInfluence::Retreat() {
		//If back at original position, change to patrol mode_
	    const auto currentPos = glm::vec3(MeshComponent->model_matrix_[3][0], MeshComponent->model_matrix_[3][1], MeshComponent->model_matrix_[3][2]);
	    const auto dist = glm::distance(oldPosition_, currentPos);
        if (dist>distance_) {
            speed_ = 0;
            mode_ = PATROL;
        }
    }
}
