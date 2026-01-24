@echo off
git.exe add . --renormalize
python pygit.py add
python pygit.py commit
python pygit.py push
pause
