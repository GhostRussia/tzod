#include "TypeReg.h"
#include "inc/gc/RigidBody.h"
#include "inc/gc/Particles.h"
#include "inc/gc/Player.h"
#include "inc/gc/Vehicle.h"
#include "inc/gc/WeapCfg.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "inc/gc/WorldEvents.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>
#include <cfloat>

IMPLEMENT_GRID_MEMBER(GC_RigidBodyStatic, grid_rigid_s);

GC_RigidBodyStatic::GC_RigidBodyStatic(vec2d pos)
  : GC_Actor(pos)
  , _health(1)
  , _health_max(1)
  , _radius(0)
  , _width(0)
  , _length(0)
{
}

GC_RigidBodyStatic::GC_RigidBodyStatic(FromFile)
  : GC_Actor(FromFile())
{
}

GC_RigidBodyStatic::~GC_RigidBodyStatic()
{
}

void GC_RigidBodyStatic::Init(World &world)
{
	GC_Actor::Init(world);
	if( GetPassability() > 0 )
		world._field.ProcessObject(this, true);
}

void GC_RigidBodyStatic::Kill(World &world)
{
	if( GetPassability() > 0 )
		world._field.ProcessObject(this, false);
    GC_Actor::Kill(world);
}

void GC_RigidBodyStatic::MoveTo(World &world, const vec2d &pos)
{
	if( GetPassability() > 0 )
		world._field.ProcessObject(this, false);
	
    GC_Actor::MoveTo(world, pos);

    if( GetPassability() > 0 )
		world._field.ProcessObject(this, true);
}

bool GC_RigidBodyStatic::CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection,
                                         vec2d &outEnterNormal, float &outEnter, float &outExit)
{
	assert(!std::isnan(lineCenter.x) && std::isfinite(lineCenter.x));
	assert(!std::isnan(lineCenter.y) && std::isfinite(lineCenter.y));
	assert(!std::isnan(lineDirection.x) && std::isfinite(lineDirection.x));
	assert(!std::isnan(lineDirection.y) && std::isfinite(lineDirection.y));

	float lineProjL = Vec2dDot(lineDirection, GetDirection());
	float lineProjW = Vec2dCross(lineDirection, GetDirection());
	float lineProjL_abs = fabs(lineProjL);
	float lineProjW_abs = fabs(lineProjW);

	//
	// project box to lineDirection axis
	//

	vec2d delta = GetPos() - lineCenter;
	float halfProjLine = lineProjL_abs * GetHalfWidth() + lineProjW_abs * GetHalfLength();
	if( fabs(Vec2dCross(delta, lineDirection)) > halfProjLine )
		return false;

	//
	// project lineDirection to box axes
	//

	float deltaDotDir = Vec2dDot(delta, GetDirection());
	if( fabs(deltaDotDir) > lineProjL_abs / 2 + GetHalfLength() )
		return false;

	float deltaCrossDir = Vec2dCross(delta, GetDirection());
	if( fabs(deltaCrossDir) > lineProjW_abs / 2 + GetHalfWidth() )
		return false;

	//
	// calc intersection point and normal
	//

	float signW = lineProjW > 0 ? 1.0f : -1.0f;
	float signL = lineProjL > 0 ? 1.0f : -1.0f;

	float b1 = deltaCrossDir * signW - GetHalfWidth();
	float b2 = deltaDotDir * signL - GetHalfLength();
	if( b1 * lineProjL_abs > b2 * lineProjW_abs )
	{
		outEnter = lineProjW_abs > std::numeric_limits<float>::epsilon() ? b1 / lineProjW_abs : 0;
		outEnterNormal = lineProjW > 0 ?
			vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
	}
	else
	{
		outEnter = lineProjL_abs > std::numeric_limits<float>::epsilon() ? b2 / lineProjL_abs : 0;
		outEnterNormal = lineProjL > 0 ? -GetDirection() : GetDirection();
	}

	float b1e = deltaCrossDir * signW + GetHalfWidth();
	float b2e = deltaDotDir * signL + GetHalfLength();
	if( b1e * lineProjL_abs < b2e * lineProjW_abs )
	{
		outExit = lineProjW_abs > std::numeric_limits<float>::epsilon() ? b1e / lineProjW_abs : 0;
	}
	else
	{
		outExit = lineProjL_abs > std::numeric_limits<float>::epsilon() ? b2e / lineProjL_abs : 0;
	}

	return true;
}

