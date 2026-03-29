#
# api_doc_gen.py - Генератор API документации.
#
# Запускать из директории docs/
#
# Пример:
# cd docs
# python api_doc_gen.py
#
# Результат:
# docs/api_doc.md
#
# <!> Большая часть кода генератора была написана ИИ <!>
#


# Подключаем:
import glob
import os
import re
import time
from collections import defaultdict


# Константы и настройки:
VERSION = "1.0.0"
output_filename = "docs/api_doc.md"  # Файл вывода.
found_formats = ["h", "hpp"]         # Какие файлы искать и обрабатывать.
find_dirs = ["src/cgdf/"]            # В каких директориях искать.
ignore_dirs = ["src/cgdf/core/libs/", "src/cgdf/graphics/opengl/glad/"]
ignore_dirs = [os.path.normpath(p) for p in ignore_dirs]  # Нормализуем пути.


# Рекурсивный поиск файлов:
def find_files(path: str, ext: str) -> list[str]:
    pattern = os.path.join(path, f"**/*.{ext}")
    return [p.replace("\\", "/") for p in glob.glob(pattern, recursive=True)]


# Рекурсивный поиск всех нужных файлов:
def find_all_target_files() -> list[str]:
    files: list[str] = []
    for src_dir in find_dirs:
        for ext in found_formats:
            for file_path in find_files(src_dir, ext):
                norm_dir = os.path.normpath(os.path.dirname(file_path))
                if norm_dir in ignore_dirs:
                    continue
                files.append(file_path)
    return sorted(set(files))


# Убирает маркеры //, /*, */ и лишние символы у комментария:
def strip_comment_markers(text: str) -> str:
    text = text.strip()
    if text.startswith("//"):
        return text[2:].strip()
    if text.startswith("/*"):
        text = text[2:]
    if text.endswith("*/"):
        text = text[:-2]
    return text.strip(" *\t")


# Удаляет //-комментарий из строки, не ломая строки внутри кавычек:
def remove_line_comment(line: str) -> str:
    in_str = False
    str_ch = ""
    esc = False
    i = 0
    while i < len(line):
        ch = line[i]
        if esc:
            esc = False
            i += 1
            continue
        if in_str:
            if ch == "\\":
                esc = True
            elif ch == str_ch:
                in_str = False
            i += 1
            continue
        if ch in ("'", "\""):
            in_str = True
            str_ch = ch
            i += 1
            continue
        if ch == "/" and i + 1 < len(line) and line[i + 1] == "/":
            return line[:i]
        i += 1
    return line


# Извлекает текст inline-комментария после // (если есть):
def inline_comment(line: str) -> str:
    in_str = False
    str_ch = ""
    esc = False
    i = 0
    while i < len(line):
        ch = line[i]
        if esc:
            esc = False
            i += 1
            continue
        if in_str:
            if ch == "\\":
                esc = True
            elif ch == str_ch:
                in_str = False
            i += 1
            continue
        if ch in ("'", '"'):
            in_str = True
            str_ch = ch
            i += 1
            continue
        if ch == "/" and i + 1 < len(line) and line[i + 1] == "/":
            return line[i + 2 :].strip()
        i += 1
    return ""


# Cчитает изменение баланса {} в строке (с учётом строк/комментариев):
def brace_delta(line: str) -> int:
    code = remove_line_comment(line)
    in_str = False
    str_ch = ""
    esc = False
    delta = 0
    for ch in code:
        if esc:
            esc = False
            continue
        if in_str:
            if ch == "\\":
                esc = True
            elif ch == str_ch:
                in_str = False
            continue
        if ch in ("'", '"'):
            in_str = True
            str_ch = ch
            continue
        if ch == "{":
            delta += 1
        elif ch == "}":
            delta -= 1
    return delta


# Чистит многострочный фрагмент от комментариев/пустых строк и склеивает в одну строку:
def cleaned_single_line(text: str) -> str:
    parts: list[str] = []
    for line in text.splitlines():
        clean = remove_line_comment(line).strip()
        if clean:
            parts.append(clean)
    return " ".join(parts)


