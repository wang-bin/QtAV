:: reg add "HKEY_CLASSES_ROOT\*\shell\OpenWithQtAV\command" /ve /t REG_SZ /d """"%~dp0player.exe""" """%%1"""" /f
reg add "HKEY_CURRENT_USER\SOFTWARE\Classes\*\shell\Open with QtAV\command" /ve /t REG_SZ /d """"%~dp0bin\player.exe""" """-vo""" """gl""" """%%1"""" /f
reg add "HKEY_CURRENT_USER\SOFTWARE\Classes\*\shell\Open with QtAV QML\command" /ve /t REG_SZ /d """"%~dp0bin\QMLPlayer.exe""" """-vd""" """DXVA;FFmpeg""" """%%1"""" /f
@echo Right click an video/music file, choose "Open with QtAV" to play
pause