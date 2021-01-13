#!/bin/sh

test -d .git || exit
# git describe --tags
git log --format=format:%D | perl -ne 'next unless s#.*tag: ##; s#,.*##; next if m#/#; print; exit;'
