#pragma once
#include <ui/Rectangle.h>

class World;
class Deathmatch;
class LangCache;
class TextureManager;

class ScoreTable : public UI::Rectangle
{
public:
	ScoreTable(UI::LayoutManager &manager, TextureManager &texman, World &world, Deathmatch &deathmatch, LangCache &lang);

protected:
	void Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _font;
	World &_world;
	Deathmatch &_deathmatch;
	LangCache &_lang;
};
