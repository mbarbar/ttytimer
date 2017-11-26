/*
 *      TTY-CLOCK Main file.
 *      Copyright © 2008 Martin Duquesnoy <xorg62@gmail.com>
 *      All rights reserved.
 *
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the  nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>

#include "ttytimer.h"

/* Maximum number of digits in a time string, hh:mm:ss. */
#define N_TIME_DIGITS 6

static bool time_is_zero(void) {
        return ttyclock->date.hour[0] == 0
               && ttyclock->date.hour[1] == 0
               && ttyclock->date.minute[0] == 0
               && ttyclock->date.minute[1] == 0
               && ttyclock->date.second[0] == 0
               && ttyclock->date.second[1] == 0;
}

/* Prints usage message and exits with exit code exit_code. */
static void usage(char *argv0, int exit_code) {
        printf("usage : %s [-xbvih] [-C [0-7]] \n"
               "        -x            Show box\n"
               "        -C [0-7]      Set the clock color\n"
               "        -b            Use bold colors\n"
               "        -v            Show tty-clock version\n"
               "        -h            Show this page\n", argv0);
        exit(exit_code);
}

void init(void) {
        struct sigaction sig;
        ttyclock->bg = COLOR_BLACK;

        initscr();

        cbreak();
        noecho();
        keypad(stdscr, True);
        start_color();
        curs_set(False);
        clear();

        /* Init default terminal color */
        if (use_default_colors() == OK) ttyclock->bg = -1;

        /* Init color pair */
        init_pair(0, ttyclock->bg, ttyclock->bg);
        init_pair(1, ttyclock->bg, ttyclock->option.color);
        init_pair(2, ttyclock->option.color, ttyclock->bg);
        refresh();

        /* Init signal handler */
        sig.sa_handler = signal_handler;
        sig.sa_flags   = 0;
        sigaction(SIGWINCH, &sig, NULL);
        sigaction(SIGTERM,  &sig, NULL);
        sigaction(SIGINT,   &sig, NULL);
        sigaction(SIGSEGV,  &sig, NULL);

        /* Init global struct */
        ttyclock->running = True;
        if(!ttyclock->geo.x) ttyclock->geo.x = 0;
        if(!ttyclock->geo.y) ttyclock->geo.y = 0;
        if(!ttyclock->geo.a) ttyclock->geo.a = 1;
        if(!ttyclock->geo.b) ttyclock->geo.b = 1;
        ttyclock->geo.w = SECFRAMEW;
        ttyclock->geo.h = 7;
        ttyclock->tm = localtime(&(ttyclock->lt));
        ttyclock->lt = time(NULL);

        /* Create clock win */
        ttyclock->framewin = newwin(ttyclock->geo.h,
                                    ttyclock->geo.w,
                                    ttyclock->geo.x,
                                    ttyclock->geo.y);
        if (ttyclock->option.box) box(ttyclock->framewin, 0, 0);

        if (ttyclock->option.bold) wattron(ttyclock->framewin, A_BLINK);

        /* Create the date win */
        ttyclock->datewin = newwin(DATEWINH, strlen(ttyclock->date.timestr) + 2,
                                   ttyclock->geo.x + ttyclock->geo.h - 1,
                                   ttyclock->geo.y + (ttyclock->geo.w / 2) -
                                   (strlen(ttyclock->date.timestr) / 2) - 1);

        if (ttyclock->option.box) box(ttyclock->datewin, 0, 0);

        clearok(ttyclock->datewin, True);

        set_center();

        nodelay(stdscr, True);

        wrefresh(ttyclock->datewin);

        wrefresh(ttyclock->framewin);
}

void signal_handler(int signal) {
        switch(signal) {
        case SIGWINCH:
                endwin();
                init();
                break;
                /* Interruption signal */
        case SIGINT:
        case SIGTERM:
                ttyclock->running = False;
                /* Segmentation fault signal */
                break;
        case SIGSEGV:
                endwin();
                fprintf(stderr, "Segmentation fault.\n");
                exit(EXIT_FAILURE);
                break;
        }
}

void cleanup(void) {
        if (ttyclock->ttyscr) delscreen(ttyclock->ttyscr);
        if (ttyclock) free(ttyclock);
}

/* Decrements ttyclock's time by 1 second. */
void update_hour(void) {
        unsigned int seconds = ttyclock->date.second[0] * 10
                               + ttyclock->date.second[1];
        unsigned int minutes = ttyclock->date.minute[0] * 10
                               + ttyclock->date.minute[1];
        unsigned int hours = ttyclock->date.hour[0] * 10
                             + ttyclock->date.hour[1];

        if (minutes == 0 && seconds == 0) hours = hours == 0 ? 59 : hours - 1;
        if (seconds == 0) minutes = minutes == 0 ? 59 : minutes - 1;
        seconds = seconds == 0 ? 59 : seconds - 1;

        /* Put it all back into ttyclock. */
        ttyclock->date.hour[0] = hours / 10;
        ttyclock->date.hour[1] = hours % 10;

        ttyclock->date.minute[0] = minutes / 10;
        ttyclock->date.minute[1] = minutes % 10;

        ttyclock->date.second[0] = seconds / 10;
        ttyclock->date.second[1] = seconds % 10;
}