# Нормализует комментарий. Чистит служебные заголовки и пунктуационный шум:
def normalize_comment(text: str) -> str:
    if not text: return ""
    lines = [x.strip() for x in text.splitlines() if x.strip()]
    cleaned: list[str] = []
    section_headers = {
        "Определения",
        "Перечисления",
        "Структуры",
        "Типы данных",
        "Функции",
        "Глобальные переменные",
        "Подключаем",
        "Объявление структур",
        "Объявления структур",
    }
    for line in lines:
        if line.endswith(":") and line[:-1].strip() in section_headers: continue
        if line in section_headers: continue
        if line.startswith("API "): continue
        cleaned.append(line.rstrip(" :.;"))
    return " ".join(cleaned).strip()


# Выбирает лучший комментарий. Cначала блоковый, иначе inline:
def merge_comments(block_comment: str, inline_cmt: str) -> str:
    # Сначала комментарий над объявлением, если он осмысленный:
    from_block = normalize_comment(block_comment)
    if from_block: return from_block
    # Если над объявлением пусто/служебно, берём inline:
    return normalize_comment(inline_cmt)


# Добавляет точку в конец, если нет финального знака препинания:
def with_period(text: str) -> str:
    text = text.strip()
    if not text: return text
    if re.search(r"[.!?]$", text): return text
    return f"{text}."


# Эвристически проверяет, похожа ли декларация на функцию:
def looks_like_function(stmt: str) -> bool:
    s = cleaned_single_line(stmt)
    if not s or s.startswith(("typedef", "#")): return False
    if "(" not in s or ")" not in s: return False
    if s.startswith(("if ", "for ", "while ", "switch ", "return ")): return False
    before = s.split("(", 1)[0].strip().split()
    if not before: return False
    return re.match(r"^[A-Za-z_]\w*$", before[-1].lstrip("*")) is not None


# Извлекает имя сущности из декларации по типу (define, function, struct, и т.д):
def get_name_from_decl(category: str, decl: str) -> str:
    s = cleaned_single_line(decl)
    if category == "define":
        m = re.search(r"#define\s+([A-Za-z_]\w*)", s)
        return m.group(1) if m else ""
    if category == "function":
        m = re.search(r"([A-Za-z_]\w*)\s*\(", s)
        return m.group(1) if m else ""
    if category in ("struct", "enum", "union"):
        m = re.search(rf"\b{category}\s+([A-Za-z_]\w*)", s)
        if m: return m.group(1)
        m = re.search(r"}\s*([A-Za-z_]\w*)\s*;", s)
        return m.group(1) if m else ""
    if category == "typedef":
        m = re.search(r"([A-Za-z_]\w*)\s*;\s*$", s)
        return m.group(1) if m else ""
    if category == "global":
        m = re.search(r"([A-Za-z_]\w*)\s*(\[[^\]]*])?\s*;\s*$", s)
        return m.group(1) if m else ""
    return ""


# Достаёт значение макроса #define:
def parse_define_value(decl: str) -> str:
    s = cleaned_single_line(decl)
    m = re.search(r"#define\s+[A-Za-z_]\w*\s+(.+)$", s)
    return m.group(1).strip() if m else ""


# Формирует аккуратную сигнатуру функции (без тела, с ;):
def function_signature(decl: str) -> str:
    s = cleaned_single_line(decl)
    if "{" in s:
        s = s.split("{", 1)[0].strip()
        if not s.endswith(";"): s += ";"
    return s


# Разбирает элементы enum и их inline-комментарии:
def parse_enum_members(decl: str) -> list[dict]:
    members: list[dict] = []
    if "{" not in decl or "}" not in decl: return members
    body = decl.split("{", 1)[1].rsplit("}", 1)[0]
    for line in body.splitlines():
        raw = line.strip()
        if not raw: continue
        cmt = inline_comment(raw)
        code = remove_line_comment(raw).strip().rstrip(",")
        if not code: continue
        name = code.split("=", 1)[0].strip()
        if re.match(r"^[A-Za-z_]\w*$", name):
            members.append({"name": name, "comment": cmt})
    return members


