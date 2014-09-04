. ../scripts/functions.sh

#TODO: auto add virtual functions

help(){
  cecho green "Usage: $0 [-class Class] [-base Base] [-template Template]"
  cecho green "Or $0 [--class=Class] [--base=Base] [--template=Template]"
  exit 0
}

while [ $# -gt 0 ]; do
  VAR=
  case "$1" in
  --*=*)
    PAIR=${1##--}
    VAR=${PAIRE%=*}
    VAL=${PAIR##*=}
    ;;
  -*)
    VAR=${1##-}
    shift
    VAL=$1
    ;;
  *)
    break
    ;;
  esac
  if [ -n "VAR" ]; then
    VAR_U=`echo $VAR | tr "[:lower:]" "[:upper:]"`
    echo "$VAR_U"
    eval $VAR_U=$VAL
  fi
  shift
done

echo "class: $CLASS, template: $TEMPLATE"
COPY=COPYRIGHT.h
[ -n "$CLASS" -a -n "$TEMPLATE" ] || help
[ -f $COPY ] || die 'NO copyright template found!'

CLASS_U=`echo $CLASS | tr "[:lower:]" "[:upper:]"`
YEAR=`date +%Y`

#TODO: author info
echo "generating ${CLASS}.h..."
cat $COPY | sed "s/%YEAR%/$YEAR/g" > ${CLASS}.h
cat ${TEMPLATE}.h |sed "s/%CLASS%/$CLASS/g" | sed "s/%CLASS:u%/$CLASS_U/g" >>${CLASS}.h
[ -n "$BASE" ] && cat ${CLASS}.h |sed "s/%BASE%/$BASE/g" >${CLASS}_.h && mv ${CLASS}_.h ${CLASS}.h

echo "generating ${CLASS}.cpp..."
cat $COPY | sed "s/%YEAR%/$YEAR/g" > ${CLASS}.cpp
cat ${TEMPLATE}.cpp | sed "s/%CLASS%/$CLASS/g" | sed "s/%CLASS:u%/$CLASS_U/g" >>${CLASS}.cpp
[ -n "$BASE" ] && cat ${CLASS}.cpp |sed "s/%BASE%/$BASE/g" >${CLASS}_.cpp && mv ${CLASS}_.cpp ${CLASS}.cpp

if [ -f ${TEMPLATE}_p.h ]; then
  echo "generating ${CLASS}_p.h..."
  cat $COPY | sed "s/%YEAR%/$YEAR/g" > ${CLASS}_p.h
  cat ${TEMPLATE}_p.h |sed "s/%CLASS%/$CLASS/g" | sed "s/%CLASS:u%/$CLASS_U/g" >>${CLASS}_p.h
  [ -n "$BASE" ] && cat ${CLASS}_p.h |sed "s/%BASE%/$BASE/g" >${CLASS}_p_.h && mv ${CLASS}_p_.h ${CLASS}_p.h
fi
