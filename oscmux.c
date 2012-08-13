/*
 * Copyright (c) 2012 Hanspeter Portner (agenthp@users.sf.net)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <lo/lo.h>
#include <lo/lo_lowlevel.h>

#include <Eina.h>

const uint32_t MAX_FRAC = 0xffffffff;

typedef struct _Input Input;
typedef struct _Output Output;

struct _Input {
	lo_server_thread serv;
	char *path;
	char *fmt;
	int duplicate;
};

struct _Output {
	lo_address addr;
	double delay;
};

static void
_error (int num, const char *msg, const char *where)
{
	fprintf (stderr, "lo server error #%i '%s' at %s\n", msg, where);
}

static int
_handler (const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *data)
{
	Eina_List *outputs = data;
	Eina_List *l;
	Output *out;

	EINA_LIST_FOREACH (outputs, l, out)
	{
		if (out->delay > 0.0)
		{
			uint32_t dsec = out->delay;
			uint32_t dfrac = (out->delay - dsec) * MAX_FRAC;

			lo_timetag tt;
			lo_timetag_now (&tt);

			//printf ("%x.%x\n", tt.sec, tt.frac);

			tt.sec += dsec;
			if (tt.frac + dfrac < tt.frac)
				tt.sec += 1;
			tt.frac += dfrac;

			lo_bundle bund;
			bund = lo_bundle_new (tt);
			lo_bundle_add_message (bund, path, msg);
			lo_send_bundle (out->addr, bund);
			lo_bundle_free (bund);
		}
		else
			lo_send_message (out->addr, path, msg);
	}

	return 0;
}

int
main (int argc, char **argv)
{
	double delay = 0.0;
	char *path = NULL;
	char *fmt = NULL;
	Eina_List *inputs = NULL;
	Eina_List *outputs = NULL;

	eina_init ();

	int c;
	while ( (c = getopt (argc, argv, "p:f:i:d:o:")) != -1)
		switch (c)
		{
			case 'p':
				path = optarg;
				break;
			case 'f':
				fmt = optarg;
				break;
			case 'i':
			{
				Input *in = calloc (1, sizeof (Input));

				int proto = lo_url_get_protocol_id (optarg);
				if (proto == -1)
					fprintf (stderr, "protocol not supported\n");
				char *port = lo_url_get_port (optarg);

				int _port = atoi (optarg);
				int exists = 0;
				Eina_List *l;
				Input *ptr;
				EINA_LIST_FOREACH (inputs, l, ptr)
					if ( (_port == lo_server_thread_get_port (ptr->serv)) && (!ptr->duplicate) )
					{
						exists = 1;
						break;
					}

				if (exists)
				{
					in->duplicate = 1;
					in->serv = ptr->serv;
				}
				else
					in->serv = lo_server_thread_new_with_proto (port, proto, _error);

				if (path)
				{
					in->path = strdup (path);
					path = NULL;
				}

				if (fmt)
				{
					in->fmt = strdup (fmt);
					fmt = NULL;
				}

				inputs = eina_list_append (inputs, in);
				break;
			}
			case 'd':
				delay = atof (optarg);
				break;
			case 'o':
			{
				Output *out = calloc (1, sizeof (Output));
				out->addr = lo_address_new_from_url (optarg);

				out->delay = delay;
				delay = 0.0;

				outputs = eina_list_append (outputs, out);
				break;
			}
			case '?':
				if ( (optopt == 'i') || (optopt == 'o') || (optopt == 'd') || (optopt == 'p') || (optopt == 'f') )
					fprintf (stderr, "Option `-%c' requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
			default:
				return (1);
		}
		
	if (!inputs || !outputs)
	{
		fprintf (stderr, "usage: %s {[-p PATH] [-f FORMAT] -i PORT} {[-d DELAY] -o [HOST:]PORT}\n\n", argv[0]);
		return (1);
	}

	// set callbacks and start servers
	Eina_List *l;
	Input *in;
	EINA_LIST_FOREACH (inputs, l, in)
	{
		lo_server_thread_add_method (in->serv, in->path, in->fmt, _handler, outputs);
		if (!in->duplicate)
			lo_server_thread_start (in->serv);
	}

	while (1)
		usleep (10);

	EINA_LIST_FREE (inputs, in)
	{
		if (!in->duplicate)
		{
			lo_server_thread_stop (in->serv);
			lo_server_free (in->serv);
		}

		if (in->path)
			free (in->path);

		if (in->fmt)
			free (in->fmt);

		free (in);
	}

	Output *out;
	EINA_LIST_FREE (outputs, out)
	{
		lo_address_free (out->addr);
		free (out);
	}

	eina_shutdown ();
	
	return 0;
}
