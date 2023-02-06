#include "httpd.h"
#include <errno.h>
#include "utils.h"

#define LF                  (u_char) '\n'
#define CR                  (u_char) '\r'
#define CRLF                "\r\n"

typedef struct  
{
    char                *key;
    char                *value;
} request_fields_t;

typedef struct
{
    char                *method;
    char                *uri;
    char                *version;
    uint8_t             fields_count;
    request_fields_t    *fields;
} request_header_t;

static void accept_callback(event_t *ev);
static void read_callback(event_t *ev);
static void write_callback(event_t *ev);

static int   read_request_header(event_t *ev, char **buf, int *size);
static void  read_request_boundary(event_t *ev);
static int   parse_request_header(char *data, request_header_t *header);
static void  release_request_header(request_header_t *header);
static void  release_event(event_t *ev);
static event_data_t *create_event_data(const char *header, const char *html);
static event_data_t *create_event_data_fp(const char *header, FILE *fp, int read_len, int total_len);
static void  release_event_data(event_t *ev);
static Bool_t  uri_decode(char* uri);
static uint8_t ishex(uint8_t x);
static char *local_file_list(char *path);
static int   reset_filename_from_formdata(event_t *ev, char **formdata, int size);
static int   parse_boundary(event_t *ev, char *data, int size, char **ptr);

static const char *reponse_content_type(char *file_name);
static const char *response_header_format();
static const char *response_body_format();
static void response_home_page(event_t *ev, char *path);
static void response_upload_page(event_t *ev, int result);
static void response_send_file_page(event_t *ev, char *file_name);
static void response_http_400_page(event_t *ev);
static void response_http_404_page(event_t *ev);
static void response_http_500_page(event_t *ev);
static void response_http_501_page(event_t *ev);
static void send_response(event_t *ev, char* title, char *status);

int http_startup(uint16_t *port)
{
    SOCKET fd;
    event_t ev = {0};

    log_info("{%s:%d} Http server start...", __FUNCTION__, __LINE__);
    network_init();
    event_init();
    network_listen(port, &fd);

    ev.fd = fd;
    ev.ip = htonl(INADDR_ANY);
    ev.type = EV_READ | EV_PERSIST;
    ev.callback = accept_callback;
    event_add(&ev);

    event_dispatch();

    closesocket(fd);
    event_uninit();
    network_unint();
    log_info("{%s:%d} Http server stop ...", __FUNCTION__, __LINE__);
    return SUCC;
}

static void accept_callback(event_t *ev)
{
    SOCKET fd;
    struct in_addr addr;
    event_t ev_ = {0};

    if (SUCC == network_accept(ev->fd, &addr, &fd))
    {   
        ev_.fd = fd;
        ev_.ip = addr.s_addr;
        ev_.type = EV_READ | EV_PERSIST;
        ev_.callback = read_callback;
        event_add(&ev_);

        log_info("{%s:%d} A new client connect. ip = %s, socket=%d", __FUNCTION__, __LINE__, inet_ntoa(addr), fd);
    }
    else
    {
        log_error("{%s:%d} accept fail. WSAGetLastError=%d", __FUNCTION__, __LINE__, WSAGetLastError());
    }
}

