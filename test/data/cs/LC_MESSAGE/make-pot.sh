#!/bin/sh

xgettext ../../../*.cpp --keyword=gettext:1,1t --keyword=pgettext:1c,2,2t --keyword=ngettext:1,2,3t --keyword=npgettext:1c,2,3,4t -o messages.pot
