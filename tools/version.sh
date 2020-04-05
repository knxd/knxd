#!/bin/sh

test -d .git || exit
git describe --tags