static void read_callback(event_t *ev)
{
    char *buf = NULL;
    int   size;
    request_header_t header;
    int   i;
    int   content_length = 0;
    char *temp = NULL;
    char  file_path[MAX_PATH2] = {0};
    int iSnprintRet = -1;
    int iTmp = 0;

    if (ev->status == EV_IDLE)
    {
        if (SUCC != read_request_header(ev, &buf, &size))
        {
            response_http_400_page(ev);
            free(buf);
            return;
        }
        if (!buf)
            return;
        parse_request_header(buf, &header);
        if ( False == uri_decode(header.uri) )
        {
            return;
        }
        header.uri = utf8_to_ansi(header.uri);
        log_info("{%s:%d} >>> Entry recv ... uri=%s", __FUNCTION__, __LINE__, header.uri);
        if (_strnicmp(header.method, "GET", 3 ) && _strnicmp(header.method, "POST", 4))
        {
            // 501 Not Implemented
            response_http_501_page(ev);
            release_request_header(&header);
            free(buf);
            return;
        }
        if (0 == _strnicmp(header.method, "POST", 4))
        {
            /***** This program is not supported now.                             *****/
            /***** Just using Content-Type = multipart/form-data when upload file *****/

            // 1. get Content-Length
            // 2. if don't have Content-Length. using chunk
            // 3. read_request_body()
            // 4. get Content-Type
            // 5. Content-Type is json or others
        }
        if (0 == _strnicmp(header.uri, "/upload", strlen("/upload")) && 0 == _strnicmp(header.method, "POST", 4))
        {
            // get upload file save path from uri
            memset(file_path, 0, sizeof(file_path));
            iSnprintRet = memcpy_s(file_path, sizeof(file_path), root_path(), strlen(root_path()));
            ASSERT( 0 == iSnprintRet );
            if (strlen(header.uri) > strlen("/upload?path="))
            {
                iTmp = strlen(file_path);
                iSnprintRet = memcpy_s(file_path+iTmp, sizeof(file_path)-iTmp, header.uri+strlen("/upload?path="), strlen(header.uri)-strlen("/upload?path="));
                ASSERT( 0 == iSnprintRet );
            }
            if (0 == _strnicmp(header.method, "POST", 4))
            {
                // get Content-Length boundary_length
                for (i=0; i<header.fields_count; i++)
                {
                    if (0 == _strnicmp(header.fields[i].key, "Content-Length", 14))
                    {
                        content_length = atoi(header.fields[i].value);
                        break;
                    }
                }
                if (content_length == 0)
                {
                    // not support
                    // 501 Not Implemented
                    response_http_501_page(ev);
                    release_request_header(&header);
                    free(buf);
                    return;
                }

                // get boundary
                ev->data = (event_data_t*)malloc(sizeof(event_data_t)-sizeof(char)+BUFFER_UNIT);
                if (!ev->data)
                {
                    log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
                    // 500 Internal Server Error
                    response_http_500_page(ev);
                    release_request_header(&header);
                    free(buf);
                    return;
                }
                memset(ev->data, 0, sizeof(event_data_t)-sizeof(char)+BUFFER_UNIT);
                for (i=0; i<header.fields_count; i++)
                {
                    if (0 == _strnicmp(header.fields[i].key, "Content-Type", 12 ))
                    {
                        temp = stristr(header.fields[i].value, "boundary=");
                        if (temp)
                        {
                            ASSERT( strlen("boundary=") < strlen(temp) );
                            temp += strlen("boundary=");
                            iSnprintRet = memcpy_s(ev->data->boundary, sizeof(ev->data->boundary), temp, strlen(temp));
                            ASSERT( 0 == iSnprintRet );
                        }
                        break;
                    }
                }
                if (ev->data->boundary[0] == 0)
                {
                    // not support
                    // 501 Not Implemented
                    response_http_501_page(ev);
                    release_request_header(&header);
                    free(buf);
                    return;
                }

                // release memory
                release_request_header(&header);
                free(buf);

                // set event
                ASSERT( strlen(file_path) < sizeof(ev->data->file) );
                iSnprintRet = memcpy_s(ev->data->file, sizeof(ev->data->file), file_path, strlen(file_path));
                ASSERT( 0 == iSnprintRet );
                ev->data->offset = 0;
                ev->data->total = content_length;

                // read & save files
                read_request_boundary(ev);
            }
            else
            {
                // not support
                // 501 Not Implemented
                response_http_501_page(ev);
                free(buf);
                release_request_header(&header);
                return;
            }
        }
        else if (header.uri[strlen(header.uri)-1] == '/')
        {
            response_home_page(ev, header.uri+1);
            free(buf);
            release_request_header(&header);
            return;
        }
        else
        {
            // send file
            memset(file_path, 0, sizeof(file_path));
            iSnprintRet = _snprintf( file_path, sizeof(file_path)-1, "%s%s", root_path(), header.uri+1 );
            if ( iSnprintRet <= 0 )
            {
                ASSERT( 0 );
                memset(file_path, 0, sizeof(file_path));
            }
            response_send_file_page(ev, file_path);
            free(buf);
            release_request_header(&header);
            return;
        }
    }
    else
    {
        // read & save files
        read_request_boundary(ev);
    }
}

static void write_callback(event_t *ev)
{
    if (!ev->data)
        return;

    if (ev->data->size != send(ev->fd, ev->data->data, ev->data->size, 0))
    {
        log_error("{%s:%d} send fail. socket=%d, WSAGetLastError=%d", __FUNCTION__, __LINE__, ev->fd, WSAGetLastError());
        shutdown(ev->fd, SD_SEND);
        release_event_data(ev);
        return;
    }
    log_debug("{%s:%d} send response completed. progress=%d%%, socket=%d", __FUNCTION__, __LINE__, ev->data->total ? ev->data->offset*100/ev->data->total : 100, ev->fd);
    if (ev->data->total == ev->data->offset)
    {
        log_info("{%s:%d} send response completed. socket=%d", __FUNCTION__, __LINE__, ev->fd);
        release_event_data(ev);
    }
    else
    {
        response_send_file_page(ev, ev->data->file);
    }
}

