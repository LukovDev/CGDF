#!/bin/bash

# Запуск скрипта сборки:
python3 api_doc_gen.py "$@"

# Удаляем папку кэша питона:
rm -rf build/tools/__pycache__
