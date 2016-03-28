:: TODO: use windeployqt

copy /y %QTDIR%\bin\av*.dll bin
copy /y %QTDIR%\bin\sw*.dll bin
if exist %QTDIR%\bin\ass.dll copy /y %QTDIR%\bin\ass.dll bin
if exist %QTDIR%\bin\libass.dll copy /y %QTDIR%\bin\libass.dll bin
if exist %QTDIR%\bin\po*.dll copy /y %QTDIR%\bin\po*.dll bin
if exist %QTDIR%\bin\msvc*.dll copy /y %QTDIR%\bin\msvc*.dll bin
if exist %QTDIR%\bin\OpenAL32*.dll copy /y %QTDIR%\bin\OpenAL32*.dll bin
if exist %QTDIR%\bin\OpenAL32-%cc%.dll copy /y %QTDIR%\bin\OpenAL32-%cc%.dll bin\OpenAL32.dll
if exist bin\OpenAL32-*.dll del bin\OpenAL32-*.dll
if exist %QTDIR%\bin\D3DCompiler_*.dll copy /y %QTDIR%\bin\D3DCompiler_*.dll bin
:: libEGL, GLESv2, gcc_s_dw2-1, stdc++-6, winpthread-1
if exist %QTDIR%\bin\lib*.dll copy /y %QTDIR%\bin\lib*.dll bin

echo [Paths] > bin\qt.conf
echo Prefix=. >> bin\qt.conf

if "%mode%" == "debug" (
  if exist bin\libEGL*.dll del bin\libEGL*.dll
  if exist bin\libGLESv2*.dll del bin\libGLESv2*.dll
) else (
  if exist %QTDIR%\qml\QtQuick.2 xcopy /syi %QTDIR%\qml\QtQuick.2 bin\qml\QtQuick.2 > NUL
  if exist %QTDIR%\plugins\platforms xcopy /syi %QTDIR%\plugins\platforms bin\plugins\platforms > NUL
  if exist %QTDIR%\plugins\imageformats xcopy /syi %QTDIR%\plugins\imageformats bin\plugins\imageformats > NUL
  if exist %QTDIR%\plugins\iconengines xcopy /syi %QTDIR%\plugins\iconengines bin\plugins\iconengines > NUL
  if exist %QTDIR%\plugins\sqldrivers xcopy /syi %QTDIR%\plugins\sqldrivers bin\plugins\sqldrivers > NUL
  if exist %QTDIR%\qml\Qt xcopy /syi %QTDIR%\qml\Qt bin\qml\Qt > NUL
  if exist %QTDIR%\qml\QtQml xcopy /syi %QTDIR%\qml\QtQml bin\qml\QtQml > NUL
  if exist %QTDIR%\qml\QtQuick xcopy /syi %QTDIR%\qml\QtQuick bin\qml\QtQuick > NUL
  if exist %QTDIR%\bin\Qt5Core.dll copy /y %QTDIR%\bin\Qt5Core.dll bin
  if exist %QTDIR%\bin\Qt5Gui.dll copy /y %QTDIR%\bin\Qt5Gui.dll bin
  if exist %QTDIR%\bin\Qt5OpenGL.dll copy /y %QTDIR%\bin\Qt5OpenGL.dll bin
  if exist %QTDIR%\bin\Qt5Sql.dll copy /y %QTDIR%\bin\Qt5Sql.dll bin
  if exist %QTDIR%\bin\Qt5Svg.dll copy /y %QTDIR%\bin\Qt5Svg.dll bin
  if exist %QTDIR%\bin\Qt5Widgets.dll copy /y %QTDIR%\bin\Qt5Widgets.dll bin
  if exist %QTDIR%\bin\Qt5Network.dll copy /y %QTDIR%\bin\Qt5Network.dll bin
  if exist %QTDIR%\bin\Qt5Qml.dll copy /y %QTDIR%\bin\Qt5Qml.dll bin
  if exist %QTDIR%\bin\Qt5Quick.dll copy /y %QTDIR%\bin\Qt5Quick.dll bin
  echo deleting debug files
  if exist bin\lib*GL*d.dll del bin\lib*GL*d.dll
:: delete debug dlls. assume no *dd.dll
  forfiles /S /M *d.dll /P bin /C "cmd /C del @path"
  forfiles /S /M *.pdb /P bin /C "cmd /C del @path"
)
dir bin
xcopy /syi tools\install_sdk\mkspecs mkspecs
copy /y ..\qtc_packaging\ifw\packages\com.qtav.product.dev\data\sdk_deploy.bat .
copy /y ..\qtc_packaging\ifw\packages\com.qtav.product.player\data\*.bat .
xcopy /syi ..\src\QtAV include\QtAV
xcopy /syi ..\widgets\QtAVWidgets include\QtAVWidgets
move lib_* lib_
if exist lib_\*AV*.lib xcopy /syi lib_\*AV*.lib lib
if exist lib_\*AV*.a xcopy /syi lib_\*AV*.a lib
7z a %APPVEYOR_BUILD_FOLDER%\QtAV-Qt%qt%-%cc%%arch%-%mode%-%APPVEYOR_REPO_COMMIT:~0,7%.7z bin lib include mkspecs sdk_deploy.bat install.bat uninstall.bat > NUL
