#include "kts_schema.hpp"

namespace schema {

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

} // namespace schema