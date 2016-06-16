SCRIPT_DIR=${0%/*}

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
include(../paths.pri)

TARGET = ${NAME}_test
SOURCES += main.cpp
LIBS += -l$NAME
EOF

YEAR=`date +%Y`
COPY=../tools/templates/COPYRIGHT.h
cat $COPY | sed "s/%YEAR%/$YEAR/g" > $NAME/main.cpp
cat >> $NAME/main.cpp <<EOF
#include <${NAME}.h>

void test() {
}
EOF

echo "test for $NAME created"
