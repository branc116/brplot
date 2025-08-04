#!/usr/bin/env sh

set -ex

test -d packages/aur/brplot-git || git clone ssh://aur@aur.archlinux.org/brplot-git.git -- packages/aur/brplot-git

echo "Update version in packages/aur/PKGBUILD"
echo "Press any key to continue..."
read

cp packages/aur/PKGBUILD packages/aur/brplot-git
cd packages/aur/brplot-git

namcap PKGBUILD
echo "Is it OK? "
read

makepkg -si
namcap *.zst
echo "Is it OK? "
read

makepkg --printsrcinfo > .SRCINFO
git add PKGBUILD .SRCINFO

echo "Write a commit message: "
read MESSAGE
git commit "$MESSAGE"

echo "Git push? "
git push origin master


