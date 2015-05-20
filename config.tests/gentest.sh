SCRIPT_DIR=${0%/*}
. $SCRIPT_DIR/../scripts/functions.sh

help_post(){
  echo  "This will create a test for $1. You may change the default value: \"#include <$1.h>\" in $1/main.cpp and \"LIBS += -l$1\" in $1/$1.pro"
}

help(){
  cecho red "Usage: $0 name"
  help_post name
  exit 0
}
[ $# -lt 1 ] && help

NAME=$1
help_post $NAME
mkdir -p $NAME
cat >$NAME/${NAME}.pro <<EOF
CONFIG -= qt app_bundle
CONFIG += console

SOURCES += main.cpp

LIBS += -l$NAME
include(../paths.pri)
EOF

YEAR=`date +%Y`
COPY=../templates/COPYRIGHT.h
cat $COPY | sed "s/%YEAR%/$YEAR/g" > $NAME/main.cpp
cat >> $NAME/main.cpp <<EOF
#include <${NAME}.h>

int main()
{
	return 0;
}
EOF

echo "test for $NAME created"
