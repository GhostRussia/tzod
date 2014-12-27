#pragma once
#include "GameEvents.h"
#include <ui/Window.h>
#include <ui/Text.h>

class World;
class WorldView;
class WorldController;
class InputManager;
class DefaultCamera;
struct Gameplay;

namespace UI
{
    class MessageArea;
    class ScoreTable;

class TimeElapsed : public Text
{
    World &_world;
public:
	TimeElapsed(Window *parent, float x, float y, enumAlignText align, World &world);
protected:
	void OnVisibleChange(bool visible, bool inherited);
	void OnTimeStep(float dt);
};

class GameLayout
    : public Window
	, private GameListener
{
public:
    GameLayout(Window *parent,
			   GameEventSource &gameEventSource,
			   World &world,
			   WorldView &worldView,
			   WorldController &worldController,
			   InputManager &inputMgr,
			   Gameplay &gameplay,
			   const DefaultCamera &defaultCamera);
    virtual ~GameLayout();
    
    // Window
	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;
	virtual void OnSize(float width, float height);
	virtual bool OnFocus(bool focus) { return true; }

private:
	void OnChangeShowTime();

	MessageArea  *_msg;
	ScoreTable   *_score;
	TimeElapsed  *_time;

	GameEventSource &_gameEventSource;
    World &_world;
    WorldView &_worldView;
	WorldController &_worldController;
	InputManager &_inputMgr;
	Gameplay &_gameplay;
    const DefaultCamera &_defaultCamera;

	// GameListener
	virtual void OnGameMessage(const char *msg) override;
};


} // end of namespace UI
