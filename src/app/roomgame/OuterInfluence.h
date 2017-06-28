#pragma once
#include "app\roomgame\IUpdateable.h"
#include "app\roomgame\RoomInteractiveGrid.h"
#include "glm\matrix.hpp"
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


		SimpleGameMesh* meshComponent;
		RoomInteractiveGrid* grid;
	private:
		int mode;
		double deltaTime;
		float actionStatus;
		glm::vec3 oldPosition;
		glm::vec3 targetPosition;
		glm::vec3 posDiff;
		void DecideNextAction();
		void Patrol();
		void Attack();
		void Retreat();
		void Move();
	};
}
