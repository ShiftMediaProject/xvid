# Microsoft Developer Studio Project File - Name="core" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=core - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "core.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "core.mak" CFG="core - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "core - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "core - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "core - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /D "NDEBUG" /D "ARCH_X86" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"bin\core.lib"

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "ARCH_X86" /D "WIN32" /D "_MBCS" /D "_LIB" /D "BFRAMES" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"bin\core.lib"

!ENDIF 

# Begin Target

# Name "core - Win32 Release"
# Name "core - Win32 Debug"
# Begin Group "docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\authors.txt
# End Source File
# Begin Source File

SOURCE=..\..\CodingStyle
# End Source File
# Begin Source File

SOURCE=..\..\gpl.txt
# End Source File
# Begin Source File

SOURCE=..\..\todo.txt
# End Source File
# End Group
# Begin Group "bitstream"

# PROP Default_Filter ""
# Begin Group "bitstream_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\bitstream\x86_asm\cbp_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\bitstream\x86_asm\cbp_mmx.asm
InputName=cbp_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\bitstream\x86_asm\cbp_mmx.asm
InputName=cbp_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\bitstream\x86_asm\cbp_sse2.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\bitstream\x86_asm\cbp_sse2.asm
InputName=cbp_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\bitstream\x86_asm\cbp_sse2.asm
InputName=cbp_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "bitstream_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\bitstream\bitstream.h
# End Source File
# Begin Source File

SOURCE=..\..\src\bitstream\cbp.h
# End Source File
# Begin Source File

SOURCE=..\..\src\bitstream\mbcoding.h
# End Source File
# Begin Source File

SOURCE=..\..\src\bitstream\vlc_codes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\bitstream\zigzag.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\bitstream\bitstream.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bitstream\cbp.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bitstream\mbcoding.c
# End Source File
# End Group
# Begin Group "dct"

# PROP Default_Filter ""
# Begin Group "dct_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\dct\x86_asm\fdct_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\dct\x86_asm\fdct_mmx.asm
InputName=fdct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\dct\x86_asm\fdct_mmx.asm
InputName=fdct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\dct\x86_asm\idct_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\dct\x86_asm\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\dct\x86_asm\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "dct_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\dct\fdct.h
# End Source File
# Begin Source File

SOURCE=..\..\src\dct\idct.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\dct\fdct.c
# End Source File
# Begin Source File

SOURCE=..\..\src\dct\idct.c
# End Source File
# End Group
# Begin Group "image"

# PROP Default_Filter ""
# Begin Group "image_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\image\x86_asm\interpolate8x8_3dn.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\interpolate8x8_3dn.asm
InputName=interpolate8x8_3dn

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\interpolate8x8_3dn.asm
InputName=interpolate8x8_3dn

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\interpolate8x8_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\interpolate8x8_mmx.asm
InputName=interpolate8x8_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\interpolate8x8_mmx.asm
InputName=interpolate8x8_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\interpolate8x8_xmm.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\interpolate8x8_xmm.asm
InputName=interpolate8x8_xmm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\interpolate8x8_xmm.asm
InputName=interpolate8x8_xmm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\rgb_to_yv12_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\rgb_to_yv12_mmx.asm
InputName=rgb_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\rgb_to_yv12_mmx.asm
InputName=rgb_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\yuv_to_yv12_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\yuv_to_yv12_mmx.asm
InputName=yuv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\yuv_to_yv12_mmx.asm
InputName=yuv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\yuyv_to_yv12_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\yuyv_to_yv12_mmx.asm
InputName=yuyv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\yuyv_to_yv12_mmx.asm
InputName=yuyv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\yv12_to_rgb24_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\yv12_to_rgb24_mmx.asm
InputName=yv12_to_rgb24_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\yv12_to_rgb24_mmx.asm
InputName=yv12_to_rgb24_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\yv12_to_rgb32_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\yv12_to_rgb32_mmx.asm
InputName=yv12_to_rgb32_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\yv12_to_rgb32_mmx.asm
InputName=yv12_to_rgb32_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\image\x86_asm\yv12_to_yuyv_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\image\x86_asm\yv12_to_yuyv_mmx.asm
InputName=yv12_to_yuyv_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\image\x86_asm\yv12_to_yuyv_mmx.asm
InputName=yv12_to_yuyv_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "image_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\image\colorspace.h
# End Source File
# Begin Source File

