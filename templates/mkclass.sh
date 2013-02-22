#http://jarit.iteye.com/blog/1070117
cecho() {
  while [[ $# > 1 ]]; do
    case $1 in
      red)      echo -e "\033[49;31m$@\033[0m";;
      green)    echo -e "\033[49;32m$@\033[0m";;
      brown)    echo -e "\033[49;33m$@\033[0m";;
      blue)     echo -e "\033[49;34m$@\033[0m";;
      purple)   echo -e "\033[49;35m$@\033[0m";;
      cyan)     echo -e "\033[49;36m$@\033[0m";;
      white)    echo -e "\033[49;37m$@\033[0m";;
      line)     echo -e "\033[4m$@\033[0m";;
      bold)     echo -e "\033[1m$@\033[0m";;
      *)        break;;
    esac
    shift
  done
}

help(){
  cecho green "Usage: $0 [-class Class] [-template Template]"
  cecho green "Or $0 [--class=Class] [--template=Template]"
  exit 0
}

die(){
  cecho red bold "$@"
  exit 1
}

while [ $# -gt 0 ]; do
  VAR=
  case "$1" in
  --*=*)
    VAR=`echo $1 | sed "s,^--\(.*\)=.*,\1,"`
    VAL=`echo $1 |cut -d= -f2` ##| sed "s,^--.*=\(.*\),\1,"`
    ;;
  -*)
    VAR=`echo $1 | sed "s,^-\(.*\),\1,"`
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

echo "generating ${CLASS}.cpp..."
cat $COPY | sed "s/%YEAR%/$YEAR/g" > ${CLASS}.cpp
cat ${TEMPLATE}.cpp | sed "s/%CLASS%/$CLASS/g" | sed "s/%CLASS:u%/$CLASS_U/g" >>${CLASS}.cpp
