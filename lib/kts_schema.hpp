#pragma once

#include <functional>
#include <iostream>
#include <string>

#include <sqlite3.h>

namespace schema {
struct Event {
  static constexpr const char *create_table_sql =
      "CREATE TABLE IF NOT EXISTS Events("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "Rank INTEGER NOT NULL,"
      "Name TEXT NOT NULL,"
      "Kind TEXT NOT NULL,"
      "Time REAL NOT NULL);";
  static constexpr const char *insert_sql =
      "INSERT INTO Events (Rank, Name, Kind, Time) VALUES (?, ?, ?, ?);";

  int rank;
  std::string name;
  std::string kind;
  double time;

  static Event from_sqlite_args(int argc, char **argv);
};

struct Span {
  static constexpr const char *create_table_sql =
      "CREATE TABLE IF NOT EXISTS Spans("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "Rank INTEGER NOT NULL,"
      "Name TEXT NOT NULL,"
      "Kind TEXT NOT NULL,"
      "Start REAL NOT NULL,"
      "Stop REAL NOT NULL);";

  static constexpr const char *insert_sql =
      "INSERT INTO Spans (Rank, Name, Kind, Start, Stop) VALUES (?, ?, ?, ?, "
      "?);";

  int rank;
  std::string name;
  std::string kind;
  double start;
  double stop;

  static Span from_sqlite_args(int argc, char **argv);
};

using SpanCallback = std::function<int(const Span &span)>;
using EventCallback = std::function<int(const Event &event)>;

template <typename Callback> void for_all_spans(sqlite3 *db, Callback &&c) {
  auto callback = [](void *data, int argc, char **argv,
                     char **azColName) -> int {
    auto f = static_cast<SpanCallback *>(data);
    return (*f)(Span::from_sqlite_args(argc, argv));
  };

  // wrap in an std::function to pass in to userdata
  SpanCallback wrapped(c);

  char *errMsg;
  if (sqlite3_exec(db, "SELECT * FROM Spans;", callback, &wrapped, &errMsg)) {
    std::cerr << "Can't exec: " << errMsg << std::endl;
    exit(1);
  }
}

template <typename Callback> void for_all_events(sqlite3 *db, Callback &&c) {
  auto callback = [](void *data, int argc, char **argv,
                     char **azColName) -> int {
    auto f = static_cast<EventCallback *>(data);
    return (*f)(Event::from_sqlite_args(argc, argv));
  };

  // wrap in an std::function to pass in to userdata
  EventCallback wrapped(c);

  char *errMsg;
  if (sqlite3_exec(db, "SELECT * FROM Events;", callback, &wrapped, &errMsg)) {
    std::cerr << "Can't exec: " << errMsg << std::endl;
    exit(1);
  }
}
} // namespace schema
