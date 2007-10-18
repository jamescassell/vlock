#!%BOURNE_SHELL%
#
# vlock.sh -- start script for vlock, the VT locking program for linux
# 
# This program is copyright (C) 2007 Frank Benkstein, and is free
# software which is freely distributable under the terms of the
# GNU General Public License version 2, included as the file COPYING in this
# distribution.  It is NOT public domain software, and any
# redistribution not permitted by the GNU General Public License is
# expressly forbidden without prior written permission from
# the author.

# Ignore some signals.
trap : HUP INT QUIT TSTP

# Exit on error.
set -e

# Magic characters to clear the terminal.
CLEAR_SCREEN="`echo -e '\033[H\033[J'`"

# Enter message that is common to different the messages.
VLOCK_ENTER_PROMPT="Please press [ENTER] to unlock."

# Message that is displayed when console switching is disabled.
VLOCK_ALL_MESSAGE="${CLEAR_SCREEN}\
The entire console display is now completely locked.
You will not be able to switch to another virtual console.

${VLOCK_ENTER_PROMPT}"

# Message that is displayed when only the current terminal is locked.
VLOCK_CURRENT_MESSAGE="${CLEAR_SCREEN}\
This TTY is now locked.

${VLOCK_ENTER_PROMPT}"

# Read user settings.
if [ -r "${HOME}/.vlockrc" ] ; then
  . "${HOME}/.vlockrc"
fi

# "Compile" time variables.
VLOCK_MAIN="%PREFIX%/sbin/vlock-main"
VLOCK_VERSION="%VLOCK_VERSION%"
# If set to "y" plugin support is enabled in vlock-main.
VLOCK_ENABLE_PLUGINS="%VLOCK_ENABLE_PLUGINS%"

print_help() {
  echo >&2 "vlock: locks virtual consoles, saving your current session."
  if [ "${VLOCK_ENABLE_PLUGINS}" = "yes" ] ; then
    echo >&2 "Usage: vlock [options] [plugins...]"
  else
    echo >&2 "Usage: vlock [options]"
  fi
  echo >&2 "       Where [options] are any of:"
  echo >&2 "-c or --current: lock only this virtual console, allowing user to"
  echo >&2 "       switch to other virtual consoles."
  echo >&2 "-a or --all: lock all virtual consoles by preventing other users"
  echo >&2 "       from switching virtual consoles."
  if [ "${VLOCK_ENABLE_PLUGINS}" = "yes" ] ; then
    echo >&2 "-n or --new: allocate a new virtual console before locking,"
    echo >&2 "       implies --all."
    echo >&2 "-s or --disable-sysrq: disable SysRq while consoles are locked to"
    echo >&2 "       prevent killing vlock with SAK, implies --all."
    echo >&2 "-t <seconds> or --timeout <seconds>: run screen saver plugins"
    echo >&2 "       after the given amount of time."
  fi
  echo >&2 "-v or --version: Print the version number of vlock and exit."
  echo >&2 "-h or --help: Print this help message and exit."
}

# Export variables only if they are set.  Some shells create an empty variable
# on export even if it was previously unset.
export_if_set() {
  while [ $# -gt 0 ] ; do
    if ( eval [ "\"\${$1+set}\"" = "set" ] ) ; then
      eval export $1
    fi
    shift
  done
}

main() {
  while [ $# -gt 0 ] ; do
    echo "\$@: $@"

    case "$1" in
      -[!-]?*)
        # convert things clashed arguments like "-foobar" to "-f -o -o -b -a -r"
        local options

        # strip "-" to get a list of option characters
        options="${1#-}"
        # pop $1 from $@
        shift

        while [ -n "${options}" ] ; do
          # get last option character
          option="$(expr substr "${options}" "${#options}" 1)"
          # strip it from the list of option characters
          options="${options%?}"
          # prepend option "-" plus option character to $@
          set -- "-${option}" "$@"
        done
      ;;
      -a|--all)
        plugins="${plugins} all"
        shift
        ;;
      -c|--current)
        unset plugins
        shift
        ;;
      -n|--new)
        plugins="${plugins} new"
        shift
        ;;
      -s|--disable-sysrq)
        plugins="${plugins} nosysrq"
        shift
        ;;
      -t|--timeout)
        shift
        VLOCK_TIMEOUT="$1"
        shift
        ;;
      -h|--help)
       print_help
       exit
       ;;
      -v|--version)
        if [ "${VLOCK_ENABLE_PLUGINS}" = "yes" ] ; then
          echo >&2 "vlock version ${VLOCK_VERSION}"
        else
          echo >&2 "vlock version ${VLOCK_VERSION} (no plugin support)"
        fi
        exit
        ;;
      -[!-]|--?*)
        echo >&1 "$0: unknown option '$1'"
        print_help
        exit 1
      ;;
      --)
        # End of option list.
        shift
        break
        ;;
      *)
        local argument has_double_dash
        for argument ; do
          if [ "${argument}" = "--" ] ; then
            has_double_dash="yes"
            break
          fi
        done

        if [ -n "${has_double_dash}" ] ; then
          set -- "$@" "$1"
        else
          set -- "$@" -- "$1"
        fi

        shift
        ;;
    esac
  done

  # Export variables for vlock-main.
  export_if_set VLOCK_TIMEOUT VLOCK_PROMPT_TIMEOUT
  export_if_set VLOCK_MESSAGE VLOCK_ALL_MESSAGE VLOCK_CURRENT_MESSAGE

  if [ "${VLOCK_ENABLE_PLUGINS}" = "yes" ] ; then
    exec "${VLOCK_MAIN}" ${plugins} ${VLOCK_PLUGINS} "$@"
  else
    exec "${VLOCK_MAIN}" ${plugins}
  fi
}

main "$@"