static int read_request_header(event_t *ev, char **buf, int *size)
{
    char c;
    const int len = 1;
    int idx = 0;
    int ret;

    while (TRUE)
    {
        ret = network_read(ev->fd, &c, len);
        if (ret == DISC)
        {
            release_event(ev);
            return SUCC;
        }
        else if (ret == SUCC)
        {
            if (*buf == NULL)
            {
                *size = BUFFER_UNIT;
                *buf = (char*)malloc(*size);
                if (!(*buf))
                {
                    log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
                    return FAIL;
                }
            }
            (*buf)[idx++] = c;
            if (idx >= *size - 1) // last char using for '\0'
            {
                // buffer is not enough
                *size += BUFFER_UNIT;
                *buf = (char*)realloc(*buf, *size);
                if (!(*buf))
                {
                    log_error("{%s:%d} realloc fail.", __FUNCTION__, __LINE__);
                    return FAIL;
                }
            }
            if (idx >= 4 && (*buf)[idx - 1] == LF && (*buf)[idx - 2] == CR
                && (*buf)[idx - 3] == LF && (*buf)[idx - 4] == CR)
            {
                (*buf)[idx] = 0;
                return SUCC;
            }
        }
        else
        {
            log_error("{%s:%d} recv unknown fail.", __FUNCTION__, __LINE__);
            return FAIL;
        }
    }

    log_error("{%s:%d} cannot found header.", __FUNCTION__, __LINE__);
    return FAIL;
}

static void read_request_boundary(event_t *ev)
{
#define WRITE_FILE(fp, buf, size, ev) do { \
    if (size) { \
        if (size != fwrite(compare_buff, 1, size, fp)) { \
        log_error("{%s:%d} write file fail. socket=%d", __FUNCTION__, __LINE__, ev->fd); \
            release_event_data(ev); \
            ev->status = EV_IDLE; \
            response_upload_page(ev, 0); \
            return; \
        } \
    } \
} while (0)

#define GET_FILENAME(ev, ptr, size_) do { \
    ret = reset_filename_from_formdata(ev, &ptr, (size_)); \
    if (ret == 0) { \
        log_error("{%s:%d} cannot found filename in formdata. socket=%d", __FUNCTION__, __LINE__, ev->fd); \
        release_event_data(ev); \
        ev->status = EV_IDLE; \
        response_upload_page(ev, 0); \
        return; \
    } else if (ret == 1) { \
        ev->data->fp = fopen(ev->data->file, "wb"); \
        if (!ev->data->fp) { \
            log_error("{%s:%d} open file fail. filename=%s, socket=%d, errno=%d", __FUNCTION__, __LINE__, ev->data->file, ev->fd, errno); \
            release_event_data(ev); \
            ev->status = EV_IDLE; \
            response_upload_page(ev, 0); \
            return; \
        } \
        compare_buff_size -= ptr - compare_buff; \
        iSnprintRet = memcpy_s(compare_buff, sizeof(compare_buff), ptr, compare_buff_size); \
        ASSERT( 0 == iSnprintRet ); \
        compare_buff[compare_buff_size] = 0; \
        goto _re_find; \
    } else { \
        ev->data->size = compare_buff + compare_buff_size - ptr; \
        iSnprintRet = memcpy_s(ev->data->data, BUFFER_UNIT, ptr, ev->data->size); \
        ASSERT( 0 == iSnprintRet ); \
    } \
} while (0)

    char    *buf    = NULL;
    char    *ptr    = NULL;
    int      ret    = 0;
    uint32_t offset = 0;
    uint32_t writen = 0;
    uint32_t compare_buff_size = 0;
    char     buffer[BUFFER_UNIT+1] = {0};
    char     compare_buff[BUFFER_UNIT*2 + 1] = {0};
    int iSnprintRet = -1;

    offset = ev->data->total - ev->data->offset > BUFFER_UNIT ? BUFFER_UNIT : ev->data->total - ev->data->offset;
    ret = network_read(ev->fd, buffer, offset);
    
    if (ret == DISC)
    {
        response_http_500_page(ev);
        release_event(ev);
        return;
    }
    else if (ret == SUCC)
    {
        memset(compare_buff, 0, sizeof(compare_buff));
        if (ev->data->size)
        {
            iSnprintRet = memcpy_s(compare_buff, sizeof(compare_buff), ev->data->data, ev->data->size);
            ASSERT( 0 == iSnprintRet );
        }
        iSnprintRet = memcpy_s(compare_buff+ev->data->size, sizeof(compare_buff)-ev->data->size, buffer, offset);
        ASSERT( 0 == iSnprintRet );
        compare_buff_size = offset + ev->data->size;
        ev->data->size = 0;
        memset(ev->data->data, 0, BUFFER_UNIT);

        // parse boundary
    _re_find:
        ret = parse_boundary(ev, compare_buff, compare_buff_size, &ptr);
        switch (ret)
        {
        case 0: // write all bytes to file
            WRITE_FILE(ev->data->fp, compare_buff, compare_buff_size, ev);
            log_debug("{%s:%d} upload [%s] progress=%d%%. socket=%d", __FUNCTION__, __LINE__, ev->data->file, ev->data->offset * 100 / ev->data->total, ev->fd);
            break;
        case 1: // first boundary
            // get file name from boundary header
            GET_FILENAME(ev, ptr, compare_buff + compare_buff_size - ptr);
            break;
        case 2: // last boundary
            ASSERT(ev->data->total == ev->data->offset + offset);
            writen = ptr - compare_buff;
            // writen bytes before boundary
            WRITE_FILE(ev->data->fp, compare_buff, writen, ev);
            fclose(ev->data->fp);
            log_info("{%s:%d} upload [%s] complete. socket=%d", __FUNCTION__, __LINE__, ev->data->file, ev->fd);
            release_event_data(ev);
            ev->status = EV_IDLE;
            response_upload_page(ev, 1);
            return;
        case 3: // middle boundary
            // writen bytes before boundary
            writen = ptr - compare_buff;
            WRITE_FILE(ev->data->fp, compare_buff, writen, ev);
            fclose(ev->data->fp);
            log_info("{%s:%d} upload [%s] complete. socket=%d", __FUNCTION__, __LINE__, ev->data->file, ev->fd);
            // get file name from boundary header
            GET_FILENAME(ev, ptr, compare_buff + compare_buff_size - ptr);
            break;
        case 4: // backup last boundary
        case 5: // backup middle boundary
            writen = ptr - compare_buff;
            WRITE_FILE(ev->data->fp, compare_buff, writen, ev);
            // backup
            ev->data->size = compare_buff + compare_buff_size - ptr;
            iSnprintRet = memcpy_s(ev->data->data, BUFFER_UNIT, ptr, ev->data->size);
            ASSERT( 0 == iSnprintRet );
            break;
        default:
            break;
        }

        ev->data->offset += offset;
        ev->status = EV_BUSY;
    }
    else
    {
        log_error("{%s:%d} recv unknown fail.", __FUNCTION__, __LINE__);
        release_event_data(ev);
        ev->status = EV_IDLE;
        response_upload_page(ev, 0);
        return;
    }
}

