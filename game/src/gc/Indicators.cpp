#include "TypeReg.h"
#include <gc/Indicators.h>
#include <gc/Macros.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <gc/SaveFile.h>
#include <MapFile.h>

IMPLEMENT_SELF_REGISTRATION(GC_SpawnPoint)
{
	ED_ACTOR("respawn_point", "obj_respawn",
	    0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, CELL_SIZE/2);
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_SpawnPoint, LIST_respawns);

GC_SpawnPoint::GC_SpawnPoint(vec2d pos)
  : GC_Actor(pos)
  , _team(0)
{
}

GC_SpawnPoint::GC_SpawnPoint(FromFile)
  : GC_Actor(FromFile())
{
}

void GC_SpawnPoint::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);
	f.Serialize(_team);
}

void GC_SpawnPoint::MapExchange(MapFile &f)
{
	GC_Actor::MapExchange(f);

	float dir = GetDirection().Angle();

	MAP_EXCHANGE_FLOAT(dir, dir, 0);
	MAP_EXCHANGE_INT(team, _team, 0);

	if( f.loading() )
	{
		SetDirection(vec2d(dir));
		if( _team > MAX_TEAMS-1 )
			_team = MAX_TEAMS-1;
	}
}


PropertySet* GC_SpawnPoint::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_SpawnPoint::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTeam( ObjectProperty::TYPE_INTEGER, "team" )
  , _propDir( ObjectProperty::TYPE_FLOAT, "dir" )
{
	_propTeam.SetIntRange(0, MAX_TEAMS - 1);
	_propDir.SetFloatRange(0, PI2);
}

int GC_SpawnPoint::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_SpawnPoint::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTeam;
	case 1: return &_propDir;
	}

	assert(false);
	return NULL;
}

void GC_SpawnPoint::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_SpawnPoint *tmp = static_cast<GC_SpawnPoint *>(GetObject());

	if( applyToObject )
	{
		tmp->_team = _propTeam.GetIntValue();
		tmp->SetDirection(vec2d(_propDir.GetFloatValue()));
	}
	else
	{
		_propTeam.SetIntValue(tmp->_team);
		_propDir.SetFloatValue(tmp->GetDirection().Angle());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_HideLabel)
{
	return true;
}

GC_HideLabel::GC_HideLabel(vec2d pos)
  : GC_Actor(pos)
{
}

GC_HideLabel::GC_HideLabel(FromFile)
  : GC_Actor(FromFile())
{
}
