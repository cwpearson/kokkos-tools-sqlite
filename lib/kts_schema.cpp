#include "kts_schema.hpp"

namespace schema {

sqlite3_stmt *Span::insert_stmt = nullptr;
sqlite3_stmt *Event::insert_stmt = nullptr;

Span Span::from_sqlite_args(int argc, char **argv) {
  if (argc != 6) {
    throw std::runtime_error("unexpected argc");
  }
  return Span{std::atoi(argv[1]), argv[2], argv[3], std::atof(argv[4]),
              std::atof(argv[5])};
}

Event Event::from_sqlite_args(int argc, char **argv) {
  if (argc != 5) {
    throw std::runtime_error("unexpected argc");
  }
  return Event{std::atoi(argv[1]), argv[2], argv[3], std::atof(argv[4])};
}

void init(sqlite3 *db) {

  // prepare Span insertion statement
  {
    int rc =
        sqlite3_prepare_v2(db, Span::insert_sql, -1, &Span::insert_stmt, 0);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
                << std::endl;
      sqlite3_close(db);
      exit(1);
    }
  }
  // prepare Event insertion statement
  {
    int rc =
        sqlite3_prepare_v2(db, Event::insert_sql, -1, &Event::insert_stmt, 0);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
                << std::endl;
      sqlite3_close(db);
      exit(1);
    }
  }
}

void finalize(sqlite3 *) {
  sqlite3_finalize(Event::insert_stmt);
  sqlite3_finalize(Span::insert_stmt);
}

void insert(sqlite3 *db, const Span &span) {
  sqlite3_bind_int(Span::insert_stmt, 1, span.rank);
  sqlite3_bind_text(Span::insert_stmt, 2, span.name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(Span::insert_stmt, 3, span.kind.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_double(Span::insert_stmt, 4, span.start);
  sqlite3_bind_double(Span::insert_stmt, 5, span.stop);

  int rc = sqlite3_step(Span::insert_stmt);

  if (rc != SQLITE_DONE) {
    std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    std::cerr << "Span was:"
              << " " << span.name << " " << span.kind << " " << span.start
              << " " << span.stop << "\n";
    exit(1);
  }
  sqlite3_reset(Span::insert_stmt);
}

void insert(sqlite3 *db, const Event &event) {
  sqlite3_bind_int(Event::insert_stmt, 1, event.rank);
  sqlite3_bind_text(Event::insert_stmt, 2, event.name.c_str(), -1,
                    SQLITE_STATIC);
  sqlite3_bind_text(Event::insert_stmt, 3, event.kind.c_str(), -1,
                    SQLITE_STATIC);
  sqlite3_bind_double(Event::insert_stmt, 4, event.time);

  int rc = sqlite3_step(Event::insert_stmt);

  if (rc != SQLITE_DONE) {
    std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    std::cerr << "Event was:"
              << " " << event.name << " " << event.kind << " " << event.time
              << "\n";
    exit(1);
  }
  sqlite3_reset(Event::insert_stmt);
}

} // namespace schema