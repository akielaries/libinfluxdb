#include "../lib/libifdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cjson/cJSON.h> // for JSON parsing


#define MAX_BUF_SIZE 4096


// response buffer to store HTTP response data
struct MemoryStruct {
  char *memory;
  size_t size;
};


// helper function to create and connect the socket
int create_socket(char *hostname, uint32_t port) {
  int sockfd;
  struct sockaddr_in server_addr;

  // create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Socket creation failed");
    return -1;
  }

  // set up server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port   = htons(port);
  if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
    perror("Invalid address or address not supported");
    close(sockfd);
    return -1;
  }

  // connect to server
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Connection failed");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

// helper function to send an HTTP request
int send_http_request(int sockfd, const char *request) {
  printf("[+] Sending request: %s\n\n", request);
  size_t total_sent  = 0;
  size_t request_len = strlen(request);

  while (total_sent < request_len) {
    ssize_t sent =
      send(sockfd, request + total_sent, request_len - total_sent, 0);
    if (sent < 0) {
      perror("Send failed");
      return -1;
    }
    total_sent += sent;
  }

  return 0;
}

// helper function to receive and print HTTP response headers
int receive_and_print_headers(int sockfd) {
  char buffer[MAX_BUF_SIZE];
  int received;
  int header_end     = 0;
  int total_received = 0;

  // initialize buffer
  memset(buffer, 0, sizeof(buffer));

  // read data until end of headers
  while ((received = recv(sockfd,
                          buffer + total_received,
                          sizeof(buffer) - total_received - 1,
                          0)) > 0) {
    total_received += received;
    buffer[total_received] = '\0';

    // check if we've reached the end of headers
    if (strstr(buffer, "\r\n\r\n") != NULL) {
      header_end = 1;
      break;
    }

    // prevent buffer overflow
    if (total_received >= sizeof(buffer) - 1) {
      break;
    }
  }

  if (received < 0) {
    perror("Receive failed");
    return -1;
  }

  if (header_end) {
    // print the headers
    char *header_ptr = buffer;
    char *body_ptr   = strstr(buffer, "\r\n\r\n");
    if (body_ptr) {
      *body_ptr = '\0'; // null-terminate the headers
    }

    printf("HTTP Response Headers:\n%s\n", header_ptr);
  } else {
    printf("No complete headers received.\n");
  }

  return 0;
}

// function to initialize the InfluxDB connection and perform a HEAD request
InfluxInfo *ifdb_init(char *token,
                      char *hostname,
                      char *organization,
                      uint32_t port,
                      char *database) {
  printf("[+] Initializing InfluxDB connection\n");
  // create socket connection
  int sockfd = create_socket(hostname, port);
  if (sockfd < 0) {
    return NULL;
  }

  // allocate memory for InfluxInfo struct
  InfluxInfo *info = malloc(sizeof(InfluxInfo));
  if (!info) {
    perror("Memory allocation failed");
    close(sockfd);
    return NULL;
  }

  // set InfluxInfo fields
  info->hostname     = hostname;
  info->database     = database;
  info->organization = organization;
  info->port         = port;
  info->token        = token;
  info->sockfd       = sockfd;

  // ------------------------
  // perform HEAD Request to get header information from the db instance
  // ------------------------
  char head_request[MAX_BUF_SIZE];
  snprintf(head_request,
           sizeof(head_request),
           "HEAD / HTTP/1.1\r\n"
           "Host: %s:%u\r\n"
           "User-Agent: custom-client/1.0\r\n"
           "Accept: */*\r\n"
           "Authorization: Token %s\r\n"
           "\r\n",
           hostname,
           port,
           token);

  if (send_http_request(sockfd, head_request) < 0) {
    free(info);
    close(sockfd);
    return NULL;
  }

  // receive and print headers
  if (receive_and_print_headers(sockfd) < 0) {
    free(info);
    close(sockfd);
    return NULL;
  }

  // close the socket after HEAD request
  // close(sockfd);
  // info->sockfd = -1; // indicate that the socket is closed

  printf("[+] HEAD request completed successfully\n");

  return info;
}

// insert data into a database/bucket
int ifdb_insert(InfluxInfo *info, char *measurement, double value) {
  printf("[+] Writing value %f to measurement %s\n", value, measurement);

  // construct the write URL
  char url[MAX_BUF_SIZE];
  snprintf(url,
           sizeof(url),
           "/api/v2/write?org=%s&bucket=%s&precision=s",
           info->organization,
           info->database);

  // build the payload dynamically
  char payload[MAX_BUF_SIZE];
  snprintf(payload, sizeof(payload), "%s value=%f", measurement, value);

  // build the POST request
  char post_request[MAX_BUF_SIZE];
  snprintf(post_request,
           sizeof(post_request),
           "POST %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Authorization: Token %s\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %lu\r\n"
           "\r\n"
           "%s",
           url,
           info->hostname,
           info->token,
           strlen(payload),
           payload);

  // send the HTTP request
  if (send_http_request(info->sockfd, post_request) < 0) {
    perror("Failed to send HTTP request");
    return -1;
  }

  return 0;
}

