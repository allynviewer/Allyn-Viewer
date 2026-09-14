; Allyn Viewer installer theme (Cyber skin: navy + purple + light text).
;
; Requires: LogicLib.nsh, MUI2.nsh, System plugin. Include after MUI2.nsh.
; Colors follow indra/newview/skins/cyber/colors.xml.
;
; Strategy:
;  * Title bar: DWM dark caption (Win10 1809+) + caption/border colors (Win11).
;  * Buttons / scrollbars: Windows' own "DarkMode_Explorer" visual style, so the
;    OS draws the frame and the (Unicode) label. Nothing is hand-painted.
;  * Static / Edit / RichEdit / progress bar: SetCtlColors + control messages.
;  * Do NOT call SetWindowTheme on $HWNDPARENT: that forces a classic caption.

!ifndef ALLYN_THEME_NSH
!define ALLYN_THEME_NSH

; NSIS SetCtlColors takes 0xRRGGBB; Win32 COLORREF is 0x00BBGGRR.
!define ALLYN_BG          0x080916   ; DefaultBackgroundColor
!define ALLYN_BG_REF      0x00160908
!define ALLYN_PANEL       0x0A0B1E   ; LoginFormBgColor
!define ALLYN_PANEL_REF   0x001E0B0A
!define ALLYN_FIELD       0x101228
!define ALLYN_TEXT        0xECEEF8   ; LabelTextColor
!define ALLYN_TEXT_REF    0x00F8EEEC
!define ALLYN_SUBTEXT     0xBABED6
!define ALLYN_MUTED       0x6E7496
!define ALLYN_PURPLE      0x8B5CF6   ; LoginProgressBarFgColor
!define ALLYN_PURPLE_REF  0x00F65C8B
!define ALLYN_BAR_BG_REF  0x00341A18 ; LoginProgressBarBgColor (24,26,52)

; Call once in .onInit / un.onInit before any window exists.
; uxtheme ordinal 135: SetPreferredAppMode(ForceDark = 2) on 1903+,
; AllowDarkModeForApp(TRUE) on 1809.
!macro ALLYN_THEME_INIT
  System::Call 'uxtheme::#135(i 2)'
!macroend

; Dark caption for the wizard window.
!macro ALLYN_DARK_TITLEBAR
  System::Call 'uxtheme::#133(p $HWNDPARENT, i 1)'
  ; DWMWA_USE_IMMERSIVE_DARK_MODE: 19 (pre-20H1) and 20 (20H1+)
  System::Call 'dwmapi::DwmSetWindowAttribute(p $HWNDPARENT, i 19, *i 1, i 4)'
  System::Call 'dwmapi::DwmSetWindowAttribute(p $HWNDPARENT, i 20, *i 1, i 4)'
  ; Win11 only: caption / text / border colors (ignored on Win10)
  System::Call 'dwmapi::DwmSetWindowAttribute(p $HWNDPARENT, i 35, *i ${ALLYN_BG_REF}, i 4)'
  System::Call 'dwmapi::DwmSetWindowAttribute(p $HWNDPARENT, i 36, *i ${ALLYN_TEXT_REF}, i 4)'
  System::Call 'dwmapi::DwmSetWindowAttribute(p $HWNDPARENT, i 34, *i ${ALLYN_PURPLE_REF}, i 4)'
!macroend

; Replace the light-gray 3D sunken edge (WS_EX_CLIENTEDGE) of a field with a
; flat 1px frame. In: $R8 = HWND. Scratch: $R6.
!macro ALLYN_FLAT_BORDER
  System::Call 'user32::GetWindowLong(p $R8, i -20) i .R6'
  IntOp $R6 $R6 & 0xFFFFFDFF          ; ~WS_EX_CLIENTEDGE
  System::Call 'user32::SetWindowLong(p $R8, i -20, i $R6)'
  System::Call 'user32::GetWindowLong(p $R8, i -16) i .R6'
  IntOp $R6 $R6 | 0x00800000          ; WS_BORDER
  System::Call 'user32::SetWindowLong(p $R8, i -16, i $R6)'
  ; SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED
  System::Call 'user32::SetWindowPos(p $R8, p 0, i 0, i 0, i 0, i 0, i 0x37)'
