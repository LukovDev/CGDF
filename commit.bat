@echo off
git add . --renormalize
python pygit.py add
python pygit.py commit
python pygit.py push
pause
