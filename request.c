#include "request.h"
#include "dicttype.h"
#include "ctype.h"



/// Check if a byte is an HTTP character.
static int is_char(char c);
/// Check if a byte is defined as an HTTP tspecial character.
static int is_tspecial(char c);

request *requestCreate() {
    request* r;
    if((r = malloc(sizeof(*r))) == NULL) return NULL;
    r->method = sdsempty();
    r->uri = sdsempty();
    r->headers = dictCreate(&sdsDictType,NULL);
    r->current_header_key = sdsempty();
    r->current_header_value = sdsempty();
    return r;
}

void requestFree(request *r) {
    sdsfree(r->method);
    sdsfree(r->uri);
    dictRelease(r->headers);
    sdsfree(r->current_header_key);
    sdsfree(r->current_header_value);
    free(r);
}

sds requestGetHeaderValue(request *r, const sds key){
    return dictFetchValue(r->headers,key);
}

void requestReset(request *r){
    sdsclear(r->method);
    sdsclear(r->uri);    
    dictRelease(r->headers);
    r->headers = dictCreate(&sdsDictType,NULL);
    sdsclear(r->current_header_key);
    sdsclear(r->current_header_value);
}

parse_state parse(request* r, char* begin, char* end)
{
    parse_state result;

    if(end>begin+MAX_REQUEST_SIZE) return parse_error;
    char current;
    char *buf = r->buf;
    char *ptr = r->ptr;
    sds header_key = r->current_header_key;
    sds header_value = r->current_header_value;

    while (begin != end)
    {
        current = *begin;
        switch (r->state)
        {
        case http_method_start:
            if (!is_char(current) || iscntrl(current) || is_tspecial(current))
            {
                return parse_error;
            }
            else
            {
                r->state = http_method;
                ptr = buf;
                *ptr++ = current;
                result =  parse_not_completed;
            }
        case http_method:
            if (current == ' ')
            {
                *ptr++='\0';
                r->method = buf;
                ptr=buf;
                r->state = http_uri;
                result =  parse_not_completed;
            }
            else if (!is_char(current) || iscntrl(current) || is_tspecial(current))
            {
                result =  parse_error;
            }
            else
            {
                *ptr++ = current;
                result =  parse_not_completed;
            }
        case http_uri:
            if (current == ' ')
            {
                *ptr++='\0';
                r->uri = buf;
                ptr=buf;
                r->state = http_version_h;
                result =  parse_not_completed;
            }
            else if (iscntrl(current))
            {
                result =  parse_error;
            }
            else
            {
                *ptr++=current;
                result =  parse_not_completed;
            }
        case http_version_h:
            if (current == 'H')
            {
                r->state = http_version_t_1;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_t_1:
            if (current == 'T')
            {
                r->state = http_version_t_2;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_t_2:
            if (current == 'T')
            {
                r->state = http_version_p;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_p:
            if (current == 'P')
            {
                r->state = http_version_slash;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_slash:
            if (current == '/')
            {
                r->version_major = 0;
                r->version_minor = 0;
                r->state = http_version_major_start;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_major_start:
            if (isdigit(current))
            {
                r->version_major = r->version_major * 10 + current - '0';
                r->state = http_version_major;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_major:
            if (current == '.')
            {
                r->state = http_version_minor_start;
                result =  parse_not_completed;
            }
            else if (isdigit(current))
            {
                r->version_major = r->version_major * 10 + current - '0';
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_minor_start:
            if (isdigit(current))
            {
                r->version_minor = r->version_minor * 10 + current - '0';
                r->state = http_version_minor;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_version_minor:
            if (current == '\r')
            {
                r->state = http_expecting_newline_1;
                result =  parse_not_completed;
            }
            else if (isdigit(current))
            {
                r->version_minor = r->version_minor * 10 + current - '0';
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_expecting_newline_1:
            if (current == '\n')
            {
                r->state = http_header_line_start;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_header_line_start:
            if (current == '\r')
            {
                r->state = http_expecting_newline_3;
                result =  parse_not_completed;
            }
            else if (r->headers->used>0 && (current == ' ' || current == '\t'))
            {
                *ptr++='\0';
                header_key = sdscatlen(header_key,buf,ptr-buf);
                ptr=buf;
                r->state = http_header_lws;
                result =  parse_not_completed;
            }
            else if (!is_char(current) || iscntrl(current) || is_tspecial(current))
            {
                result =  parse_error;
            }
            else
            {
                // create a new header
                dictAdd(r->headers,header_key,header_value);
                *ptr++=current;
                //req.headers.back().name.push_back(current);
                r->state = http_header_name;
                result =  parse_not_completed;
            }
        case http_header_lws:
            if (current == '\r')
            {
                *ptr++='\0';
                header_value = sdscatlen(header_value,buf,ptr-buf);
                ptr=buf;
                r->state = http_expecting_newline_2;
                result =  parse_not_completed;
            }
            else if (current == ' ' || current == '\t')
            {
                result =  parse_not_completed;
            }
            else if (iscntrl(current))
            {
                result =  parse_error;
            }
            else
            {
                r->state = http_header_value;
                *ptr++=current;
                //req.headers.back().value.push_back(current);
                result =  parse_not_completed;
            }
        case http_header_name:
            if (current == ':') // end of header_name
            {
                *ptr++='\0';
                header_key = sdscatlen(header_key,buf,ptr-buf);
                ptr=buf;
                r->state = http_space_before_header_value;
                result =  parse_not_completed;
            }
            else if (!is_char(current) || iscntrl(current) || is_tspecial(current))
            {
                result =  parse_error;
            }
            else
            {
                *ptr++=current;
                //req.headers.back().name.push_back(current);
                result =  parse_not_completed;
            }
        case http_space_before_header_value:
            if (current == ' ')
            {
                r->state = http_header_value;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_header_value:
            if (current == '\r')
            {
                *ptr++='\0';
                header_value= sdscatlen(header_value,buf,ptr-buf);

                ptr=buf;
                r->state = http_expecting_newline_2;
                result =  parse_not_completed;
            }
            else if (iscntrl(current))
            {
                result =  parse_error;
            }
            else
            {
                *ptr++=current;
                //req.headers.back().value.push_back(current);
                result =  parse_not_completed;
            }
        case http_expecting_newline_2:
            if (current == '\n')
            {
                r->state = http_header_line_start;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
        case http_expecting_newline_3:
            if (current == '\n') result =  parse_completed;
            else result =  parse_error;
        default:
            result =  parse_error;
        }
        if (result==parse_error|| result==parse_completed)
            return result;
    }
    r->ptr = ptr;
    r->current_header_key = header_key;
    r->current_header_value = header_value;
    return  parse_not_completed;
}

int is_char(char c)
{
    //return c >= 0 && c <= 127;
    return ~(c >>7);
}


int is_tspecial(char c)
{

    switch (c)
    {
    case '(': case ')': case '<': case '>': case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?': case '=':
    case '{': case '}': case ' ': case '\t':
        return 1;
    default:
        return 0;
    }
    // NO. They are not punctuations.
    //return ispunct(c);
}