# Kokkos Tools Sqlite (KTS)

* Capture Kokkos profiling information in an sqlite database.
* Answer your analysis questions with SQL (see examples).
* For MPI programs, each Kokkos process writes to a different database file.

## Getting Started

```bash
# build KTS
mkdir build
cmake -S . -B build

# set as kokkos tools library
export KOKKOS_TOOLS_LIBS=$(realpath build/libkts.so)

# run your kokkos program (and KTS generates output sqlite database)
your/kokkos/program
```

Optionally, you can control the path to the generated sqlite file(s). The provided prefix will be appended with the MPI rank (or `0` in the case of no MPI).
```bash
# KTS will generate path/to/output/prefix_{rank}.sqlite
export KTS_SQLITE_PREFIX=path/to/output/prefix_
```

## Schema

### Spans Table

| Column | Type    | Constraints                |
|--------|---------|----------------------------|
| ID     | INTEGER | PRIMARY KEY, AUTOINCREMENT |
| Rank   | INTEGER | NOT NULL                   |
| Name   | TEXT    | NOT NULL                   |
| Kind   | TEXT    | NOT NULL                   |
| Start  | REAL    | NOT NULL                   |
| Stop   | REAL    | NOT NULL                   |

* `Rank` is the MPI rank or the process ID

### Events Table

| Column | Type    | Constraints                |
|--------|---------|----------------------------|
| ID     | INTEGER | PRIMARY KEY, AUTOINCREMENT |
| Rank   | INTEGER | NOT NULL                   |
| Name   | TEXT    | NOT NULL                   |
| Kind   | TEXT    | NOT NULL                   |
| Time   | REAL    | NOT NULL                   |

* `Rank` is the MPI rank or the process ID

## Examples

**Find the average time consumed by a parallel region**

```sql
SELECT AVG(Spans.Stop - Spans.Start) FROM Spans WHERE Spans.Kind LIKE '%PARALLEL%';
```

**Find all `DEEPCOPY` events that happen in a region with a name that includes `SPGEMM`**

```sql
SELECT Events.*
FROM Events
JOIN Spans ON Events.Time BETWEEN Spans.Start AND Spans.Stop
WHERE Events.Kind = 'DEEPCOPY'
  AND Spans.Name LIKE '%SPGEMM%'
  AND Spans.Kind = 'REGION';
```

**Convert trace database to chrome-tracing format**

```bash
# record trace (produces kts_0.sqlite)
export KOKKOS_TOOLS_LIBS=$(realpath build/libkts.so)
./your/kokkos/program

# convert to chrome-tracing json format (produces test.json)
build/bin/chrome-tracing kts_0.sqlite

# drag `test.json` into chrome://tracing
```

## Roadmap

- [x] parallel_for
- [ ] parallel_reduce
- [ ] parallel_scan
- [x] deep_copy
- [x] fence
- [x] profiling regions
- [x] allocate
- [x] deallocate
- Chrome Tracing
  - [x] Tool to convert sqlite to chrome-tracing JSON format
  - [x] use `pid` field for MPI rank
  - [ ] use `tid` field for execution space instance
- [ ] Tool to convert sqlite to `kokkosp_` callbacks
- [ ] Tool to merge multi-process databases

## Contributing

This project accepts contributions through Github pull requests.

* Examples of the Kokkos profiling interface
  * [Kokkos_Profiling_C_Interface.hpp](https://github.com/kokkos/kokkos/blob/develop/core/src/impl/Kokkos_Profiling_C_Interface.h)
  * [Simple Kernel Logger](https://github.com/kokkos/kokkos-tools/blob/develop/debugging/kernel-logger/kp_kernel_logger.cpp)

Format with clang-format-16

```bash
shopt -s globstar
podman run --rm -v ${PWD}:/src ghcr.io/cwpearson/clang-format-16 clang-format -i *.[ch]pp {bin,lib,perf_test,unit_test}/**/*.[ch]pp
```
