#include "UpdateManager.h"
namespace roomgame {

	UpdateManager::UpdateManager()
	{
		timer = 0;
		max_time = 1;
	}


	UpdateManager::~UpdateManager()
	{
		updateables.clear();
	}

	void UpdateManager::ManageUpdates(double deltaTime, bool master)
	{
		if (timer > max_time && master) {
			for (std::shared_ptr<IUpdateable> upd : updateables)
			{
				upd.get()->Update();
				upd.get()->UpdateSlow();
			}
			timer = 0;
		}
		else {
			for (std::shared_ptr<IUpdateable> upd : updateables)
			{
				upd.get()->Update();
			}
		}
		if (master) {
			timer += deltaTime;
		}
	}

	void UpdateManager::AddUpdateable(std::shared_ptr<IUpdateable> obj)
	{
		updateables.push_back(obj);
	}

	void UpdateManager::RemoveUpdateable(std::shared_ptr<IUpdateable> obj)
	{
		updateables.erase(std::remove(updateables.begin(), updateables.end(), obj), updateables.end());
	}

}
