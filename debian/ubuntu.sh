CHANGELOG_BAK=/tmp/qtav_changelog
mkchangelog() {
  cat >debian/changelog<<EOF
qtav ($1) $2; urgency=low

  * Upstream update

 -- Wang Bin (Lucas) <wbsecg1@gmail.com>  $(LANG=C date -R)

`cat $CHANGELOG_BAK`
EOF
}

DISTRIBUTIONS=(trusty utopic vivid wily)
DATE=`date -d @$(git log -n1 --format="%at") +%Y%m%d`
for D in ${DISTRIBUTIONS[@]}; do
  git checkout -- debian/changelog
  cp -avf debian/changelog $CHANGELOG_BAK
  VER=1.9.0~`git log -1 --pretty=format:"git${DATE}.%h~${D}" 2> /dev/null`
  mkchangelog $VER $D
  debuild -S -sa
  dput -f ppa:wbsecg1/qtav ../qtav_${VER}_source.changes
  cp -avf $CHANGELOG_BAK debian/changelog
done
