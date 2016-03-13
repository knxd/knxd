#!/bin/sh
grep '^knxd (' debian/changelog | head -1 | awk '{print $2}' | tr -d '()'
