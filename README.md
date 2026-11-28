# mycal

[![CI](https://github.com/GordenArcher/cal-c/actions/workflows/ci.yml/badge.svg)](https://github.com/GordenArcher/cal-c/actions/workflows/ci.yml)

A rich terminal calendar written in C. Show a month or a whole year, highlight today, keep your own events, and display ISO week numbers. No dependencies beyond a C compiler and a POSIX system.

```
    October 2026
Su Mo Tu We Th Fr Sa
             1  2  3
 4  5  6  7  8  9 10
11 12 13 14 15 16 17
18 19 20 21 22 23 24
25 26 27 28 29 30 31

15  Team offsite
31  Halloween
```

## Features

- Month view and full-year view (three months per row)
- Today is highlighted, weekends are colored
- **Events**: days with events are underlined and listed under the month
- `mycal add 12-25 Christmas` to add an event without opening a file
- ISO 8601 week numbers, including year-boundary weeks
- Sunday-first or Monday-first layout
- Month names accepted as input (`oct`, `october`)
- Colors turn off automatically when piped, or with `-n` / `NO_COLOR`
- Hand-rolled date math, works for any year from 1 to 9999

## Build

Requires `gcc` or `clang` and `make`.

```bash
make          # builds build/mycal
make test     # builds and runs the test suite
```

## Usage

```bash
mycal [options] [[month] year]
mycal add [-e FILE] DATE LABEL...
```

| Command            | Result                                |
| ------------------ | ------------------------------------- |
| `mycal`              | Current month                         |
| `mycal 10 2026`      | October 2026                          |
| `mycal oct 2026`     | October 2026 (month names work)       |
| `mycal 10`           | October of the current year           |
| `mycal 2026`         | The whole year                        |
| `mycal -m 10 2026`   | October 2026, Monday first            |
| `mycal -w 2026`      | Whole year with ISO week numbers      |
| `mycal -mw 2026`     | Monday first plus week numbers        |

### Options

| Flag                  | Description                                        |
| --------------------- | -------------------------------------------------- |
| `-m`                  | Monday as the first day of the week                |
| `-w`                  | Show ISO week numbers                              |
| `-n`, `--no-color`    | Disable colors (also honours the `NO_COLOR` env)   |
| `-e`, `--events FILE` | Read events from FILE                              |
| `--no-events`         | Do not load any events                             |
| `-h`, `--help`        | Show help                                          |

Short flags can be combined, so `-mw` is the same as `-m -w`.

### Single argument rule

One argument is ambiguous, so it is resolved like this: if it parses as a month (1 to 12, or a name of 3+ letters) it is a month of the current year, otherwise it is treated as a year. That means `mycal 10` is October and `mycal 2026` is the whole year. Years 1 to 12 need the two-argument form, for example `mycal 1 5`.

## Events

Events live in a plain text file, one per line:

```
# my events
2026-10-15|Team offsite       <- one specific day
12-25|Christmas               <- every year
02-29|Leap day                <- every year, only appears in leap years
```

- Blank lines and lines starting with `#` are ignored.
- The label may contain anything except line breaks, including `|`.
- A line that cannot be read is skipped with a warning (`file:LINE: reason`), it never hides the rest of the file.

**Which file is used**, first match wins:

1. `-e FILE` on the command line
2. the `CAL_EVENTS` environment variable
3. `~/.cal_events`

A missing default file just means "no events yet". A missing file you named with `-e` is an error.

**What you see:** in the month view, event days are underlined and listed under the grid. The year view only underlines, because twelve legends would bury the calendar. Underline stacks with the other styles, so an event on today or on a weekend keeps its highlight.

**Adding events:**

```bash
mycal add 12-25 Christmas              # every year
mycal add 2026-10-15 Team offsite      # one day, quotes are optional
mycal add -e work.txt 2026-10-20 Sprint review
```

The label is joined with single spaces and trimmed. Invalid dates, empty labels, labels with line breaks and labels over 127 characters are refused before anything is written.

### Exit codes

| Code | Meaning                                                      |
| ---- | ------------------------------------------------------------ |
| 0    | Success                                                      |
| 1    | Invalid input, unreadable explicit events file, or clock error |

## Install

```bash
sudo make install          # installs to /usr/local/bin/mycal
sudo make uninstall
make install PREFIX=$HOME/.local   # no sudo, per-user install
```

The program is called `mycal` on purpose: Linux and macOS already ship a `mycal` in `/usr/bin`, and a program with the same name either shadows it or loses to it depending on `PATH`. To use another name, run `make install BIN_NAME=whatever`.

The events file keeps its original names (`~/.cal_events`, `CAL_EVENTS`). They describe your data, not the program, so renaming the binary never orphans events you already saved.

## Testing

```bash
make test
```

```
ok    tests/render/test_month.c (420 checks)
ok    tests/events/test_store.c (57 checks)
...
```

Every `tests/**/test_*.c` is its own program with its own `main()`, linked against the whole program except `main.c`. There is no framework, only `tests/support/test.h` (a few assertion macros) and `tests/support/fixtures.h`. All of them run even when one fails, and `make test` exits non-zero if any did.

What is covered:

- **Calendar math**: known dates checked against Python, and a walk over every single day from 0001-01-01 to 9999-12-31 checking weekday, day-of-year and ISO week stay consistent between neighbours.
- **Events**: parsing and formatting edge cases, reading and appending real files (missing trailing newline, overlong lines, bad lines), path resolution.
- **Rendering**: output captured from `view_*` into a temp file, constant line widths (the year view depends on it), underline and highlight rules.
- **Command line**: flag parsing, the single-argument rule, every error message, and the `add` command.

To catch memory bugs, run the same suite under the AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
make sanitize
```

It builds into `build/sanitize/`, separate from the normal build, so the two never get mixed. (Mixing them is what causes a pile of `undefined symbol ___asan_*` link errors.) Leak detection is not available on every platform, for example Apple Silicon Macs, but invalid memory access and undefined behavior are still caught.

GitHub Actions runs `make test` and `make sanitize` on Ubuntu and macOS for every push and pull request (`.github/workflows/ci.yml`).

## Project structure

```
cal-c/
├── src/
│   ├── main.c                  # wiring only: pick a command, own all printing
│   ├── core/                   # no I/O, no terminal, depends on nothing
│   │   ├── calendar/
│   │   │   ├── calendar.h      # constants, names, date math, ISO week
│   │   │   └── calendar.c
│   │   └── clock/
│   │       ├── clock.h         # today's date from the system clock
│   │       └── clock.c
│   ├── events/                 # the events feature (depends on core)
│   │   ├── model/
│   │   │   ├── event.h         # Event, EventList, "does it occur on this day"
│   │   │   └── event.c
│   │   ├── format/
│   │   │   ├── format.h        # text <-> Event, pure, no I/O
│   │   │   └── format.c
│   │   └── store/
│   │       ├── store.h         # load, append, find the events file
│   │       └── store.c
│   ├── render/                 # everything that draws (depends on core, events)
│   │   ├── style/
│   │   │   ├── options.h       # RenderOptions
│   │   │   ├── style.h         # ANSI codes, gated on color on/off
│   │   │   └── style.c
│   │   ├── line/
│   │   │   ├── line.h          # fixed-width line buffer
│   │   │   └── line.c
│   │   ├── month/
│   │   │   ├── month.h         # renders one month block
│   │   │   └── month.c
│   │   └── view/
│   │       ├── view.h          # month view (with legend) and year view
│   │       └── view.c
│   └── cli/                    # command line (depends on everything above)
│       ├── args/
│       │   ├── args.h          # flags and positional args into a Config
│       │   └── args.c
│       ├── parse/
│       │   ├── parse.h         # month and year parsing
│       │   └── parse.c
│       ├── usage/
│       │   ├── usage.h         # help text
│       │   └── usage.c
│       └── add/
│           ├── add.h           # the "mycal add" command
│           └── add.c
├── tests/
│   ├── support/
│   │   ├── test.h              # assertion macros and temp file helpers
│   │   └── fixtures.h          # shared test data builders
│   ├── core/
│   │   └── test_calendar.c
│   ├── events/
│   │   ├── test_event.c
│   │   ├── test_format.c
│   │   └── test_store.c
│   ├── render/
│   │   ├── test_month.c
│   │   └── test_view.c
│   └── cli/
│       ├── test_parse.c
│       ├── test_args.c
│       └── test_add.c
├── Makefile                    # build, run, test, install, clean
├── README.md
└── .gitignore
```

`build/` is created by `make` and is git-ignored.

### Rules

- **Dependencies point one way:** `main` -> `cli` -> `render` -> `events` -> `core`. `core` never includes anything above it, so the date math can be tested or reused without dragging in terminal code or files. If you catch yourself including upward, the code is in the wrong folder.
- **Only `main.c` touches stdout and stderr** (plus the modules that take a `FILE *`). Argument parsing returns its error message instead of printing it, and the views and the `add` command write to a `FILE *` they are given, which is what makes them testable.
- **At most 3 files per folder.** When a folder is full, the new module gets its own subfolder.
- **Includes are written from the `src` root** (`#include "core/calendar/calendar.h"`), never with `../`.

### Adding a file

Drop a `.c` file anywhere under `src/` and `make` picks it up at any depth. A new `tests/<domain>/test_<name>.c` is picked up the same way.

## How it works

- **Day of week**: Sakamoto's algorithm, chosen over Zeller because it has no negative-modulo edge cases.
- **ISO week**: computed from day-of-year and ISO weekday, with explicit handling for days that belong to the previous or next year's week.
- **No `mktime`**: it depends on `time_t` limits and the local timezone, so years far from 1970 behave badly. Pure arithmetic does not.
- **Year view alignment**: every rendered month line is padded to a constant visible width, so three months can be printed side by side even though the lines contain invisible ANSI color codes. A test guards this for every month and layout.
- **Safe appends**: `mycal add` checks whether the file ends with a newline first, so a hand-edited file never gets two events glued onto one line.

## Roadmap

- [x] Events file with marked days and a legend
- [x] `mycal add`
- [x] Test suite
- [ ] `mycal list` and `mycal remove` to manage events without a text editor
- [ ] Holidays layer loaded from data files, so movable holidays are not hardcoded
- [ ] Interactive mode with arrow-key month navigation (ncurses)
- [ ] CI that runs `make test` on every push

## License

MIT, see [LICENSE](LICENSE).
