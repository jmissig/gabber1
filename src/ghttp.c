/*
 * ghttp.c --  This is a completely mutilated version of a file by the same
 * name that comes with libghttp; the description of the much less tainted
 * orignial follows:
 *
 * Implementation of the public interface to http functions
 * Created: Christopher Blizzard <blizzard@appliedtheory.com>, 21-Aug-1998
 *
 * Copyright (C) 1998 Free Software Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* strings that are used all over the place */

typedef struct http_uri_tag
{
  char             *full;                          /* full URL */
  char             *proto;                         /* protocol */
  char             *host;                          /* copy semantics */
  unsigned short    port;
  char             *resource;                      /* copy semantics */
} http_uri;

typedef enum uri_parse_state_tag
{
  parse_state_read_host = 0,
  parse_state_read_port,
  parse_state_read_resource
} uri_parse_state;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


int
http_uri_parse(char *a_string,
	       http_uri *a_uri)
{
  /* Everyone chant... "we love state machines..." */
  uri_parse_state l_state = parse_state_read_host;
  char *l_start_string = NULL;
  char *l_end_string = NULL;
  char  l_temp_port[6];

  /* init the array */
  memset(l_temp_port, 0, 6);
  /* check the parameters */
  if (a_string == NULL)
    goto ec;
  if (a_uri) {
    a_uri->full = strdup(a_string);
  }
  l_start_string = strchr(a_string, ':');
  /* check to make sure that there was a : in the string */
  if (!l_start_string)
    goto ec;
  if (a_uri) {
    a_uri->proto = (char *)malloc(l_start_string - a_string + 1);
    memcpy(a_uri->proto, a_string, (l_start_string - a_string));
    a_uri->proto[l_start_string - a_string] = '\0';
  }
  /* check to make sure it starts with "http://" */
  if (strncmp(l_start_string, "://", 3) != 0)
    goto ec;
  /* start at the beginning of the string */
  l_start_string = l_end_string = &l_start_string[3];
  while(*l_end_string)
    {
      if (l_state == parse_state_read_host)
	{
	  if (*l_end_string == ':')
	    {
	      l_state = parse_state_read_port;
	      if ((l_end_string - l_start_string) == 0)
		goto ec;
	      /* allocate space */
	      if ((l_end_string - l_start_string) == 0)
		goto ec;
	      /* only do this if a uri was passed in */
	      if (a_uri)
		{
		  a_uri->host = (char *)malloc(l_end_string - l_start_string + 1);
		  /* copy the data */
		  memcpy(a_uri->host, l_start_string, (l_end_string - l_start_string));
		  /* terminate */
		  a_uri->host[l_end_string - l_start_string] = '\0';
		}
	      /* reset the counters */
	      l_end_string++;
	      l_start_string = l_end_string;
	      continue;
	    }
	  else if (*l_end_string == '/')
	    {
	      l_state = parse_state_read_resource;
	      if ((l_end_string - l_start_string) == 0)
		goto ec;
	      if (a_uri)
		{
		  a_uri->host = (char *)malloc(l_end_string - l_start_string + 1);
		  memcpy(a_uri->host, l_start_string, (l_end_string - l_start_string));
		  a_uri->host[l_end_string - l_start_string] = '\0';
		}
	      l_start_string = l_end_string;
	      continue;
	    }
	}
      else if (l_state == parse_state_read_port)
	{
	  if (*l_end_string == '/')
	    {
	      l_state = parse_state_read_resource;
	      /* check to make sure we're not going to overflow */
	      if (l_end_string - l_start_string > 5)
		goto ec;
	      /* check to make sure there was a port */
	      if ((l_end_string - l_start_string) == 0)
		goto ec;
	      /* copy the port into a temp buffer */
	      memcpy(l_temp_port, l_start_string, l_end_string - l_start_string);
	      /* convert it. */
	      if (a_uri)
		a_uri->port = atoi(l_temp_port);
	      l_start_string = l_end_string;
	      continue;
	    }
	  else if (isdigit(*l_end_string) == 0)
	    {
	      /* check to make sure they are just digits */
	      goto ec;
	    }
	}
      /* next.. */
      l_end_string++;
      continue;
    }
  
  if (l_state == parse_state_read_host)
    {
      if ((l_end_string - l_start_string) == 0)
	goto ec;
      if (a_uri)
	{
	  a_uri->host = (char *)malloc(l_end_string - l_start_string + 1);
	  memcpy(a_uri->host, l_start_string, (l_end_string - l_start_string));
	  a_uri->host[l_end_string - l_start_string] = '\0';
	  /* for a "/" */
	  a_uri->resource = strdup("/");
	}
    }
  else if (l_state == parse_state_read_port)
    {
      if (strlen(l_start_string) == 0)
	/* oops.  that's not a valid number */
	goto ec;
      if (a_uri)
	{
	  a_uri->port = atoi(l_start_string);
	  a_uri->resource = strdup("/");
	}
    }
  else if (l_state == parse_state_read_resource)
    {
      if (strlen(l_start_string) == 0)
	{
	  if (a_uri)
	    a_uri->resource = strdup("/");
	}
      else
	{
	  if (a_uri)
	    a_uri->resource = strdup(l_start_string);
	}
    }
  else
    {
      /* uhh...how did we get here? */
      goto ec;
    }
  return 0;
	  
 ec:
  return -1;
} 

/* Validate a uri
 * This will return -1 if a uri is invalid
 */
int
ghttp_uri_validate(char *a_uri)
{
  if (!a_uri)
    return -1;
  /* you can do this... */
  return(http_uri_parse(a_uri, NULL));
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
