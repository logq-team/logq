# logq

Параллельный анализатор логов: Go-обвязка + C++-движок.

## Сборка во время разработки

```sh
make
mkdir -p build
CGO_ENABLED=1 CXX="g++-14" go build -o build/logq ./src/cli
```

## Демка Go + C++ через cgo

Нужны Go 1.22+ и GCC 14+ с поддержкой C++23 — для отладки GDB

Запуск:

```sh
make
./build/logq --text hello --number 21
```

Тесты:

```sh
make test
```

Отладка с Go `main`:

```sh
make debug-demo
```

## C++: последовательный анализ файла

```sh
make overview-demo
make overview-demo OVERVIEW_FILE=/path/to/file.log
```

Отладка

```sh
make overview-demo-debug
```

Тесты C++

```sh
sudo apt-get install g++-14 libgtest-dev pkg-config
make cpp-test
```

Стенд общего интерфейса

```sh
make overview-api-demo
```

Отладка

```sh
make overview-api-demo-debug
```

Бенчмарк overview, раскидывает файлы по ядрам, прогрев и 7 замеров каждого режима

```sh
make overview-bench BENCH_WORKERS=1,2,4,8 BENCH_FILES="/path/to/logs/*.txt"
make overview-bench-debug BENCH_WORKERS=2
```

TUI с логами в отдельном терминале

Во втором терминале выполни `tty` и оставь его открытым

```sh
tty
```

В терминале проекта подставь полученный путь вместо `/dev/pts/7`

```sh
make overview-api-demo-debug GDB='gdb -tui -ex "set inferior-tty /dev/pts/7"'
```

## Как мы работаем

Задачи ведём на доске [Projects](https://github.com/orgs/logq-team/projects/1). Любая работа начинается с issue, а любой код попадает в `main` только через PR.

### Первый запуск

1. Добавь SSH-ключ в GitHub: **Settings → SSH and GPG keys**.
2. Склонируй репо и включи хук для коммитов:
   ```bash
   git clone git@github.com:logq-team/logq.git
   cd logq
   make hooks
   ```

### Цикл работы над задачей

Карточка задачи проходит по доске путь `Backlog → Todo → In Progress → In Review → Done`. Часть переходов происходит автоматически, они отмечены ниже.

#### 1. Создать или взять задачу

- **Новая задача:** **Issues → New issue → Задача**. Заполни, что нужно сделать и как понять, что готово. Справа поставь метку `area:*` и milestone `Week N`. Карточка сама появится на доске в **Backlog** *(авто)*.
- **Задача должна быть небольшой**, на 1–2 дня работы. Если больше, разбей на несколько.
- **Взять задачу:** поставь себя в **Assignees** и перетащи карточку в **In Progress**. Так остальные видят, что ей уже занимаются.

#### 2. Создать ветку

В issue справа: **Development → Create a branch → Create branch**. GitHub создаст ветку `<номер>-<название>`, например `12-mmap-reader`, и сразу привяжет её к задаче. Затем у себя:

```bash
git switch main && git pull           # начинаем со свежего main
git fetch origin
git switch 12-mmap-reader
```

Одна задача — одна ветка. Не смешивай в ветке работу по разным задачам.

#### 3. Писать код и коммитить

Формат сообщения: `<область>: <что сделано> (#<номер задачи>)`.

```bash
git add src/engine/reader.cpp
git commit -m "cpp: mmap file reader (#12)"
git push -u origin 12-mmap-reader     # при первом пуше ветки; дальше просто git push
```

- **Номер обязателен.** Без `#номер` хук не пропустит коммит. По номеру коммит появится в истории задачи.
- **Область:** `cpp`, `go`, `bench`, `docs`, `ci`, `chore`.
- **Пушить можно часто**, хоть после каждого коммита. Пока нет PR, это никому не мешает, зато работа сохранена на сервере.

#### 4. Открыть pull request

Когда задача готова к слиянию, на странице репо нажми **Compare & pull request** (или **Pull requests → New**).

- **Заголовок** — в том же формате, что и коммит, с номером задачи: `cpp: mmap file reader (#12)`. При мерже он станет сообщением коммита в `main`, и ссылка на задачу останется в истории.
- **Описание** подставится из шаблона. Обязательно оставь `Closes #12`: так PR привяжется к задаче, а карточка уедет в **In Review** *(авто)*. Кратко напиши, что сделано и как проверить.
- **Projects, Labels, Milestone** на PR не заполняй: это всё ставится на задаче, а PR связан с ней через `Closes #12`.
- Не открывай PR в самом начале работы, иначе карточка сразу уедет в In Review. Если нужно показать незаконченное, открой **Draft pull request**.

#### 5. Дождаться CI

CI запускается на каждом пуше в PR и выполняет `make` и `make test`. Результат проверок отображается внизу PR.

- **Проверка не прошла:** нажми **Details** рядом с проверкой и посмотри лог. Почини, закоммить и запушь, CI перезапустится сам.
- **Конфликт с `main`** (кто-то успел изменить те же файлы): нажми **Update branch** в PR или сделай у себя `git pull origin main`, разреши конфликты и запушь.

#### 6. Мерж и уборка

Когда все проверки CI прошли, автор PR нажимает **Squash and merge**. Все коммиты ветки сольются в `main` одним коммитом. Дальше автоматически:

- задача закроется, карточка уедет в **Done**;
- ветка удалится на GitHub.

Локально переключись на свежий `main` и удали старую ветку:

```bash
git switch main && git pull
git branch -D 12-mmap-reader
```

### Правила

- В `main` напрямую не пушим, только через PR.
- PR не мержится, пока CI (`make` и `make test`) не зелёный.
- Метки: `area:cpp`, `area:go`, `area:benchmark`, `area:report`, `bug`, `experiment`.
- Milestone `Week N` — к какой неделе задача должна быть готова.
- Перед новой задачей обновляй `main`: `git switch main && git pull`.
- На доске только задачи, PR туда не добавляем.
- Задачу с открытым PR руками не закрываем, она закроется сама при мерже.