static int parse_request_header(char *data, request_header_t *header)
{
#define move_next_line(x)   while (*x && *(x + 1) && *x != CR && *(x + 1) != LF) x++;
#define next_header_line(x) while (*x && *(x + 1) && *x != CR && *(x + 1) != LF) x++; *x=0;
#define next_header_word(x) while (*x && *x != ' ' && *x != ':' && *(x + 1) && *x != CR && *(x + 1) != LF) x++; *x=0;

    char *p = data;
    char *q;
    int idx = 0;

    memset(header, 0, sizeof(request_header_t));
    // method
    next_header_word(p);
    header->method = data;
    // uri
    data = ++p;
    next_header_word(p);
    header->uri = data;
    // version
    data = ++p;
    next_header_word(p);
    header->version = data;
    // goto fields data
    next_header_line(p);
    data = ++p + 1;
    p++;
    // fields_count
    q = p;
    while (*p)
    {
        move_next_line(p);
        data = ++p + 1;
        p++;
        header->fields_count++;
        if (*data && *(data + 1) && *data == CR && *(data + 1) == LF)
            break;
    }
    // malloc fields
    header->fields = (request_fields_t*)malloc(sizeof(request_fields_t)*header->fields_count);
    if (!header->fields)
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return FAIL;
    }
    // set fields
    data = p = q;
    while (*p)
    {
        next_header_word(p);
        header->fields[idx].key = data;
        data = ++p;
        if (*data == ' ')
        {
            data++;
            p++;
        }
        next_header_line(p);
        header->fields[idx++].value = data;
        data = ++p + 1;
        p++;
        if (*data && *(data + 1) && *data == CR && *(data + 1) == LF)
            break;
    }
    ASSERT(idx == header->fields_count);
    return SUCC;
}

static void release_request_header(request_header_t *header)
{
    if (header->uri)
    {
        free(header->uri);
    }
    if (header->fields)
    {
        free(header->fields);
    }
}

static void release_event(event_t *ev)
{
    closesocket(ev->fd);
    release_event_data(ev);
    event_del(ev);
}

static event_data_t *create_event_data(const char *header, const char *html)
{
    event_data_t* ev_data = NULL;
    int header_length = 0;
    int html_length = 0;
    int data_length = 0;
    int iSnprintRet = -1;

    if (header)
        header_length = strlen(header);
    if (html)
        html_length = strlen(html);

    data_length = sizeof(event_data_t) - sizeof(char) + header_length + html_length;
    ev_data = (event_data_t*)malloc(data_length);
    if (!ev_data)
    {
        log_error("{%s:%d} malloc failed", __FUNCTION__, __LINE__);
        return ev_data;
    }
    memset(ev_data, 0, data_length);
    ev_data->total = header_length + html_length;
    ev_data->offset = header_length + html_length;
    ev_data->size = header_length + html_length;
    if (header)
    {
        iSnprintRet = memcpy_s(ev_data->data, header_length, header, header_length);
        ASSERT( 0 == iSnprintRet );
    }
    if (html)
    {
        iSnprintRet = memcpy_s(ev_data->data + header_length, html_length, html, html_length);
        ASSERT( 0 == iSnprintRet );
    }
    return ev_data;
}

