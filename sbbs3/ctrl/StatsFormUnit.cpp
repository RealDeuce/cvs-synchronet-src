/* Synchronet Control Panel (GUI Borland C++ Builder Project for Win32) */

/* $Id: StatsFormUnit.cpp,v 1.1 2000/10/10 11:26:34 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2000 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "StatsFormUnit.h"
#include "StatsLogFormUnit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TStatsForm *StatsForm;
//---------------------------------------------------------------------------
__fastcall TStatsForm::TStatsForm(TComponent* Owner)
	: TForm(Owner)
{
//    OutputDebugString("StatsForm constructor\n");

	MainForm=(TMainForm*)Application->MainForm;
}
//---------------------------------------------------------------------------
void __fastcall TStatsForm::FormShow(TObject *Sender)
{
//    OutputDebugString("StatsForm::FormShow\n");

	MainForm->ViewStatsMenuItem->Checked=true;
	MainForm->ViewStatsButton->Down=true;
}
//---------------------------------------------------------------------------

void __fastcall TStatsForm::FormHide(TObject *Sender)
{
	MainForm->ViewStatsMenuItem->Checked=false;
	MainForm->ViewStatsButton->Down=false;
}
//---------------------------------------------------------------------------


void __fastcall TStatsForm::LogButtonClick(TObject *Sender)
{
	Application->CreateForm(__classid(TStatsLogForm), &StatsLogForm);
	StatsLogForm->ShowModal();
    delete StatsLogForm;
}
//---------------------------------------------------------------------------

