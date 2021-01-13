#!/bin/sh

dpkg-parsechangelog -SVersion | sed -e 's/.*://' -e 's/-.*//'
