#!/bin/sh
sed -ne '1s/.*(\(.*\)-.*).*/\1/p' debian/changelog
