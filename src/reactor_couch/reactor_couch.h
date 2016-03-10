#ifndef REACTOR_COUCH_H_INCLUDED
#define REACTOR_COUCH_H_INCLUDED

#ifndef REACTOR_COUCH_RECONNECT_TIME
#define REACTOR_COUCH_RECONNECT_TIME 5000000000
#endif

enum reactor_couch_events
{
  REACTOR_COUCH_ERROR,
  REACTOR_COUCH_UPDATE,
  REACTOR_COUCH_DELETE,
  REACTOR_COUCH_SYNC,
  REACTOR_COUCH_CLOSE
};

enum reactor_couch_state
{
  REACTOR_COUCH_CLOSED,
  REACTOR_COUCH_CONNECTING,
  REACTOR_COUCH_OPEN,
  REACTOR_COUCH_CLOSING
};

enum reactor_couch_flags
{
  REACTOR_COUCH_FLAGS_RECONNECT = 0x01,
  REACTOR_COUCH_FLAGS_SYNC      = 0x02
};

typedef struct reactor_couch reactor_couch;
struct reactor_couch
{
  int                  state;
  int                  flags;
  string               uri;
  reactor_user         user;
  reactor_timer        timer;
  reactor_http_client  client;
  json_t              *database;
};

void  reactor_couch_init(reactor_couch *, reactor_user_call *, void *);
int   reactor_couch_open(reactor_couch *, char *);
void  reactor_couch_connect(reactor_couch *);
void  reactor_couch_error(reactor_couch *);
void  reactor_couch_close(reactor_couch *);
void  reactor_couch_timer_event(void *, int, void *);
void  reactor_couch_client_event(void *, int, void *);

#endif /* REACTOR_COUCH_H_INCLUDED */
