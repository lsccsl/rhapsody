# Microsoft Developer Studio Project File - Name="frame" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=frame - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "frame.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "frame.mak" CFG="frame - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "frame - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "frame - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "frame - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "../output/frame/Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MBCSV6" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md        ..\include\       	copy        ..\frame\*.h        ..\include\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "frame - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "frame___Win32_Debug"
# PROP BASE Intermediate_Dir "frame___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "../output/frame/debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include\\" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MBCSV6" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../bin\frame-d.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md        ..\include\       	copy        ..\frame\*.h        ..\include\ 
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "frame - Win32 Release"
# Name "frame - Win32 Debug"
# Begin Source File

SOURCE=..\frame\mydefmempool.c
# End Source File
# Begin Source File

SOURCE=..\frame\mydefmempool.h
# End Source File
# Begin Source File

SOURCE=..\frame\mylisterner.c
# End Source File
# Begin Source File

SOURCE=..\frame\mylisterner.h
# End Source File
# Begin Source File

SOURCE=..\frame\mylisterner_linux.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\frame\mylisterner_win.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\frame\mytimer_thread.c
# End Source File
# Begin Source File

SOURCE=..\frame\mytimer_thread.h
# End Source File
# End Target
# End Project
