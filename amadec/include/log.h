#ifndef LOG_H
#define LOG_H

#ifdef  __cplusplus
extern "C"
{
#endif

        extern void log_print(const int level, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

#ifdef  __cplusplus
}
#endif

#endif /* LOG_H */
