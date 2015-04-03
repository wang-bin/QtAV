@echo QtAV SDK deployment tool for installer
@echo off
choice /c iu /m "Press 'I' to install QtAV SDK, 'U' to uninstall"
set ACT=%errorlevel%
@echo Input your absolute Qt folder contains bin, include, lib etc.
@echo Or drag Qt folder here
@echo Then press ENTER
@echo example: C:\Qt5.4.1\5.4\mingw32_49

set /p QTDIR=

if "%ACT%" == "2" (
  goto uninstall
) else (
  goto install
)

:install
copy /y bin\Qt*AV*.dll %QTDIR%\bin
copy /y bin\av*.dll %QTDIR%\bin
copy /y bin\sw*.dll %QTDIR%\bin
copy /y bin\*openal*.dll %QTDIR%\bin
copy /y bin\*portaudio*.dll %QTDIR%\bin
xcopy /syi  include %QTDIR%\include
copy /y lib\*Qt*AV*.lib %QTDIR%\lib
copy /y lib\*Qt*AV*.a %QTDIR%\lib
xcopy /syi  qml\QtAV %QTDIR%\qml\QtAV
xcopy /syi mkspecs %QTDIR%\mkspecs
@echo QtAV SDK is installed
goto END

:uninstall
del %QTDIR%\bin\Qt*AV*.dll
del %QTDIR%\bin\av*.dll
del %QTDIR%\bin\sw*.dll
del %QTDIR%\bin\*openal*.dll
del %QTDIR%\bin\*portaudio*.dll
rd /s /q %QTDIR%\include\QtAV
rd /s /q %QTDIR%\include\QtAVWidgets
del %QTDIR%\lib\*Qt*AV*
del %QTDIR%\mkspecs\modules\qt_lib_av*.pri
del %QTDIR%\mkspecs\features\av*.prf
rd /s /q %QTDIR%\qml\QtAV
@echo QtAV SDK is uninstalled
goto END

:END
pause
@echo on