static event_data_t *create_event_data_fp(const char *header, FILE *fp, int read_len, int total_len)
{
    event_data_t* ev_data = NULL;
    int header_length = 0;
    int data_length = 0;
    int iSnprintRet = -1;

    if (header)
        header_length = strlen(header);

    data_length = sizeof(event_data_t) - sizeof(char) + header_length + read_len;
    ev_data = (event_data_t*)malloc(data_length);
    if (!ev_data)
    {
        log_error("{%s:%d} malloc failed", __FUNCTION__, __LINE__);
        return ev_data;
    }
    memset(ev_data, 0, data_length);
    ev_data->total = total_len;
    ev_data->size = read_len + header_length;
    if (header)
    {
        iSnprintRet = memcpy_s(ev_data->data, header_length, header, header_length);
        ASSERT( 0 == iSnprintRet );
    }
    if (read_len != fread(ev_data->data + header_length, 1, read_len, fp))
    {
        log_error("{%s:%d} fread failed", __FUNCTION__, __LINE__);
        free(ev_data);
        ev_data = NULL;
    }
    return ev_data;
}

static void release_event_data(event_t *ev)
{
    if (ev->data)
    {
        if (ev->data->fp)
        {
            fclose(ev->data->fp);
            ev->data->fp = NULL;
        }
        free(ev->data);
        ev->data = NULL;
    }
}

static Bool_t uri_decode(char* uri)
{
    const unsigned int len = strlen(uri);
    char *out = NULL;
    char *o = NULL;
    char *s = uri;
    const char *end = uri + strlen(uri);
    int c;
    int iSnprintRet = -1;


    ASSERT( *end == 0 );
    out = (char*)malloc(len+1);
    if (!out)
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return False;
    }

    for (o = out; s <= end; o++)
    {
        c = *s++;
        if (c == '+')
        {
            c = ' ';
        }
        else if (c == '%' 
            && (!ishex(*s++) || !ishex(*s++) || !sscanf(s - 2, "%2x", &c)))
        {
            // bad uri
            free(out);
            return False;
        }

        *o = c;
    }

    {
    unsigned int nTmp = strlen(out);
    ASSERT(nTmp <= len);
    iSnprintRet = memcpy_s(uri, len, out, nTmp);
    ASSERT(0 == iSnprintRet);
    uri[nTmp] = 0;
    free(out);
    }
    return True;
}

static uint8_t ishex(uint8_t x)
{
    return (x >= '0' && x <= '9') ||
        (x >= 'a' && x <= 'f') ||
        (x >= 'A' && x <= 'F');
}

static int reset_filename_from_formdata(event_t *ev, char **formdata, int size)
{
    char *file_name = NULL;
    char *p         = NULL;
    int   i         = 0;
    int   found     = 0;
    char *anis      = NULL;
    int iSnprintRet = -1;

    // find "\r\n\r\n"
    p = *formdata;
    for ( i = 0; i <= size-4; i++)
    {
        if (0 == memcmp(*formdata + i, CRLF CRLF, strlen(CRLF CRLF)))
        {
            found = 1;
            (*formdata)[i] = 0;
            (*formdata) += i + strlen(CRLF CRLF);
            break;
        }
    }
    if (!found)
        return 2;

    // file upload file name from formdata
    file_name = stristr(p, "filename=\"");
    if (!file_name)
        return 0;
    file_name += strlen("filename=\"");
    *strstr(file_name, "\"") = 0;

    anis = utf8_to_ansi(file_name);

    // IE browser: remove client file path
    p = anis;
    while (*p)
    {
        if (*p == '\\' || *p == '/')
        if (*(p + 1))
            anis = p + 1;
        p++;
    }

    // reset filepath
    for (i = strlen(ev->data->file) - 1; i >= 0; i--)
    {
        if (ev->data->file[i] == '/')
        {
            iSnprintRet = memcpy_s(ev->data->file + i + 1, sizeof(ev->data->file) -i -1, anis, strlen(anis) + 1);
            ASSERT( 0 == iSnprintRet );
            break;
        }
    }

    // if file exist delete file
    if (file_exist(ev->data->file))
    {
        remove_file(ev->data->file);
    }
    free(anis);
    return 1;
}

