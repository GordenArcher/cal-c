#ifndef CAL_CORE_CLOCK_CLOCK_H
#define CAL_CORE_CLOCK_CLOCK_H

typedef struct {
    int valid;
    int year;
    int month;
    int day;
} Today;

// valid == 0 means the system clock could not be read, callers must check
// before trusting the other fields.
Today clock_today(void);

#endif