SOURCE=..\..\src\image\font.h
# End Source File
# Begin Source File

SOURCE=..\..\src\image\image.h
# End Source File
# Begin Source File

SOURCE=..\..\src\image\interpolate8x8.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\image\colorspace.c
# End Source File
# Begin Source File

SOURCE=..\..\src\image\font.c
# End Source File
# Begin Source File

SOURCE=..\..\src\image\image.c
# End Source File
# Begin Source File

SOURCE=..\..\src\image\interpolate8x8.c
# End Source File
# End Group
# Begin Group "motion"

# PROP Default_Filter ""
# Begin Group "motion_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\motion\x86_asm\sad_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\motion\x86_asm\sad_mmx.asm
InputName=sad_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\motion\x86_asm\sad_mmx.asm
InputName=sad_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\motion\x86_asm\sad_sse2.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\motion\x86_asm\sad_sse2.asm
InputName=sad_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\motion\x86_asm\sad_sse2.asm
InputName=sad_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\motion\x86_asm\sad_xmm.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\motion\x86_asm\sad_xmm.asm
InputName=sad_xmm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\motion\x86_asm\sad_xmm.asm
InputName=sad_xmm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "motion_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\motion\motion.h
# End Source File
# Begin Source File

SOURCE=..\..\src\motion\sad.h
# End Source File
# Begin Source File

SOURCE=..\..\src\motion\smp_motion_est.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\motion\motion_comp.c
# End Source File
# Begin Source File

SOURCE=..\..\src\motion\motion_est.c
# End Source File
# Begin Source File

SOURCE=..\..\src\motion\sad.c
# End Source File
# Begin Source File

SOURCE=..\..\src\motion\smp_motion_est.c
# End Source File
# End Group
# Begin Group "prediction"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\prediction\mbprediction.c
# End Source File
# Begin Source File

SOURCE=..\..\src\prediction\mbprediction.h
# End Source File
# End Group
# Begin Group "quant"

# PROP Default_Filter ""
# Begin Group "quant_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\quant\x86_asm\quantize4_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\quant\x86_asm\quantize4_mmx.asm
InputName=quantize4_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=..\..\src\quant\x86_asm\quantize4_mmx.asm
InputName=quantize4_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\quant\x86_asm\quantize_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\quant\x86_asm\quantize_mmx.asm
InputName=quantize_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=..\..\src\quant\x86_asm\quantize_mmx.asm
InputName=quantize_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "quant_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\quant\adapt_quant.h
# End Source File
# Begin Source File

SOURCE=..\..\src\quant\quant_h263.h
# End Source File
# Begin Source File

SOURCE=..\..\src\quant\quant_matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\src\quant\quant_mpeg4.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\quant\adapt_quant.c
# End Source File
# Begin Source File

SOURCE=..\..\src\quant\quant_h263.c
# End Source File
# Begin Source File

SOURCE=..\..\src\quant\quant_matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\src\quant\quant_mpeg4.c
# End Source File
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Group "utils_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\utils\x86_asm\cpuid.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\utils\x86_asm\cpuid.asm
InputName=cpuid

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\utils\x86_asm\cpuid.asm
InputName=cpuid

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\utils\x86_asm\mem_transfer_mmx.asm

!IF  "$(CFG)" == "core - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=..\..\src\utils\x86_asm\mem_transfer_mmx.asm
InputName=mem_transfer_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=..\..\src\utils\x86_asm\mem_transfer_mmx.asm
InputName=mem_transfer_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "utils_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\utils\emms.h
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\mbfunctions.h
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\mem_align.h
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\mem_transfer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\ratecontrol.h
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\timer.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\utils\emms.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\mbtransquant.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\mem_align.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\mem_transfer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\ratecontrol.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\timer.c
# End Source File
# End Group
# Begin Group "xvidcore_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\src\divx4.h
# End Source File
# Begin Source File

SOURCE=..\..\src\encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\src\global.h
# End Source File
# Begin Source File

SOURCE=..\..\src\portab.h
# End Source File
# Begin Source File

SOURCE=..\..\src\xvid.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\decoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src\divx4.c
# End Source File
# Begin Source File

SOURCE=..\..\src\encoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src\xvid.c
# End Source File
# End Target
# End Project