static int parse_boundary(event_t *ev, char *data, int size, char **ptr)
{
    char first_boundary[BOUNDARY_MAX_LEN]  = { 0 };
    char middle_boundary[BOUNDARY_MAX_LEN] = { 0 };
    char last_boundary[BOUNDARY_MAX_LEN]   = { 0 };
    int  first_len, middle_len, last_len;
    int i;

    sprintf(first_boundary, "--%s\r\n", ev->data->boundary);      //------WebKitFormBoundaryOG3Viw9MEZcexbvT\r\n
    sprintf(middle_boundary, "\r\n--%s\r\n", ev->data->boundary);   //\r\n------WebKitFormBoundaryOG3Viw9MEZcexbvT\r\n
    sprintf(last_boundary, "\r\n--%s--\r\n", ev->data->boundary); //\r\n------WebKitFormBoundaryOG3Viw9MEZcexbvT--\r\n
    first_len  = strlen(first_boundary);
    middle_len = strlen(middle_boundary);
    last_len   = strlen(last_boundary);

    ASSERT(size > first_len);
    ASSERT(size > middle_len);
    ASSERT(size > last_len);
    if (0 == memcmp(data, first_boundary, first_len))
    {
        *ptr = data + first_len;
        return 1; // first boundary
    }

    for (i = 0; i < size; i++)
    {
        if (size - i >= last_len)
        {
            if (0 == memcmp(data + i, middle_boundary, middle_len))
            {
                *ptr = data + i/* + middle_len*/;
                return 3; // middle boundary
            }

            if (0 == memcmp(data + i, last_boundary, last_len))
            {
                *ptr = data + (size - last_len);
                return 2; // last boundary
            }
        }
        else if (size - i >= middle_len && size - i < last_len)
        {
            if (0 == memcmp(data + i, middle_boundary, middle_len))
            {
                *ptr = data + i/* + middle_len*/;
                return 3; // middle boundary
            }
            if (0 == memcmp(data + i, last_boundary, size - i))
            {
                *ptr = data + i;
                return 4; // backup last boundary
            }
        }
        else if (size - i >= 7 && size - i < middle_len)
        {
            if (0 == memcmp(data + i, middle_boundary, size - i))
            {
                *ptr = data + i;
                return 5; // backup middle boundary
            }
            if (0 == memcmp(data + i, last_boundary, size - i))
            {
                *ptr = data + i;
                return 4; // backup last boundary
            }
        }
        else
        {
            if (0 == memcmp(data + i, middle_boundary, size - i))
            {
                *ptr = data + i;
                return 5; // backup middle boundary
            }
            if (0 == memcmp(data + i, last_boundary, size - i))
            {
                *ptr = data + i;
                return 4; // backup last boundary
            }
        }
    }

    return 0;
}

#define LOOP_FREE \
    free(utf8); utf8=NULL;\
    free(ansi); ansi=NULL;\
    free(escape_html); escape_html=NULL;\
    free(escape_uri); escape_uri=NULL;

