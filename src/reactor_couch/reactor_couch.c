#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <regex.h>
#include <syslog.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <jansson.h>

#include <dynamic.h>
#include <clo.h>
#include <reactor_core.h>
#include <reactor_net.h>
#include <reactor_http.h>

#include "reactor_couch.h"

void reactor_couch_init(reactor_couch *couch, reactor_user_call *call, void *state)
{
  *couch = (reactor_couch) {.state = REACTOR_COUCH_CLOSED};
  reactor_user_init(&couch->user, call, state);
  reactor_timer_init(&couch->timer, reactor_couch_timer_event, couch);
}

int reactor_couch_open(reactor_couch *couch, char *uri)
{
  if (couch->state != REACTOR_COUCH_CLOSED)
    return -1;

  string_init(&couch->uri, uri);
  string_append(&couch->uri, "/_changes?include_docs=true&feed=continuous&since=0&heartbeat=1000");

  couch->database = json_object();
  if (!couch->database)
    return -1;

  reactor_couch_connect(couch);

  return 0;
}

void reactor_couch_connect(reactor_couch *couch)
{
  int e;

  couch->flags = REACTOR_COUCH_FLAGS_RECONNECT;
  couch->state = REACTOR_COUCH_CONNECTING;
  json_object_clear(couch->database);
  e = reactor_timer_open(&couch->timer, 1000000000, 0);
  if (e == -1)
    reactor_couch_error(couch);
}

void reactor_couch_error(reactor_couch *couch)
{
  reactor_timer_close(&couch->timer);
  reactor_http_client_close(&couch->client);
  reactor_user_dispatch(&couch->user, REACTOR_COUCH_ERROR, NULL);
}

void reactor_couch_close(reactor_couch *couch)
{
  if (couch->state == REACTOR_COUCH_CLOSED)
    return;

  if (couch->state != REACTOR_COUCH_CLOSING)
    {
      couch->flags = 0;
      couch->state = REACTOR_COUCH_CLOSING;
      reactor_timer_close(&couch->timer);
      reactor_http_client_close(&couch->client);
    }

  if (couch->state != REACTOR_COUCH_CLOSED &&
      couch->timer.state == REACTOR_TIMER_CLOSED &&
      couch->client.state == REACTOR_HTTP_CLIENT_CLOSED)
    {
      couch->state = REACTOR_COUCH_CLOSED;
      json_decref(couch->database);
      buffer_clear(&couch->uri.buffer);
      reactor_user_dispatch(&couch->user, REACTOR_COUCH_CLOSE, NULL);
    }
}

void reactor_couch_timer_event(void *state, int type, void *data)
{
  reactor_couch *couch;
  int e;

  (void) data;
  if (type != REACTOR_TIMER_TIMEOUT)
    return;

  couch = state;
  reactor_timer_close(&couch->timer);
  switch (couch->state)
    {
    case REACTOR_COUCH_CONNECTING:
      reactor_http_client_init(&couch->client, reactor_couch_client_event, couch);
      e = reactor_http_client_open(&couch->client, "GET", string_data(&couch->uri), NULL, 0, REACTOR_HTTP_PARSER_FLAGS_STREAM);
      if (e == -1)
        reactor_couch_error(couch);
      break;
    }
}

void reactor_couch_heartbeat(reactor_couch *couch)
{
  if (~couch->flags & REACTOR_COUCH_FLAGS_SYNC)
    {
      couch->flags |= REACTOR_COUCH_FLAGS_SYNC;
      reactor_user_dispatch(&couch->user, REACTOR_COUCH_SYNC, couch->database);
      json_object_clear(couch->database);
    }
}

void reactor_couch_message(reactor_couch *couch, reactor_stream_data *data)
{
  json_t *message, *doc, *id;
  json_error_t error;

  message = json_loadb(data->base, data->size, JSON_DISABLE_EOF_CHECK, &error);
  if (message && json_is_object(message) && !json_object_get(message, "last_seq"))
    {
      if (json_object_get(message, "deleted"))
        {
          id = json_object_get(message, "id");
          if (id && json_is_string(id))
            {
              if (couch->flags & REACTOR_COUCH_FLAGS_SYNC)
                reactor_user_dispatch(&couch->user, REACTOR_COUCH_DELETE, (void *) json_string_value(id));
              else
                json_object_del(couch->database, "id");
            }
        }
      else
        {
          id = json_object_get(message, "id");
          doc = json_object_get(message, "doc");
          if (id && doc && json_is_string(id) && json_is_object(doc))
            {
              if (couch->flags & REACTOR_COUCH_FLAGS_SYNC)
                reactor_user_dispatch(&couch->user, REACTOR_COUCH_UPDATE, doc);
              else
                json_object_set(couch->database, json_string_value(id), doc);
            }
        }
    }
  json_decref(message);
}

void reactor_couch_client_event(void *state, int type, void *data)
{
  reactor_couch *couch;
  reactor_http_response *response;
  reactor_stream_data *chunk;

  couch = state;
  switch (type)
    {
    case REACTOR_HTTP_CLIENT_ERROR:
      reactor_couch_error(couch);
      break;
    case REACTOR_HTTP_CLIENT_HEADER:
      response = data;
      if (response->status != 200)
        reactor_couch_error(couch);
      break;
    case REACTOR_HTTP_CLIENT_CHUNK:
      chunk = data;
      if (chunk->size == 1)
        reactor_couch_heartbeat(couch);
      else
        reactor_couch_message(couch, data);
      break;
    case REACTOR_HTTP_CLIENT_CLOSE:
      if (couch->flags & REACTOR_COUCH_FLAGS_RECONNECT)
        reactor_couch_connect(couch);
      else
        reactor_couch_close(couch);
      break;
    default:
      break;
    }
}
