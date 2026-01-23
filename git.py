#
# git.py - Специальный скрипт упрощающий работу с системой версий git и публикаций изменений в GitHub.
#
# В случае ошибки доверенности пропишите:
# git config --global --add safe.directory <полный/путь/до/текущей/директории>
#


# Импортируем:
import sys
import subprocess
from datetime import datetime


# Функция для показа отсутствующих аргументов команды:
def error_missing_args(name: str) -> None:
    raise Exception(f"Error ({name}): Missing arguments.")


# Добавить изменения к коммиту:
def add(path: str) -> None:
    subprocess.run(["git", "add", path], check=True)


# Создать коммит:
def commit() -> None:
    commit_time = datetime.now().strftime("commit %d %B %Y %H:%M").lower()
    subprocess.run(["git", "commit", "-m", commit_time], check=True)


# Получить изменения:
def fetch() -> None:
    subprocess.run(["git", "fetch"], check=True)


# Получить изменения:
def merge() -> None:
    subprocess.run(["git", "merge"], check=True)


# Получить изменения:
def pull() -> None:
    subprocess.run(["git", "pull"], check=True)


# Отправить изменения:
def push(branch: str) -> None:
    subprocess.run(["git", "push", "origin", branch], check=True)


# Сменить ветку (старый способ):
def checkout(args: list) -> None:
    subprocess.run(["git", "checkout", *args], check=True)


# Сменить ветку (новый способ):
def switch(args: list) -> None:
    subprocess.run(["git", "switch", *args], check=True)


# Создать локальную ветку:
def branch(args: list) -> None:
    subprocess.run(["git", "branch", *args], check=True)


# Откат/удаление/отмена коммита:
def reset(args: list) -> None:
    """
    git reset --soft HEAD~1
    Пример:
    Ты хочешь отменить коммит, но оставить изменения staged (для повторного коммита, например с другим сообщением).

    git reset --mixed HEAD~1
    # то же, что просто:
    git reset HEAD~1
    Пример: ты хочешь откатить коммит и подготовку файлов (git add), но не терять изменения.

    git reset --hard HEAD~1
    Пример: ты хочешь всё откатить до состояния предыдущего коммита, как будто изменений не было вообще.

    Чтобы отправить на гитхаб:
    git push --force-with-lease origin master
    """

    # Проверяем уверен ли пользователь:
    sure = input("You are sure? y/n: ")
    if sure != "y": print("Cancelled."); return
    sure = input("<!> ARE YOU SURE ABOUT YOUR ACTIONS USING THIS FUNCTION? <!> y/n: ")
    if sure != "y": print("Cancelled."); return

    subprocess.run(["git", "reset", *args], check=True)


# Если этот скрипт запускают:
if __name__ == "__main__":
    arguments = {
        "add":      lambda a: add(a[0] if a else "."),
        "commit":   lambda a: commit(),
        "fetch":    lambda a: fetch(),
        "merge":    lambda a: merge(),
        "pull":     lambda a: pull(),
        "push":     lambda a: push(a[0] if a else "master"),
        "checkout": lambda a: checkout(a if a else error_missing_args("checkout")),
        "switch":   lambda a: switch(a if a else error_missing_args("switch")),
        "branch":   lambda a: branch(a if a else error_missing_args("branch")),
        "reset":    lambda a: reset(a if a else error_missing_args("reset")),
    }

    # Обрабатываем аргументы:
    try:
        args = sys.argv[1:]
        arg = args[0].lower()
        if arg in ["-h", "-help"]: print(f"List of arguments ({len(arguments.keys())}):\n{', '.join(arguments.keys())}")
        elif arg in arguments.keys(): arguments[arg](args[1:])
        else: print(f"Error ({arg}): Argument not found. Try -h or -help.")
    except subprocess.CalledProcessError as error:
        print(f"Error ({arg}): {error}")
