#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <arpa/inet.h>

#include "reactor.h"
#include "reactor_fd.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"
#include "reactor_timer.h"
#include "reactor_socket.h"
#include "reactor_resolver.h"

struct app
{
  reactor          *reactor;
  reactor_timer    *timer;
  reactor_socket   *socket;
  reactor_signal   *signal;
};

void resolver_handler(reactor_event *e)
{
  reactor_resolver *s = e->data;
  int i;
  struct addrinfo *ai;
  struct sockaddr_in *sin;
  struct gaicb *ar;
  char name[256];
  int found; //error, found;

  (void) fprintf(stderr, "[resolver reply]\n");
  for (i = 0; i < s->nitems; i ++)
    {
      ar = s->list[i];
      (void) fprintf(stderr, "%s\n", ar->ar_name);
      found = 0;
      for (ai = ar->ar_result; ai; ai = ai->ai_next)
	{
	  if (ai->ai_family == AF_INET && ai->ai_socktype == SOCK_STREAM)
	    {
	      sin = (struct sockaddr_in *) ai->ai_addr;
	      (void) fprintf(stderr, "-> %s\n", inet_ntop(AF_INET, &sin->sin_addr, name, sizeof name));
	      found = 1;
	    }
	}
      if (!found)
	(void) fprintf(stderr, "-> (failed)\n");
    }

  /*
  error = reactor_resolver_delete(s);
  if (error == -1)
    err(1, "reactor_resolver_delete");
  */
  reactor_halt(e->call->data);
}

int main()
{
  struct app app;
  struct reactor_resolver *s;
  int e;

  app.reactor = reactor_new();
  if (!app.reactor)
    err(1, "reactor_new");

  s = reactor_resolver_new(app.reactor, resolver_handler,
			   (struct gaicb *[]){
			     (struct gaicb[]){{.ar_name = "www.sunet.se"}},
			     (struct gaicb[]){{.ar_name = "www.dontexist.blah"}}
			   }, 2, app.reactor);
  if (!s)
    err(1, "reactor_resolver_new");
  
  e = reactor_run(app.reactor);
  if (e == -1)
    err(1, "reactor_run");

  e = reactor_resolver_delete(s);
  if (e == -1)
    err(1, "reactor_resolver_delete");

  e = reactor_delete(app.reactor);
  if (e == -1)
    err(1, "reactor_delete");

  sleep(1);
}