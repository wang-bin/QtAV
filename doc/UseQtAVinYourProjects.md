
For QtAV version >= 1.3.4, QtAV can be installed as Qt5 modules easily. Integrating QtAV in your project is very easy. 

- Install QtAV SDK
 * Install without building QtAV https://github.com/wang-bin/QtAV/wiki/Deploy-SDK-Without-Building-QtAV
 * Or use the latest code and build yourself. Then go to building directory, `sdk_install.sh` or `sdk_install.bat`.
OSX is a little different because the shared library id must be modified but sdk_install.sh just simply copy files. you have to run QtAV/tools/sdk_osx.sh. Assume your Qt is installed in `$HOME/Qt5.3`, then run
`qtav_src_dir/sdk_osx.sh  qtav_build_dir/lib_mac_x86_64/QtAV*.framework  ~/Qt5.3/5.3/clang_64/lib`

- qmake Project

  To use QtAV, just add the following line in your Qt4 project

      CONFIG += avwidgets

  or add the following line in your Qt5 project

      QT += avwidgets

  (In Qt5, if QtWidgets module and QtAV widget based renderers are not required by your project, you can simply add `QT += av`)

- C++ Code

  add 

      #include <QtAV>
      #include <QtAVWidgets>

  Make sure `QtAV::Widgets::registerRenderers()` is called before creating a renderer.

### Try the Example

In the latest code, `examples/simpleplayer_sdk` is a complete example to show how to use QtAV SDK to write an multimedia app. You can use `simpleplayer_sdk.pro` as a template.
 
### Link Error

Because qt automatically rename the module if it's name contains `Qt`, so you may get `cannot find -lQt5AVWidgets`. As a temporary workaround, please manually rename `libQtAVWidgets.so` to `libQt5AVWidgets.so` (windows is Qt5AV.lib or Qt5AV.a) in `$QTDIR/lib`. `-lQt5AV` error is the same.

It should be fixed in 1.9.0