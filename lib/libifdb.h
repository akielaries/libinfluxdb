#ifndef LIBIFDB_H
#define LIBIFDB_H

#include <stdint.h>

// Structure to hold InfluxDB connection information
struct InfluxInfo {
  char *hostname;
  uint32_t port;
  char *database;
  char *user;
  char *pass;
  char *tags;
  int socket_fd;
};

// Structure to hold query result data
typedef char **InfluxRow;
typedef struct InfluxResult {
  uint64_t num_rows;
  uint64_t num_fields;
  InfluxRow *rows;
} InfluxResult;

// Initializes connection to the InfluxDB instance and returns an InfluxInfo
// struct
struct InfluxInfo *ifdb_init(char *hostname,
                             uint32_t port,
                             char *database,
                             char *user,
                             char *pass,
                             char *tags);

// Executes an InfluxDB query and returns the result as an InfluxResult struct
InfluxResult *ifdb_query(struct InfluxInfo *ifdb_info, const char *query, ...);

// Fetches the next row from the query result
InfluxRow ifdb_fetch_row(InfluxResult *result);

// Returns the number of fields (columns) in the result set
uint64_t ifdb_num_fields(InfluxResult *result);

// Closes the connection to the InfluxDB instance
void ifdb_close(struct InfluxInfo *ifdb_info);

#endif // LIBIFDB_H
