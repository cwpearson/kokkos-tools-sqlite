# Kokkos Tools Sqlite

Capture Kokkos profiling information in sqlite library

## Getting Started

```bash
mkdir build
cmake -S build

export KOKKOS_TOOLS_LIBS=$(realpath build/lib
```

## Examples

```sql
SELECT * FROM Events
WHERE EXISTS (
    SELECT 1
    FROM Spans
    WHERE Event.Time >= Spans.Start
    AND Event.Time <= Spans.Stop
);
```
