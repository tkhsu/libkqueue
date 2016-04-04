/*
 * Copyright (c) 2009 Mark Heily <mark@heily.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <sys/select.h>

#include "sys/event.h"
#include "private.h"

const struct filter evfilt_proc = EVFILT_NOTIMPL;


int
posix_kevent_wait(
        struct kqueue *kq, 
        int nevents,
        const struct timespec *timeout)
{
    int n, nfds;
    fd_set rfds;
    struct timespec ts;
    const struct timespec * tsp = timeout;

    int i ;

    nfds = kq->kq_nfds;
    rfds = kq->kq_fds;

    dbg_puts("waiting for events");

    for (i = 0; i < nfds; i++) {
        if (FD_ISSET(i, &rfds)) {
            int ret;
            struct stat info;
            
            ret = fstat(i, &info);
            if (ret < 0) {
                perror ("fstat");
                fprintf(stderr, "fd=%d is NOT a valid file descriptor\n", i);
            } else {
                fprintf(stderr, "fd=%d is a valid file descriptor\n", i);
            }
        }
    }


    if (timeout == NULL) {
       ts.tv_sec  = 2592000;
       ts.tv_nsec = 0;
       timeout = &ts;
    }
 
    n = pselect(nfds, &rfds, NULL , NULL, timeout, NULL);
    if (n < 0) {
        if (errno == EINTR) {
            dbg_puts("signal caught");
            return (-1);
        }
        dbg_perror("pselect(2)");
        return (-1);
    }

    kq->kq_rfds = rfds;

    return (n);
}

int
posix_kevent_copyout(struct kqueue *kq, int nready,
        struct kevent *eventlist, int nevents)
{
    struct filter *filt;
    int i, rv, nret;
    struct knote *kn;

    nret = 0;
    for (i = 0; (i < EVFILT_SYSCOUNT && nready > 0 && nevents > 0); i++) {
//        dbg_printf("eventlist: n = %d nevents = %d", nready, nevents);
        filt = &kq->kq_filt[i]; 
//        dbg_printf("pfd[%d] = %d", i, filt->kf_pfd);
        if (FD_ISSET(filt->kf_pfd, &kq->kq_rfds)) {
            dbg_printf("pending events for filter %d (%s)", filt->kf_id, filter_name(filt->kf_id));

            kn = (struct knote *) NULL; //FIXME we need to retrive a knote from somewhere

            rv = filt->kf_copyout(eventlist, kn, NULL);
            if (rv < 0) {
                dbg_puts("kevent_copyout failed");
                nret = -1;
                break;
            }
            nret += rv;
            eventlist += rv;
            nevents -= rv;
            nready--;
        }
    }

    return (nret);
}