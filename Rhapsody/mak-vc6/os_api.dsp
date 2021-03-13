# Microsoft Developer Studio Project File - Name="os_api" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=os_api - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "os_api.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "os_api.mak" CFG="os_api - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "os_api - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "os_api - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "os_api - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "../output/os_api/Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "./sync/" /I "../os_api/" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
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
PostBuild_Cmds=md             ..\include\       	copy             ..\os_api\*.h             ..\include\            	copy             ..\os_api\advance_sync\*.h             ..\include\            	copy             ..\os_api\sync\*.h             ..\include\            	copy             ..\os_api\ipc\*.h             ..\include\            	copy             ..\os_api\file\*.h             ..\include\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "os_api - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "os_api___Win32_Debug"
# PROP BASE Intermediate_Dir "os_api___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "../output/os_api/debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "./sync/" /I "../os_api/" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../bin\os_api-d.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=md             ..\include\            	copy             ..\os_api\*.h             ..\include\            	copy             ..\os_api\advance_sync\*.h             ..\include\            	copy             ..\os_api\sync\*.h             ..\include\            	copy             ..\os_api\ipc\*.h             ..\include\            	copy             ..\os_api\file\*.h             ..\include\ 
PostBuild_Cmds=md             ..\include\       	copy             ..\os_api\*.h             ..\include\            	copy             ..\os_api\advance_sync\*.h             ..\include\            	copy             ..\os_api\sync\*.h             ..\include\            	copy             ..\os_api\ipc\*.h             ..\include\            	copy             ..\os_api\file\*.h             ..\include\ 
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "os_api - Win32 Release"
# Name "os_api - Win32 Debug"
# Begin Group "file"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\os_api\file\myOsFile.c
# End Source File
# Begin Source File

SOURCE=..\os_api\file\myOsFile.h
# End Source File
# End Group
# Begin Group "ipc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\os_api\ipc\mymmap.c
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mymmap.h
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mymmap_linux.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mymmap_win32.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mynamepipe.c
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mynamepipe.h
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mynamepipe_linux.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mynamepipe_win32.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mypipe.c
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mypipe.h
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mypipe_linux.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mypipe_win32.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mysock.c
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mysock.h
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mysysvmsg.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\os_api\ipc\mysysvmsg.h
# End Source File
# End Group
# Begin Group "sync"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\os_api\sync\myevent.c
# End Source File
# Begin Source File

SOURCE=..\os_api\sync\myevent.h
# End Source File
# Begin Source File

SOURCE=..\os_api\sync\mymutex.c
# End Source File
# Begin Source File

SOURCE=..\os_api\sync\mymutex.h
# End Source File
# Begin Source File

SOURCE=..\os_api\sync\mysem.c
# End Source File
# Begin Source File

SOURCE=..\os_api\sync\mysem.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\os_api\gettimeofday.h
# End Source File
# Begin Source File

SOURCE=..\os_api\gettimeofday_win32.c
# End Source File
# Begin Source File

SOURCE=..\os_api\myhandleSet.h
# End Source File
# Begin Source File

SOURCE=..\os_api\myos.c
# End Source File
# Begin Source File

SOURCE=..\os_api\myrand.c
# End Source File
# Begin Source File

SOURCE=..\os_api\myrand.h
# End Source File
# Begin Source File

SOURCE=..\os_api\mythread.c
# End Source File
# Begin Source File

SOURCE=..\os_api\mythread.h
# End Source File
# Begin Source File

SOURCE=..\os_api\os_def.h
# End Source File
# End Target
# End Project
