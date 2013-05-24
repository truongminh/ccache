#ifndef _REQUEST_H
#define _REQUEST_H

#include "sds.h"
#include "dict.h"

#define MAX_REQUEST_SIZE 8096


typedef enum {
    parse_completed = 0,
    parse_not_completed = 1,
    parse_error = 4
} parse_state;

typedef enum
{
  http_method_start,
  http_method,
  http_uri,
  http_version_h,
  http_version_t_1,
  http_version_t_2,
  http_version_p,
  http_version_slash,
  http_version_major_start,
  http_version_major,
  http_version_minor_start,
  http_version_minor,
  http_expecting_newline_1,
  http_header_line_start,
  http_header_lws,
  http_header_name,
  http_space_before_header_value,
  http_header_value,
  http_expecting_newline_2,
  http_expecting_newline_3
}  http_state;

/// A request received from a client.
typedef struct
{
    sds method;
    sds uri;
    int version_major;
    int version_minor;
    dict *headers;
    /// The current state of the parser.
    http_state state;
    char *ptr;
    char buf[MAX_REQUEST_SIZE];
    sds current_header_key;
    sds current_header_value;
} request;

request *requestCreate();
void requestFree(request *r);
sds requestGetHeaderValue(request *r, const sds key);
void requestReset(request *r);
parse_state parse(request* r, char* begin, char* end);


#endif
