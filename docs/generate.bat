@echo off

python api_doc_gen.py %*
rmdir /s /q "__pycache__"
