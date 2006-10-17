# Microsoft Developer Studio Project File - Name="GStaris" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GStaris - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GStaris.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GStaris.mak" CFG="GStaris - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GStaris - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GStaris - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GStaris - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "D:\Taris\pcsx2\Plugins"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GSTARIS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /O2 /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GSTARIS_EXPORTS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "GStaris - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "D:\Taris\pcsx2\Plugins"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GSTARIS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GSTARIS_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "GStaris - Win32 Release"
# Name "GStaris - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\gif.c
# End Source File
# Begin Source File

SOURCE=..\GStaris.c
# End Source File
# Begin Source File

SOURCE=..\idt.c
# End Source File
# Begin Source File

SOURCE=..\misc.c
# End Source File
# Begin Source File

SOURCE=..\prim.c
# End Source File
# Begin Source File

SOURCE=..\registers.c
# End Source File
# Begin Source File

SOURCE=..\textures.c
# End Source File
# Begin Source File

SOURCE=.\win32.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "PS2E Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\PS2Edefs.h
# End Source File
# Begin Source File

SOURCE=..\PS2Etypes.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\GStaris.h
# End Source File
# End Group
# Begin Group "Misc Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GStaris.def
# End Source File
# Begin Source File

SOURCE=.\GStaris.rc
# End Source File
# End Group
# Begin Group "Notes"

# PROP Default_Filter ""
# Begin Group "Old"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Notes\OldFiles\README.txt
# End Source File
# Begin Source File

SOURCE="..\Notes\OldFiles\Special notes 1.txt"
# End Source File
# End Group
# Begin Source File

SOURCE="..\Betatest\0.6-2\alphalog_gst06beta3.txt"
# End Source File
# Begin Source File

SOURCE=..\Notes\readme.txt
# End Source File
# Begin Source File

SOURCE=..\Notes\techs.txt
# End Source File
# Begin Source File

SOURCE=..\Notes\TODO.TXT
# End Source File
# End Group
# Begin Source File

SOURCE=..\Notes\gpl.txt
# End Source File
# End Target
# End Project
