// World.cpp

#include "World.h"
#include "World.inl"

#include "DefaultCamera.h"
#include "Macros.h"
#include "MapFile.h"
#include "SaveFile.h"
#include "script.h"

#include "core/debug.h"

#include "config/Config.h"
#include "config/Language.h"

//#include "network/TankClient.h"
//#include "network/TankServer.h"

#include "GameClasses.h"
#include "RigidBodyDinamic.h"
#include "Player.h"
#include "Sound.h"

#include <FileSystem.h>

#include <GuiManager.h>
#include <ConsoleBuffer.h>


extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <pluto.h>

#include <GLFW/glfw3.h>

UI::ConsoleBuffer& GetConsole();

#define MAX_THEME_NAME  128

struct SaveHeader
{
    uint32_t dwVersion;
    bool  nightmode;
    float timelimit;
    int   fraglimit;
    float time;
    int   width;
    int   height;
    char  theme[MAX_THEME_NAME];
};


// don't create game objects in the constructor
World::World()
  : _serviceListener(nullptr)
  , _messageListener(nullptr)
  , _frozen(false)
  , _limitHit(false)
  , _sx(0)
  , _sy(0)
  , _locationsX(0)
  , _locationsY(0)
  , _seed(1)
  , _time(0)
  , _safeMode(true)
#ifdef NETWORK_DEBUG
  , _checksum(0)
  , _frame(0)
  , _dump(NULL)
#endif
{
	TRACE("Constructing the world");

	// register config handlers
	g_conf.s_volume.eventChange = std::bind(&World::OnChangeSoundVolume, this);
	g_conf.sv_nightmode.eventChange = std::bind(&World::OnChangeNightMode, this);
}

bool World::IsEmpty() const
{
	return GetList(LIST_objects).empty();
}