static char* local_file_list(char *path)
{
    const char* format_dir = "<a href=\"%s/\">%s/</a>" CRLF;
    const char* format_file = "<a href=\"%s\">%s</a>";
    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind;
    wchar_t filter[MAX_PATH2] = {0};
    char *result = NULL;
    char line[BUFFER_UNIT] = {0};
    int line_length;
    int size = BUFFER_UNIT;
    int offset = 0;
    char *size_str = NULL;
    char *utf8 = NULL;
    int i;
    int iSnprintRet = -1;
    char digit[MAXIMAL_64BIT_UNSIGN_DEC + 1] = {0};

    {
    wchar_t* pathW = ansi_to_unicode(path); 
    ASSERT (NULL != pathW);
    i = swprintf(filter, _countof(filter), L"%s*", pathW );
    ASSERT( 0 < i );
	free(pathW);
    }

    // list directory
	memset( &FindFileData, 0, sizeof(FindFileData) );
    hFind = FindFirstFileW(filter, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) 
    {
        log_error("{%s:%d} Invalid File Handle. GetLastError=%d", __FUNCTION__, __LINE__, GetLastError());
        return NULL;
    }
    do 
    {
        if (FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes)
        {
            if (!result)
            {
                result = (char*)malloc(size);
                if ( NULL == result )
                {
                    log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
                    FindClose(hFind);
                    return NULL;
                }
            }
            utf8 = unicode_to_utf8(FindFileData.cFileName);
            sprintf(line, format_dir, utf8, utf8);
            line_length = strlen(line);
            line[line_length] = 0;
            if (offset+line_length > size-1)
            {
                size += BUFFER_UNIT;
                result = (char*)realloc(result, size);
                if ( NULL == result )
                {
                    log_error("{%s:%d} realloc fail.", __FUNCTION__, __LINE__);
                    free(utf8);
                    FindClose(hFind);
                    return NULL;
                }
            }
            iSnprintRet = memcpy_s(result+offset, size - offset, line, line_length);
            ASSERT( 0 == iSnprintRet );
            offset += line_length;
            free(utf8);
        }
		memset( &FindFileData, 0, sizeof(FindFileData) );
    } while (FindNextFileW(hFind, &FindFileData));
    FindClose(hFind);

    // list files
	memset( &FindFileData, 0, sizeof(FindFileData) );
    hFind = FindFirstFileW(filter, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) 
    {
        log_error("{%s:%d} Invalid File Handle. GetLastError=%d", __FUNCTION__, __LINE__, GetLastError());
        return NULL;
    }
    do
    {
        if (!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
        {
            // Points to be aware of when it comes to memory leaks:
            //    1.Resource release in globe scope
            //       result
            //    2.Resource release in function scope
            //       FindClose(hFind);
            //    3.Resource release of "IF scope"
            //       utf8
            //       ansi
            //       escape_html
            //       escape_uri
            charp2free_t escape_html = NULL;
            charp2free_t escape_uri = NULL;
            charp2free_t ansi = NULL;

            size_t size_str_len = 0;

            ASSERT(NULL == utf8 &&  NULL == ansi &&  NULL == escape_html &&  NULL == escape_uri);
            if (!result)
            {
                result = (char*)malloc(size);
                if ( NULL == result )
                {
                    log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
                    FindClose(hFind);
                    return NULL;
                }
            }
            utf8 = unicode_to_utf8(FindFileData.cFileName);
            if ( NULL == utf8 )
            {
                log_error("{%s:%d} unicode_to_utf8 fail.", __FUNCTION__, __LINE__);
                FindClose(hFind);
                free(result);
                return NULL;
            }
            ansi = unicode_to_ansi(FindFileData.cFileName);
            if ( NULL == ansi )
            {
                log_error("{%s:%d} unicode_to_ansi fail.", __FUNCTION__, __LINE__);                
                FindClose(hFind);
                free(result);
                LOOP_FREE
                return NULL;
            }

            escape_html = html_escape(utf8);
            if (NULL == escape_html) {
                log_error("{%s:%d} html_escape fail.", __FUNCTION__, __LINE__);
                FindClose(hFind);
                free(result);
                LOOP_FREE
                return NULL;
            }
            escape_uri = url_escape(utf8);
            if (NULL == escape_uri) {
                log_error("{%s:%d} url_escape fail.", __FUNCTION__, __LINE__);
                FindClose(hFind);
                free(result);
                LOOP_FREE
                return NULL;
            }

            sprintf(line, format_file, escape_uri, escape_html);
            line_length = strlen(line);
            line[line_length++] = 0x20;
            for (i=strlen(ansi); i<60; i++)
            {
                line[line_length++] = 0x20;
            }
            size_str = _ui64toa(filesize64(FindFileData.nFileSizeHigh, FindFileData.nFileSizeLow), digit, 10);
            iSnprintRet = prepend_chars(line+line_length, sizeof(line)-line_length, size_str, MAXIMAL_32BIT_UNSIGN_DEC, '\x20');
            ASSERT( 0 == iSnprintRet );
            size_str_len = strlen(size_str);
            line_length += ( MAXIMAL_32BIT_UNSIGN_DEC < size_str_len ? size_str_len : MAXIMAL_32BIT_UNSIGN_DEC );
            line[line_length++] = CR;
            line[line_length++] = LF;
            line[line_length] = 0;

            if (offset+line_length > size-1)
            {
                size += BUFFER_UNIT;
                result = (char*)realloc(result, size);
                if ( NULL == result )
                {
                    log_error("{%s:%d} realloc fail.", __FUNCTION__, __LINE__);
                    FindClose(hFind);
                    free(result);
                    LOOP_FREE
                    return NULL;
                }
            }
            iSnprintRet = memcpy_s(result+offset, size-offset, line, line_length);
            ASSERT( 0 == iSnprintRet );
            offset += line_length;
            LOOP_FREE
        }
		memset( &FindFileData, 0, sizeof(FindFileData) );
    } while (FindNextFileW(hFind, &FindFileData));
    FindClose(hFind);
    result[offset] = 0;

    return result;
}

static const char *reponse_content_type(char *file_name)
{
    const request_fields_t content_type[] =     {
        { "html",   "text/html"                 },
        { "css",    "text/css"                  },
        { "txt",    "text/plain"                },
        { "log",    "text/plain"                },
        { "cpp",    "text/plain"                },
        { "c",      "text/plain"                },
        { "h",      "text/plain"                },
        { "js",     "application/x-javascript"  },
        { "png",    "application/x-png"         },
        { "jpg",    "image/jpeg"                },
        { "jpeg",   "image/jpeg"                },
        { "jpe",    "image/jpeg"                },
        { "gif",    "image/gif"                 },
        { "ico",    "image/x-icon"              },
        { "doc",    "application/msword"        },
        { "docx",   "application/msword"        },
        { "ppt",    "application/x-ppt"         },
        { "pptx",   "application/x-ppt"         },
        { "xls",    "application/x-xls"         },
        { "xlsx",   "application/x-xls"         },
        { "mp4",    "video/mpeg4"               },
        { "mp3",    "audio/mp3"                 }
    };

    char *ext = NULL;
    int i;

    if (!file_name)
    {
        return "text/html";
    }
    ext = file_ext(file_name);
    if (ext)
    {
        for (i=0; i<sizeof(content_type)/sizeof(content_type[0]); i++)
        {
            if (0 == _stricmp(content_type[i].key, ext))
            {
                return content_type[i].value;
            }
        }
    }
    return "application/octet-stream";
}

static const char *response_header_format()
{
    const char *http_header_format =
        "HTTP/1.1 %s" CRLF
        "Content-Type: %s" CRLF
        "Content-Length: %d" CRLF
        CRLF;

    return http_header_format;
}

static const char *response_body_format()
{
    const char *http_body_format =
        "<html>" CRLF
        "<head><title>%s</title></head>" CRLF
        "<body bgcolor=\"white\">" CRLF
        "<center><h1>%s</h1></center>" CRLF
        "</body></html>";

    return http_body_format;
}

static void response_home_page(event_t *ev, char *path)
{
    const char *html_format = 
        "<html>" CRLF
        "<head>" CRLF
        "<meta charset=\"utf-8\">" CRLF
        "<title>Index of /%s</title>" CRLF
        "</head>" CRLF
        "<body bgcolor=\"white\">" CRLF
        "<h1>Index of /%s</h1><hr>" CRLF
        "<form action=\"/upload?path=%s\" method=\"post\" enctype=\"multipart/form-data\">" CRLF
        "<input type=\"file\" name=\"file\" multiple=\"true\" />" CRLF
        "<input type=\"submit\" value=\"Upload\" /></form><hr><pre>" CRLF
        "%s" CRLF
        "</body></html>";

    char header[BUFFER_UNIT] = { 0 };
    event_data_t* ev_data = NULL;
    int length;
    char *file_list = NULL;
    char *html = NULL;
    event_t ev_ = {0};
    char *utf8 = NULL;

    utf8 = ansi_to_utf8(path);
    file_list = local_file_list(path);
    if (!file_list)
        return;

    length = strlen(html_format) + strlen(file_list) + (strlen(utf8) - strlen("%s"))*3 + 1;
    html = (char*)malloc(length);
    if (!html)
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return;
    }
    sprintf(html, html_format, utf8, utf8, utf8, file_list);
    free(utf8);
    free(file_list);
    sprintf(header, response_header_format(), "200 OK", reponse_content_type(NULL), strlen(html));
    ev_data = create_event_data(header, html);
    free(html);

    ev_.fd = ev->fd;
    ev_.ip = ev->ip;
    ev_.type = EV_WRITE;
    ev_.param = ev->param;
    ev_.data = ev_data;
    ev_.callback = write_callback;
    event_add(&ev_);
}

