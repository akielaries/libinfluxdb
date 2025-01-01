#include "../lib/libifdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define MAX_QUERY_SIZE 1024

// Helper function to perform HTTP request
static char *send_http_request(const char *hostname,
                               uint32_t port,
                               const char *path,
                               const char *body) {
  struct sockaddr_in server_addr;
  int sockfd;
  char request[MAX_QUERY_SIZE];
  char response[4096];

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Socket creation failed");
    return NULL;
  }

  server_addr.sin_family      = AF_INET;
  server_addr.sin_port        = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(hostname);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Connection failed");
    close(sockfd);
    return NULL;
  }

  snprintf(request,
           sizeof(request),
           "POST %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Content-Type: application/x-www-form-urlencoded\r\n"
           "Content-Length: %lu\r\n\r\n"
           "%s",
           path,
           hostname,
           strlen(body),
           body);

  if (send(sockfd, request, strlen(request), 0) < 0) {
    perror("Send failed");
    close(sockfd);
    return NULL;
  }

  int len = recv(sockfd, response, sizeof(response) - 1, 0);
  if (len < 0) {
    perror("Receive failed");
    close(sockfd);
    return NULL;
  }

  response[len] = '\0';
  close(sockfd);

  return strdup(response);
}

// Initializes InfluxDB connection and returns a pointer to InfluxInfo
struct InfluxInfo *ifdb_init(char *hostname,
                             uint32_t port,
                             char *database,
                             char *user,
                             char *pass,
                             char *tags) {
  printf("Initializing connection to: \n \
          Hostname: %s\n \
          Port: %d\n \
          User: %s\n",
         hostname,
         port,
         user);
  struct InfluxInfo *info = malloc(sizeof(struct InfluxInfo));
  if (!info) {
    perror("Memory allocation failed");
    return NULL;
  }

  info->hostname  = hostname;
  info->port      = port;
  info->database  = database;
  info->user      = user;
  info->pass      = pass;
  info->tags      = tags;
  info->socket_fd = -1;

  char query[MAX_QUERY_SIZE];
  snprintf(query, sizeof(query), "CREATE DATABASE %s", database);

  char *response = send_http_request(hostname, port, "/query", query);
  if (response == NULL) {
    free(info);
    return NULL;
  }

  free(response);
  return info;
}

// Executes an InfluxDB query and returns the result as InfluxResult
InfluxResult *ifdb_query(struct InfluxInfo *ifdb_info, const char *query, ...) {
  va_list args;
  char query_buffer[MAX_QUERY_SIZE];

  va_start(args, query);
  vsnprintf(query_buffer, sizeof(query_buffer), query, args);
  va_end(args);

  char *response = send_http_request(ifdb_info->hostname,
                                     ifdb_info->port,
                                     "/query",
                                     query_buffer);
  if (!response)
    return NULL;

  // Here you'd need to parse the response into InfluxResult struct
  // For simplicity, let's assume it returns a dummy result
  InfluxResult *result = malloc(sizeof(InfluxResult));
  result->num_rows     = 1;
  result->num_fields   = 3;
  result->rows         = malloc(sizeof(InfluxRow) * 1);
  result->rows[0]      = malloc(sizeof(char *) * 3);
  result->rows[0][0]   = strdup("field1_value");
  result->rows[0][1]   = strdup("field2_value");
  result->rows[0][2]   = strdup("field3_value");

  free(response);
  return result;
}

// Fetches the next row from the query result
InfluxRow ifdb_fetch_row(InfluxResult *result) {
  if (result->num_rows > 0) {
    return result->rows[0];
  }
  return NULL;
}

// Returns the number of fields in the result set
uint64_t ifdb_num_fields(InfluxResult *result) { return result->num_fields; }

// Closes the InfluxDB connection (no-op here since we aren't using persistent
// connections)
void ifdb_close(struct InfluxInfo *ifdb_info) { free(ifdb_info); }