!macroend

; "DarkMode_Explorer" checkboxes/radios get a dark box but the OS still paints
; their label black. Put a Static in our colors over the label area instead.
; A Static without SS_NOTIFY answers HTTRANSPARENT, so clicks fall through to
; the checkbox underneath. Created once per control (tracked via a window prop).
; In: $R8 = checkbox HWND. Scratch: $R6, $R7.
!macro ALLYN_CHECKBOX_LABEL
  System::Call 'user32::GetPropW(p $R8, w "AllynLabel") p .R6'
  ${If} $R6 == 0
    Push $0
    Push $1
    Push $2
    Push $3
    Push $4
    Push $5
    System::Call 'user32::GetWindowTextW(p $R8, w .r0, i 1024)'
    ${If} $0 != ""
      System::Call 'user32::GetParent(p $R8) p .r1'
      System::Call '*(i 0, i 0, i 0, i 0) p .r2'
      System::Call 'user32::GetWindowRect(p $R8, p r2)'
      System::Call 'user32::MapWindowPoints(p 0, p r1, p r2, i 2)'
      System::Call '*$2(i .r3, i .r4, i .r5, i .R7)'
      System::Free $2
      ; label starts after the 13px box + 4px gap (scaled to the window DPI)
      System::Call 'user32::GetDpiForWindow(p $R8) i .R6'
      ${If} $R6 < 96
        StrCpy $R6 96
      ${EndIf}
      IntOp $R6 $R6 * 17
      IntOp $R6 $R6 / 96
      IntOp $3 $3 + $R6           ; x
      IntOp $5 $5 - $3            ; width
      IntOp $R7 $R7 - $4          ; height
      ; WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE
      System::Call 'user32::CreateWindowExW(i 0, w "Static", w r0, i 0x50000200, i r3, i r4, i r5, i R7, p r1, p 0, p 0, p 0) p .R6'
      ${If} $R6 != 0
        System::Call 'user32::SendMessageW(p $R8, i 0x31, p 0, p 0) p .r2'  ; WM_GETFONT
        System::Call 'user32::SendMessageW(p $R6, i 0x30, p r2, p 1)'       ; WM_SETFONT
        SetCtlColors $R6 ${ALLYN_TEXT} ${ALLYN_BG}
        ; keep the overlay above the checkbox in z-order (SWP_NOSIZE|NOMOVE|NOACTIVATE)
        System::Call 'user32::SetWindowPos(p $R6, p 0, i 0, i 0, i 0, i 0, i 0x13)'
        System::Call 'user32::SetPropW(p $R8, w "AllynLabel", p R6)'
        ; drop the OS-drawn (black) label; WS_CLIPSIBLINGS keeps the checkbox from painting over the overlay
        System::Call 'user32::SetWindowTextW(p $R8, w "")'
        System::Call 'user32::GetWindowLong(p $R8, i -16) i .r2'
        IntOp $2 $2 | 0x04000000
        System::Call 'user32::SetWindowLong(p $R8, i -16, i r2)'
      ${EndIf}
    ${EndIf}
    Pop $5
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0
  ${EndIf}
!macroend

