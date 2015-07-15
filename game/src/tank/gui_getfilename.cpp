// gui_getfilename.cpp

#include "gui_getfilename.h"

#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/Text.h>
#include <ui/List.h>
#include <ui/Button.h>
#include <ui/Edit.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>
#include <GLFW/glfw3.h>

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

GetFileNameDlg::GetFileNameDlg(Window *parent, const Params &param)
  : Dialog(parent, 512, 460)
  , _folder(param.folder)
  , _changing(false)
{
	Text *t = Text::Create(this, GetWidth() / 2, 16, param.title, alignTextCT);
	t->SetFont("font_default");

	_ext = param.extension;
	_files = DefaultListBox::Create(this);
	_files->Move(20, 56);
	_files->Resize(472, 300);
	if( _folder )
	{
		auto files = _folder->EnumAllFiles("*." + _ext);
		for( auto it = files.begin(); it != files.end(); ++it )
		{
			it->erase(it->length() - _ext.length() - 1); // cut out the file extension
			_files->GetData()->AddItem(*it);
		}
	}
	_files->GetData()->Sort();
	_files->eventChangeCurSel = std::bind(&GetFileNameDlg::OnSelect, this, std::placeholders::_1);

	Text::Create(this, 16, 370, g_lang.get_file_name_title.Get(), alignTextLT);
	_fileName = Edit::Create(this, 20, 385, 472);
	_fileName->eventChange = std::bind(&GetFileNameDlg::OnChangeName, this);

	Button::Create(this, g_lang.common_ok.Get(), 290, 420)->eventClick = std::bind(&GetFileNameDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 400, 420)->eventClick = std::bind(&GetFileNameDlg::OnCancel, this);

	GetManager().SetFocusWnd(_fileName);
}

GetFileNameDlg::~GetFileNameDlg()
{
}

std::string GetFileNameDlg::GetFileName() const
{
	return _fileName->GetText() + "." + _ext;
}

std::string GetFileNameDlg::GetFileTitle() const
{
	return _fileName->GetText();
}

void GetFileNameDlg::OnSelect(int index)
{
	if( _changing || -1 == index ) return;
	_fileName->SetText(_files->GetData()->GetItemText(index, 0));
}

void GetFileNameDlg::OnChangeName()
{
	_changing = true;
	size_t match = 0;
	std::string txt = _fileName->GetText();
	for( int i = 0; i < _files->GetData()->GetItemCount(); ++i )
	{
		std::string fn = _files->GetData()->GetItemText(i, 0);
		size_t n = 0;
		while( n < fn.length() && n < txt.length() )
		{
			if( fn[n] != txt[n] ) break;
			++n;
		}
		if( n > match )
		{
			match = n;
			_files->SetCurSel(i, true);
		}
	}
	_changing = false;
}

bool GetFileNameDlg::OnRawChar(int c)
{
	switch( c )
	{
	//case GLFW_KEY_UP:
	//case GLFW_KEY_DOWN:
	//	static_cast<Window *>(_files)->OnRawChar(c);
	//	break;
	case GLFW_KEY_ENTER:
		OnOK();
		break;
	default:
		return Dialog::OnRawChar(c);
	}
	return true;
}

void GetFileNameDlg::OnOK()
{
	Close(_resultOK);
}

void GetFileNameDlg::OnCancel()
{
	Close(_resultCancel);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