bool GC_RigidBodyStatic::CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection,
                                         vec2d &outWhere, vec2d &outNormal, float &outDepth)
{
	assert(!std::isnan(rectHalfSize.x) && std::isfinite(rectHalfSize.x));
	assert(!std::isnan(rectHalfSize.y) && std::isfinite(rectHalfSize.y));
	assert(!std::isnan(rectCenter.x) && std::isfinite(rectCenter.x));
	assert(!std::isnan(rectCenter.y) && std::isfinite(rectCenter.y));
	assert(!std::isnan(rectDirection.x) && std::isfinite(rectDirection.x));
	assert(!std::isnan(rectDirection.y) && std::isfinite(rectDirection.y));

	vec2d delta = GetPos() - rectCenter;
	float depth[4];

	//
	// project this to rectDirection axes
	//

	float projL = Vec2dDot(rectDirection, GetDirection());
	float projW = Vec2dCross(rectDirection, GetDirection());
	float projL_abs = fabs(projL);
	float projW_abs = fabs(projW);

	float halfProjRectL = projL_abs * GetHalfWidth() + projW_abs * GetHalfLength();
	float deltaCrossRectDir = Vec2dCross(delta, rectDirection);
	depth[0] = rectHalfSize.y + halfProjRectL - fabs(deltaCrossRectDir);
	if( depth[0] < 0 ) return false;


	float halfProjRectW = projW_abs * GetHalfWidth() + projL_abs * GetHalfLength();
	float deltaDotRectDir = Vec2dDot(delta, rectDirection);
	depth[1] = rectHalfSize.x + halfProjRectW - fabs(deltaDotRectDir);
	if( depth[1] < 0 ) return false;


	//
	// project rectDirection to this axes
	//
	
	float halfProjThisL = projW_abs * rectHalfSize.x + projL_abs * rectHalfSize.y;
	float deltaCrossDir = Vec2dCross(delta, GetDirection());
	depth[2] = GetHalfWidth() + halfProjThisL - fabs(deltaCrossDir);
	if( depth[2] < 0 ) return false;

	float halfProjThisW = projL_abs * rectHalfSize.x + projW_abs * rectHalfSize.y;
	float deltaDotDir = Vec2dDot(delta, GetDirection());
	depth[3] = GetHalfLength() + halfProjThisW - fabs(deltaDotDir);
	if( depth[3] < 0 ) return false;


	//
	// calculate penetration depth
	//

	outDepth = depth[0];
	unsigned int mdIndex = 0;
	for( int i = 1; i < 4; ++i )
	{
		if( depth[i] < outDepth )
		{
			outDepth = depth[i];
			mdIndex = i;
		}
	}
	
	// calc normal
	switch( mdIndex )
	{
	case 0:
		outNormal = deltaCrossRectDir > 0 ?
			vec2d(-rectDirection.y, rectDirection.x) : vec2d(rectDirection.y, -rectDirection.x);
		break;
	case 1:
		outNormal = deltaDotRectDir > 0 ? -rectDirection : rectDirection;
		break;
	case 2:
		outNormal = deltaCrossDir > 0 ?
			vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
		break;
	case 3:
		outNormal = deltaDotDir > 0 ? -GetDirection() : GetDirection();
		break;
	default:
		assert(false);
	}

//	int group = mdIndex/2;
//	int ingrp = mdIndex%2;
//	int neighbor = group*2 + 1 - ingrp


	// contact manifold

	float xx, xy, yx, yy;
	float sign;
	vec2d center;
	if( mdIndex < 2 )
	{
		xx = GetHalfLength()*GetDirection().x;
		xy = GetHalfLength()*GetDirection().y;
		yx = GetHalfWidth()*GetDirection().x;
		yy = GetHalfWidth()*GetDirection().y;
		center = GetPos();
		sign = -1;
	}
	else
	{
		xx = rectHalfSize.x*rectDirection.x;
		xy = rectHalfSize.x*rectDirection.y;
		yx = rectHalfSize.y*rectDirection.x;
		yy = rectHalfSize.y*rectDirection.y;
		center = rectCenter;
		sign = 1;
	}

	vec2d v[4] = 
	{
		vec2d(xx - yy, yx + xy),
		vec2d(xx + yy, -yx + xy),
		vec2d(-xx - yy, yx - xy),
		vec2d(-xx + yy, -yx - xy)
	};
	unsigned int idx;
	float mindot = FLT_MAX;
	for( unsigned int i = 0; i < 4; ++i )
	{
		float dot = sign*Vec2dDot(outNormal, v[i]);
		if( mindot > dot )
		{
			mindot = dot;
			idx = i;
		}
	}

	if( projL_abs < 1e-3 || projW_abs < 1e-3 )
	{
		// for edge-edge collision find second closest point
		mindot = FLT_MAX;
		unsigned int idx2;
		for( unsigned int i = 0; i < 4; ++i )
		{
			if( i != idx )
			{
				float dot = sign*Vec2dDot(outNormal, v[i]);
				if( mindot > dot )
				{
					mindot = dot;
					idx2 = i;
				}
			}
		}

		outWhere = center + (v[idx] + v[idx2]) * 0.5f;
	}
	else
	{
		outWhere = center + v[idx];
	}


	return true;
}

