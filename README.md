# Kokkos Tools Sqlite

Capture Kokkos profiling information in sqlite library

## Getting Started

```bash
mkdir build
cmake -S build

export KOKKOS_TOOLS_LIBS=$(realpath build/lib
```

## Examples

**Find all `DEEPCOPY` events that happen in a region with a name that includes `SPGEMM`**

```sql
SELECT Events.*
FROM Events
JOIN Spans ON Events.Time BETWEEN Spans.Start AND Spans.Stop
WHERE Events.Kind = 'DEEPCOPY'
  AND Spans.Name LIKE '%SPGEMM%';
```
