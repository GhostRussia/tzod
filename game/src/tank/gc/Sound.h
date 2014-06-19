// Sound.h

#pragma once

#include "Actor.h"

#include <SoundTemplates.h>

/////////////////////////////////////////////////////////////

enum enumSoundMode
{
    SMODE_UNKNOWN = 0,
    SMODE_PLAY,  // the GC_Soung object will be destroyed at the end
    SMODE_LOOP,
    SMODE_STOP,  // pause with the resource releasing
    SMODE_WAIT,  // forced pause
};

class GC_Sound : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Sound);
    typedef GC_Actor base;

	enumSoundTemplate   _soundTemplate;
#ifndef NOSOUND
    unsigned int _source;
#endif
    
protected:
	bool          _freezed;
	enumSoundMode _mode;
    float _speed;

public:
	float _volume;  // 0 - min;  1 - max

public:
    DECLARE_LIST_MEMBER();
	GC_Sound(World &world, enumSoundTemplate sound);
	GC_Sound(FromFile);
	virtual ~GC_Sound();
    virtual void Kill(World &world, ObjectList::id_type id);
	virtual void Serialize(World &world, ObjectList::id_type id, SaveFile &f);

	void KillWhenFinished(World &world, ObjectList::id_type id);
	virtual void MoveTo(World &world, ObjectList::id_type id, const vec2d &pos) override;

	void SetMode(World &world, enumSoundMode mode);
	void Pause(World &world, bool pause);
	void Freeze(bool freeze);

	void SetSpeed(float speed);
	void SetVolume(float vol);
	void UpdateVolume();  // should be called each time the conf.s_volume changes

public:
	static int _countMax;
	static int _countActive;
	static int _countWaiting;
};

/////////////////////////////////////////////////////////////
// got destroyed together with the target object
class GC_Sound_link : public GC_Sound
{
	DECLARE_SELF_REGISTRATION(GC_Sound_link);
    typedef GC_Sound base;

protected:
	ObjPtr<GC_Actor> _object;

public:
    DECLARE_LIST_MEMBER();
	GC_Sound_link(World &world, enumSoundTemplate sound, GC_Actor *object);
	GC_Sound_link(FromFile);
	virtual void Serialize(World &world, ObjectList::id_type id, SaveFile &f);
	virtual void TimeStepFixed(World &world, ObjectList::id_type id, float dt);

public:
	bool CheckObject(const GC_Object *object) const
	{
		return _object == object;
	}
};

/////////////////////////////////////////////////////////////

#if !defined NOSOUND
#define PLAY(s, pos)                                            \
do {                                                            \
    auto obj = new GC_Sound(world, (s));                        \
    auto id = obj->Register(world);                             \
    obj->MoveTo(world, id, (pos));                              \
    obj->SetMode(world, SMODE_PLAY);                            \
} while(0)
#else
#define PLAY(s, pos) ((void) 0) // no sound
#endif

// end of file
