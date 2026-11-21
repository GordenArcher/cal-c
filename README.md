# cal

A rich terminal calendar written in C. Show a single month or a whole year, highlight today, color weekends, and display ISO week numbers. No dependencies beyond a C compiler and a POSIX system.

```
    October 2026
Su Mo Tu We Th Fr Sa
             1  2  3
 4  5  6  7  8  9 10
11 12 13 14 15 16 17
18 19 20 21 22 23 24
25 26 27 28 29 30 31
```

## Features

- Month view and full-year view (three months per row)
- Today is highlighted, weekends are colored
- ISO 8601 week numbers, including year-boundary weeks
- Sunday-first or Monday-first layout
- Month names accepted as input (`oct`, `october`)
- Colors turn off automatically when piped, or with `-n` / `NO_COLOR`
- Hand-rolled date math, works for any year from 1 to 9999

## Build

Requires `gcc` or `clang` and `make`.

```bash
make
```

The binary is written to `build/cal`.

## Usage

```bash
cal [options] [[month] year]
```

| Command            | Result                                |
| ------------------ | ------------------------------------- |
| `cal`              | Current month                         |
| `cal 10 2026`      | October 2026                          |
| `cal oct 2026`     | October 2026 (month names work)       |
| `cal 10`           | October of the current year           |
| `cal 2026`         | The whole year                        |
| `cal -m 10 2026`   | October 2026, Monday first            |
| `cal -w 2026`      | Whole year with ISO week numbers      |
| `cal -mw 2026`     | Monday first plus week numbers        |

### Options

| Flag              | Description                                      |
| ----------------- | ------------------------------------------------ |
| `-m`              | Monday as the first day of the week              |
| `-w`              | Show ISO week numbers                            |
| `-n`, `--no-color`| Disable colors (also honours the `NO_COLOR` env) |
| `-h`, `--help`    | Show help                                        |

Short flags can be combined, so `-mw` is the same as `-m -w`.

### Single argument rule

One argument is ambiguous, so it is resolved like this: if it parses as a month (1 to 12, or a name of 3+ letters) it is a month of the current year, otherwise it is treated as a year. That means `cal 10` is October and `cal 2026` is the whole year. Years 1 to 12 need the two-argument form, for example `cal 1 5`.

### Exit codes

| Code | Meaning                                      |
| ---- | -------------------------------------------- |
| 0    | Success                                      |
| 1    | Invalid month, year, option, or clock error  |

## Install

```bash
sudo make install          # installs to /usr/local/bin/cal
sudo make uninstall
make install PREFIX=$HOME/.local   # no sudo, per-user install
```

Most systems already ship a `cal` in `/usr/bin`. Make sure the install prefix comes first in your `PATH`, or rename the binary in the Makefile.

## Project structure

```
cal-c/
├── src/
│   ├── main.c                  # wiring only: parse args, pick a view, print
│   ├── core/                   # no I/O, no terminal, depends on nothing
│   │   ├── calendar/
│   │   │   ├── calendar.h      # constants, names, date math, ISO week
│   │   │   └── calendar.c
│   │   └── clock/
│   │       ├── clock.h         # today's date from the system clock
│   │       └── clock.c
│   ├── render/                 # everything that draws, depends on core
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
│   │       ├── view.h          # month view and year view layouts
│   │       └── view.c
│   └── cli/                    # command line, depends on core and render
│       ├── args/
│       │   ├── args.h          # flags and positional args into a Config
│       │   └── args.c          # also holds the help text
│       └── parse/
│           ├── parse.h         # month and year parsing
│           └── parse.c
├── Makefile                    # build, run, install, clean
├── README.md
└── .gitignore
```

`build/` is created by `make` and is git-ignored.

### Rules

- **Dependencies point one way:** `main` -> `cli` -> `render` -> `core`. `core` never includes anything from `render` or `cli`, so the date math can be tested or reused without dragging in terminal code. If you catch yourself including upward, the code is in the wrong folder.
- **At most 3 files per folder.** When a folder is full, the new module gets its own subfolder.
- **Includes are written from the `src` root** (`#include "core/calendar/calendar.h"`), never with `../`.

### Adding a file

Drop a `.c` file anywhere under `src/` and `make` picks it up at any depth.

## How it works

- **Day of week**: Sakamoto's algorithm, chosen over Zeller because it has no negative-modulo edge cases.
- **ISO week**: computed from day-of-year and ISO weekday, with explicit handling for days that belong to the previous or next year's week.
- **No `mktime`**: it depends on `time_t` limits and the local timezone, so years far from 1970 behave badly. Pure arithmetic does not.
- **Year view alignment**: every rendered month line is padded to a constant visible width, so three months can be printed side by side even though the lines contain invisible ANSI color codes.

## Roadmap

- [ ] Events file (`~/.cal_events`, one `YYYY-MM-DD|label` per line) with marked days and a legend
- [ ] Holidays layer loaded from data files, so movable holidays are not hardcoded
- [ ] Interactive mode with arrow-key month navigation (ncurses)
- [ ] Tests for leap years, century rules, and ISO week boundaries

## License

MIT