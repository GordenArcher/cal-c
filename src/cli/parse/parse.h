#ifndef CAL_CLI_PARSE_PARSE_H
#define CAL_CLI_PARSE_PARSE_H

// Both return 1 and fill *out on success, 0 on bad input.
int parse_month(const char *s, int *out);
int parse_year(const char *s, int *out);

// A line number as shown by "list": a whole number of 1 or more.
int parse_line_number(const char *s, int *out);

#endif