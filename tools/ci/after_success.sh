exit 0
set -ev

# download ifw for linux, mingw

# upload packages

# TRAVIS_TAG
echo TRAVIS_TAG=$TRAVIS_TAG

PKG=$TRAVIS_BUILD_DIR/$QTAV_OUT.tar.xz
tar Jcvf $PKG $QTAV_OUT/{bin,lib*,build.log}

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
#rsync --password-file=$PWF -avP -e ssh $PKG $SF_USER@frs.sourceforge.net:/home/frs/project/qtav/ci/
#curl -H "Accept: application/json" -X PUT -d "default=linux&default=bsd&default=solaris" -d "api_key=$SF_API_KEY" https://sourceforge.net/projects/qtav/files/ci/$PKG
