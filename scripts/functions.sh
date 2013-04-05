
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

die(){
  cecho red bold "$@"
  exit 1
}

print_copyright(){
  cecho red bold "Author: wbsecg1@gmail.com"
}

#http://jarit.iteye.com/blog/1070117
