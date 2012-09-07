#!/bin/bash

set -e
rm -f ../ripinpeace_*
debuild -S -sa
cd ..
dput -f ppa:r-launchpad-gavofyork-fastmail-fm/ripinpeace ripinpeace_*_source.changes

