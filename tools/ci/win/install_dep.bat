:: TODO: check md5
if not exist AV (
  echo downloading qtav dep "http://sourceforge.net/projects/qtav/files/depends/QtAV-depends-windows-x86+x64.7z/download"
  appveyor DownloadFile "http://sourceforge.net/projects/qtav/files/depends/QtAV-depends-windows-x86+x64.7z/download" -FileName av.7z
  7z x av.7z > NUL
  move QtAV-depends* AV
)
xcopy /syi AV\include %QTDIR%\include > NUL
if "%arch%" == "x64" (
  xcopy /syi AV\lib\x64 %QTDIR%\lib > NUL
  copy /y AV\bin\x64\*.dll %QTDIR%\bin > NUL
) else (
  xcopy /syi AV\lib %QTDIR%\lib > NUL
  copy /y AV\bin\*.dll %QTDIR%\bin > NUL
)