; Theme a single control. In: $R8 = HWND. Out: $R3 = class name. Scratch: $R6, $R7.
!macro ALLYN_THEME_CONTROL
  System::Call 'user32::GetClassName(p $R8, t .R3, i 64)'
  ${If} $R3 == "Static"
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_BG}

  ${ElseIf} $R3 == "Button"
    System::Call 'user32::GetWindowLong(p $R8, i -16) i .R6'
    IntOp $R7 $R6 & 0xF
    ${If} $R7 == 0
    ${OrIf} $R7 == 1
      ; Push / default push button: OS dark style draws frame + label.
      System::Call 'uxtheme::#133(p $R8, i 1)'
      System::Call 'uxtheme::SetWindowTheme(p $R8, w "DarkMode_Explorer", p 0)'
      SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_BG}
    ${ElseIf} $R7 == 7
      ; Groupbox: classic rendering honours SetCtlColors text color.
      System::Call 'uxtheme::SetWindowTheme(p $R8, w " ", w " ")'
      SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_BG}
    ${Else}
      ; Checkbox / radio: dark OS box + our own light label overlay.
      System::Call 'uxtheme::#133(p $R8, i 1)'
      System::Call 'uxtheme::SetWindowTheme(p $R8, w "DarkMode_Explorer", p 0)'
      SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_BG}
      !insertmacro ALLYN_CHECKBOX_LABEL
    ${EndIf}

  ${ElseIf} $R3 == "Edit"
    System::Call 'uxtheme::#133(p $R8, i 1)'
    System::Call 'uxtheme::SetWindowTheme(p $R8, w "DarkMode_CFD", p 0)'
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_FIELD}
    !insertmacro ALLYN_FLAT_BORDER

  ${ElseIf} $R3 == "ListBox"
    System::Call 'uxtheme::#133(p $R8, i 1)'
    System::Call 'uxtheme::SetWindowTheme(p $R8, w "DarkMode_Explorer", p 0)'
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_FIELD}
    !insertmacro ALLYN_FLAT_BORDER

  ${ElseIf} $R3 == "ComboBox"
    System::Call 'uxtheme::#133(p $R8, i 1)'
    System::Call 'uxtheme::SetWindowTheme(p $R8, w "DarkMode_CFD", p 0)'
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_FIELD}

  ${ElseIf} $R3 == "RichEdit20W"
  ${OrIf} $R3 == "RichEdit20A"
  ${OrIf} $R3 == "RichEdit50W"
    System::Call 'uxtheme::#133(p $R8, i 1)'
    System::Call 'uxtheme::SetWindowTheme(p $R8, w "DarkMode_Explorer", p 0)'
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_PANEL}
    ; EM_SETBKGNDCOLOR
    SendMessage $R8 0x443 0 ${ALLYN_PANEL_REF}
    ; CHARFORMAT2W { cbSize, dwMask=CFM_COLOR, dwEffects, yHeight, yOffset, crTextColor }
    System::Call '*(i 116, i 0x40000000, i 0, i 0, i 0, i ${ALLYN_TEXT_REF}, &i88) p .R6'
    SendMessage $R8 0x444 4 $R6   ; EM_SETCHARFORMAT SCF_ALL
    System::Free $R6
    !insertmacro ALLYN_FLAT_BORDER

  ${ElseIf} $R3 == "SysListView32"
    System::Call 'uxtheme::#133(p $R8, i 1)'
    System::Call 'uxtheme::SetWindowTheme(p $R8, w "DarkMode_Explorer", p 0)'
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_PANEL}
    SendMessage $R8 0x1001 0 ${ALLYN_PANEL_REF}  ; LVM_SETBKCOLOR
    SendMessage $R8 0x1026 0 ${ALLYN_PANEL_REF}  ; LVM_SETTEXTBKCOLOR
    SendMessage $R8 0x1024 0 ${ALLYN_TEXT_REF}   ; LVM_SETTEXTCOLOR
    !insertmacro ALLYN_FLAT_BORDER

  ${ElseIf} $R3 == "msctls_progress32"
    System::Call 'uxtheme::SetWindowTheme(p $R8, w " ", w " ")'
    SendMessage $R8 0x2001 0 ${ALLYN_BAR_BG_REF} ; PBM_SETBKCOLOR
    SendMessage $R8 0x409 0 ${ALLYN_PURPLE_REF}  ; PBM_SETBARCOLOR

  ${ElseIf} $R3 == "SysLink"
    SetCtlColors $R8 ${ALLYN_PURPLE} ${ALLYN_BG}
  ${EndIf}
!macroend

