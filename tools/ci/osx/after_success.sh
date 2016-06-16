exit 0
cd tools
git clone https://github.com/andreyvit/create-dmg.git

./deploy_osx.sh $TRAVIS_BUILD_DIR/$QTAV_OUT


PKG=$TRAVIS_BUILD_DIR/${QTAV_OUT}-QMLPlayer.dmg
mv QtAV-QMLPlayer.dmg $PKG

wget http://sourceforge.net/projects/sshpass/files/sshpass/1.05/sshpass-1.05.tar.gz/download -O sshpass.tar.gz
tar zxf sshpass.tar.gz
cd sshpass-1.05
./configure
make
sudo make install
export PATH=$PWD:$PATH
cd $TRAVIS_BUILD_DIR
type -a sshpass
# ignore known hosts: -o StrictHostKeyChecking=no
sshpass -p $SF_PWD scp -o StrictHostKeyChecking=no $PKG $SF_USER,qtav@frs.sourceforge.net:/home/frs/project/q/qt/qtav/ci/