# Разбирает поля struct и комментарии к ним:
def parse_struct_fields(decl: str) -> list[dict]:
    fields: list[dict] = []
    if "{" not in decl or "}" not in decl: return fields
    body = decl.split("{", 1)[1].rsplit("}", 1)[0]
    depth = 0
    for line in body.splitlines():
        stripped = line.strip()
        if not stripped: continue
        if depth == 0 and ";" in stripped and "{" not in remove_line_comment(stripped):
            cmt = inline_comment(stripped)
            code = remove_line_comment(stripped).strip()
            if code: fields.append({"decl": code, "comment": cmt})
        depth += brace_delta(line)
    return fields


# Главный парсер заголовка. Собирает defines/typedefs/structs/enums/unions/functions/globals:
def parse_header_file(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    data = {
        "defines": [],
        "typedefs": [],
        "structs": [],
        "enums": [],
        "unions": [],
        "functions": [],
        "globals": [],
    }

    comments: list[str] = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i].rstrip("\n")
        stripped = line.strip()

        if not stripped:
            comments = []
            i += 1
            continue

        if stripped.startswith("//"):
            comments.append(strip_comment_markers(stripped))
            i += 1
            continue

        if stripped.startswith("/*"):
            block = [stripped]
            if "*/" not in stripped:
                i += 1
                while i < n:
                    nxt = lines[i].rstrip("\n")
                    block.append(nxt)
                    if "*/" in nxt:
                        break
                    i += 1
            comment = " ".join(strip_comment_markers(x) for x in block).strip()
            if comment:
                comments.append(comment)
            i += 1
            continue

        if stripped.startswith("#define"):
            start = i
            i += 1
            while i < n and lines[i - 1].rstrip().endswith("\\"): i += 1

            decl = "\n".join(lines[j].rstrip("\n") for j in range(start, i))
            name = get_name_from_decl("define", decl)
            block_comment = "\n".join(comments).strip()
            line_inline_comment = inline_comment(lines[start].rstrip("\n"))
            comment = merge_comments(block_comment, line_inline_comment)

            data["defines"].append({
                "name": name,
                "decl": decl,
                "value": parse_define_value(decl),
                "comment": comment,
            })
            comments = []
            continue

        if stripped.startswith("#") or stripped.startswith('extern "C"') or stripped in ("{", "}"):
            # На препроцессорных строках не сбрасываем comments,
            # чтобы комментарии перед #ifdef применялись к следующей декларации.
            if stripped in ("{", "}"): comments = []
            i += 1
            continue

        stmt_lines = [line]
        i += 1

        # Читаем объявление целиком.
        # Если встретили блок {...}, дочитываем его до конца (баланс скобок),
        # чтобы не парсить внутренние строки функции как отдельные сущности.
        while i < n:
            last_code = remove_line_comment(stmt_lines[-1])

            # Обычное объявление завершилось:
            if ";" in last_code and "{" not in last_code: break

            # Блок уже начался на текущей строке:
            if "{" in last_code:
                balance = sum(brace_delta(x) for x in stmt_lines)
                while i < n and balance > 0:
                    nxt = lines[i].rstrip("\n")
                    stmt_lines.append(nxt)
                    balance += brace_delta(nxt)
                    i += 1
                break

            # Иначе продолжаем читать многострочное объявление.
            nxt = lines[i].rstrip("\n")
            stmt_lines.append(nxt)
            i += 1

        stmt = "\n".join(stmt_lines).strip()
        compact = cleaned_single_line(stmt)
        if not compact:
            comments = []
            continue

        block_comment = "\n".join(comments).strip()
        tail_comment = inline_comment(stmt_lines[-1])
        comment = merge_comments(block_comment, tail_comment)
        comments = []

        if compact.startswith("typedef enum") and "{" in compact:
            data["enums"].append({"name": get_name_from_decl("enum", stmt), "decl": stmt, "comment": comment,
                                  "members": parse_enum_members(stmt)})
            continue
        if compact.startswith("typedef struct") and "{" in compact:
            data["structs"].append({"name": get_name_from_decl("struct", stmt), "decl": stmt, "comment": comment,
                                    "fields": parse_struct_fields(stmt)})
            continue
        if compact.startswith("typedef union") and "{" in compact:
            data["unions"].append({"name": get_name_from_decl("union", stmt), "decl": stmt, "comment": comment})
            continue
        if compact.startswith("typedef"):
            data["typedefs"].append({"name": get_name_from_decl("typedef", stmt), "decl": stmt, "comment": comment})
            continue

        if re.match(r"^enum(\s+\w+)?\s*{", compact):
            data["enums"].append({"name": get_name_from_decl("enum", stmt), "decl": stmt, "comment": comment,
                                  "members": parse_enum_members(stmt)})
            continue
        if re.match(r"^struct(\s+\w+)?\s*{", compact):
            data["structs"].append({"name": get_name_from_decl("struct", stmt), "decl": stmt, "comment": comment,
                                    "fields": parse_struct_fields(stmt)})
            continue
        if re.match(r"^union(\s+\w+)?\s*{", compact):
            data["unions"].append({"name": get_name_from_decl("union", stmt), "decl": stmt, "comment": comment})
            continue

        if looks_like_function(stmt):
            fname = get_name_from_decl("function", stmt)
            if not fname or fname.startswith("_"):
                continue
            data["functions"].append({"name": fname, "decl": function_signature(stmt), "comment": comment})
            continue

        if compact.endswith(";"):
            data["globals"].append({"name": get_name_from_decl("global", stmt), "decl": compact, "comment": comment})
    return data


