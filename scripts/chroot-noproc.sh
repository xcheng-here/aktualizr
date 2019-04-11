#!/bin/sh

set -eu

EXCLUDE_MORE=$1

shift 1

FAKECHROOT_EXCLUDE_PATH="/bin:/lib:/usr:/var:/dev:/proc/self:/proc/sys:/sys:/tmp:$EXCLUDE_MORE" fakeroot fakechroot /usr/sbin/chroot . "$@"
