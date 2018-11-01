#!/bin/sh

if [ "$(uname)" = "Darwin" ]; then
	include=" -I/opt/local/share/aclocal"
fi

autoreconf --install --verbose --force $include
