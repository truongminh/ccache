#include "request.h"
#include "dicttype.h"
#include "ctype.h"

/*
 * ********************************* Sample HTTP Request *******************************
 * GET /status HTTP/1.1\r\n                                                            *
 * Host: 192.168.1.1:8888 \r\n                                                         *
 * Connection: keep-alive\r\n                                                          *
 * Cache-Control: max-age=0\r\n                                                        *
 * User-Agent: Mozilla/5.0 (X11; Linux i686) AppleWebKit/536.11 (KHTML, like Gecko) \  *
 *             Chrome/20.0.1132.57 Safari/536.11\r\n                                   *
 * Accept-Encoding: gzip,deflate,sdch\r\n                                              *
 * Accept-Language: en-US,en;q=0.8\r\n                                                 *
 * Accept-Charset: UTF-8,*;q=0.5\r\n                                                   *
 * Cookie: __uid=1342231131906312522; __create=1342231131; __C=4\r\n                   *
 * \r\n                                                                                *
 ***************************************************************************************
 */


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
    r->state = http_method_start;
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
    r->state = http_method_start;
}

request_parse_state requestParse(request* r, char* begin, char* end)
{
    request_parse_state result = parse_not_completed;

    if(end>begin+MAX_REQUEST_SIZE) return parse_error;
    char current;
    char *buf = r->buf;
    char *ptr = r->ptr;
    sds header_key = r->current_header_key;
    sds header_value = r->current_header_value;
    http_state state = r->state;
    while (begin != end)
    {
        current = *begin++;
        switch (state)
        {
        case http_method_start:
            if (!is_char(current) || iscntrl(current) || is_tspecial(current))
            {
                result = parse_error;
            }
            else
            {
                state = http_method;
                ptr = buf;
                *ptr++ = current;
                result =  parse_not_completed;
            }
            break;
        case http_method:
            if (current == ' ')
            {
                *ptr++='\0';
                r->method = sdsnewlen(buf,ptr-buf);
                ptr=buf;
                state = http_uri;
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
            break;
        case http_uri:
            if (current == ' ')
            {
                *ptr++='\0';
                r->uri = sdsnewlen(buf,ptr-buf);
                ptr=buf;
                state = http_version_h;
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
            break;

        case http_version_h:
            if (current == 'H')
            {
                state = http_version_t_1;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_version_t_1:
            if (current == 'T')
            {
                state = http_version_t_2;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_version_t_2:
            if (current == 'T')
            {
                state = http_version_p;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;
        case http_version_p:
            if (current == 'P')
            {
                state = http_version_slash;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_version_slash:
            if (current == '/')
            {
                r->version_major = 0;
                r->version_minor = 0;
                state = http_version_major_start;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;
        case http_version_major_start:
            if (isdigit(current))
            {
                r->version_major = r->version_major * 10 + current - '0';
                state = http_version_major;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_version_major:
            if (current == '.')
            {
                state = http_version_minor_start;
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
            break;

        case http_version_minor_start:
            if (isdigit(current))
            {
                r->version_minor = r->version_minor * 10 + current - '0';
                state = http_version_minor;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_version_minor:
            if (current == '\r')
            {
                state = http_expecting_newline_1;
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
            break;

        case http_expecting_newline_1:
            if (current == '\n')
            {
                state = http_header_line_start;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_header_line_start:
            if (current == '\r')
            {
                /* new line after new line, meaning all headeres are parsed
                 * header_key and header_value are assigned to sdsempty
                 * to simplify the treatment of current_header fields in the request
                 * In particular, assigning the values to sdsempty(),
                 * there is no need to check NULL in requestReset and requestFree
                */
                header_key = sdsempty();
                header_value = sdsempty();
                state = http_expecting_newline_3;
                result =  parse_not_completed;
            }
            else if (r->headers->used>0 && (current == ' ' || current == '\t'))
            {
                *ptr++='\0';
                header_key = sdscatlen(header_key,buf,ptr-buf);
                ptr=buf;
                state = http_header_lws;
                result =  parse_not_completed;
            }
            else if (!is_char(current) || iscntrl(current) || is_tspecial(current))
            {
                result =  parse_error;
            }
            else
            {
                /* new header in the current line */
                header_key = sdsempty();
                header_value = sdsempty();
                *ptr++=current;
                state = http_header_name;
                result =  parse_not_completed;
            }
            break;

        case http_header_lws:
            if (current == '\r')
            {
                *ptr++='\0';
                header_value = sdscatlen(header_value,buf,ptr-buf);
                dictAdd(r->headers,header_key,header_value);
                ptr=buf;
                state = http_expecting_newline_2;
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
                state = http_header_value;
                *ptr++=current;
                //req.headers.back().value.push_back(current);
                result =  parse_not_completed;
            }
            break;

        case http_header_name:
            if (current == ':') // end of header_name
            {
                *ptr++='\0';
                header_key = sdscatlen(header_key,buf,ptr-buf);
                ptr=buf;
                state = http_space_before_header_value;
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
            break;

        case http_space_before_header_value:
            if (current == ' ')
            {
                state = http_header_value;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_header_value:
            if (current == '\r')
            {
                *ptr++='\0';
                header_value= sdscatlen(header_value,buf,ptr-buf);
                dictAdd(r->headers,header_key,header_value);
                ptr=buf;
                state = http_expecting_newline_2;
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
            break;

        case http_expecting_newline_2:
            if (current == '\n')
            {
                state = http_header_line_start;
                result =  parse_not_completed;
            }
            else
            {
                result =  parse_error;
            }
            break;

        case http_expecting_newline_3:
            if (current == '\n') result =  parse_completed;
            else result =  parse_error;
            break;

        default:
            result = parse_error;
            break;
        }
        if (result==parse_error|| result==parse_completed) break;
    }
    r->ptr = ptr;
    r->current_header_key = header_key;
    r->current_header_value = header_value;
    r->state = state;
    return  result;
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

void requestPrint(request *r){
    printf("%s %s\n",r->method,r->uri);
    dictIterator *di = dictGetIterator(r->headers);
    dictEntry *de;
    while((de = dictNext(di))){
        printf("%s: %s\n",(sds)dictGetEntryKey(de),(sds)dictGetEntryVal(de));
    }
    printf("Current Key: %s\n",r->current_header_key);
    printf("Current Value: %s\n",r->current_header_value);
    printf("STATE: %d\n",r->state);
}
