/*
    Copyright (C) 2019 Proxmox Server Solutions GmbH

    Copyright: mini-journal is under GNU GPL, the GNU General Public License.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 dated June, 1991.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
    02111-1307, USA.

    Author: Dominik Csapak <d.csapak@proxmox.com>
*/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 4096

static char BUF[BUFSIZE];
bool json = false;
bool first_line = true;

static uint64_t get_timestamp(sd_journal *j) {
    uint64_t timestamp;
    int r = sd_journal_get_realtime_usec(j, &timestamp);
    if (r < 0) {
        fprintf(stderr, "Failed  %s\n", strerror(-r));
        return (uint64_t) -1;
    }
    return timestamp;
}

static void print_to_buf(const char * string, size_t length) {
    if (!length) {
        return;
    }

    size_t r = fwrite_unlocked(string, 1, length, stdout);
    if (r < length) {
        fprintf(stderr, "Failed to write\n");
        exit(1);
    }
}

static void print_cursor(sd_journal *j) {
    int r;
    char *cursor = NULL;
    r = sd_journal_get_cursor(j, &cursor);
    if (r < 0) {
        fprintf(stderr, "Failed to get cursor: %s\n", strerror(-r));
        exit(1);
    }
    if (json) {
	if (!first_line) {
	    print_to_buf(",\"", 2);
	} else {
	    print_to_buf("\"", 1);
	}
    }
    print_to_buf(cursor, strlen(cursor));
    if (json) {
	print_to_buf("\"", 1);
    }
    print_to_buf("\n", 1);
    first_line = false;
    free(cursor);
}

static void print_first_cursor(sd_journal *j) {
    static bool printed_first_cursor = false;
    if (!printed_first_cursor) {
        print_cursor(j);
        printed_first_cursor = true;
    }
}

static void print_reboot(sd_journal *j) {
    const char *d;
    size_t l;
    int r = sd_journal_get_data(j, "_BOOT_ID", (const void **)&d, &l);
    if (r < 0) {
        fprintf(stderr, "Failed  %s\n", strerror(-r));
        return;
    }

    // remove '_BOOT_ID='
    d += 9;
    l -= 9;

    static char bootid[32];
    if (bootid[0] != '\0') { // we have some bootid
        if (memcmp(bootid, d, l)) { // a new bootid found
            memcpy(bootid, d, l);
            if (json) {
                if (!first_line) {
                    print_to_buf(",", 1);
                }
                print_to_buf("\"-- Reboot --\"\n", 15);
                first_line = false;
            } else {
                print_to_buf("-- Reboot --\n", 13);
            }
        }
    } else {
        memcpy(bootid, d, l);
    }
}

static void print_timestamp(sd_journal *j) {
    uint64_t timestamp = get_timestamp(j);
    if (timestamp == (uint64_t) -1) {
        return;
    }

    static uint64_t last_timestamp;
    static char timestring[16];
    if (timestamp >= (last_timestamp+(1000*1000))) {
        timestamp = timestamp / (1000*1000); // usec to sec
        struct tm time;
        localtime_r((time_t *)&timestamp, &time);
        strftime(timestring, 16, "%b %d %T", &time);
        last_timestamp = timestamp;
    }

    print_to_buf(timestring, 15);
}

static void print_pid(sd_journal *j) {
    const char *d;
    size_t l;
    int r = sd_journal_get_data(j, "_PID", (const void **)&d, &l);
    if (r < 0) {
        // we sometimes have no pid, e.g., kernel messages
        return;
    }

    // remove '_PID='
    d += 5;
    l -= 5;

    print_to_buf("[", 1);
    print_to_buf(d, l);
    print_to_buf("]", 1);
}

static bool print_field(sd_journal *j, const char *field) {
    const char *d;
    size_t l;
    int r = sd_journal_get_data(j, field, (const void **)&d, &l);
    if (r < 0) {
        // some fields do not exists
        return false;
    }

    size_t fieldlen = strlen(field)+1;
    d += fieldlen;
    l -= fieldlen;

    if (json) {
	char tmp[7];
	for (size_t i = 0; i < l;i++) {
	    if (d[i] == '"' || d[i] == '\\' || (d[i] >= 0 && d[i] <= 0x1F)) {
		sprintf(tmp, "\\u%04X", d[i]);
		print_to_buf(tmp, 6);
	    } else {
		print_to_buf(d+i, 1);
	    }
	}
    } else {
	print_to_buf(d, l);
    }
    return true;
}


static void print_line(sd_journal *j) {
    print_reboot(j);

    if (json) {
        if (!first_line) {
            print_to_buf(",", 1);
        }
        print_to_buf("\"", 1);
        first_line = false;
    }

    print_timestamp(j);
    print_to_buf(" ", 1);
    print_field(j, "_HOSTNAME");
    print_to_buf(" ", 1);
    if (!print_field(j, "SYSLOG_IDENTIFIER") &&
            !print_field(j, "_COMM")) {
        print_to_buf("unknown", strlen("unknown") - 1);
    }
    print_pid(j);
    print_to_buf(": ", 2);
    print_field(j, "MESSAGE");

    if (json) {
        print_to_buf("\"", 1);
    }

    print_to_buf("\n", 1);
}

static char *progname;

