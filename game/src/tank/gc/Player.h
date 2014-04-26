// Player.h

#pragma once

#include "Service.h"
#include "network/ControlPacket.h"

#include <deque>


struct VehicleState;
class GC_Vehicle;


#define GC_FLAG_PLAYER_ISHUMAN     (GC_FLAG_SERVICE_ << 0)
#define GC_FLAG_PLAYER_            (GC_FLAG_SERVICE_ << 1)


class GC_Player
	: public GC_Service
{
	MemberOfGlobalList<LIST_players> _memberOf;

	float     _timeRespawn;

	int       _team;
	int       _score;
	std::string  _nick;
	std::string  _class;
	std::string  _skin;
	std::string  _vehname;
	std::string  _scriptOnDie;
	std::string  _scriptOnRespawn;

	ObjPtr<GC_Vehicle> _vehicle;
    
protected:
	VehicleState _ctrlState;

protected:
	class MyPropertySet : public GC_Service::MyPropertySet
	{
		typedef GC_Service::MyPropertySet BASE;

		ObjectProperty _propTeam;
		ObjectProperty _propScore;
		ObjectProperty _propNick;
		ObjectProperty _propClass;
		ObjectProperty _propSkin;
		ObjectProperty _propOnDie;
		ObjectProperty _propOnRespawn;
		ObjectProperty _propVehName;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

protected:
	virtual void OnRespawn();
	virtual void OnDie();

public:
	GC_Vehicle* GetVehicle() const { return _vehicle; }
	void SetControllerState(const VehicleState &vs);

	void SetSkin(const std::string &skin);
	const std::string& GetSkin() const { return _skin; }

	void SetNick(const std::string &nick);
	const std::string& GetNick() const { return _nick; }

	void SetClass(const std::string &c);
	const std::string& GetClass() const { return _class; }

	void SetTeam(int team);
	int GetTeam() const { return _team; }

	void SetScore(int score);
	int GetScore() const { return _score; }

    void SetIsHuman(bool isHuman) { SetFlags(GC_FLAG_PLAYER_ISHUMAN, isHuman); }
    bool GetIsHuman() const { return CheckFlags(GC_FLAG_PLAYER_ISHUMAN); }

public:
	GC_Player();
	GC_Player(FromFile);
	virtual ~GC_Player();
	void UpdateSkin();

    size_t GetIndex() const;

	// GC_Object
	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);
	virtual void TimeStepFixed(float dt);

private:
	void OnVehicleDestroy(GC_Object *sender, void *param);
	void OnVehicleKill(GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerLocal
	: public GC_Player
{
	DECLARE_SELF_REGISTRATION(GC_PlayerLocal);

	std::deque<VehicleState> _stateHistory;

public:
	GC_PlayerLocal();
	GC_PlayerLocal(FromFile);
	virtual ~GC_PlayerLocal();

	void StepPredicted(VehicleState &vs, float dt);

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
