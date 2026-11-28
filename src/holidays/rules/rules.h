#ifndef CAL_HOLIDAYS_RULES_RULES_H
#define CAL_HOLIDAYS_RULES_RULES_H

typedef enum {
    RULE_OK,
    RULE_SKIP,      // valid, but it does not land in the requested year
    RULE_BAD_DATE,  // a fixed date that is not a real date
    RULE_BAD_RULE   // not a date and not a rule we know
} RuleStatus;

// Resolves one holiday date expression for year:
//   YYYY-MM-DD          a single day, skipped in other years
//   MM-DD               every year, Feb 29 only in leap years
//   easter[+N|-N]       Easter Sunday, optionally moved by up to 100 days
//   1st-mon-1 .. 5th-mon-1  the nth weekday of a month
//   last-mon-5          the last weekday of a month
// Returns RULE_SKIP for a date in another year, a Feb 29 that year lacks, and
// an nth weekday the month does not have (a 5th Monday, say). Text is expected
// trimmed; anything else is the caller's job.
RuleStatus rule_resolve(const char *text, int year, int *month, int *day);

const char *rule_status_message(RuleStatus status);

#endif