int ifdb_delete(InfluxInfo *info,
                const char *measurement,
                const char *start_time,
                const char *stop_time) {
  printf("[+] Deleting record from measurement %s between %s - %s\n",
         measurement,
         start_time,
         stop_time);

  // construct the predicate for filtering (optional, can match measurement).
  char predicate[MAX_BUF_SIZE];
  snprintf(predicate, sizeof(predicate), "_measurement=\"%s\"", measurement);

  // construct the JSON payload for the delete request.
  // https://docs.influxdata.com/influxdb/v2/write-data/delete-data/#delete-data-using-the-api
  cJSON *payload_json = cJSON_CreateObject();
  cJSON_AddStringToObject(payload_json, "start", start_time);
  cJSON_AddStringToObject(payload_json, "stop", stop_time);
  cJSON_AddStringToObject(payload_json, "predicate", predicate);
  char *payload = cJSON_PrintUnformatted(payload_json); // no pretty print
  cJSON_Delete(payload_json);                           // free the cJSON object

  // construct the DELETE request URL
  char url[MAX_BUF_SIZE];
  snprintf(url,
           sizeof(url),
           "/api/v2/delete?org=%s&bucket=%s",
           info->organization,
           info->database);

  // construct the HTTP DELETE request
  char post_request[MAX_BUF_SIZE];
  snprintf(post_request,
           sizeof(post_request),
           "POST %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Authorization: Token %s\r\n"
           "Content-Type: application/json\r\n"
           "Content-Length: %lu\r\n"
           "\r\n"
           "%s",
           url,
           info->hostname,
           info->token,
           strlen(payload),
           payload);

  // send the HTTP request
  if (send_http_request(info->sockfd, post_request) < 0) {
    perror("Failed to send HTTP request");
    free(payload); // free allocated payload
    return -1;
  }

  // free the allocated payload
  free(payload);

  printf("[+] Delete request sent successfully\n");
  return 0;
}


/*****************************************************************************/
// custom query
InfluxResult *ifdb_query(InfluxInfo *ifdb_info, const char *query_format, ...) {
  // allocate memory for InfluxResult (if you plan to extend this later)
  InfluxResult *result = malloc(sizeof(InfluxResult));
  if (!result) {
    perror("Memory allocation failed for InfluxResult");
    return NULL;
  }

  // format the query string
  char query[MAX_BUF_SIZE];
  va_list args;
  va_start(args, query_format);
  vsnprintf(query, sizeof(query), query_format, args);
  va_end(args);

  printf("[+] Performing query: %s\n", query);

  // construct the HTTP POST request
  char request[MAX_BUF_SIZE];
  snprintf(request,
           sizeof(request),
           "POST /api/v2/query?org=%s&bucket=%s&precision=s HTTP/1.1\r\n"
           "Host: %s:%u\r\n"
           "Authorization: Token %s\r\n"
           "Content-Type: application/json\r\n"
           "Accept: application/json\r\n"
           "Content-Length: %lu\r\n"
           "\r\n"
           "%s",
           ifdb_info->organization,
           ifdb_info->database,
           ifdb_info->hostname,
           ifdb_info->port,
           ifdb_info->token,
           strlen(query),
           query);

  // send the HTTP request
  int sockfd = ifdb_info->sockfd;
  if (send_http_request(sockfd, request) < 0) {
    perror("Failed to send HTTP request");
    free(result);
    return NULL;
  }

  // receive the response
  char response[MAX_BUF_SIZE];
  int received = recv(sockfd, response, sizeof(response) - 1, 0);
  if (received < 0) {
    perror("Failed to receive response");
    free(result);
    return NULL;
  }
  response[received] = '\0'; // null-terminate the response

  // print the raw response
  printf("[+] Raw response:\n%s\n", response);

  // optionally, free the allocated result structure
  free(result);

  return NULL; // return NULL, since we are not processing the result yet
}

// function to fetch the number of fields in the result
uint64_t ifdb_num_fields(InfluxResult *result) { return result->field_count; }

// function to fetch a row from the result set
InfluxRow ifdb_fetch_row(InfluxResult *result) {
  if (result->row_count > 0) {
    result->row_count--;
    // placeholder: actual row retrieval logic should be here.
    InfluxRow row;
    return row;
  }
  return NULL;
}

void ifdb_show_db(InfluxResult *result) {
  if (result == NULL) {
    printf("[-] No data to display.\n");
    return;
  }

  // print headers
  for (uint64_t i = 0; i < result->field_count; i++) {
    printf("Field %lu\t", i + 1);
  }
  printf("\n");

  // print rows
  for (uint64_t i = 0; i < result->row_count; i++) {
    for (uint64_t j = 0; j < result->field_count; j++) {
      printf("%s\t", result->rows[i][j] ? result->rows[i][j] : "NULL");
    }
    printf("\n");
  }
}

// function to close the InfluxDB connection
void ifdb_close(InfluxInfo *info) {
  // close the HTTP connection, Connection: close
  if (info) {
    if (info->sockfd != -1) {
      close(info->sockfd);
    }
    free(info);
  }
}
