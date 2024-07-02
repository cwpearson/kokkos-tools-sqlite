# Kokkos Tools Sqlite (KTS)

Capture Kokkos profiling information in an sqlite database.

## Getting Started

```bash
mkdir build
cmake -S build

export KOKKOS_TOOLS_LIBS=$(realpath build/libkts.so)

./your/kokkos/program
```

Optionally, you can control the path to the generated sqlite file. The provided prefix will be appended with the MPI rank (or `0` in the case of no MPI).
```bash
export KTS_SQLITE_PREFIX=path/to/output/prefix
```

## Schema

### Spans Table

| Column | Type    | Constraints                |
|--------|---------|----------------------------|
| ID     | INTEGER | PRIMARY KEY, AUTOINCREMENT |
| Name   | TEXT    | NOT NULL                   |
| Kind   | TEXT    | NOT NULL                   |
| Start  | REAL    | NOT NULL                   |
| Stop   | REAL    | NOT NULL                   |

### Events Table

| Column | Type    | Constraints                |
|--------|---------|----------------------------|
| ID     | INTEGER | PRIMARY KEY, AUTOINCREMENT |
| Name   | TEXT    | NOT NULL                   |
| Kind   | TEXT    | NOT NULL                   |
| Time   | REAL    | NOT NULL                   |

## Examples

**Find all `DEEPCOPY` events that happen in a region with a name that includes `SPGEMM`**

```sql
SELECT Events.*
FROM Events
JOIN Spans ON Events.Time BETWEEN Spans.Start AND Spans.Stop
WHERE Events.Kind = 'DEEPCOPY'
  AND Spans.Name LIKE '%SPGEMM%'
  AND Spans.Kind = 'REGION';
```
