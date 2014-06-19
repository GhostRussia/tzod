// Object.h

#pragma once

#include "GlobalListHelper.h"
#include "notify.h"
#include "ObjPtr.h"
#include "ObjectProperty.h"
#include "TypeSystem.h"
#include "core/SafePtr.h"
#include "core/MemoryManager.h"


class MapFile;
class SaveFile;

class GC_Object;
class World;

typedef PtrList<GC_Object> ObjectList;


#define DECLARE_SELF_REGISTRATION(cls)          \
    DECLARE_POOLED_ALLOCATION(cls)              \
    private:                                    \
        typedef cls __ThisClass;                \
        static ObjectType _sType;               \
        static bool __registered;               \
        static bool __SelfRegister();           \
    public:                                     \
        static ObjectType GetTypeStatic()       \
        {                                       \
            return _sType;                      \
        }                                       \
        virtual ObjectType GetType()            \
        {                                       \
            return _sType;                      \
        }                                       \
    private:


#define IMPLEMENT_SELF_REGISTRATION(cls)                           \
    IMPLEMENT_POOLED_ALLOCATION(cls)                               \
	ObjectType cls::_sType = RTTypes::Inst().RegType<cls>(#cls);   \
    bool cls::__registered = cls::__SelfRegister();                \
    bool cls::__SelfRegister()


class PropertySet : public RefCounted
{
	GC_Object       *_object;
	ObjectProperty   _propName;

protected:
	virtual void MyExchange(World &world, bool applyToObject);

public:
	PropertySet(GC_Object *object);

	GC_Object* GetObject() const;
	void LoadFromConfig();
	void SaveToConfig();
	void Exchange(World &world, bool applyToObject);

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);
};

////////////////////////////////////////////////////////////
// object flags

#define GC_FLAG_OBJECT_NAMED                  0x00000001
#define GC_FLAG_OBJECT_                       0x00000002


typedef void (GC_Object::*NOTIFYPROC) (World &world, ObjectList::id_type id, GC_Object *sender, void *param);

///////////////////////////////////////////////////////////////////////////////
class GC_Object
{
	GC_Object(const GC_Object&) = delete;
	GC_Object& operator = (const GC_Object&) = delete;

private:
	struct Notify
	{
		DECLARE_POOLED_ALLOCATION(Notify);

		Notify *next;
		ObjPtr<GC_Object>   subscriber;
		NOTIFYPROC           handler;
		NotifyType           type;
		bool IsRemoved() const
		{
			return !subscriber;
		}
		void Serialize(World &world, SaveFile &f);
		explicit Notify(Notify *nxt) : next(nxt) {}
	};


	//
	// attributes
	//

private:
	unsigned int           _flags;             // define various object properties

//    ObjectList::id_type _posLIST_objects;

	Notify *_firstNotify;
	int  _notifyProtectCount;

public:
	void SetFlags(unsigned int flags, bool value)
	{
		_flags = value ? (_flags|flags) : (_flags & ~flags);
	}
	unsigned int GetFlags() const
	{
		return _flags;
	}
	// return true if one of the flags is set
	bool CheckFlags(unsigned int flags) const
	{
		return 0 != (_flags & flags);
	}


	//
	// access functions
	//

public:
    DECLARE_LIST_MEMBER();

	GC_Object();
	virtual ~GC_Object();
    
//    ObjectList::id_type GetId() const { return _posLIST_objects; }


	//
	// operations
	//

protected:
	void PulseNotify(World &world, NotifyType type, void *param = NULL);

public:
	const char* GetName(World &world) const;
	void SetName(World &world, const char *name);

	void Subscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler);
	void Unsubscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler);


	//
	// serialization
	//

public:
	virtual void Serialize(World &world, ObjectList::id_type id, SaveFile &f);

public:
	virtual ObjectType GetType() = 0;


	//
	// properties
	//
protected:
	typedef PropertySet MyPropertySet;
	virtual PropertySet* NewPropertySet();

public:
	SafePtr<PropertySet> GetProperties(World &world);

	//
	// overrides
	//

public:
	virtual void Kill(World &world, ObjectList::id_type id);

	virtual void TimeStepFixed(World &world, ObjectList::id_type id, float dt);
	virtual void TimeStepFloat(World &world, ObjectList::id_type id, float dt);

	virtual void MapExchange(World &world, MapFile &f);


	//
	// debug helpers
	//

#ifdef NETWORK_DEBUG
public:
	virtual uint32_t checksum(void) const
	{
		return 0;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////
// end of file