void GC_RigidBodyStatic::SetHealth(float cur, float max)
{
	assert(cur <= max);
	_health = cur;
	_health_max = max;
}

void GC_RigidBodyStatic::SetHealth(float hp)
{
	assert(hp <= _health_max);
	_health = hp;
}

void GC_RigidBodyStatic::SetHealthMax(float hp)
{
	assert(hp >= _health);
	_health_max = hp;
}

void GC_RigidBodyStatic::OnDestroy(World &world, const DamageDesc &dd)
{
	for( auto ls: world.eGC_RigidBodyStatic._listeners )
		ls->OnDestroy(*this, dd);
}

void GC_RigidBodyStatic::OnDamage(World &world, DamageDesc &dd)
{
}

void GC_RigidBodyStatic::TakeDamage(World &world, DamageDesc dd)
{
	if( CheckFlags(GC_FLAG_RBSTATIC_DESTROYED) )
	{
		return;
	}
	
	ObjPtr<GC_Object> watch(this);
	for( auto ls: world.eGC_RigidBodyStatic._listeners )
	{
		ls->OnDamage(*this, dd);
		if( !watch )
			return;
	}
	
	OnDamage(world, dd);

	if( dd.damage > 0 && _health_max > 0 )
	{
		SetHealth(GetHealth() - dd.damage);
		if( GetHealth() <= 0 )
		{
			SetFlags(GC_FLAG_RBSTATIC_DESTROYED, true);
			OnDestroy(world, dd);
			Kill(world);
		}
	}
}

void GC_RigidBodyStatic::SetSize(float width, float length)
{
	_width = width;
	_length = length;
	_radius = sqrt(width*width + length*length) / 2;
}

vec2d GC_RigidBodyStatic::GetVertex(int index) const
{
	float x, y;
	switch( index )
	{
		default: assert(false);
		case 0: x =  _length / 2; y =  _width / 2; break;
		case 1: x = -_length / 2; y =  _width / 2; break;
		case 2: x = -_length / 2; y = -_width / 2; break;
		case 3: x =  _length / 2; y = -_width / 2; break;
	}
	return vec2d(GetPos().x + x * GetDirection().x - y * GetDirection().y,
				 GetPos().y + x * GetDirection().y + y * GetDirection().x);
}

void GC_RigidBodyStatic::MapExchange(MapFile &f)
{
	GC_Actor::MapExchange(f);

	MAP_EXCHANGE_FLOAT(  health,     _health,     GetDefaultHealth());
	MAP_EXCHANGE_FLOAT(  health_max, _health_max, GetDefaultHealth());
	MAP_EXCHANGE_STRING( on_destroy, _scriptOnDestroy, "");
	MAP_EXCHANGE_STRING( on_damage,  _scriptOnDamage,  "");

	if( f.loading() )
	{
		_health = std::min(_health, _health_max);
	}
}

void GC_RigidBodyStatic::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_scriptOnDestroy);
	f.Serialize(_scriptOnDamage);
	f.Serialize(_health);
	f.Serialize(_health_max);
	f.Serialize(_radius);
	f.Serialize(_width);
	f.Serialize(_length);

	if( f.loading() && GetPassability() > 0 )
		world._field.ProcessObject(this, true);
}


PropertySet* GC_RigidBodyStatic::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_RigidBodyStatic::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propOnDestroy( ObjectProperty::TYPE_STRING,   "on_destroy"  )
  , _propOnDamage(  ObjectProperty::TYPE_STRING,   "on_damage"   )
  , _propHealth(    ObjectProperty::TYPE_FLOAT,    "health"      )
  , _propMaxHealth( ObjectProperty::TYPE_FLOAT,    "max_health"  )
{
	_propMaxHealth.SetFloatRange(0, 100000);
	_propHealth.SetFloatRange(0, 100000);
}

int GC_RigidBodyStatic::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 4;
}

ObjectProperty* GC_RigidBodyStatic::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propOnDestroy;
	case 1: return &_propOnDamage;
	case 2: return &_propHealth;
	case 3: return &_propMaxHealth;
	}

	assert(false);
	return NULL;
}

