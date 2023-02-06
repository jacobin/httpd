#include "httpd.h"


static void log_print(log_level_t lv, const char *msg);
static const char* log_file_name();

void log_debug(const char *fmt, ...)
{
    char buffer[BUFFER_UNIT] = { 0 };
    va_list args;

    if (LOG_DEBU < LOG_LEVEL)
        return;
    
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);

    log_print(LOG_DEBU, buffer);
}

void log_info(const char *fmt, ...)
{
    char buffer[BUFFER_UNIT] = { 0 };
    va_list args;

    if (LOG_INFO < LOG_LEVEL)
        return;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);

    log_print(LOG_INFO, buffer);
}

void log_warn(const char *fmt, ...)
{
    char buffer[BUFFER_UNIT] = { 0 };
    va_list args;

    if (LOG_WARN < LOG_LEVEL)
        return;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);

    log_print(LOG_WARN, buffer);
    //ASSERT(false);
}

void log_error(const char *fmt, ...)
{
    char buffer[BUFFER_UNIT] = { 0 };
    va_list args;

    if (LOG_ERRO < LOG_LEVEL)
        return;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);

    log_print(LOG_ERRO, buffer);
    //ASSERT(false);
}

static void log_print(log_level_t lv, const char *msg)
{
    const char *str = NULL;
    const char *file_name = NULL;
    FILE* fp = NULL;
    char datetime[30] = {0};
    time_t t = 0;
    struct tm *p = NULL;
    t = time(NULL) + 8 * 3600;
    p = gmtime(&t);

    switch (lv)
    {
    case LOG_DEBU:
        str = "[DEBU]";
        break;
    case LOG_INFO:
        str = "[INFO]";
        break;
    case LOG_WARN:
        str = "[WARN]";
        break;
    case LOG_ERRO:
        str = "[ERRO]";
        break;
    }

    sprintf(datetime, "%d-%02d-%02d_%02d:%02d:%02d", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, (p->tm_hour)%24, p->tm_min, p->tm_sec);

    if (SAVE_FILE)
    {
        // write to file
        file_name = log_file_name();
        fp = fopen(file_name, "ab");
        if (fp)
        {
            fwrite(datetime, 1, strlen(datetime), fp);
            fwrite(" ", 1, 1, fp);
            fwrite(str, 1, strlen(str), fp);
            fwrite(" ", 1, 1, fp);
            fwrite(msg, 1, strlen(msg), fp);
            fwrite("\n", 1, 1, fp);
            fclose(fp);
        }
    }
    printf("%s %s %s\n", datetime, str, msg);
}

static const char* log_file_name()
{
    static char file_name[MAX_PATH2] = {0};
    static struct tm last = {0};
    int iSnprintRet = -1;

    time_t t = time(0);
    struct tm now;
    now = *localtime(&t);

    if (last.tm_year == now.tm_year
        && last.tm_mon == now.tm_mon
        && last.tm_mday == now.tm_mday)
    {
        return file_name;
    }
    memcpy(&last, &now, sizeof(struct tm));
    memset(file_name, 0, sizeof(file_name));
    iSnprintRet = memcpy_s(file_name, sizeof(file_name), root_path(), strlen(root_path()));
    ASSERT( 0 == iSnprintRet );
    strftime(file_name+strlen(root_path()), sizeof(file_name)-strlen(root_path()), "%Y-%m-%d.log", &now);
    return file_name;
}
