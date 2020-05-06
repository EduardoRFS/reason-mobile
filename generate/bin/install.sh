#! /bin/bash

cp not-esy-gen-findlib $cur__bin
cp not-esy-setup $cur__bin

echo "#! $(which node)" > $cur__bin/not-esy-installer
cat not-esy-installer >> $cur__bin/not-esy-installer
chmod +x $cur__bin/not-esy-installer