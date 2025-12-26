::slice2cpp.exe -I. -I..\Ice -ID:\ICE-3.4.2\slice --output-dir ..\Ice QDatabase.ice			QDatabase.h;QDatabase.cpp
::slice2cpp.exe -I. -I..\Ice -ID:\ICE-3.4.2\slice --output-dir ..\Ice QStruc.ice				QStruc.h;QStruc.cpp


::E:\IBSys\ThirdParty\ice.3.7
::E:\IBSys\ThirdParty\ice.3.7\tools
::add ice path
set slice2cpp=%MyIce%\tools;
set PATH=%PATH%%slice2cpp%


::E:\IBSys\ThirdParty\ice.3.7\slice
::add appDll path
set slicePath=%MyIce%\slice;
::set PATH=%PATH%%slicePath%

slice2cpp.exe -I. -I%slicePath% --impl-c++11 --output-dir ..\Ice QStruc.ice	
slice2cpp.exe -I. -I%slicePath% --impl-c++11 --output-dir ..\Ice QDatabase.ice	




