echo cc=%cc%
if "%cc%" == "MinGW" (
  qmake %APPVEYOR_BUILD_FOLDER%
  mingw32-make -j%NUMBER_OF_PROCESSORS%
) else (
  if "%mode%" == "debug" (
    qmake %APPVEYOR_BUILD_FOLDER% -r -tp vc "CONFIG+=debug"
    msbuild /m /p:Configuration=DEBUG
  ) else (
    qmake %APPVEYOR_BUILD_FOLDER%
    nmake
  )
)