_Noreturn static void usage(char *error) {
    if (error) {
        fprintf(stderr, "ERROR: %s\n", error);
    }
    fprintf(stderr, "usage: %s [OPTIONS]\n", progname);
    fprintf(stderr,
        "  -b <timestamp>\tbegin at this UNIX epoch based timestamp\n"
        "  -e <timestamp>\tend at this UNIX epoch based timestamp\n"
        "  -d <directory>\tpath to a journal directory\n"
        "  -n <integer>\t\tprint the last number entries logged\n"
        "  -f <cursor>\t\tprint from this cursor\n"
        "  -t <cursor>\t\tprint to this cursor\n"
        "  -j \t\t\tprint as json"
        "  -h \t\t\tthis help\n"
        "\n"
        "Passing no range option will dump all the available journal\n"
        "Giving a range conflicts with -n\n"
        "-b and -f conflict\n"
        "-e and -t conflict\n");
    exit(error ? 1 : 0);
}

static uint64_t arg_to_uint64(const char *argument) {
    errno = 0;
    char * end;
    uint64_t value = strtoull(argument, &end, 10);
    if (errno != 0 || *end != '\0') {
        fprintf(stderr, "%s is not a valid integer number\n", argument);
        exit(1);
    }

    return value;
}


int main(int argc, char *argv[]) {
    uint64_t number = 0;
    const char *directory = NULL;
    const char *startcursor = NULL;
    const char *endcursor = NULL;
    uint64_t begin = 0;
    uint64_t end = 0;
    char c;

    progname = argv[0];

    while ((c = (char)getopt (argc, argv, "b:e:d:n:f:t:jh")) != -1) {
        switch (c) {
            case 'b':
                begin = arg_to_uint64(optarg);
                begin = begin * 1000 * 1000; // µs
                break;
            case 'e':
                end = arg_to_uint64(optarg);
                end = end * 1000 * 1000; // µs
                break;
            case 'd':
                directory = optarg;
                break;
            case 'n':
                number = arg_to_uint64(optarg);
                break;
            case 'f':
                startcursor = optarg;
                break;
            case 't':
                endcursor = optarg;
                break;
            case 'j':
                json = true;
                break;
            case 'h':
                usage(NULL);
            case '?':
            default:
                usage("invalid option or missing argument");
        }
    }

    if (number && (begin || startcursor)) {
        usage("-n conflicts with -b and/or -f");
    }

    if (begin && startcursor) {
        usage("-b and -f conflict");
    }

    if (end && endcursor) {
        usage("-e and -t conflict");
    }

    if (argc > optind) {
        usage("unkown, or to many arguments");
    }

    // setup stdout buffer
    if (setvbuf(stdout, BUF, _IOFBF, BUFSIZE)) {
        fprintf(stderr, "Failed to set buffer for stdout: %s\n", strerror(errno));
        return 1;
    }

    // to prevent calling it everytime we generate a timestamp
    tzset();

    int r;
    sd_journal *j;
    if (directory == NULL) {
        r = sd_journal_open(&j, SD_JOURNAL_LOCAL_ONLY);
    } else {
        r = sd_journal_open_directory(&j, directory, 0);
    }

    if (r < 0) {
        fprintf(stderr, "Failed to open journal: %s\n", strerror(-r));
        return 1;
    }

    if (json) {
        print_to_buf("{\"data\":[", 9);
    }

    // if we want to print the last x entries, seek to cursor or end,
    // then x entries back, print the cursor and finally print the
    // entries until end or cursor
    if (number) {
        if (end) {
            r = sd_journal_seek_realtime_usec(j, end);
        } else if (endcursor != NULL) {
            r = sd_journal_seek_cursor(j, endcursor);
            number++;
        } else {
            r = sd_journal_seek_tail(j);
        }

        if (r < 0) {
            fprintf(stderr, "Failed to seek to end/cursor: %s\n", strerror(-r));
            exit(1);
        }

        // seek back number entries and print cursor
        r = sd_journal_previous_skip(j, number + 1);
        if (r < 0) {
            fprintf(stderr, "Failed to seek back: %s\n", strerror(-r));
            exit(1);
        }
    } else {
        if (begin) {
            r = sd_journal_seek_realtime_usec(j, begin);
        } else if (startcursor) {
            r = sd_journal_seek_cursor(j, startcursor);
        } else {
            r = sd_journal_seek_head(j);
        }

        if (r < 0) {
            fprintf(stderr, "Failed to seek to begin/cursor: %s\n", strerror(-r));
            exit(1);
        }

        // if we have a start cursor, we want to skip the first entry
        if (startcursor) {
            r = sd_journal_next(j);
            if (r < 0) {
                fprintf(stderr, "Failed to seek to begin/cursor: %s\n", strerror(-r));
                exit(1);
            }
            print_first_cursor(j);
        }
    }


    while ((r = sd_journal_next(j)) > 0 && (end == 0 || get_timestamp(j) < end)) {
        print_first_cursor(j);
        if (endcursor != NULL && sd_journal_test_cursor(j, endcursor)) {
            break;
        }
        print_line(j);
    }

    // print optional reboot
    print_reboot(j);

    // print final cursor
    print_cursor(j);
    sd_journal_close(j);

    if (json) {
        print_to_buf("],\"success\":1}", 14);
    }

    // print remaining buffer
    fflush_unlocked(stdout);

    return 0;
}

