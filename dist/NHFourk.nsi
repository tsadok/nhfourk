Name "NHFourk"
OutFile "NHFourk.exe"
InstallDir "$PROGRAMFILES\NHFourk"

;--------------------------------

Page directory
Page instfiles

;--------------------------------

Section ""
  SetOutPath $INSTDIR
  File NHFourk\*
  FileOpen $0 $INSTDIR\record "a"
  FileClose $0
  AccessControl::GrantOnFile \
    "$INSTDIR\record" "(BU)" "GenericRead + GenericWrite"
  FileOpen $0 $INSTDIR\logfile "a"
  FileClose $0
  AccessControl::GrantOnFile \
    "$INSTDIR\logfile" "(BU)" "GenericRead + GenericWrite"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NHFourk" "DisplayName" "NetHackFourk"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NHFourk" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NHFourk" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NHFourk" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Desktop icon" SecDesktop
  SetOutPath $INSTDIR
  CreateShortcut "$DESKTOP\NHFourk.lnk" "$INSTDIR\NHFourk.exe" "" "$INSTDIR\NHFourk.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
  SetOutPath $INSTDIR
  CreateShortCut "$SMPROGRAMS\NHFourk.lnk" "$INSTDIR\NHFourk.exe" "" "$INSTDIR\NHFourk.exe" 0
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NHFourk"
  DeleteRegKey HKLM SOFTWARE\NHFourk
  Delete $INSTDIR\*
  RMDir  $INSTDIR
  Delete $SMPROGRAMS\NHFourk.lnk
  Delete $DESKTOP\NHFourk.lnk
SectionEnd
