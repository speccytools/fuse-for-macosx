
!define FUSE_VERSION "0.10.0.1"
!define DISPLAY_NAME "Free Unix Spectrum Emulator (Fuse) ${FUSE_VERSION}"

;Include Modern UI
!include "MUI2.nsh"

;--------------------------------
;General

Name "${DISPLAY_NAME}"
outFile "fuse-${FUSE_VERSION}-setup.exe"
Caption "${DISPLAY_NAME}"
 
installDir "C:\Program Files\Fuse"

; [Additional Installer Settings ]
XPStyle on
SetCompress force
SetCompressor lzma

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "COPYING"
  ;!insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !define MUI_FINISHPAGE_RUN "$INSTDIR\fuse.exe"
  !define MUI_FINISHPAGE_NOREBOOTSUPPORT
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

; start default section
section
 
    ; set the installation directory as the destination for the following actions
    setOutPath "$INSTDIR"
    
    ; Installation files
    File "fuse.exe"
    File "COPYING"
    File "fuse.html"
    File "*.dll"
    SetOutPath $INSTDIR\lib
    File "lib\*"
    SetOutPath "$INSTDIR\roms"
    File "roms\*"
 
    ; create the uninstaller
    writeUninstaller "$INSTDIR\uninstall.exe"

    ; create shortcuts
    CreateDirectory "$SMPROGRAMS\Fuse"
    createShortCut "$SMPROGRAMS\Fuse\Fuse.lnk" "$INSTDIR\fuse.exe"
    createShortCut "$SMPROGRAMS\Fuse\Manual.lnk" "$INSTDIR\fuse.html"
    createShortCut "$SMPROGRAMS\Fuse\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    ; Write the uninstall keys for Windows
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fuse" "DisplayName" "${DISPLAY_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fuse" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fuse" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fuse" "NoRepair" 1

sectionEnd
 
; uninstaller section start
section "uninstall"
 
    ; delete the uninstaller
    Delete "$INSTDIR\uninstall.exe"
 
    ; delete the links
    Delete "$SMPROGRAMS\Fuse\Fuse.lnk"
    Delete "$SMPROGRAMS\Fuse\Manual.lnk"
    Delete "$SMPROGRAMS\Fuse\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\Fuse"
 
    ; Installation files
    Delete "$INSTDIR\lib\*"
    RMDir  "$INSTDIR\lib"
    Delete "$INSTDIR\roms\*"
    RMDir  "$INSTDIR\roms"
    Delete "$INSTDIR\*"
    RMDir "$INSTDIR"

    ; Remove the uninstall keys for Windows
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dev-C++"
 
; uninstaller section end
sectionEnd