void GC_RigidBodyStatic::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_RigidBodyStatic *tmp = static_cast<GC_RigidBodyStatic *>(GetObject());

	if( applyToObject )
	{
		tmp->_scriptOnDestroy = _propOnDestroy.GetStringValue();
		tmp->_scriptOnDamage  = _propOnDamage.GetStringValue();
		tmp->SetHealth(std::min(_propMaxHealth.GetFloatValue(), _propHealth.GetFloatValue()),
                       _propMaxHealth.GetFloatValue());
	}
	else
	{
		_propHealth.SetFloatValue(tmp->GetHealth());
		_propMaxHealth.SetFloatValue(tmp->GetHealthMax());
		_propOnDestroy.SetStringValue(tmp->_scriptOnDestroy);
		_propOnDamage.SetStringValue(tmp->_scriptOnDamage);
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wall)
{
	ED_LAND("wall_brick", "obj_wall_brick",  2 );
	return true;
}

IMPLEMENT_GRID_MEMBER(GC_Wall, grid_walls);

GC_Wall::GC_Wall(vec2d pos)
  : GC_RigidBodyStatic(pos)
{
	SetHealth(50, 50);
	SetSize(CELL_SIZE, CELL_SIZE);
}

GC_Wall::GC_Wall(FromFile)
  : GC_RigidBodyStatic(FromFile())
{
}

GC_Wall::~GC_Wall()
{
}

static void RemoveCorner(Field &field, GC_RigidBodyStatic &obj, int corner)
{
	if (corner)
	{
		vec2d p = obj.GetPos() / CELL_SIZE;
		int x, y;
		switch( corner )
		{
			default:
				assert(false);
			case 1:
				x = int(p.x+1);
				y = int(p.y+1);
				break;
			case 2:
				x = int(p.x);
				y = int(p.y + 1);
				break;
			case 3:
				x = int(p.x + 1);
				y = int(p.y);
				break;
			case 4:
				x = int(p.x);
				y = int(p.y);
				break;
		}
		field(x, y).RemoveObject(&obj);
		if( 0 == x || 0 == y || field.GetX() - 1 == x || field.GetX() - 1 == y )
		{
			field(x, y)._prop = 0xFF;
		}
	}
}

void GC_Wall::Init(World &world)
{
	GC_RigidBodyStatic::Init(world); // adds all corners
	RemoveCorner(world._field, *this, GetCorner());
}

void GC_Wall::Kill(World &world)
{
	SetCorner(world, 0);
    GC_RigidBodyStatic::Kill(world);
}

static const vec2d angles[4] = {vec2d(5*PI4), vec2d(7*PI4), vec2d(PI4), vec2d(3*PI4)};

