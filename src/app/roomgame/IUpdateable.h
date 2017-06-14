#pragma once
namespace roomgame {
	__interface IUpdateable
	{
	public:
		void Update() = 0;
		void UpdateSlow() = 0;
	};
}
