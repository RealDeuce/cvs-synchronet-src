//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "MainFormUnit.h"
#include "SpyFormUnit.h"
#include "sbbsdefs.h"

#define SPYBUF_LEN  10000
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "Emulvt"
#pragma resource "*.dfm"
TSpyForm *SpyForm;
//---------------------------------------------------------------------------
__fastcall TSpyForm::TSpyForm(TComponent* Owner)
    : TForm(Owner)
{
    Width=MainForm->SpyTerminalWidth;
    Height=MainForm->SpyTerminalHeight;
    KeyboardActive->Checked=MainForm->SpyTerminalKeyboardActive;
    Terminal = new TEmulVT(this);
    Terminal->Parent=this;
    Terminal->Align=alClient;
    Terminal->OnKeyPress=FormKeyPress;
    Terminal->OnMouseUp=FormMouseUp;
}
bool strip_ansi(char* str)
{
    char*   p=str;
    char    newstr[SPYBUF_LEN];
    int     newlen=0;
    bool    ff=false;

    for(p=str;*p && newlen<(sizeof(newstr)-1);) {

        switch(*p) {
            case BEL:   /* bell */
                Beep();
                break;
            case BS:    /* backspace */
                if(newlen)
                    newlen--;
                break;
             case FF:   /* form feed */
                newlen=0;
                ff=true;
                break;
             case ESC:
                if(*(p+1)=='[') {    /* ANSI */
                    p+=2;
                    if(!strncmp(p,"2J",2)) {
                        newlen=0;
                        ff=true;
                    }
                    while(*p && !isalpha(*p)) p++;
                }
                break;
            case CR:
                if(*(p+1)!=LF) {
                    while(newlen) {
                        newlen--;
                        if(newstr[newlen]==LF) {
                            newlen++;
                            break;
                        }
                    }
                    break;
                } /* Fall through */
             default:
                newstr[newlen++]=*p;
                break;
        }
        if(*p==0)
            break;
        p++;
    }
    newstr[newlen]=0;
    strcpy(str,newstr);
    return(ff);
}
//---------------------------------------------------------------------------
void __fastcall TSpyForm::SpyTimerTick(TObject *Sender)
{
    char    buf[1024];
    int     rd;

    if(*outbuf==NULL)
        return;

    rd=RingBufRead(*outbuf,buf,sizeof(buf)-1);
    if(rd) {
#if 0
        buf[rd]=0;
        if(strip_ansi(buf))
            Log->Lines->Clear();
        Log->SelLength=0;
        Log->SelStart=Log->Lines->Text.Length();
        Log->SelText=AnsiString(buf);
#else
        Terminal->WriteBuffer(buf,rd);
#endif
        Timer->Interval=100;
    } else
        Timer->Interval=500;
}
//---------------------------------------------------------------------------
void __fastcall TSpyForm::FormShow(TObject *Sender)
{
    if((*outbuf=(RingBuf*)malloc(sizeof(RingBuf)))==NULL) {
        Terminal->WriteStr("Malloc failure!");
        return;
    }
    RingBufInit(*outbuf,SPYBUF_LEN);

    Timer->Enabled=true;

    Terminal->Font=MainForm->SpyTerminalFont;
    Terminal->Clear();
    Terminal->WriteStr("*** Synchronet Local Spy ***\r\n\r\n");
    Terminal->WriteStr("ANSI Terminal Emulation:"+CopyRight+"\r\n\r\n");
}
//---------------------------------------------------------------------------

void __fastcall TSpyForm::FormClose(TObject *Sender, TCloseAction &Action)
{
    Timer->Enabled=false;
    if(*outbuf!=NULL) {
        RingBufDispose(*outbuf);
        free(*outbuf);
        *outbuf=NULL;
    }
    MainForm->SpyTerminalWidth=Width;
    MainForm->SpyTerminalHeight=Height;
    MainForm->SpyTerminalKeyboardActive=KeyboardActive->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TSpyForm::ChangeFontClick(TObject *Sender)
{
	TFontDialog *FontDialog=new TFontDialog(this);

    FontDialog->Font=MainForm->SpyTerminalFont;
    FontDialog->Execute();
    MainForm->SpyTerminalFont->Assign(FontDialog->Font);
    Terminal->Font=MainForm->SpyTerminalFont;
    delete FontDialog;
    Terminal->UpdateScreen();
}
//---------------------------------------------------------------------------

void __fastcall TSpyForm::FormKeyPress(TObject *Sender, char &Key)
{
    if(KeyboardActive->Checked && inbuf!=NULL && *inbuf!=NULL)
        RingBufWrite(*inbuf,&Key,1);
}
//---------------------------------------------------------------------------

void __fastcall TSpyForm::KeyboardActiveClick(TObject *Sender)
{
    KeyboardActive->Checked=!KeyboardActive->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TSpyForm::FormMouseUp(TObject *Sender, TMouseButton Button,
      TShiftState Shift, int X, int Y)
{
    if(Button==mbRight)
        PopupMenu->Popup(ClientOrigin.x+X,ClientOrigin.y+Y);
}
//---------------------------------------------------------------------------


