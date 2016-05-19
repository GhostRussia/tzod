#pragma once
#include "Window.h"
#include <functional>

class TextureManager;

namespace UI
{

class ButtonBase : public Window
{
public:
	enum State
	{
		stateNormal,
		stateHottrack,
		statePushed,
		stateDisabled,
	};

	explicit ButtonBase(LayoutManager &manager);

	std::function<void(void)> eventClick;
	std::function<void(float, float)> eventMouseDown;
	std::function<void(float, float)> eventMouseUp;
	std::function<void(float, float)> eventMouseMove;

	State GetState() const { return _state; }

protected:
	bool OnPointerMove(float x, float y, PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerDown(float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerUp  (float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnMouseLeave() override;
	bool OnTap(float x, float y) override;

	void OnEnabledChange(bool enable, bool inherited) override;
	virtual void OnChangeState(State state);

private:
	virtual void OnClick();

	State _state;
	void SetState(State s);
};

///////////////////////////////////////////////////////////////////////////////

class Button : public ButtonBase
{
public:
	static std::shared_ptr<Button> Create(Window *parent, TextureManager &texman, const std::string &text, float x, float y, float w=-1, float h=-1);

	Button(LayoutManager &manager, TextureManager &texman);

	void SetIcon(TextureManager &texman, const char *spriteName);

protected:
	void SetFont(TextureManager &texman, const char *fontName);

	// ButtonBase
	void OnChangeState(State state) override;

	// Window
	void Draw(bool focused, vec2d size, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _font;
	size_t _icon;
};

///////////////////////////////////////////////////////////////////////////////

class TextButton : public ButtonBase
{
public:
	explicit TextButton(LayoutManager &manager, TextureManager &texman);

	void SetFont(const char *fontName);

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

protected:

	void AlignSizeToContent();

	void OnTextChange() override;
	void Draw(bool focused, vec2d size, DrawingContext &dc, TextureManager &texman) const override;


private:
	size_t _fontTexture;
	bool   _drawShadow;
};

///////////////////////////////////////////////////////////////////////////////

class ImageButton : public ButtonBase
{
public:
	explicit ImageButton(LayoutManager &manager);

protected:
	virtual void OnChangeState(State state);
};

///////////////////////////////////////////////////////////////////////////////

class CheckBox : public ButtonBase
{
public:
	static std::shared_ptr<CheckBox> Create(Window *parent, TextureManager &texman, float x, float y, const std::string &text);

	explicit CheckBox(LayoutManager &manager, TextureManager &texman);

	void SetCheck(bool checked);
	bool GetCheck() const { return _isChecked; }

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

protected:
	void AlignSizeToContent();

	void OnClick() override;
	void OnTextChange() override;
	void OnChangeState(State state) override;

	// Window
	void Draw(bool focused, vec2d size, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _fontTexture;
	size_t _boxTexture;
	bool   _drawShadow;
	bool   _isChecked;
};

} // namespace UI
