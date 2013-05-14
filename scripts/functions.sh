
cecho() {
  while [ $# -gt 1 ]; do
    case $1 in
      red)      echo -ne "\033[49;31m";;
      green)    echo -ne "\033[49;32m";;
      brown)    echo -ne "\033[49;33m";;
      blue)     echo -ne "\033[49;34m";;
      purple)   echo -ne "\033[49;35m";;
      cyan)     echo -ne "\033[49;36m";;
      white)    echo -ne "\033[49;37m";;
      line)     echo -ne "\033[4m";;
      bold)     echo -ne "\033[1m";;
      *)        echo  $*; break;;
    esac
    shift
  done
  echo -ne "\033[0m"
}

die(){
  cecho red bold "$@" >&2
  exit 1
}

print_copyright(){
  cecho red bold "Author: wbsecg1@gmail.com"
}

#http://jarit.iteye.com/blog/1070117