bool GC_Wall::CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection, vec2d &outEnterNormal, float &outEnter, float &outExit)
{
	assert(!std::isnan(lineCenter.x) && std::isfinite(lineCenter.x));
	assert(!std::isnan(lineCenter.y) && std::isfinite(lineCenter.y));
	assert(!std::isnan(lineDirection.x) && std::isfinite(lineDirection.x));
	assert(!std::isnan(lineDirection.y) && std::isfinite(lineDirection.y));

	unsigned int corner = GetCorner();
	if( corner )
	{
		vec2d delta = GetPos() - lineCenter;

		//
		// project lineDirection to triangle axes
		//

		float lineProjL = Vec2dDot(lineDirection, GetDirection());
		float lineProjL_abs = fabs(lineProjL);
		float deltaDotDir = Vec2dDot(delta, GetDirection());
		if( fabs(deltaDotDir) > lineProjL_abs / 2 + GetHalfLength() )
			return false;

		float lineProjW = Vec2dCross(lineDirection, GetDirection());
		float lineProjW_abs = fabs(lineProjW);
		float deltaCrossDir = Vec2dCross(delta, GetDirection());
		if( fabs(deltaCrossDir) > lineProjW_abs / 2 + GetHalfWidth() )
			return false;

		vec2d diagonal = Vec2dAddDirection(angles[corner-1], GetDirection());
		float halfDiag = sqrt(GetHalfWidth()*GetHalfWidth() + GetHalfLength()*GetHalfLength());
		float lineProjD = Vec2dDot(lineDirection, diagonal);
		float lineProjD_abs = fabs(lineProjD);
		float deltaDotDiagonal = Vec2dDot(delta, diagonal);
		if( fabs(deltaDotDiagonal + halfDiag / 2) > lineProjD_abs / 2 + halfDiag / 2 )
			return false;

		//
		// project triangle to lineDirection axis
		//

		float halfProjLineW = lineProjL_abs * GetHalfWidth();
		float halfProjLineL = lineProjW_abs * GetHalfLength();
		float halfProjLineD = lineProjD_abs * halfDiag;
		float halfProjLine = std::max(halfProjLineW, std::max(halfProjLineL, halfProjLineD));
		float dirLen = lineDirection.len();
		vec2d offset = diagonal * (halfProjLineW + halfProjLineL - halfProjLine);
		if( fabs(Vec2dCross(delta * dirLen + offset, lineDirection)) > halfProjLine * dirLen )
			return false;

		//
		// calc intersection points and normal
		//

		float signW = lineProjW > 0 ? 1.0f : -1.0f;
		float signL = lineProjL > 0 ? 1.0f : -1.0f;

		float bW = deltaCrossDir * signW - GetHalfWidth();
		float bL = deltaDotDir * signL - GetHalfLength();
		float bD = deltaDotDiagonal;

		if( lineProjD > 0 && bD * lineProjW_abs > bW * lineProjD_abs && bD * lineProjL_abs > bL * lineProjD_abs )
		{
			outEnter = lineProjD_abs > 0 ? bD / lineProjD_abs : 0;
			outEnterNormal = -diagonal;
		}
		else if( bW * lineProjL_abs > bL * lineProjW_abs )
		{
			outEnter = lineProjW_abs > 0 ? bW / lineProjW_abs : 0;
			outEnterNormal = lineProjW > 0 ?
				vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
		}
		else
		{
			outEnter = lineProjL_abs > 0 ? bL / lineProjL_abs : 0;
			outEnterNormal = lineProjL > 0 ? -GetDirection() : GetDirection();
		}

		float bWe = deltaCrossDir * signW + GetHalfWidth();
		float bLe = deltaDotDir * signL + GetHalfLength();
		float bDe = -deltaDotDiagonal;

		if( lineProjD < 0 && bDe * lineProjW_abs < bWe * lineProjD_abs && bDe * lineProjL_abs < bLe * lineProjD_abs )
		{
			outExit = lineProjD_abs > 0 ? bDe / lineProjD_abs : 0;
		}
		else if( bWe * lineProjL_abs < bLe * lineProjW_abs )
		{
			outExit = lineProjW_abs > 0 ? bWe / lineProjW_abs : 0;
		}
		else
		{
			outExit = lineProjL_abs > 0 ? bLe / lineProjL_abs : 0;
		}

		return true;
	}
	else
	{
		return GC_RigidBodyStatic::CollideWithLine(lineCenter, lineDirection, outEnterNormal, outEnter, outExit);
	}
}

