CHANGELOG_BAK=/tmp/qtav_changelog
CONTROL_BAK=/tmp/qtav_control
mkchangelog() {
  cat >debian/changelog<<EOF
qtav ($1) $2; urgency=low

  * Upstream update

 -- Wang Bin (Lucas) <wbsecg1@gmail.com>  $(LANG=C date -R)

`cat $CHANGELOG_BAK`
EOF
}

DISTRIBUTIONS=(trusty vivid wily xenial yakkety zesty artful)
DATE=`date -d @$(git log -n1 --format="%at") +%Y%m%d`
idx=0
for D in ${DISTRIBUTIONS[@]}; do
  git checkout -- debian/changelog debian/control
  cp -avf debian/control $CONTROL_BAK
  cp -avf debian/changelog $CHANGELOG_BAK
  VER=1.12.0~`git log -1 --pretty=format:"git${DATE}.%h~${D}" 2> /dev/null`
  mkchangelog $VER $D
  if [ $idx -gt 3 ]; then
    sed -i 's,qtdeclarative5-controls-plugin,qml-module-qtquick-controls,g;s,qtdeclarative5-folderlistmodel-plugin,qml-module-qt-labs-folderlistmodel,g;s,qtdeclarative5-settings-plugin,qml-module-qt-labs-settings,g;s,qtdeclarative5-dialogs-plugin,qml-module-qtquick-dialogs,g' debian/control
  fi
  debuild -S -sa
  dput -f ppa:wbsecg1/qtav ../qtav_${VER}_source.changes
  cp -avf $CHANGELOG_BAK debian/changelog
  cp -avf $CONTROL_BAK debian/control
  idx=$((idx+1))
done
