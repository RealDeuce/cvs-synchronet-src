/* Synchronet Control Panel (GUI Borland C++ Builder Project for Win32) */

/* $Id: WebCfgDlgUnit.cpp,v 1.5 2014/11/20 05:18:51 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2014 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include "MainFormUnit.h"
#include "WebCfgDlgUnit.h"
#include "TextFileEditUnit.h"
#include <stdio.h>			// sprintf()
#include <mmsystem.h>		// sndPlaySound()
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TWebCfgDlg *WebCfgDlg;
//---------------------------------------------------------------------------
__fastcall TWebCfgDlg::TWebCfgDlg(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TWebCfgDlg::FormShow(TObject *Sender)
{
    char str[128];
    char** p;

    if(MainForm->web_startup.interface_addr==0)
        NetworkInterfaceEdit->Text="<ANY>";
    else {
        sprintf(str,"%d.%d.%d.%d"
            ,(MainForm->web_startup.interface_addr>>24)&0xff
            ,(MainForm->web_startup.interface_addr>>16)&0xff
            ,(MainForm->web_startup.interface_addr>>8)&0xff
            ,MainForm->web_startup.interface_addr&0xff
        );
        NetworkInterfaceEdit->Text=AnsiString(str);
    }
    if(MainForm->web_startup.max_clients==0)
        MaxClientsEdit->Text="infinite";
    else
        MaxClientsEdit->Text=AnsiString((int)MainForm->web_startup.max_clients);
    MaxInactivityEdit->Text=AnsiString((int)MainForm->web_startup.max_inactivity);
	PortEdit->Text=AnsiString((int)MainForm->web_startup.port);
    AutoStartCheckBox->Checked=MainForm->WebAutoStart;

    HtmlRootEdit->Text=AnsiString(MainForm->web_startup.root_dir);
    ErrorSubDirEdit->Text=AnsiString(MainForm->web_startup.error_dir);
    CGIDirEdit->Text=AnsiString(MainForm->web_startup.cgi_dir);
    EmbeddedJsExtEdit->Text=AnsiString(MainForm->web_startup.js_ext);
    ServerSideJsExtEdit->Text=AnsiString(MainForm->web_startup.ssjs_ext);

    CGIContentEdit->Text=AnsiString(MainForm->web_startup.default_cgi_content);
    CGIMaxInactivityEdit->Text=AnsiString((int)MainForm->web_startup.max_cgi_inactivity);

    IndexFileEdit->Text.SetLength(0);
    for(p=MainForm->web_startup.index_file_name;*p;p++) {
        if(p!=MainForm->web_startup.index_file_name)
            IndexFileEdit->Text=IndexFileEdit->Text+",";
        IndexFileEdit->Text=IndexFileEdit->Text+AnsiString(*p);
    }

    CGIExtEdit->Text.SetLength(0);
    for(p=MainForm->web_startup.cgi_ext;*p;p++) {
        if(p!=MainForm->web_startup.cgi_ext)
            CGIExtEdit->Text=CGIExtEdit->Text+",";
        CGIExtEdit->Text=CGIExtEdit->Text+AnsiString(*p);
    }

    CGICheckBox->Checked=!(MainForm->web_startup.options&WEB_OPT_NO_CGI);

    AnswerSoundEdit->Text=AnsiString(MainForm->web_startup.answer_sound);
    HangupSoundEdit->Text=AnsiString(MainForm->web_startup.hangup_sound);
    HackAttemptSoundEdit->Text=AnsiString(MainForm->web_startup.hack_sound);

	DebugTxCheckBox->Checked=MainForm->web_startup.options&WEB_OPT_DEBUG_TX;
	DebugRxCheckBox->Checked=MainForm->web_startup.options&WEB_OPT_DEBUG_RX;
    AccessLogCheckBox->Checked=MainForm->web_startup.options&WEB_OPT_HTTP_LOGGING;
    AccessLogCheckBoxClick(Sender);

    VirtualHostsCheckBox->Checked=MainForm->web_startup.options&WEB_OPT_VIRTUAL_HOSTS;

    HostnameCheckBox->Checked
        =!(MainForm->web_startup.options&BBS_OPT_NO_HOST_LOOKUP);

    PageControl->ActivePage=GeneralTabSheet;

    CGICheckBoxClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TWebCfgDlg::OKBtnClick(TObject *Sender)
{
    char    str[128],*p;
    DWORD   addr;

    SAFECOPY(str,NetworkInterfaceEdit->Text.c_str());
    p=str;
    while(*p && *p<=' ') p++;
    if(*p && isdigit(*p)) {
        addr=atoi(p)<<24;
        while(*p && *p!='.') p++;
        if(*p=='.') p++;
        addr|=atoi(p)<<16;
        while(*p && *p!='.') p++;
        if(*p=='.') p++;
        addr|=atoi(p)<<8;
        while(*p && *p!='.') p++;
        if(*p=='.') p++;
        addr|=atoi(p);
        MainForm->web_startup.interface_addr=addr;
    } else
        MainForm->web_startup.interface_addr=0;
    MainForm->web_startup.max_clients=MaxClientsEdit->Text.ToIntDef(0);
    MainForm->web_startup.max_inactivity=MaxInactivityEdit->Text.ToIntDef(WEB_DEFAULT_MAX_INACTIVITY);
    MainForm->web_startup.port=PortEdit->Text.ToIntDef(IPPORT_HTTP);
    MainForm->WebAutoStart=AutoStartCheckBox->Checked;

    SAFECOPY(MainForm->web_startup.root_dir
        ,HtmlRootEdit->Text.c_str());
    SAFECOPY(MainForm->web_startup.error_dir
        ,ErrorSubDirEdit->Text.c_str());
    SAFECOPY(MainForm->web_startup.cgi_dir
        ,CGIDirEdit->Text.c_str());
    SAFECOPY(MainForm->web_startup.js_ext
        ,EmbeddedJsExtEdit->Text.c_str());
    SAFECOPY(MainForm->web_startup.ssjs_ext
        ,ServerSideJsExtEdit->Text.c_str());

    SAFECOPY(MainForm->web_startup.default_cgi_content
        ,CGIContentEdit->Text.c_str());
    MainForm->web_startup.max_cgi_inactivity
        =CGIMaxInactivityEdit->Text.ToIntDef(WEB_DEFAULT_MAX_CGI_INACTIVITY);

    strListFree(&MainForm->web_startup.index_file_name);
    strListSplitCopy(&MainForm->web_startup.index_file_name,
        IndexFileEdit->Text.c_str(),",");

    strListFree(&MainForm->web_startup.cgi_ext);
    strListSplitCopy(&MainForm->web_startup.cgi_ext,
        CGIExtEdit->Text.c_str(),",");

	if(CGICheckBox->Checked==false)
    	MainForm->web_startup.options|=WEB_OPT_NO_CGI;
    else
	    MainForm->web_startup.options&=~WEB_OPT_NO_CGI;

    SAFECOPY(MainForm->web_startup.answer_sound
        ,AnswerSoundEdit->Text.c_str());
    SAFECOPY(MainForm->web_startup.hangup_sound
        ,HangupSoundEdit->Text.c_str());
    SAFECOPY(MainForm->web_startup.hack_sound
        ,HackAttemptSoundEdit->Text.c_str());

	if(DebugTxCheckBox->Checked==true)
    	MainForm->web_startup.options|=WEB_OPT_DEBUG_TX;
    else
	    MainForm->web_startup.options&=~WEB_OPT_DEBUG_TX;
	if(DebugRxCheckBox->Checked==true)
    	MainForm->web_startup.options|=WEB_OPT_DEBUG_RX;
    else
	    MainForm->web_startup.options&=~WEB_OPT_DEBUG_RX;
	if(AccessLogCheckBox->Checked==true)
    	MainForm->web_startup.options|=WEB_OPT_HTTP_LOGGING;
    else
	    MainForm->web_startup.options&=~WEB_OPT_HTTP_LOGGING;
	if(HostnameCheckBox->Checked==false)
    	MainForm->web_startup.options|=BBS_OPT_NO_HOST_LOOKUP;
    else
	    MainForm->web_startup.options&=~BBS_OPT_NO_HOST_LOOKUP;
	if(VirtualHostsCheckBox->Checked==true)
    	MainForm->web_startup.options|=WEB_OPT_VIRTUAL_HOSTS;
    else
	    MainForm->web_startup.options&=~WEB_OPT_VIRTUAL_HOSTS;

    MainForm->SaveIniSettings(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TWebCfgDlg::AnswerSoundButtonClick(TObject *Sender)
{
	OpenDialog->FileName=AnswerSoundEdit->Text;
	if(OpenDialog->Execute()==true) {
    	AnswerSoundEdit->Text=OpenDialog->FileName;
        sndPlaySound(OpenDialog->FileName.c_str(),SND_ASYNC);
    }
}
//---------------------------------------------------------------------------
void __fastcall TWebCfgDlg::HangupSoundButtonClick(TObject *Sender)
{
	OpenDialog->FileName=HangupSoundEdit->Text;
	if(OpenDialog->Execute()==true) {
    	HangupSoundEdit->Text=OpenDialog->FileName;
        sndPlaySound(OpenDialog->FileName.c_str(),SND_ASYNC);
	}
}
//---------------------------------------------------------------------------
void __fastcall TWebCfgDlg::HackAttemptSoundButtonClick(TObject *Sender)
{
	OpenDialog->FileName=HackAttemptSoundEdit->Text;
	if(OpenDialog->Execute()==true) {
    	HackAttemptSoundEdit->Text=OpenDialog->FileName;
        sndPlaySound(OpenDialog->FileName.c_str(),SND_ASYNC);
	}
}
//---------------------------------------------------------------------------

void __fastcall TWebCfgDlg::AccessLogCheckBoxClick(TObject *Sender)
{
    LogBaseLabel->Enabled=AccessLogCheckBox->Checked;
    LogBaseNameEdit->Enabled=AccessLogCheckBox->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TWebCfgDlg::CGIEnvButtonClick(TObject *Sender)
{
	char path[MAX_PATH+1];

    iniFileName(path,sizeof(path),MainForm->cfg.ctrl_dir,"cgi_env.ini");
	Application->CreateForm(__classid(TTextFileEditForm), &TextFileEditForm);
	TextFileEditForm->Filename=AnsiString(path);
    TextFileEditForm->Caption="CGI Environment Variables";
	TextFileEditForm->ShowModal();
    delete TextFileEditForm;
}
//---------------------------------------------------------------------------

void __fastcall TWebCfgDlg::WebHandlersButtonClick(TObject *Sender)
{
	char path[MAX_PATH+1];

    iniFileName(path,sizeof(path),MainForm->cfg.ctrl_dir,"web_handler.ini");
	Application->CreateForm(__classid(TTextFileEditForm), &TextFileEditForm);
	TextFileEditForm->Filename=AnsiString(path);
    TextFileEditForm->Caption="Web Content Handlers";
	TextFileEditForm->ShowModal();
    delete TextFileEditForm;
}
//---------------------------------------------------------------------------

void __fastcall TWebCfgDlg::CGICheckBoxClick(TObject *Sender)
{
    bool enabled=CGICheckBox->Checked;

    CGIDirLabel->Enabled=enabled;
    CGIDirEdit->Enabled=enabled;
    CGIExtLabel->Enabled=enabled;
    CGIExtEdit->Enabled=enabled;
    CGIContentLabel->Enabled=enabled;
    CGIContentEdit->Enabled=enabled;
    CGIMaxInactivityLabel->Enabled=enabled;
    CGIMaxInactivityEdit->Enabled=enabled;
    CGIEnvButton->Enabled=enabled;
}
//---------------------------------------------------------------------------