bool GC_Wall::CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection, vec2d &outWhere, vec2d &outNormal, float &outDepth)
{
	assert(!std::isnan(rectHalfSize.x) && std::isfinite(rectHalfSize.x));
	assert(!std::isnan(rectHalfSize.y) && std::isfinite(rectHalfSize.y));
	assert(!std::isnan(rectCenter.x) && std::isfinite(rectCenter.x));
	assert(!std::isnan(rectCenter.y) && std::isfinite(rectCenter.y));
	assert(!std::isnan(rectDirection.x) && std::isfinite(rectDirection.x));
	assert(!std::isnan(rectDirection.y) && std::isfinite(rectDirection.y));

	unsigned int corner = GetCorner();
	if( corner )
	{
		vec2d delta = GetPos() - rectCenter;
		float depth[5];

		float projL = Vec2dDot(rectDirection, GetDirection());
		float projW = Vec2dCross(rectDirection, GetDirection());
		float projL_abs = std::fabs(projL);
		float projW_abs = std::fabs(projW);

		//
		// project rectDirection to this axes
		//

		float halfProjThisL = projW_abs * rectHalfSize.x + projL_abs * rectHalfSize.y;
		float deltaCrossDir = Vec2dCross(delta, GetDirection());
		depth[2] = GetHalfWidth() + halfProjThisL - fabs(deltaCrossDir);
		if( depth[2] < 0 ) return false;

		float halfProjThisW = projL_abs * rectHalfSize.x + projW_abs * rectHalfSize.y;
		float deltaDotDir = Vec2dDot(delta, GetDirection());
		depth[3] = GetHalfLength() + halfProjThisW - fabs(deltaDotDir);
		if( depth[3] < 0 ) return false;

		vec2d diagonal = Vec2dAddDirection(angles[corner-1], GetDirection());
		float halfDiag = sqrt(GetHalfWidth()*GetHalfWidth() + GetHalfLength()*GetHalfLength());
		float deltaDotDiag = Vec2dDot(delta, diagonal);
		float projLd = Vec2dDot(rectDirection, diagonal);
		float projWd = Vec2dCross(rectDirection, diagonal);
		float projLd_abs = fabs(projLd);
		float projWd_abs = fabs(projWd);
		float halfProjThisD = projLd_abs * rectHalfSize.x + projWd_abs * rectHalfSize.y;
		depth[4] = halfDiag/2 + halfProjThisD - fabs(deltaDotDiag + halfDiag/2);
		if( depth[4] < 0 ) return false;


		//
		// project this to rectDirection axes
		//

		float halfProjLineWp = projW_abs * GetHalfWidth();
		float halfProjLineLp = projL_abs * GetHalfLength();
		float halfProjLineDp = projLd_abs * halfDiag;
		float halfProjRectL = std::max(halfProjLineWp, std::max(halfProjLineLp, halfProjLineDp));
		vec2d offsetL = diagonal * (halfProjLineWp + halfProjLineLp - halfProjRectL);

		float deltaCrossRectDir = Vec2dCross(delta, rectDirection);
		depth[0] = rectHalfSize.y + halfProjRectL - fabs(Vec2dCross(delta+offsetL, rectDirection));
		if( depth[0] < 0 ) return false;


		float halfProjLineW = projL_abs * GetHalfWidth();
		float halfProjLineL = projW_abs * GetHalfLength();
		float halfProjLineD = projWd_abs * halfDiag;
		float halfProjRectW = std::max(halfProjLineW, std::max(halfProjLineL, halfProjLineD));
		vec2d offsetW = diagonal * (halfProjLineW + halfProjLineL - halfProjRectW);

		float deltaDotRectDir = Vec2dDot(delta, rectDirection);
		depth[1] = rectHalfSize.x + halfProjRectW - fabs(Vec2dDot(delta+offsetW, rectDirection));
		if( depth[1] < 0 ) return false;


		//
		// calculate penetration depth
		//

		outDepth = depth[0];
		unsigned int mdIndex = 0;
		for( int i = 1; i < 5; ++i )
		{
			if( depth[i] < outDepth )
			{
				outDepth = depth[i];
				mdIndex = i;
			}
		}
		
		// calc normal
		switch( mdIndex )
		{
		case 0:
			outNormal = deltaCrossRectDir > 0 ?
				vec2d(-rectDirection.y, rectDirection.x) : vec2d(rectDirection.y, -rectDirection.x);
			break;
		case 1:
			outNormal = deltaDotRectDir > 0 ? -rectDirection : rectDirection;
			break;
		case 2:
			outNormal = deltaCrossDir > 0 ?
				vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
			break;
		case 3:
			outNormal = deltaDotDir > 0 ? -GetDirection() : GetDirection();
			break;
		case 4:
			outNormal = -diagonal;
			break;
		default:
			assert(false);
		}


		// contact manifold
		float xx, xy, yx, yy;
		float sign;
		vec2d center;
		unsigned int vcount = 0;
		vec2d v[4];
		if( mdIndex < 2 )
		{
			xx = GetHalfLength()*GetDirection().x;
			xy = GetHalfLength()*GetDirection().y;
			yx = GetHalfWidth()*GetDirection().x;
			yy = GetHalfWidth()*GetDirection().y;
			center = GetPos();
			sign = -1;

			if(corner != 1) v[vcount++].Set(xx - yy, yx + xy);
			if(corner != 4) v[vcount++].Set(xx + yy, -yx + xy);
			if(corner != 2) v[vcount++].Set(-xx - yy, yx - xy);
			if(corner != 3) v[vcount++].Set(-xx + yy, -yx - xy);
		}
		else
		{
			xx = rectHalfSize.x*rectDirection.x;
			xy = rectHalfSize.x*rectDirection.y;
			yx = rectHalfSize.y*rectDirection.x;
			yy = rectHalfSize.y*rectDirection.y;
			center = rectCenter;
			sign = 1;

			vcount = 4;
			v[0].Set(xx - yy, yx + xy);
			v[1].Set(xx + yy, -yx + xy);
			v[2].Set(-xx - yy, yx - xy);
			v[3].Set(-xx + yy, -yx - xy);
		}

		unsigned int idx;
		float mindot = FLT_MAX;
		for( unsigned int i = 0; i < vcount; ++i )
		{
			float dot = sign*Vec2dDot(outNormal, v[i]);
			if( mindot > dot )
			{
				mindot = dot;
				idx = i;
			}
		}

		if( projLd_abs < 1e-3 || projWd_abs < 1e-3 || projL_abs < 1e-3 || projW_abs < 1e-3 )
		{
			// for edge-edge collision find second closest point
			mindot = FLT_MAX;
			unsigned int idx2;
			for( unsigned int i = 0; i < vcount; ++i )
			{
				if( i != idx )
				{
					float dot = sign*Vec2dDot(outNormal, v[i]);
					if( mindot > dot )
					{
						mindot = dot;
						idx2 = i;
					}
				}
			}

			outWhere = center + (v[idx] + v[idx2]) * 0.5f;
		}
		else
		{
			outWhere = center + v[idx];
		}

		return true;
	}
	else
	{
		return GC_RigidBodyStatic::CollideWithRect(rectHalfSize, rectCenter, rectDirection, outWhere, outNormal, outDepth);
	}
}

