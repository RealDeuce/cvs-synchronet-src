object CodeInputForm: TCodeInputForm
  Left = 569
  Top = 374
  BorderStyle = bsDialog
  Caption = 'Parameter Required'
  ClientHeight = 90
  ClientWidth = 407
  Color = clBtnFace
  ParentFont = True
  OldCreateOrder = True
  Position = poScreenCenter
  OnShow = FormShow
  DesignSize = (
    407
    90)
  PixelsPerInch = 120
  TextHeight = 16
  object Bevel1: TBevel
    Left = 10
    Top = 10
    Width = 280
    Height = 68
    Anchors = [akLeft, akTop, akRight, akBottom]
    Shape = bsFrame
  end
  object Label: TLabel
    Left = 16
    Top = 32
    Width = 129
    Height = 24
    Alignment = taRightJustify
    AutoSize = False
    Caption = 'Name/ID/Code'
  end
  object OKBtn: TButton
    Left = 304
    Top = 10
    Width = 92
    Height = 31
    Anchors = [akTop, akRight]
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 0
  end
  object CancelBtn: TButton
    Left = 304
    Top = 47
    Width = 92
    Height = 31
    Anchors = [akTop, akRight]
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
  end
  object ComboBox: TComboBox
    Left = 152
    Top = 32
    Width = 121
    Height = 24
    ItemHeight = 16
    TabOrder = 2
  end
  object Edit: TEdit
    Left = 152
    Top = 32
    Width = 121
    Height = 24
    TabOrder = 3
    Visible = False
  end
end
