// Combo.h

#pragma once

#include "Base.h"
#include "Window.h"


namespace UI
{

// ComboBox control

class ComboBox : public Window
{
	TextButton  *_text;
	List        *_list;
	ButtonBase  *_btn;

	int _curSel;

public:
	ComboBox(Window *parent, float x, float y, float width);

	void SetCurSel(int index);
	int GetCurSel() const;

	List* GetList() const;
	void DropList();

	Delegate<void(int)> eventChangeCurSel;

protected:
	void OnRawChar(int c);
	bool OnFocus(bool focus);
	void OnSize(float width, float height);

	void OnClickItem(int index);
	void OnChangeSelection(int index);

	void OnListLostFocus();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file