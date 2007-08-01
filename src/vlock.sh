#!%BOURNE_SHELL%

# ignore some signals
trap : HUP INT QUIT TSTP

VLOCK_ALL=%PREFIX%/sbin/vlock-all
VLOCK_NEW=%PREFIX%/sbin/vlock-new
VLOCK_NOSYSRQ=%PREFIX%/sbin/vlock-nosysrq
VLOCK_CURRENT=%PREFIX%/sbin/vlock-current
VLOCK_VERSION=%VLOCK_VERSION%

print_help() {
  echo >&2 "vlock: locks virtual consoles, saving your current session."
  echo >&2 "Usage: vlock [options]"
  echo >&2 "       Where [options] are any of:"
  echo >&2 "-c or --current: lock only this virtual console, allowing user to"
  echo >&2 "       switch to other virtual consoles."
  echo >&2 "-a or --all: lock all virtual consoles by preventing other users"
  echo >&2 "       from switching virtual consoles."
  echo >&2 "-n or --new: allocate a new virtual console before locking,"
  echo >&2 "       implies --all."
  echo >&2 "-s or --disable-sysrq: disable sysrq while consoles are locked to"
  echo >&2 "       prevent killing vlock with SAK, requires --all."
  echo >&2 "-v or --version: Print the version number of vlock and exit."
  echo >&2 "-h or --help: Print this help message and exit."
  exit $1
}


main() {
  local opts lock_all lock_new nosysrq

  opts=`getopt -o acnsvh --long current,all,new,disable-sysrq,version,help \
        -n vlock -- "$@"`

  if [ $? -ne 0 ] ; then
    print_help 1
  fi

  eval set -- "$opts"

  lock_all=0
  lock_new=0
  nosysrq=0

  while true ; do
    case "$1" in
      -a|--all)
        lock_all=1
        shift
        ;;
      -c|--current)
        lock_all=0
        shift
        ;;
      -s|--disable-sysrq)
        nosysrq=1
        shift
        ;;
      -n|--new)
        lock_new=1
        lock_all=1
        shift
        ;;
      -h|--help)
       print_help 0
       ;;
      -v|--version)
        echo "vlock version $VLOCK_VERSION" >&2
        exit
        ;;
      --) shift ; break ;;
      *) 
        echo "getopt error: $1" >&2
        exit 1
        ;;
    esac
  done

  if [ $lock_new -ne 0 ] ; then
    # work around an annoying X11 bug
    sleep 1
  fi

  if [ $lock_all -ne 0 ] ; then
    VLOCK_MESSAGE="\
The entire console display is now completely locked.
You will not be able to switch to another virtual console.
"
    export VLOCK_MESSAGE

    if [ $nosysrq -ne 0 ] ; then
      if [ $lock_new -ne 0 ] ; then
        export VLOCK_NEW=1
      else
        unset VLOCK_NEW
      fi

      exec "$VLOCK_NOSYSRQ"
    elif [ $lock_new -ne 0 ] ; then
      exec "$VLOCK_NEW"
    else
      exec "$VLOCK_ALL"
    fi
  else
    VLOCK_MESSAGE="This TTY is now locked."
    export VLOCK_MESSAGE

    exec "$VLOCK_CURRENT"
  fi
}

main "$@"
