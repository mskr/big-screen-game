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

		// Geerbt über IUpdateable
		virtual void Update(double deltaTime) override;
		virtual void UpdateSlow(double deltaTime) override;
		void SetGridPointer(RoomInteractiveGrid* newGrid);
	private:
		SimpleGameMesh* meshComponent;
		RoomInteractiveGrid * grid = nullptr;
		int mode;
		float actionStatus;
		glm::vec3 position;
		glm::vec3 targetPosition;
		void DecideNextAction();
		void Patrol();
		void Attack();
		void Retreat();
		void Move();
	};
}
