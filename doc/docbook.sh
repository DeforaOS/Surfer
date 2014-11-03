#!/bin/sh
#$Id$
#Copyright (c) 2012-2014 Pierre Pronchery <khorben@defora.org>
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
PREFIX="/usr/local"
[ -f "../config.sh" ] && . "../config.sh"
PROGNAME="docbook.sh"
#executables
DEBUG="_debug"
FOP="fop"
INSTALL="install -m 0644"
MKDIR="mkdir -m 0755 -p"
RM="rm -f"
XMLLINT="xmllint"
XSLTPROC="xsltproc --nonet --xinclude"


#functions
#debug
_debug()
{
	echo "$@" 1>&2
	"$@"
}


#docbook
_docbook()
{
	target="$1"

	source="${target%.*}.xml"
	[ -f "$source" ] || source="${source#$OBJDIR}"
	ext="${target##*.}"
	ext="${ext##.}"
	case "$ext" in
		html)
			XSL="http://docbook.sourceforge.net/release/xsl/current/xhtml/docbook.xsl"
			[ -f "${source%.*}.xsl" ] && XSL="${source%.*}.xsl"
			[ -f "${target%.*}.xsl" ] && XSL="${target%.*}.xsl"
			if [ -f "${target%.*}.css.xml" ]; then
				XSLTPROC="$XSLTPROC --param custom.css.source \"${target%.*}.css.xml\" --param generate.css.header 1"
			elif [ -f "${source%.*}.css.xml" ]; then
				XSLTPROC="$XSLTPROC --param custom.css.source \"${source%.*}.css.xml\" --param generate.css.header 1"
			fi
			$DEBUG $XSLTPROC -o "$target" "$XSL" "$source"
			;;
		pdf)
			XSL="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"
			[ -f "${source%.*}.xsl" ] && XSL="${source%.*}.xsl"
			[ -f "${target%.*}.xsl" ] && XSL="${target%.*}.xsl"
			$DEBUG $XSLTPROC -o "${target%.*}.fo" "$XSL" "$source" &&
			$DEBUG $FOP -fo "${target%.*}.fo" -pdf "$target"
			$RM -- "${target%.*}.fo"
			;;
		1|2|3|4|5|6|7|8|9)
			XSL="http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl"
			$DEBUG $XSLTPROC -o "$target" "$XSL" "$source"
			;;
		*)
			_error "$target: Unknown type"
			return 2
			;;
	esac

	if [ $? -ne 0 ]; then
		_error "$target: Could not create page"
		$RM -- "$target"
		return 2
	fi
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


#main
clean=0
install=0
uninstall=0
while getopts "ciuP:" name; do
	case "$name" in
		c)
			clean=1
			;;
		i)
			uninstall=0
			install=1
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
shift $((OPTIND - 1))
if [ $# -eq 0 ]; then
	_usage
	exit $?
fi

#check the variables
if [ -z "$PACKAGE" ]; then
	_error "The PACKAGE variable needs to be set"
	exit $?
fi

[ -z "$DATADIR" ] && DATADIR="$PREFIX/share"
[ -z "$MANDIR" ] && MANDIR="$DATADIR/man"

while [ $# -gt 0 ]; do
	target="$1"
	shift

	#determine the type
	ext="${target##*.}"
	ext="${ext##.}"
	case "$ext" in
		html)
			instdir="$DATADIR/doc/$ext/$PACKAGE"
			source="${target#$OBJDIR}"
			source="${source%.*}.xml"
			xpath="string(/refentry/refmeta/manvolnum)"
			section=$($XMLLINT --xpath "$xpath" "$source")
			if [ $? -eq 0 -a -n "$section" ]; then
				instdir="$DATADIR/man/html$section"
			fi
			;;
		pdf)
			instdir="$DATADIR/doc/$ext/$PACKAGE"
			;;
		1|2|3|4|5|6|7|8|9)
			instdir="$MANDIR/man$ext"
			;;
		*)
			_error "$target: Unknown type"
			exit 2
			;;
	esac

	#clean
	[ "$clean" -ne 0 ] && continue

	#uninstall
	if [ "$uninstall" -eq 1 ]; then
		target="${target#$OBJDIR}"
		$DEBUG $RM -- "$instdir/$target"		|| exit 2
		continue
	fi

	#install
	if [ "$install" -eq 1 ]; then
		source="${target#$OBJDIR}"
		$DEBUG $MKDIR -- "$instdir"			|| exit 2
		$DEBUG $INSTALL "$target" "$instdir/$source"	|| exit 2
		continue
	fi

	#create
	#XXX ignore errors
	_docbook "$target"					|| break
done