void draw_number(int n, int x, int y, unsigned int color) {
        int i, sy = y;

        for(i = 0; i < 30; ++i, ++sy) {
                if(sy == y + 6) {
                        sy = y;
                        ++x;
                }

                if (ttyclock->option.bold) wattron(ttyclock->framewin, A_BLINK);
                else wattroff(ttyclock->framewin, A_BLINK);

                wbkgdset(ttyclock->framewin,
                         COLOR_PAIR(number[n][i/2] * color));
                mvwaddch(ttyclock->framewin, x, sy, ' ');
        }

        wrefresh(ttyclock->framewin);
}

void draw_clock(void) {
        chtype dotcolor = COLOR_PAIR(1);
        unsigned int numcolor = 1;

        /* Change the colours to blink at certain times. */
        if (time(NULL) % 2 == 0) {
                dotcolor = COLOR_PAIR(2);
                if (time_is_zero()) numcolor = 2;
        }

        /* Draw hour numbers */
        draw_number(ttyclock->date.hour[0], 1, 1, numcolor);
        draw_number(ttyclock->date.hour[1], 1, 8, numcolor);

        /* 2 dot for number separation */
        wbkgdset(ttyclock->framewin, dotcolor);
        mvwaddstr(ttyclock->framewin, 2, 16, "  ");
        mvwaddstr(ttyclock->framewin, 4, 16, "  ");

        /* Draw minute numbers */
        draw_number(ttyclock->date.minute[0], 1, 20, numcolor);
        draw_number(ttyclock->date.minute[1], 1, 27, numcolor);

        /* Draw the date */
        if (ttyclock->option.bold) wattron(ttyclock->datewin, A_BOLD);
        else wattroff(ttyclock->datewin, A_BOLD);

        wbkgdset(ttyclock->datewin, (COLOR_PAIR(2)));
        mvwprintw(ttyclock->datewin, (DATEWINH / 2), 1, ttyclock->date.timestr);
        wrefresh(ttyclock->datewin);

        /* Draw second frame. */
        /* Again 2 dot for number separation */
        wbkgdset(ttyclock->framewin, dotcolor);
        mvwaddstr(ttyclock->framewin, 2, NORMFRAMEW, "  ");
        mvwaddstr(ttyclock->framewin, 4, NORMFRAMEW, "  ");

        /* Draw second numbers */
        draw_number(ttyclock->date.second[0], 1, 39, numcolor);
        draw_number(ttyclock->date.second[1], 1, 46, numcolor);
}