void World::Resize(int X, int Y)
{
	assert(IsEmpty());


	//
	// Resize
	//

	_locationsX  = (X * CELL_SIZE / LOCATION_SIZE + ((X * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_locationsY  = (Y * CELL_SIZE / LOCATION_SIZE + ((Y * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_sx          = (float) X * CELL_SIZE;
	_sy          = (float) Y * CELL_SIZE;

	grid_rigid_s.resize(_locationsX, _locationsY);
	grid_walls.resize(_locationsX, _locationsY);
	grid_wood.resize(_locationsX, _locationsY);
	grid_water.resize(_locationsX, _locationsY);
	grid_pickup.resize(_locationsX, _locationsY);
	grid_sprites.resize(_locationsX, _locationsY);

	_field.Resize(X + 1, Y + 1);
}

void World::Clear()
{
	assert(IsSafeMode());

    ObjectList &ls = GetList(LIST_objects);
    while( !ls.empty() )
    {
        ls.at(ls.begin())->Kill(*this, ls.begin());
    }

	// reset info
	_infoAuthor.clear();
	_infoEmail.clear();
	_infoUrl.clear();
	_infoDesc.clear();
	_infoTheme.clear();
	_infoOnInit.clear();

	// reset variables
	_time = 0;
	_limitHit = false;
	_frozen = false;
#ifdef NETWORK_DEBUG
	_checksum = 0;
	_frame = 0;
	if( _dump )
	{
		fclose(_dump);
		_dump = NULL;
	}
#endif
    assert(IsEmpty());
}

void World::HitLimit()
{
	assert(!_limitHit);
//	PauseLocal(true);
	_limitHit = true;
    World &world = *this;
	PLAY(SND_Limit, vec2d(0,0));
}

World::~World()
{
	assert(IsSafeMode());
	TRACE("Destroying the world");

	// unregister config handlers
	g_conf.s_volume.eventChange = nullptr;
	g_conf.sv_nightmode.eventChange = nullptr;

	assert(IsEmpty() && _garbage.empty());
	assert(!g_env.nNeedCursor);
}

void World::AddListener(ObjectType type, ObjectListener &ls)
{
	if (GC_Player::GetTypeStatic() == type)
	{
		_playerListeners.push_back(&ls);
	}
	else
	{
		assert(false);
	}
}

void World::RemoveListener(ObjectType type, ObjectListener &ls)
{
	if (GC_Player::GetTypeStatic() == type)
	{
		auto it = std::find(_playerListeners.begin(), _playerListeners.end(), &ls);
		assert(_playerListeners.end() != it);
		*it = _playerListeners.back();
		_playerListeners.pop_back();
	}
	else
	{
		assert(false);
	}
}

ObjectList::id_type World::GetId(GC_Object *o)
{
    ObjectList &ls = GetList(LIST_objects);
    for( auto it = ls.begin(); it != ls.end(); it = ls.next(it) )
        if (ls.at(it) == o)
            return it;
    return ls.end();
}

void World::Unserialize(const char *fileName)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	TRACE("Loading saved game from file '%s'...", fileName);

	std::shared_ptr<FS::Stream> stream = g_fs->Open(fileName, FS::ModeRead)->QueryStream();
	SaveFile f(stream, true);

	try
	{
		SaveHeader sh;
		if( 1 != stream->Read(&sh, sizeof(SaveHeader), 1) )
            throw std::runtime_error("unexpected end of file");

		if( VERSION != sh.dwVersion )
			throw std::runtime_error("invalid version");

		g_conf.sv_timelimit.SetFloat(sh.timelimit);
		g_conf.sv_fraglimit.SetInt(sh.fraglimit);
		g_conf.sv_nightmode.Set(sh.nightmode);

		_time = sh.time;
		Resize(sh.width, sh.height);


		// fill pointers cache
		for(;;)
		{
			ObjectType type;
			f.Serialize(type);
			if( INVALID_OBJECT_TYPE == type ) // end of list signal
				break;
			if( GC_Object *obj = RTTypes::Inst().CreateFromFile(*this, type) )
			{
				f.RegPointer(obj);
			}
			else
			{
				TRACE("ERROR: unknown object type - %u", type);
				throw std::runtime_error("Load error: unknown object type");
			}
		}

		// read objects contents in the same order as pointers
		for( ObjectList::id_type it = GetList(LIST_objects).begin(); it != GetList(LIST_objects).end(); it = GetList(LIST_objects).next(it) )
		{
            GetList(LIST_objects).at(it)->Serialize(*this, it, f);
		}


		//
		// restore lua user environment
		//

		struct ReadHelper
		{
			static const char* r(lua_State *L, void* data, size_t *sz)
			{
				static char buf[1];
				try
				{
					*sz = reinterpret_cast<FS::Stream*>(data)->Read(buf, 1, sizeof(buf));
				}
				catch( const std::exception &e )
				{
					*sz = 0;
					luaL_error(L, "deserialize error - %s", e.what());
				}
				return buf;
			}
			static int read_user(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				lua_pushstring(L, "any_id_12345");
				lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
				lua_settable(L, -3);
				pluto_unpersist(L, &r, ud);
				lua_setglobal(L, "user");    // unpersisted object
				return 0;
			}
			static int read_queue(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				pluto_unpersist(L, &r, ud);
				lua_getglobal(L, "pushcmd");
				assert(LUA_TFUNCTION == lua_type(L, -1));
				lua_pushvalue(L, -2);
				lua_setupvalue(L, -2, 1);    // unpersisted object
				return 0;
			}
			static int restore_ptr(lua_State *L)
			{
				assert(1 == lua_gettop(L));
				size_t id = (size_t) lua_touserdata(L, 1);
				SaveFile *f = (SaveFile *) lua_touserdata(L, lua_upvalueindex(1));
				assert(f);
				GC_Object *obj;
				try
				{
					obj = id ? f->RestorePointer(id) : NULL;
				}
				catch( const std::exception &e )
				{
					return luaL_error(L, "%s", e.what());
				}
				luaT_pushobject(L, obj);
				return 1;
			}
		};
		lua_pushlightuserdata(g_env.L, &f);
		lua_pushcclosure(g_env.L, &ReadHelper::restore_ptr, 1);
		lua_setfield(g_env.L, LUA_REGISTRYINDEX, "restore_ptr");
		if( lua_cpcall(g_env.L, &ReadHelper::read_user, stream.get()) )
		{
			std::string err = "[pluto read user] ";
			err += lua_tostring(g_env.L, -1);
			lua_pop(g_env.L, 1);
			throw std::runtime_error(err);
		}
		if( lua_cpcall(g_env.L, &ReadHelper::read_queue, stream.get()) )
		{
			std::string err = "[pluto read queue] ";
			err += lua_tostring(g_env.L, -1);
			lua_pop(g_env.L, 1);
			throw std::runtime_error(err);
		}

		// apply the theme
		_infoTheme = sh.theme;
		_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(sh.theme));

		// update skins
		FOREACH( GetList(LIST_players), GC_Player, pPlayer )
		{
			pPlayer->UpdateSkin();
		}
	}
	catch( const std::runtime_error& )
	{
		Clear();
		throw;
	}
}

void World::Serialize(const char *fileName)
{
	assert(IsSafeMode());

	TRACE("Saving game to file '%S'...", fileName);

	std::shared_ptr<FS::Stream> stream = g_fs->Open(fileName, FS::ModeWrite)->QueryStream();
	SaveFile f(stream, false);

	SaveHeader sh = {0};
	strcpy(sh.theme, _infoTheme.c_str());
	sh.dwVersion    = VERSION;
	sh.fraglimit    = g_conf.sv_fraglimit.GetInt();
	sh.timelimit    = g_conf.sv_timelimit.GetFloat();
	sh.nightmode    = g_conf.sv_nightmode.Get();
	sh.time         = _time;
	sh.width        = (int) _sx / CELL_SIZE;
	sh.height       = (int) _sy / CELL_SIZE;

	stream->Write(&sh, sizeof(SaveHeader));


	//
	// pointers to game objects
	//
    ObjectList &objects = GetList(LIST_objects);
	for( auto it = objects.begin(); it != objects.end(); it = objects.next(it) )
	{
		GC_Object *object = objects.at(it);
		ObjectType type = object->GetType();
		stream->Write(&type, sizeof(type));
		f.RegPointer(object);
	}
	ObjectType terminator(INVALID_OBJECT_TYPE);
	f.Serialize(terminator);


	//
	// write objects contents in the same order as pointers
	//

	for( auto it = objects.begin(); it != objects.end(); it = objects.next(it) )
	{
        objects.at(it)->Serialize(*this, it, f);
	}


	//
	// write lua user environment
	//

	struct WriteHelper
	{
		static int w(lua_State *L, const void* p, size_t sz, void* ud)
		{
			try
			{
				reinterpret_cast<SaveFile*>(ud)->GetStream()->Write(p, sz);
			}
			catch( const std::exception &e )
			{
				return luaL_error(L, "[file write] %s", e.what());
			}
			return 0;
		}
		static int write_user(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(g_env.L);       // permanent objects
			lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
			lua_pushstring(L, "any_id_12345");
			lua_settable(L, -3);
			lua_getglobal(L, "user");    // object to persist
			pluto_persist(L, &w, ud);
			return 0;
		}
		static int write_queue(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(g_env.L);       // permanent objects
			lua_getglobal(L, "pushcmd");
			assert(LUA_TFUNCTION == lua_type(L, -1));
			lua_getupvalue(L, -1, 1);    // object to persist
			lua_remove(L, -2);
			pluto_persist(L, &w, ud);
			return 0;
		}
	};
	lua_newuserdata(g_env.L, 0); // placeholder for restore_ptr
	lua_setfield(g_env.L, LUA_REGISTRYINDEX, "restore_ptr");
	if( lua_cpcall(g_env.L, &WriteHelper::write_user, &f) )
	{
		std::string err = "[pluto write user] ";
		err += lua_tostring(g_env.L, -1);
		lua_pop(g_env.L, 1);
		throw std::runtime_error(err);
	}
	if( lua_cpcall(g_env.L, &WriteHelper::write_queue, &f) )
	{
		std::string err = "[pluto write queue] ";
		err += lua_tostring(g_env.L, -1);
		lua_pop(g_env.L, 1);
		throw std::runtime_error(err);
	}
	lua_setfield(g_env.L, LUA_REGISTRYINDEX, "restore_ptr");
}

void World::Import(std::shared_ptr<FS::Stream> s)
{
	assert(IsEmpty());
	assert(IsSafeMode());

	MapFile file(s, false);

	int width, height;
	if( !file.getMapAttribute("width", width) ||
		!file.getMapAttribute("height", height) )
	{
		throw std::runtime_error("unknown map size");
	}

	file.getMapAttribute("theme", _infoTheme);
	_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(_infoTheme));

	file.getMapAttribute("author",   _infoAuthor);
	file.getMapAttribute("desc",     _infoDesc);
	file.getMapAttribute("link-url", _infoUrl);
	file.getMapAttribute("e-mail",   _infoEmail);
	file.getMapAttribute("on_init",  _infoOnInit);

	Resize(width, height);

	while( file.NextObject() )
	{
		float x = 0;
		float y = 0;
		file.getObjectAttribute("x", x);
		file.getObjectAttribute("y", y);
		ObjectType t = RTTypes::Inst().GetTypeByName(file.GetCurrentClassName());
		if( INVALID_OBJECT_TYPE == t )
			continue;
		GC_Object *object = RTTypes::Inst().GetTypeInfo(t).Create(*this, x, y);
		object->MapExchange(*this, file);
	}
}

void World::Export(std::shared_ptr<FS::Stream> s)
{
	assert(IsSafeMode());

	MapFile file(s, true);

	//
	// map info
	//

	file.setMapAttribute("type", "deathmatch");

	std::ostringstream str;
	str << VERSION;
	file.setMapAttribute("version", str.str());

	file.setMapAttribute("width",  (int) _sx / CELL_SIZE);
	file.setMapAttribute("height", (int) _sy / CELL_SIZE);

	file.setMapAttribute("author",   _infoAuthor);
	file.setMapAttribute("desc",     _infoDesc);
	file.setMapAttribute("link-url", _infoUrl);
	file.setMapAttribute("e-mail",   _infoEmail);

	file.setMapAttribute("theme",    _infoTheme);

	file.setMapAttribute("on_init",  _infoOnInit);

	// objects
	FOREACH( GetList(LIST_objects), GC_Object, object )
	{
		if( RTTypes::Inst().IsRegistered(object->GetType()) )
		{
			file.BeginObject(RTTypes::Inst().GetTypeName(object->GetType()));
			object->MapExchange(*this, file);
			file.WriteCurrentObject();
		}
	}
}

void World::PauseSound(bool pause)
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->Freeze(pause);
	}
}

int World::net_rand()
{
	return ((_seed = _seed * 214013L + 2531011L) >> 16) & RAND_MAX;
}

float World::net_frand(float max)
{
	return (float) net_rand() / (float) RAND_MAX * max;
}

vec2d World::net_vrand(float len)
{
	return vec2d(net_frand(PI2)) * len;
}

bool World::CalcOutstrip( const vec2d &fp, // fire point
                          float vp,        // speed of the projectile
                          const vec2d &tx, // target position
                          const vec2d &tv, // target velocity
                          vec2d &out_fake) // out: fake target position
{
	float vt = tv.len();

	if( vt >= vp || vt < 1e-7 )
	{
		out_fake = tx;
		return false;
	}

	float cg = tv.x / vt;
	float sg = tv.y / vt;

	float x   = (tx.x - fp.x) * cg + (tx.y - fp.y) * sg;
	float y   = (tx.y - fp.y) * cg - (tx.x - fp.x) * sg;
	float tmp = vp*vp - vt*vt;

	float fx = x + vt * (x*vt + sqrt(x*x * vp*vp + y*y * tmp)) / tmp;

	out_fake.x = std::max(0.0f, std::min(_sx, fp.x + fx*cg - y*sg));
	out_fake.y = std::max(0.0f, std::min(_sy, fp.y + fx*sg + y*cg));
	return true;
}

GC_RigidBodyStatic* World::TraceNearest( Grid<std::vector<id_type>> &list,
                                         const GC_RigidBodyStatic* ignore,
                                         const vec2d &x0,      // origin
                                         const vec2d &a,       // direction with length
                                         vec2d *ht,
                                         vec2d *norm) const
{
//	DbgLine(x0, x0 + a);

	struct SelectNearest
	{
		const GC_RigidBodyStatic *ignore;
		vec2d x0;
		vec2d lineCenter;
		vec2d lineDirection;

		GC_RigidBodyStatic *result;
		vec2d resultPos;
		vec2d resultNorm;

		SelectNearest()
			: result(NULL)
		{
		}

		bool Select(GC_RigidBodyStatic *obj, vec2d norm, float enter, float exit)
		{
			if( ignore != obj )
			{
				result = obj;
				resultPos = lineCenter + lineDirection * enter;
				resultNorm = norm;

				lineDirection *= enter + 0.5f;
				lineCenter = x0 + lineDirection / 2;
			}
			return false;
		}
		inline const vec2d& GetCenter() const { return lineCenter; }
		inline const vec2d& GetDirection() const { return lineDirection; }
	};
	SelectNearest selector;
	selector.ignore = ignore;
	selector.x0 = x0;
	selector.lineCenter = x0 + a/2;
	selector.lineDirection = a;
	RayTrace(list, selector);
	if( selector.result )
	{
		if( ht ) *ht = selector.resultPos;
		if( norm ) *norm = selector.resultNorm;
	}
	return selector.result;
}

void World::TraceAll( Grid<std::vector<id_type>> &list,
                      const vec2d &x0,      // origin
                      const vec2d &a,       // direction with length
                      std::vector<CollisionPoint> &result) const
{
	struct SelectAll
	{
		vec2d lineCenter;
		vec2d lineDirection;

		std::vector<CollisionPoint> &result;

		explicit SelectAll(std::vector<CollisionPoint> &r)
			: result(r)
		{
		}

		bool Select(GC_RigidBodyStatic *obj, vec2d norm, float enter, float exit)
		{
			CollisionPoint cp;
			cp.obj = obj;
			cp.normal = norm;
			cp.enter = enter;
			cp.exit = exit;
			result.push_back(cp);
			return false;
		}
		inline const vec2d& GetCenter() const { return lineCenter; }
		inline const vec2d& GetDirection() const { return lineDirection; }
	};
	SelectAll selector(result);
	selector.lineCenter = x0 + a/2;
	selector.lineDirection = a;
	RayTrace(list, selector);
}

void World::Step(float dt)
{
	_time += dt;

	if( !_frozen )
	{
		_safeMode = false;
        ObjectList &ls = GetList(LIST_timestep);
        ls.for_each([=](ObjectList::id_type id, GC_Object *o){
            ObjPtr<GC_Object> watch(o);
            o->TimeStepFixed(*this, id, dt);
            if (watch)
                o->TimeStepFloat(*this, id, dt);
        });
		GC_RigidBodyDynamic::ProcessResponse(*this, dt);
		_safeMode = true;
	}

    assert(_safeMode);
	RunCmdQueue(g_env.L, dt);

	if( g_conf.sv_timelimit.GetInt() && g_conf.sv_timelimit.GetInt() * 60 <= _time )
	{
		HitLimit();
	}

	GetList(LIST_sounds).for_each([=](ObjectList::id_type id, GC_Object *o) {
		static_cast<GC_Sound *>(o)->KillWhenFinished(*this, id);
	});


	//
	// sync lost error detection
	//

#ifdef NETWORK_DEBUG
	if( !_dump )
	{
		char fn[MAX_PATH];
		sprintf_s(fn, "network_dump_%u_%u.txt", GetTickCount(), GetCurrentProcessId());
		_dump = fopen(fn, "w");
		assert(_dump);
	}
	++_frame;
	fprintf(_dump, "\n### frame %04d ###\n", _frame);

	DWORD dwCheckSum = 0;
	for( ObjectList::safe_iterator it = ts_fixed.safe_begin(); it != ts_fixed.end(); ++it )
	{
		if( DWORD cs = (*it)->checksum() )
		{
			dwCheckSum = dwCheckSum ^ cs ^ 0xD202EF8D;
			dwCheckSum = (dwCheckSum >> 1) | ((dwCheckSum & 0x00000001) << 31);
			fprintf(_dump, "0x%08x -> local 0x%08x, global 0x%08x  (%s)\n", (*it), cs, dwCheckSum, typeid(**it).name());
		}
	}
	_checksum = dwCheckSum;
	fflush(_dump);
#endif
}

GC_Object* World::FindObject(const std::string &name) const
{
	std::map<std::string, const GC_Object*>::const_iterator it = _nameToObjectMap.find(name);
	return _nameToObjectMap.end() != it ? const_cast<GC_Object*>(it->second) : NULL;
}

void World::OnChangeSoundVolume()
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->UpdateVolume();
	}
}

void World::OnChangeNightMode()
{
	FOREACH( GetList(LIST_lights), GC_Light, pLight )
	{
		pLight->Update();
	}
}

///////////////////////////////////////////////////////////////////////////////

GC_Player* World::GetPlayerByIndex(size_t playerIndex)
{
	GC_Player *player = NULL;
	FOREACH(GetList(LIST_players), GC_Player, p)
	{
		if( 0 == playerIndex-- )
		{
			player = p;
			break;
		}
	}
	return player;
}

void World::Seed(unsigned long seed)
{
    _seed = seed;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
