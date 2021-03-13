# Microsoft Developer Studio Project File - Name="container" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=container - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "container.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "container.mak" CFG="container - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "container - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "container - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "container - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "../output/container/Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\container\bbstree" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MBCSV6" /YX /FD /c
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
PostBuild_Cmds=md         ..\include\        	copy         ..\container\*.h         ..\include\        	copy         ..\container\bbstree\*.h         ..\include\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "container - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "../output/container/debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\container\bbstree" /I "..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MBCSV6" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../bin\container-d.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md         ..\include\        	copy         ..\container\*.h         ..\include\        	copy         ..\container\bbstree\*.h         ..\include\ 
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "container - Win32 Release"
# Name "container - Win32 Debug"
# Begin Group "bbstree"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\container\bbstree\__avl_tree.c
# End Source File
# Begin Source File

SOURCE=..\container\bbstree\__avl_tree.h
# End Source File
# Begin Source File

SOURCE=..\container\bbstree\__bstree.c
# End Source File
# Begin Source File

SOURCE=..\container\bbstree\__bstree.h
# End Source File
# Begin Source File

SOURCE=..\container\bbstree\myAVLTree.c
# End Source File
# Begin Source File

SOURCE=..\container\bbstree\myAVLTree.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\container\__vector_inter.c
# End Source File
# Begin Source File

SOURCE=..\container\__vector_inter.h
# End Source File
# Begin Source File

SOURCE=..\container\mybitvector.c
# End Source File
# Begin Source File

SOURCE=..\container\mybitvector.h
# End Source File
# Begin Source File

SOURCE=..\container\myBTree.c
# End Source File
# Begin Source File

SOURCE=..\container\myBTree.h
# End Source File
# Begin Source File

SOURCE=..\container\mybuffer.c
# End Source File
# Begin Source File

SOURCE=..\container\mybuffer.h
# End Source File
# Begin Source File

SOURCE=..\container\mydeque.c
# End Source File
# Begin Source File

SOURCE=..\container\mydeque.h
# End Source File
# Begin Source File

SOURCE=..\container\myhashmap.h
# End Source File
# Begin Source File

SOURCE=..\container\myhashtable.c
# End Source File
# Begin Source File

SOURCE=..\container\myhashtable.h
# End Source File
# Begin Source File

SOURCE=..\container\myheap.h
# End Source File
# Begin Source File

SOURCE=..\container\mylist.c
# End Source File
# Begin Source File

SOURCE=..\container\mylist.h
# End Source File
# Begin Source File

SOURCE=..\container\mylistex.h
# End Source File
# Begin Source File

SOURCE=..\container\mymap.h
# End Source File
# Begin Source File

SOURCE=..\container\myobj.c
# End Source File
# Begin Source File

SOURCE=..\container\myobj.h
# End Source File
# Begin Source File

SOURCE=..\container\myrbtree.c
# End Source File
# Begin Source File

SOURCE=..\container\myrbtree.h
# End Source File
# Begin Source File

SOURCE=..\container\mySkipList.c
# End Source File
# Begin Source File

SOURCE=..\container\mySkipList.h
# End Source File
# Begin Source File

SOURCE=..\container\MyStringSet.c
# End Source File
# Begin Source File

SOURCE=..\container\MyStringSet.h
# End Source File
# Begin Source File

SOURCE=..\container\MyStringSetEx.h
# End Source File
# Begin Source File

SOURCE=..\container\myTTree.c
# End Source File
# Begin Source File

SOURCE=..\container\myTTree.h
# End Source File
# Begin Source File

SOURCE=..\container\myvector.c
# End Source File
# Begin Source File

SOURCE=..\container\myvector.h
# End Source File
# Begin Source File

SOURCE=..\container\string_set.c
# End Source File
# Begin Source File

SOURCE=..\container\string_set.h
# End Source File
# End Target
# End Project
