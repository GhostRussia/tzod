#include "Controller.h"
#include "Game.h"
#include "InputManager.h"
#include "MessageArea.h"
#include "ScoreTable.h"
#include "inc/shell/detail/DefaultCamera.h"
#include "inc/shell/Config.h"

#include <app/Deathmatch.h>
#include <app/GameContext.h>
#include <app/WorldController.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>
#include <ui/UIInput.h>
#include <video/DrawingContext.h>

#include <sstream>

TimeElapsed::TimeElapsed(UI::Window *parent, float x, float y, enumAlignText align, World &world)
  : Text(parent)
  , _world(world)
{
	SetTimeStep(true);
	Move(x, y);
	SetAlign(align);
}

void TimeElapsed::OnVisibleChange(bool visible, bool inherited)
{
	SetTimeStep(visible);
}

void TimeElapsed::OnTimeStep(float dt)
{
	std::ostringstream text;
	int time = (int) _world.GetTime();
	text << (time / 60) << ":";
	if( time % 60 < 10 )
		text << "0";
	text << (time % 60);
	SetText(text.str());
}

///////////////////////////////////////////////////////////////////////////////

GameLayout::GameLayout(Window *parent,
                       GameContext &gameContext,
                       WorldView &worldView,
                       WorldController &worldController,
                       const DefaultCamera &defaultCamera,
                       ConfCache &conf,
                       UI::ConsoleBuffer &logger)
  : Window(parent)
  , _gameContext(gameContext)
  , _gameViewHarness(gameContext.GetWorld(), worldController)
  , _worldView(worldView)
  , _worldController(worldController)
  , _defaultCamera(defaultCamera)
  , _conf(conf)
  , _inputMgr(conf, logger)
{
	_msg = new MessageArea(this, _conf, logger);
	_msg->Move(100, 100);

	_score = new ScoreTable(this, _gameContext.GetWorld(), _gameContext.GetGameplay());
	_score->SetVisible(false);

	_time = new TimeElapsed(this, 0, 0, alignTextRB, _gameContext.GetWorld());
	_conf.ui_showtime.eventChange = std::bind(&GameLayout::OnChangeShowTime, this);
	OnChangeShowTime();

	SetTimeStep(true);
	_gameContext.GetGameEventSource().AddListener(*this);
}

GameLayout::~GameLayout()
{
	_gameContext.GetGameEventSource().RemoveListener(*this);
	_conf.ui_showtime.eventChange = nullptr;
}

void GameLayout::OnTimeStep(float dt)
{
	bool tab = GetManager().GetInput().IsKeyPressed(UI::Key::Tab);
	_score->SetVisible(tab || _gameContext.GetGameplay().IsGameOver());

	_gameViewHarness.Step(dt);

	bool readUserInput = !GetManager().GetFocusWnd() || this == GetManager().GetFocusWnd();
	WorldController::ControllerStateMap controlStates;

	if (readUserInput)
	{
		std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
		for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
		{
			if( GC_Vehicle *vehicle = players[playerIndex]->GetVehicle() )
			{
				if( Controller *controller = _inputMgr.GetController(playerIndex) )
				{
					vec2d mouse = GetManager().GetInput().GetMousePos();
					auto c2w = _gameViewHarness.CanvasToWorld(playerIndex, (int) mouse.x, (int) mouse.y);

					VehicleState vs;
					controller->ReadControllerState(GetManager().GetInput(), _gameContext.GetWorld(),
					                                vehicle, c2w.visible ? &c2w.worldPos : nullptr, vs);
					controlStates.insert(std::make_pair(vehicle->GetId(), vs));
				}
			}
		}

		_worldController.SendControllerStates(std::move(controlStates));
	}
}

void GameLayout::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
    vec2d eye(_defaultCamera.GetPos().x + GetWidth() / 2, _defaultCamera.GetPos().y + GetHeight() / 2);
    float zoom = _defaultCamera.GetZoom();
    _gameViewHarness.RenderGame(dc, _worldView, eye, zoom);
	dc.SetMode(RM_INTERFACE);
	Window::DrawChildren(dc, sx, sy);
}

void GameLayout::OnSize(float width, float height)
{
	_time->Move(GetWidth() - 1, GetHeight() - 1);
	_msg->Move(_msg->GetX(), GetHeight() - 50);
    _gameViewHarness.SetCanvasSize((int) GetWidth(), (int) GetHeight());
}

void GameLayout::OnChangeShowTime()
{
	_time->SetVisible(_conf.ui_showtime.Get());
}

void GameLayout::OnGameMessage(const char *msg)
{
    _msg->WriteLine(msg);
}