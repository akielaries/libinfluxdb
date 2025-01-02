#ifndef LIBIFDB_H
#define LIBIFDB_H

#define __LIBINFLUXDB__

#include <stdint.h>
#include <stdarg.h>

// influxDB Info structure
typedef struct {
  char *hostname;
  uint32_t port;
  char *database;
  char *token;
  char *organization;
  int sockfd;
} InfluxInfo;

// structure to represent a single row of data
typedef char **InfluxRow;

// structure to hold the result of a query
typedef struct {
  uint64_t row_count;   // the number of rows in the result set
  uint64_t field_count; // the number of fields (columns) in each row
  InfluxRow *rows; // an array of rows (each row is an array of field values)
} InfluxResult;


/* Public API functions */
// void ifdb_init(char *host, long port, char *db, InfluxInfo *info);
/**
 * @brief initialize and connect to InfluxDB instance
 *
 * @param[in] token         InfluxDB API token for authentication
 * @param[in] hostname      host name of the device influx is running on
 * @param[in] organization  organization bucket parameter
 * @param[in] port          port number of the influx connection
 * @param[in] database      database name
 *
 * @return populated InfluxInfo pointer
 */
InfluxInfo *ifdb_init(char *token,
                      char *hostname,
                      char *organization,
                      uint32_t port,
                      char *database);

int ifdb_insert(InfluxInfo *info, char *measurement, double value);


int ifdb_delete(InfluxInfo *info,
                const char *measurement,
                const char *start_time,
                const char *stop_time);


InfluxResult *ifdb_query(InfluxInfo *ifdb_info, const char *query_format, ...);


void ifdb_show_db(InfluxResult *result);


void ifdb_close(InfluxInfo *info);

#endif // LIBIFDB_H