; Theme the wizard frame + every control of the current page.
!macro ALLYN_APPLY_THEME
  Push $R1
  Push $R2
  Push $R3
  Push $R4
  Push $R5
  Push $R6
  Push $R7
  Push $R8

  !insertmacro ALLYN_DARK_TITLEBAR
  SetCtlColors $HWNDPARENT ${ALLYN_TEXT} ${ALLYN_BG}

  ; MUI frame controls:
  ;   1037 header title, 1038 header subtitle, 1034/1039 header background
  ;   1036 etched line under header, 1035 etched line above buttons
  ;   1028 branding text, 1256 branding "shadow" copy (white ghost on dark bg)
  GetDlgItem $R8 $HWNDPARENT 1037
  ${If} $R8 != 0
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_BG}
  ${EndIf}
  GetDlgItem $R8 $HWNDPARENT 1038
  ${If} $R8 != 0
    SetCtlColors $R8 ${ALLYN_SUBTEXT} ${ALLYN_BG}
  ${EndIf}
  GetDlgItem $R8 $HWNDPARENT 1034
  ${If} $R8 != 0
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_BG}
  ${EndIf}
  GetDlgItem $R8 $HWNDPARENT 1039
  ${If} $R8 != 0
    SetCtlColors $R8 ${ALLYN_TEXT} ${ALLYN_BG}
  ${EndIf}
  ; 1028 is a *disabled* Static, which Windows draws embossed (white ghost).
  ; Enable it so it renders as flat text in our colors.
  GetDlgItem $R8 $HWNDPARENT 1028
  ${If} $R8 != 0
    EnableWindow $R8 1
    SetCtlColors $R8 ${ALLYN_MUTED} ${ALLYN_BG}
  ${EndIf}
  ; Etched separators and the branding shadow cannot be recolored: hide them.
  GetDlgItem $R8 $HWNDPARENT 1036
  ${If} $R8 != 0
    ShowWindow $R8 0
  ${EndIf}
  GetDlgItem $R8 $HWNDPARENT 1035
  ${If} $R8 != 0
    ShowWindow $R8 0
  ${EndIf}
  GetDlgItem $R8 $HWNDPARENT 1256
  ${If} $R8 != 0
    ShowWindow $R8 0
  ${EndIf}

  ; Walk direct children of the wizard (Back/Next/Cancel live here) and,
  ; for the inner page dialog (#32770), its children as well.
  System::Call 'user32::GetWindow(p $HWNDPARENT, i 5) p .R1'
  StrCpy $R2 0
  ${While} $R1 != 0
    IntOp $R2 $R2 + 1
    ${If} $R2 > 100
      ${Break}
    ${EndIf}
    StrCpy $R8 $R1
    !insertmacro ALLYN_THEME_CONTROL
    ${If} $R3 == "#32770"
      SetCtlColors $R1 ${ALLYN_TEXT} ${ALLYN_BG}
      System::Call 'user32::GetWindow(p $R1, i 5) p .R4'
      StrCpy $R5 0
      ${While} $R4 != 0
        IntOp $R5 $R5 + 1
        ${If} $R5 > 100
          ${Break}
        ${EndIf}
        StrCpy $R8 $R4
        !insertmacro ALLYN_THEME_CONTROL
        System::Call 'user32::GetWindow(p $R4, i 2) p .R4'
      ${EndWhile}
    ${EndIf}
    System::Call 'user32::GetWindow(p $R1, i 2) p .R1'
  ${EndWhile}

  System::Call 'user32::RedrawWindow(p $HWNDPARENT, p 0, p 0, i 0x0485)'

  Pop $R8
  Pop $R7
  Pop $R6
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Pop $R1
!macroend

; Emit the callback functions. UN is "" for the installer, "un." for the uninstaller.
!macro ALLYN_THEME_FUNCTIONS UN
  Function ${UN}allyn_gui_init
    !insertmacro ALLYN_DARK_TITLEBAR
    SetCtlColors $HWNDPARENT ${ALLYN_TEXT} ${ALLYN_BG}
  FunctionEnd

  Function ${UN}allyn_page_show
    !insertmacro ALLYN_APPLY_THEME
  FunctionEnd
!macroend

!insertmacro ALLYN_THEME_FUNCTIONS ""
!ifdef ALLYN_THEME_UNINSTALLER
  !insertmacro ALLYN_THEME_FUNCTIONS "un."
!endif

!endif ; ALLYN_THEME_NSH
