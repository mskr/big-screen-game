#pragma once
#include "app\roomgame\IUpdateable.h"
#include "app\roomgame\RoomInteractiveGrid.h"
#include "glm\gtx\transform.hpp"
#include <random>
#include <ctime>

namespace roomgame {
	class OuterInfluence : public roomgame::IUpdateable
	{
	public:
		OuterInfluence();
		~OuterInfluence();

		// Geerbt �ber IUpdateable
		virtual void Update(double deltaTime) override;
		virtual void UpdateSlow(double deltaTime) override;

        glm::mat4 viewPersMat;
		SynchronizedGameMesh* meshComponent;
		RoomInteractiveGrid* grid;
        std::vector<glm::mat4> positions;
	private:
        float speed;
        float distance;
        int attackChance;
        int attackChanceGrowth;
        std::default_random_engine rndGenerator;
        std::uniform_int_distribution<int> distributor100;
		int mode;
		double deltaTime;
		float actionStatus;
		glm::vec3 oldPosition;
		glm::vec3 targetPosition;
		glm::vec3 posDiff;
		void DecideNextAction();
		void calcPositions(bool init);
		void Patrol();
		void Attack();
		void Retreat();
		void Move();
        void ChooseTarget();
	};
}
