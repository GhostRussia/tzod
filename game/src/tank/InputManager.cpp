// InputManager.cpp

#include "InputManager.h"
#include "Controller.h"
#include "Config.h"
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>

InputManager::InputManager(World &world)
	: _world(world)
{
	_world.eWorld.AddListener(*this);
    g_conf.dm_profiles.eventChange = std::bind(&InputManager::OnProfilesChange, this);
    OnProfilesChange();
}

InputManager::~InputManager()
{
    g_conf.dm_profiles.eventChange = nullptr;
	_world.eWorld.RemoveListener(*this);
}

Controller* InputManager::GetController(GC_Player *player) const
{
	auto it = _controllers.find(player);
	return _controllers.end() != it ? it->second.second.get() : nullptr;
}

void InputManager::AssignController(GC_Player *player, std::string profile)
{
    std::unique_ptr<Controller> ctrl(new Controller());
    ctrl->SetProfile(profile.c_str());
    _controllers[player] = std::make_pair(std::move(profile), std::move(ctrl));
}

void InputManager::OnProfilesChange()
{
    for (auto &pcpair: _controllers)
    {
        pcpair.second.second->SetProfile(pcpair.second.first.c_str());
    }
}

void InputManager::OnKill(GC_Object &obj)
{
	if (GC_Player::GetTypeStatic() == obj.GetType())
	{
		_controllers.erase(static_cast<GC_Player *>(&obj));
	}
}


// end of file
