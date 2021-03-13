# Microsoft Developer Studio Project File - Name="lincox_media" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=lincox_media - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lincox_media.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lincox_media.mak" CFG="lincox_media - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lincox_media - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lincox_media - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lincox_media - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\output\lincox_media\Release"
# PROP Intermediate_Dir "..\output\lincox_media\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINCOX_MEDIA_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\vendor\rhapsody\include" /I "..\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINCOX_MEDIA_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 winmm.lib /nologo /dll /machine:I386 /out:"..\bin/lincox_media.dll" /libpath:"..\vendor\rhapsody\bin"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy .\lcx_audio_record.h ..\include	copy .\lcx_audio_play.h ..\include	copy .\lcx_record2file.h ..\include
# End Special Build Tool

!ELSEIF  "$(CFG)" == "lincox_media - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\output\lincox_media\Debug"
# PROP Intermediate_Dir "..\output\lincox_media\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINCOX_MEDIA_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\vendor\rhapsody\include" /I "..\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINCOX_MEDIA_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib /nologo /dll /debug /machine:I386 /out:"..\bin/lincox_media-d.dll" /pdbtype:sept /libpath:"..\vendor\rhapsody\bin"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy .\lcx_audio_record.h ..\include	copy .\lcx_audio_play.h ..\include	copy .\lcx_record2file.h ..\include
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "lincox_media - Win32 Release"
# Name "lincox_media - Win32 Debug"
# Begin Source File

SOURCE=.\lcx_audio_play.c
# End Source File
# Begin Source File

SOURCE=.\lcx_audio_play.h
# End Source File
# Begin Source File

SOURCE=.\lcx_audio_record.c
# End Source File
# Begin Source File

SOURCE=.\lcx_audio_record.h
# End Source File
# Begin Source File

SOURCE=.\lcx_record2file.c
# End Source File
# Begin Source File

SOURCE=.\lcx_record2file.h
# End Source File
# Begin Source File

SOURCE=.\mymempool.c
# End Source File
# Begin Source File

SOURCE=.\myOsFile.c
# End Source File
# Begin Source File

SOURCE=.\version.rc
# End Source File
# End Target
# End Project