void clock_move(int x, int y, int w, int h) {
        /* Erase border for a clean move */
        wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
        wborder(ttyclock->framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        werase(ttyclock->framewin);
        wrefresh(ttyclock->framewin);

        wbkgdset(ttyclock->datewin, COLOR_PAIR(0));
        wborder(ttyclock->datewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        werase(ttyclock->datewin);
        wrefresh(ttyclock->datewin);

        /* Frame win move */
        mvwin(ttyclock->framewin, (ttyclock->geo.x = x), (ttyclock->geo.y = y));
        wresize(ttyclock->framewin, (ttyclock->geo.h = h),
                (ttyclock->geo.w = w));

        /* Date win move */
        mvwin(ttyclock->datewin,
              ttyclock->geo.x + ttyclock->geo.h - 1,
              ttyclock->geo.y + (ttyclock->geo.w / 2)
              - (strlen(ttyclock->date.timestr) / 2) - 1);
        wresize(ttyclock->datewin, DATEWINH,
                strlen(ttyclock->date.timestr) + 2);

        if (ttyclock->option.box) box(ttyclock->datewin,  0, 0);

        if (ttyclock->option.box) box(ttyclock->framewin, 0, 0);

        wrefresh(ttyclock->framewin);
        wrefresh(ttyclock->datewin); 
}

void set_second(void) {
        int new_w = SECFRAMEW;
        int y_adj;

        for(y_adj = 0; (ttyclock->geo.y - y_adj) > (COLS - new_w - 1); ++y_adj);

        clock_move(ttyclock->geo.x, (ttyclock->geo.y - y_adj), new_w,
                   ttyclock->geo.h);

        set_center();
}

void set_center(void) {
        clock_move((LINES / 2 - (ttyclock->geo.h / 2)),
                   (COLS  / 2 - (ttyclock->geo.w / 2)),
                   ttyclock->geo.w,
                   ttyclock->geo.h);
}

void set_box(Bool b) {
        ttyclock->option.box = b;

        wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
        wbkgdset(ttyclock->datewin, COLOR_PAIR(0));

        if(ttyclock->option.box) {
                wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
                wbkgdset(ttyclock->datewin, COLOR_PAIR(0));
                box(ttyclock->framewin, 0, 0);
                box(ttyclock->datewin,  0, 0);
        } else {
                wborder(ttyclock->framewin, ' ', ' ', ' ', ' ', ' ', ' ',
                        ' ', ' ');
                wborder(ttyclock->datewin, ' ', ' ', ' ', ' ', ' ', ' ',
                        ' ', ' ');
        }

        wrefresh(ttyclock->datewin);
        wrefresh(ttyclock->framewin);
}

void key_event(void) {
        int i, c;

        // s delay and ns delay.
        struct timespec length = { 1, 0 };
        switch(c = wgetch(stdscr)) {
        case 'q':
        case 'Q':
                ttyclock->running = False;
                break;

        case 'b':
        case 'B':
                ttyclock->option.bold = !ttyclock->option.bold;
                break;

        case 'x':
        case 'X':
                set_box(!ttyclock->option.box);
                break;

        default:
                nanosleep(&length, NULL);
                for (i = 0; i < 8; ++i) {
                        if (c == (i + '0')) {
                                ttyclock->option.color = i;
                                init_pair(1, ttyclock->bg, i);
                                init_pair(2, i, ttyclock->bg);
                        }
                }

                break;
        }
}

/* Fills two elements from digits into time, handling the -1 case. */
static void fill_ttyclock_time(int *digits, unsigned int *time) {
        if (digits[1] == -1) {
                time[0] = 0;
                if (digits[0] == -1) time[1] = 0;
                else time[1] = digits[0];
        } else {
                time[0] = digits[0];
                time[1] = digits[1];
        }
}

/* Parses time into ttyclock->date.hour/minute/second. Exits with
 * an error message on bad time format. Sets timestr to what was
 * parsed.
 * time format: hh:mm:ss, where all but the colons are optional.
 */
static void parse_time_arg(char *time) {
        int digits[N_TIME_DIGITS];
        for (int i = 0; i < N_TIME_DIGITS; ++i) digits[i] = -1;

        int i = 0, remaining = 2;
        while (*time != '\0') {
                if (isdigit(*time)) {
                        if (remaining == 0) {
                                puts("Too many digits in time argument");
                                exit(EXIT_FAILURE);
                        }

                        digits[i] = *time - '0';
                        ++i;
                        --remaining;
                } else if (*time == ':') {
                        i += remaining;
                        remaining = 2;
                } else {
                        puts("Invalid character in time argument");
                        exit(EXIT_FAILURE);
                }

                ++time;
        }

        fill_ttyclock_time(digits, ttyclock->date.hour);
        fill_ttyclock_time(digits + 2, ttyclock->date.minute);
        fill_ttyclock_time(digits + 4, ttyclock->date.second);

        ttyclock->date.timestr[0] = ttyclock->date.hour[0] + '0';
        ttyclock->date.timestr[1] = ttyclock->date.hour[1] + '0';
        ttyclock->date.timestr[2] = ':';
        ttyclock->date.timestr[3] = ttyclock->date.minute[0] + '0';
        ttyclock->date.timestr[4] = ttyclock->date.minute[1] + '0';
        ttyclock->date.timestr[5] = ':';
        ttyclock->date.timestr[6] = ttyclock->date.second[0] + '0';
        ttyclock->date.timestr[7] = ttyclock->date.second[1] + '0';
        ttyclock->date.timestr[8] = '\0';
}

int main(int argc, char **argv) {
        int c;

        /* Alloc ttyclock */
        ttyclock = malloc(sizeof(ttyclock_t));
        assert(ttyclock != NULL);
        memset(ttyclock, 0, sizeof(ttyclock_t));

        /* Default color */
        ttyclock->option.color = COLOR_GREEN; /* COLOR_GREEN = 2 */

        atexit(cleanup);

        while ((c = getopt(argc, argv, "vbhxC:")) != -1) {
                switch(c) {
                case 'h':
                        usage(argv[0], EXIT_SUCCESS);
                        break;
                case 'v':
                        puts("ttytimer v0.1");
                        exit(EXIT_SUCCESS);
                        break;
                case 'b':
                        ttyclock->option.bold = True;
                        break;
                case 'C':
                        if(atoi(optarg) >= 0 && atoi(optarg) < 8)
                                ttyclock->option.color = atoi(optarg);
                        break;
                case 'x':
                        ttyclock->option.box = True;
                        break;
                default:
                        usage(argv[0], EXIT_FAILURE);
                        break;
                }
        }

        /* We're missing the final time argument. */
        if (optind == argc) usage(argv[0], EXIT_FAILURE);

        parse_time_arg(argv[optind]);
        /* Ensure input is anything but 0. */
        if (time_is_zero()) {
                puts("Time argument is zero");
                exit(EXIT_FAILURE);
        }

        init();
        attron(A_BLINK);
        while (ttyclock->running) {
                draw_clock();
                key_event();
                if (!time_is_zero()) update_hour();
        }

        endwin();

        return 0;
}
