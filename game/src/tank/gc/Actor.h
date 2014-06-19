// Actor.h

#pragma once

#include "Object.h"
#include "core/Grid.h"
#include <list>


#define GC_FLAG_ACTOR_KNOWNPOS      (GC_FLAG_OBJECT_ << 0)
#define GC_FLAG_ACTOR_              (GC_FLAG_OBJECT_ << 1)


class GC_Pickup;
class World;

class GC_Actor : public GC_Object
{
	vec2d _pos;

protected:
	virtual void Serialize(World &world, ObjectList::id_type id, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);
    
	int _locationX;
	int _locationY;
    virtual void EnterContexts(World &) {}
    virtual void LeaveContexts(World &) {}

public:
	const vec2d& GetPos() const { return _pos; }

	virtual void MoveTo(World &world, const vec2d &pos);
	virtual void OnPickup(World &world, GC_Pickup *pickup, bool attached); // called by a pickup
    
    virtual void Kill(World &world);
};


#define DECLARE_GRID_MEMBER()                                               \
    virtual void EnterContexts(World &world) override;                      \
    virtual void LeaveContexts(World &world) override;

#define IMPLEMENT_GRID_MEMBER(cls, grid)                                    \
    void cls::EnterContexts(World &world)                                   \
    {                                                                       \
        base::EnterContexts(world);                                         \
        world.grid.element(_locationX, _locationY).insert(this);            \
    }                                                                       \
    void cls::LeaveContexts(World &world)                                   \
    {                                                                       \
        auto &cell = world.grid.element(_locationX,_locationY);             \
        for (auto id = cell.begin(); id != cell.end(); id = cell.next(id))  \
        {                                                                   \
            if (cell.at(id) == this)                                        \
            {                                                               \
                cell.erase(id);                                             \
                base::LeaveContexts(world);                                 \
                return;                                                     \
            }                                                               \
        }                                                                   \
        assert(false);                                                      \
    }

// end of file