# Пишет раздел Определения в markdown:
def write_defines(f, items: list) -> None:
    if not items: return
    f.write("  **Определения:**</br>\n")
    for item in items:
        f.write(f"  `{item['name']}`:\n")
        if item["comment"]:
            f.write(f"  - {with_period(item['comment'])}\n")
        if item["value"]:
            f.write(f"  - Значение: `{item['value']}`.\n")
        f.write("\n")


# Пишет раздел Перечисления:
def write_enums(f, items: list) -> None:
    if not items: return
    f.write("  **Перечисления:**</br>\n")
    for item in items:
        f.write(f"  enum `{item['name']}`:\n")
        if item["comment"]:
            f.write(f"  - {with_period(item['comment'])}\n")
        if item["members"]:
            f.write("  - Значения:\n")
            for member in item["members"]:
                if member["comment"]:
                    f.write(f"    - `{member['name']}` - {with_period(member['comment'])}\n")
                else: f.write(f"    - `{member['name']}`\n")
        f.write("\n")


# Пишет раздел Структуры:
def write_structs(f, items: list) -> None:
    if not items: return
    f.write("  **Структуры:**</br>\n")
    for item in items:
        f.write(f"  struct `{item['name']}`:\n")
        if item["comment"]:
            f.write(f"  - {with_period(item['comment'])}\n")
        for field in item["fields"]:
            if field["comment"]:
                f.write(f"  - `{field['decl']}` - {with_period(field['comment'])}\n")
            else: f.write(f"  - `{field['decl']}`.\n")
        f.write("\n")


# Пишет раздел Типы данных:
def write_typedefs(f, items: list) -> None:
    if not items: return
    f.write("  **Типы данных:**</br>\n")
    for item in items:
        name = item["name"] or "(без имени)"
        f.write(f"  typedef `{name}`:\n")
        if item["comment"]:
            f.write(f"  - {with_period(item['comment'])}\n")
        f.write(f"  - Объявление: `{cleaned_single_line(item['decl'])}`\n\n")


# Пишет раздел Функции:
def write_functions(f, items: list) -> None:
    if not items: return
    f.write("  **Функции:**</br>\n")
    for item in items:
        title = item["comment"].rstrip(":. ") if item["comment"] else f"Функция `{item['name']}`"
        f.write(f"  - {title}:</br>\n")
        f.write(f"    `{item['decl']}`\n\n")


# Пишет раздел Глобальные переменные:
def write_globals(f, items: list) -> None:
    if not items: return
    f.write("  **Глобальные переменные:**</br>\n")
    for item in items:
        name = item["name"] or cleaned_single_line(item["decl"])
        f.write(f"  `{name}`:\n")
        if item["comment"]:
            f.write(f"  - {with_period(item['comment'])}\n")
        f.write(f"  - Объявление: `{cleaned_single_line(item['decl'])}`.\n\n")