static void response_send_file_page(event_t *ev, char *file_name)
{
    char header[BUFFER_UNIT] = { 0 };
    FILE* fp = NULL;
    int total;
    int len;
    event_data_t* ev_data = NULL;
    event_t ev_ = {0};
    int iSnprintRet = -1;

    fp = fopen(file_name, "rb");
    if (!fp)
    {
        log_error("{%s:%d} open [%s] failed, errno=%d", __FUNCTION__, __LINE__, file_name, errno);
        release_event_data(ev);
        response_http_404_page(ev);
        goto end;
    }
    if (ev->data == NULL)
    {
        fseek(fp, 0, SEEK_END);
        total = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        len = total > BUFFER_UNIT ? BUFFER_UNIT : total;
        sprintf(header, response_header_format(), "200 OK", reponse_content_type(file_name), total);
        ev_data = create_event_data_fp(header, fp, len, total);
        iSnprintRet = memcpy_s(ev_data->file, sizeof(ev_data->file), file_name, strlen(file_name));
        ASSERT( 0 == iSnprintRet );
        ev_data->offset += len;
    }
    else
    {
        fseek(fp, ev->data->offset, SEEK_SET);
        len = ev->data->total - ev->data->offset > BUFFER_UNIT ? BUFFER_UNIT : ev->data->total - ev->data->offset;
        ev_data = create_event_data_fp(NULL, fp, len, ev->data->total);
        iSnprintRet = memcpy_s(ev_data->file, sizeof(ev_data->file), file_name, strlen(file_name));
        ASSERT( 0 == iSnprintRet );
        ev_data->offset = ev->data->offset + len;
        release_event_data(ev);
    }

    ev_.fd = ev->fd;
    ev_.ip = ev->ip;
    ev_.type = EV_WRITE;
    ev_.param = ev->param;
    ev_.data = ev_data;
    ev_.callback = write_callback;
    event_add(&ev_);

end:
    if (fp)
        fclose(fp);
}

static void response_upload_page(event_t *ev, int result)
{
    if (result)
    {
        send_response(ev, "Upload completed", "200 OK");
    }
    else
    {
        send_response(ev, "Upload failed", "200 OK");
    }
}

static void response_http_400_page(event_t *ev)
{
    send_response(ev, "400 Bad Request", NULL);
}

static void response_http_404_page(event_t *ev)
{
    send_response(ev, "404 Not Found", NULL);
}

static void response_http_500_page(event_t *ev)
{
    send_response(ev, "500 Internal Server Error", NULL);
}

static void response_http_501_page(event_t *ev)
{
    send_response(ev, "501 Not Implemented", NULL);
}

static void send_response(event_t *ev, char *title, char *status)
{
    char header[BUFFER_UNIT] = { 0 };
    char body[BUFFER_UNIT]   = { 0 };
    event_data_t* ev_data = NULL;
    event_t ev_ = {0};

    sprintf(body, response_body_format(), title, title);
    sprintf(header, response_header_format(), status ? status : title, reponse_content_type(NULL), strlen(body));
    ev_data = create_event_data(header, body);

    ev_.fd = ev->fd;
    ev_.ip = ev->ip;
    ev_.type = EV_WRITE;
    ev_.param = ev->param;
    ev_.data = ev_data;
    ev_.callback = write_callback;
    event_add(&ev_);
}