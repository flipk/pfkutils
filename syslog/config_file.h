
#define DEFAULT_CONFIG_FILE "syslogd.ini"

extern int line_number;
extern int syslogd_port_number;
extern FILE * raw_output_file;

#if DEBUG_LOG
extern FILE * debug_log_file;
#endif

void syntax_error(void);
void parse_config_file(char *config_file);