# Проверяет, есть ли в файле хоть какие-то распарсенные API-сущности:
def has_public_items(parsed: dict) -> bool:
    return any(parsed[k] for k in ("defines", "enums", "structs", "typedefs", "functions", "globals", "unions"))


# Генерирует markdown-anchor для файла:
def make_anchor(file_path: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "-", file_path.lower()).strip("-")
    return f"api-{slug}"


# Получить описание файла из его начала:
def get_file_header_desc(file_path: str) -> str:
    with open(file_path, "r", encoding="utf-8") as f:
        head = [next(f, "") for _ in range(25)]  # Читаем только начало файла.

    # Ищем строку вида: // file.h - Описание:
    for line in head:
        s = line.strip()
        m = re.match(rf"^//\s*{re.escape(os.path.basename(file_path))}\s*-\s*(.+?)\s*$", s)
        if m:
            return m.group(1).rstrip(". ")
    return ""


# Основная функция:
def main() -> None:
    # Переходим в корень проекта:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.abspath(os.path.join(script_dir, ".."))
    os.chdir(project_root)
    print(f"{'-'*80}")

    # Ищем файлы для генерации:
    start = time.time()
    files = find_all_target_files()

    # Пробуем удалить старый файл:
    final_output = output_filename
    if os.path.isfile(final_output):
        try: os.remove(final_output)
        except PermissionError:
            base, ext = os.path.splitext(final_output)
            final_output = f"{base}.new{ext}"
            print(f"[warn] Файл занят: {output_filename}. Запись в: {final_output}")

    files_by_dir: dict[str, list[str]] = defaultdict(list)
    for file_path in files:
        dir_path = os.path.dirname(file_path).replace("\\", "/")
        if not dir_path.endswith("/"): dir_path += "/"
        files_by_dir[dir_path].append(file_path)

    # Сразу собираем только непустые файлы:
    non_empty_by_dir: dict[str, list[tuple[str, dict]]] = {}
    for dir_path in sorted(files_by_dir.keys()):
        items = []
        for file_path in sorted(files_by_dir[dir_path]):
            parsed = parse_header_file(file_path)
            if has_public_items(parsed):
                items.append((file_path, parsed))
        if items: non_empty_by_dir[dir_path] = items

    # Открываем файл генерации для записи:
    with open(final_output, "w", encoding="utf-8") as f:
        f.write("# API CGDF\n\n")
        f.write("Обратно к [**главной документации.**](readme.md)\n\n")
        f.write(f"_Сгенерировано автоматически. Версия генератора: {VERSION}_\n\n")

        # Красивое содержание:
        f.write("<a id=\"content\"></a>\n")
        f.write("## Содержание\n\n")
        for dir_path, items in non_empty_by_dir.items():
            f.write(f"- **{dir_path}**\n")
            for file_path, _ in items:
                short_name = os.path.basename(file_path)
                anchor = make_anchor(file_path)
                f.write(f"  - [{short_name}](#{anchor})\n")
        f.write("\n")

        # Разделы API:
        for dir_path, items in non_empty_by_dir.items():
            f.write(f"#\n\n### {dir_path}\n\n")
            for idx, (file_path, parsed) in enumerate(items):
                short_name = os.path.basename(file_path)
                anchor = make_anchor(file_path)

                f.write(f"<a id=\"{anchor}\"></a>\n")
                f.write(f"- ### {short_name}:\n")
                file_desc = get_file_header_desc(file_path)
                if file_desc: f.write(f"  > Описание: {file_desc}.\n\n")
                f.write("  [Назад](#content)\n\n")
                write_defines(f, parsed["defines"])
                write_enums(f, parsed["enums"])
                write_structs(f, parsed["structs"])
                write_typedefs(f, parsed["typedefs"])
                write_functions(f, parsed["functions"])
                write_globals(f, parsed["globals"])
                if idx < len(items)-1: f.write("\n")
        f.write("#\n\n### Конец генерации.\n")
        f.write(f"Всего файлов обработано: {len(files)}\n")
    print(f"Done in {round(time.time() - start, 2)} s.")
    print(f"Files: {len(files)}.")
    print(f"Output: {final_output}")
    print(f"{'-'*80}")


# Если этот скрипт запускают:
if __name__ == "__main__":
    main()
