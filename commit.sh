#!/bin/sh

git add . --renormalize
python3 pygit.py add
python3 pygit.py commit
python3 pygit.py push