static unsigned int FlagsFromCornerIndex(unsigned int corner)
{
	assert(corner < 5);
	static const unsigned int flags[] = {
		0,
		GC_FLAG_WALL_CORNER_1,
		GC_FLAG_WALL_CORNER_2,
		GC_FLAG_WALL_CORNER_3,
		GC_FLAG_WALL_CORNER_4
	};
	return flags[corner];
}

void GC_Wall::MapExchange(MapFile &f)
{
	GC_RigidBodyStatic::MapExchange(f);
	int corner = GetCorner();
	int style = GetStyle();
	MAP_EXCHANGE_INT(corner, corner, 0);
	MAP_EXCHANGE_INT(style, style, 0);

	if( f.loading() )
	{
		SetFlags(FlagsFromCornerIndex(corner % 5), true);
		SetStyle(style % 4);
	}
}

void GC_Wall::Serialize(World &world, SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(world, f);

	if( f.loading() )
	{
		RemoveCorner(world._field, *this, GetCorner());
	}
}

void GC_Wall::OnDestroy(World &world, const DamageDesc &dd)
{
	for( int n = 0; n < 5; ++n )
	{
		world.New<GC_BrickFragment>(GetPos() + vrand(GetRadius()), vec2d(frand(100.0f) - 50, -frand(100.0f)));
	}
	world.New<GC_Particle>(GetPos(), SPEED_SMOKE, PARTICLE_SMOKE, frand(0.2f) + 0.3f);

	GC_RigidBodyStatic::OnDestroy(world, dd);
}

void GC_Wall::OnDamage(World &world, DamageDesc &dd)
{
	if( dd.damage >= DAMAGE_BULLET && GetHealthMax() > 0 )
	{
		vec2d v = dd.hit - GetPos();
		if( fabsf(v.x) > fabsf(v.y) )
		{
			v.x = v.x > 0 ? 50.0f : -50.0f;
			v.y = 0;
		}
		else
		{
			v.x = 0;
			v.y = v.y > 0 ? 50.0f : -50.0f;
		}
		v += vrand(25);

		world.New<GC_BrickFragment>(dd.hit, v);
	}
	GC_RigidBodyStatic::OnDamage(world, dd);
}

void GC_Wall::SetCorner(World &world, unsigned int index) // 0 means normal view
{
	// restore current corner
	vec2d p = GetPos() / CELL_SIZE;
	if( CheckFlags(GC_FLAG_WALL_CORNER_ALL) )
	{
		int x, y;
		switch( GetCorner() )
		{
		default:
			assert(false);
		case 1:
			x = int(p.x+1);
			y = int(p.y+1);
			break;
		case 2:
			x = int(p.x);
			y = int(p.y + 1);
			break;
		case 3:
			x = int(p.x + 1);
			y = int(p.y);
			break;
		case 4:
			x = int(p.x);
			y = int(p.y);
			break;
		}
		world._field(x, y).AddObject(this);
	}

	SetFlags(GC_FLAG_WALL_CORNER_ALL, false);
	SetFlags(FlagsFromCornerIndex(index), true);
	RemoveCorner(world._field, *this, GetCorner());
}

unsigned int GC_Wall::GetCorner(void) const
{
	switch( GetFlags() & GC_FLAG_WALL_CORNER_ALL )
	{
	default: assert(false);
	case 0:                     return 0;
	case GC_FLAG_WALL_CORNER_1: return 1;
	case GC_FLAG_WALL_CORNER_2: return 2;
	case GC_FLAG_WALL_CORNER_3: return 3;
	case GC_FLAG_WALL_CORNER_4: return 4;
	}
}

void GC_Wall::SetStyle(int style) // 0-3
{
	assert(style >= 0 && style < 4);
	static const int s[] = 
	{
		0, 
		GC_FLAG_WALL_STYLE_BIT_0,
		GC_FLAG_WALL_STYLE_BIT_1,
		GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1
	};
	SetFlags(GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1, false);
	SetFlags(s[style], true);
}

