#pragma once
#include "app/roomgame/IUpdateable.h"
#include "app/roomgame/RoomInteractiveGrid.h"
#include <random>

namespace roomgame {
	class OuterInfluence : public roomgame::IUpdateable
	{
	public:
		OuterInfluence();
	    virtual ~OuterInfluence();

		// Geerbt über IUpdateable
	    void Update(double deltaTime) override;
	    void UpdateSlow(double deltaTime) override;

        // Modify the outer influence on the fly
        int getAttackChanceGrowth();
        void setAttackChanceGrowth(int newChance);

        float getBaseSpeed();
        void setBaseSpeed(float speed);

        void resetValues();

        glm::mat4 ViewPersMat;
		SynchronizedGameMesh* MeshComponent;
		RoomInteractiveGrid* Grid;
	private:
        float movementType_ = 0; // 0 is patrolling movement, 1 is attacking movement
        float changeSpeed_ = 1.f; // How fast the influence changes its movement pattern
        float speed_;
        float distance_;
        float baseSpeed_;
        int attackChance_;
        int attackChanceGrowth_;
        std::default_random_engine rndGenerator_;
        std::uniform_int_distribution<int> distributor100_;
        GridCell* targetCell_;
		int mode_;
		double deltaTime_;
		float actionStatus_;
		glm::vec3 oldPosition_;
		glm::vec3 targetPosition_;
		glm::vec3 posDiff_;
		void CalcPositions(bool init);
		void CheckForPatrolEnd();
		void Attack();
		void Retreat();
		void Move() const;
        void ChooseTarget();
	};
}
