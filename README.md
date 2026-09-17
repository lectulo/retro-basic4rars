# Интерпретатор Minimal BASIC для RARS

Исходники на C99 транслируются Clang в RV32IMF; фильтр AWK подготавливает
ассемблер для RARS 1.6. Для сборки нужны CMake, компилятор C, Clang и AWK.
Для исполнения в RARS нужна Java. Хостовая версия предназначена для отладки.

## Сборка

```sh
cmake -S . -B build -DRARS_JAR=/absolute/path/to/rars1_6.jar
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

В текущей структуре каталогов путь к JAR по умолчанию —
`../../materials/rars/rars1_6.jar`. Если Java не находится автоматически,
задайте `-DRARS_JAVA=/absolute/path/to/java`. При отсутствии Java или JAR
собирается ассемблер, но тесты RARS не регистрируются.

Только ассемблер интерпретатора:

```sh
cmake --build build --target asm_basic -j 4
```

Результат — все файлы `.s` из `build/rars/basic/`.
Файлы `.gas` — промежуточный результат компилятора.

## Запуск

```sh
./build/basic test/progs/demo_func_table.bas
java -jar /absolute/path/to/rars1_6.jar nc me sm we se1 ae2 20000000 build/rars/basic/*.s pa test/progs/demo_func_table.bas
```

В графическом RARS откройте `build/rars/basic/port_rars.s`. В Settings включите
`Assemble all files in directory`,
`Initialize Program Counter to global 'main' if defined`,
`Program arguments provided to program`,
`Permit extended (pseudo) instructions and formats`.
Режим `64 bit` должен быть выключен. Нажмите F3. В поле Program Arguments
вкладки Execute укажите абсолютный путь к `demo_func_table.bas`, нажмите F5.
Вывод появится в Run I/O.

## Примеры и испытания

`test/progs/` содержит шесть примеров: квадратное уравнение, таблицу функции,
сортировку, интегрирование, игру и обработку текста. Файлы `.expected` задают
ожидаемый вывод, `.in` — ввод. CTest проверяет примеры и один модульный тест
на хосте и в RARS (14 проверок при доступном симуляторе).

В `test/nbs/` сохранён внешний набор из 208 программ NBS вместе с исходными
комментариями и описанием происхождения. Отдельную программу можно запустить
так же, как демонстрационную. Некоторые программы намеренно содержат ошибки.

Скрипт сравнения всего набора требует внешних каталогов `NBS_run_input` и
`NBS_run_output` из копии проекта Ham, указанного в `test/nbs/NOTICE.md`:

```sh
python3 test/nbs_runner.py build/basic test/nbs /path/to/reference-directory
```

Этот скрипт классифицирует различия с выводом другой реализации; он не
подтверждает полное соответствие стандарту. В данном интерпретаторе ошибки
вычислений завершают выполнение, а стандарт предусматривает восстановление
в ряде случаев. Повторный ввод при ошибке INPUT поддерживается.

Описание языка находится в `../docs/language-spec.md`.
