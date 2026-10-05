#!/bin/bash

# Q1. How many requests failed?
grep -c "FAIL" access.log

# Q2. How many different pages were requested, and which?
cut -d' ' -f5 access.log | sort | uniq -c

# Q3. How many requests did each page get?
cut -d' ' -f5 access.log | sort | uniq -c | sort -rn

# Q4. Which user or users have the most failed requests, and how many?
grep "FAIL" access.log | cut -d' ' -f3 | sort | uniq -c | sort -rn | head -n 1

# Q5. How many requests did user3 make, and how many of them failed?
echo "Total requests by user3: $(grep -c 'user3' access.log) | Failed: $(grep 'user3' access.log | grep -c 'FAIL')"

# Q6. Print the last 3 failed requests, showing only time and user.
grep "FAIL" access.log | tail -n 3 | cut -d' ' -f2,3

# Q7. Which login shells appear in /etc/passwd, and how many accounts use each?
cut -d':' -f7 /etc/passwd | sort | uniq -c | sort -rn

# Bonus: print only the lines where the minute ends in 0 and the request failed. How many?
echo "Bonus - Failed at exact 10 minutes:"
grep -E "10:[0-5]0.*FAIL" access.log | wc -l
