#!/bin/sh
#$Id$
#Copyright (c) 2010-2020 Pierre Pronchery <khorben@defora.org>
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



#variables
CONFIGSH="${0%/gettext.sh}/../config.sh"
PREFIX="/usr/local"
LOCALEDIR="$PREFIX/share/locale"
POTFILES="POTFILES"
PROGNAME="gettext.sh"
#executables
DEBUG="_debug"
INSTALL="install -m 0644"
MKDIR="mkdir -p"
MSGFMT="msgfmt"
MSGINIT="msginit"
MSGMERGE="msgmerge"
RM="rm -f"
XGETTEXT="xgettext --force-po"

[ -f "$CONFIGSH" ] && . "$CONFIGSH"


#functions
#debug
_debug()
{
	echo "$@" 1>&3
	"$@"
}


#error
_error()
{
	echo "$PROGNAME: $@" 1>&2
	return 2
}


#usage
_usage()
{
	echo "Usage: $PROGNAME [-c|-i|-u][-P prefix] target..." 1>&2
	return 1
}


#gettext_mo
_gettext_mo()
{
	package="$1"
	lang="$2"
	potfile="$3"
	pofile="$4"
	mofile="$5"

	_gettext_po "$package" "$lang" "$potfile" "$pofile"	|| return 1
	$DEBUG $MSGFMT -c -v -o "$mofile" "$pofile"		|| return 1
}


#gettext_po
_gettext_po()
{
	package="$1"
	lang="$2"
	potfile="$3"
	pofile="$4"

	if [ -f "$pofile" ]; then
		$DEBUG $MSGMERGE -U "$pofile" "$potfile"	|| return 1
	else
		$DEBUG $MSGINIT -l "$lang" -o "$pofile" -i "$potfile" \
								|| return 1
	fi
}


#gettext_pot
_gettext_pot()
{
	package="$1"
	potfile="$2"

	$DEBUG $XGETTEXT -d "$package" -o "$potfile" --keyword="_" \
			--keyword="N_" -f "$POTFILES"		|| return 1
}


#main
clean=0
install=0
uninstall=0
while getopts "ciO:uP:" name; do
	case "$name" in
		c)
			clean=1
			;;
		i)
			uninstall=0
			install=1
			;;
		O)
			export "${OPTARG%%=*}"="${OPTARG#*=}"
			;;
		u)
			install=0
			uninstall=1
			;;
		P)
			PREFIX="$OPTARG"
			;;
		?)
			_usage
			exit $?
			;;
	esac
done
shift $(($OPTIND - 1))
if [ $# -lt 1 ]; then
	_usage
	exit $?
fi

#check the variables
if [ -z "$PACKAGE" ]; then
	_error "The PACKAGE variable needs to be set"
	exit $?
fi

LOCALEDIR="$PREFIX/share/locale"
exec 3>&1
while [ $# -gt 0 ]; do
	target="$1"
	source="${target#$OBJDIR}"
	lang="${source%%.mo}"
	lang="${lang%%.po}"
	shift

	#clean
	[ "$clean" -ne 0 ] && continue

	#uninstall
	if [ "$uninstall" -eq 1 ]; then
		$DEBUG $RM "$LOCALEDIR/$lang/LC_MESSAGES/$PACKAGE.mo" \
								|| exit 2
		continue
	fi

	#install
	if [ "$install" -eq 1 ]; then
		$DEBUG $MKDIR "$LOCALEDIR/$lang/LC_MESSAGES"	|| exit 2
		$DEBUG $INSTALL "$target" \
			"$LOCALEDIR/$lang/LC_MESSAGES/$PACKAGE.mo" \
								|| exit 2
		continue
	fi

	#create
	case "$target" in
		*.mo)
			#XXX may not match
			if [ -n "$OBJDIR" ]; then
				potfile="$OBJDIR/$PACKAGE.pot"
			else
				potfile="$PACKAGE.pot"
			fi
			mofile="$target"
			pofile="${source%%.mo}.po"
			_gettext_mo "$PACKAGE" "$lang" "$potfile" "$pofile" \
				"$mofile"			|| exit 2
			;;
		*.po)
			#XXX may not match
			if [ -n "$OBJDIR" ]; then
				potfile="$OBJDIR/$PACKAGE.pot"
			else
				potfile="$PACKAGE.pot"
			fi
			pofile="$target"
			_gettext_po "$PACKAGE" "$lang" "$potfile" "$pofile" \
								|| exit 2
			;;
		*.pot)
			package="${source%%.pot}"
			potfile="$target"
			_gettext_pot "$package" "$potfile"	|| exit 2
			;;
		*)
			exit 2
			;;
	esac
done