int GC_Wall::GetStyle() const
{
	switch( GetFlags() & (GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1) )
	{
	case GC_FLAG_WALL_STYLE_BIT_0:
		return 1;
	case GC_FLAG_WALL_STYLE_BIT_1:
		return 2;
	case GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1:
		return 3;
	}
	return 0;
}

PropertySet* GC_Wall::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Wall::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propCorner( ObjectProperty::TYPE_INTEGER,   "corner"  )
  , _propStyle(  ObjectProperty::TYPE_INTEGER,   "style"   )
{
	_propCorner.SetIntRange(0, 4);
	_propStyle.SetIntRange(0, 3);
}

int GC_Wall::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_Wall::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propCorner;
	case 1: return &_propStyle;
	}

	assert(false);
	return NULL;
}

void GC_Wall::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_Wall *tmp = static_cast<GC_Wall *>(GetObject());

	if( applyToObject )
	{
		tmp->SetCorner(world, _propCorner.GetIntValue());
		tmp->SetStyle(_propStyle.GetIntValue());
	}
	else
	{
		_propCorner.SetIntValue(tmp->GetCorner());
		_propStyle.SetIntValue(tmp->GetStyle());
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wall_Concrete)
{
	ED_LAND("wall_concrete", "obj_wall_concrete", 1 );
	return true;
}

GC_Wall_Concrete::GC_Wall_Concrete(vec2d pos)
  : GC_Wall(pos)
{
	SetSize(CELL_SIZE, CELL_SIZE);
}

void GC_Wall_Concrete::OnDamage(World &world, DamageDesc &dd)
{
	dd.damage = 0;
	GC_Wall::OnDamage(world, dd);
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Water)
{
	ED_LAND( "water", "obj_water", 0 );
	return true;
}

IMPLEMENT_GRID_MEMBER(GC_Water, grid_water);

GC_Water::GC_Water(vec2d pos)
  : GC_RigidBodyStatic(pos)
  , _tile(0)
{
	SetSize(CELL_SIZE, CELL_SIZE);
	SetFlags(GC_FLAG_RBSTATIC_TRACE0, true);
}

GC_Water::GC_Water(FromFile)
  : GC_RigidBodyStatic(FromFile())
{
}

GC_Water::~GC_Water()
{
}

void GC_Water::MoveTo(World &world, const vec2d &pos)
{
    if( CheckFlags(GC_FLAG_WATER_INTILE) )
        UpdateTile(world, false);
    GC_RigidBodyStatic::MoveTo(world, pos);
    UpdateTile(world, true);
    SetFlags(GC_FLAG_WATER_INTILE, true);
}

void GC_Water::Kill(World &world)
{
    if( CheckFlags(GC_FLAG_WATER_INTILE) )
        UpdateTile(world, false);
    GC_RigidBodyStatic::Kill(world);
}

void GC_Water::UpdateTile(World &world, bool flag)
{
	static char tile1[9] = {5, 6, 7, 4,-1, 0, 3, 2, 1};
	static char tile2[9] = {1, 2, 3, 0,-1, 4, 7, 6, 5};

	FRECT frect;
	frect.left   = (GetPos().x - CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;
	frect.top    = (GetPos().y - CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;
	frect.right  = (GetPos().x + CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;
	frect.bottom = (GetPos().y + CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;

    std::vector<ObjectList*> receive;
	world.grid_water.OverlapRect(receive, frect);

	for( auto rit = receive.begin(); rit != receive.end(); ++rit )
	{
        ObjectList *ls = *rit;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			GC_Water *object = (GC_Water *) ls->at(it);
			if( this == object ) continue;

			vec2d dx = (GetPos() - object->GetPos()) / CELL_SIZE;
			if( dx.sqr() < 2.5f )
			{
				int x = int(dx.x + 1.5f);
				int y = int(dx.y + 1.5f);

				object->SetTile(tile1[x + y * 3], flag);
				SetTile(tile2[x + y * 3], flag);
			}
		}
	}
}

void GC_Water::Serialize(World &world, SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(world, f);
	f.Serialize(_tile);
}

void GC_Water::SetTile(char nTile, bool value)
{
	assert(0 <= nTile && nTile < 8);

	if( value )
		_tile |= (1 << nTile);
	else
		_tile &= ~(1 << nTile);
}

void GC_Water::OnDamage(World &world, DamageDesc &dd)
{
	dd.damage = 0;
}

// end of file
