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

typedef struct _Group Group;
typedef struct _Input Input;
typedef struct _Output Output;

struct _Group {
	Eina_List *inputs;
	Eina_List *outputs;
};

struct _Input {
	lo_server_thread serv;
	char *path;
	char *fmt;
	int duplicate;
	int queue;
};

struct _Output {
	lo_address addr;
	double delay;
	Eina_List *bundles;
};

static void
_error (int num, const char *msg, const char *where)
{
	fprintf (stderr, "lo server error #%i '%s' at %s\n", num, msg, where);
}

static int
_bundle_start_handler (lo_timetag time, void *data)
{
	Eina_List *outputs = data;
	Eina_List *l;
	Output *out;

	EINA_LIST_FOREACH (outputs, l, out)
	{
		lo_timetag tt = time;
		if ( (time.sec == LO_TT_IMMEDIATE.sec) && (time.frac == LO_TT_IMMEDIATE.frac) )
			lo_timetag_now (&tt);

		if (out->delay > 0.0)
		{
			uint32_t dsec = out->delay;
			uint32_t dfrac = (out->delay - dsec) * MAX_FRAC;

			tt.sec += dsec;
			if (tt.frac + dfrac < tt.frac)
				tt.sec += 1;
			tt.frac += dfrac;
		}

		lo_bundle bundle = lo_bundle_new (tt);
		out->bundles = eina_list_prepend (out->bundles, bundle);
	}

	return 1;
}

static int
_bundle_end_handler (void *data)
{
	Eina_List *outputs = data;
	Eina_List *l;
	Output *out;

	EINA_LIST_FOREACH (outputs, l, out)
	{
		Eina_List *ptr = eina_list_nth_list (out->bundles, 0);
		lo_bundle bundle = eina_list_data_get (ptr);
		lo_send_bundle (out->addr, bundle);
		lo_bundle_free_messages (bundle);
		out->bundles = eina_list_remove_list (out->bundles, ptr);
	}

	return 1;
}

static int
_msg_handler (const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *data)
{
	Eina_List *outputs = data;
	Eina_List *l;
	Output *out;

	EINA_LIST_FOREACH (outputs, l, out)
	{
		lo_bundle bundle = eina_list_nth (out->bundles, 0);

		if (bundle)
		{
			lo_message *clone = lo_message_clone (msg);
			lo_bundle_add_message (bundle, path, clone);
		}
		else // !bundle
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

				bundle = lo_bundle_new (tt);
				lo_bundle_add_message (bundle, path, msg);
				lo_send_bundle (out->addr, bundle);
				lo_bundle_free (bundle);
			}
			else // out->delay == 0.0
				lo_send_message (out->addr, path, msg);
		}
	}

	return 1;
}

int
main (int argc, char **argv)
{
	double delay = 0.0;
	char *path = NULL;
	char *fmt = NULL;
	int queue = 0;
	Eina_List *groups = NULL;
	Eina_List *inputs = NULL;
	Eina_List *outputs = NULL;
	Group *grp;
	int state_outputs = 1;

	eina_init ();

	int c;
	while ( (c = getopt (argc, argv, "qQp:f:i:d:o:")) != -1)
		switch (c)
		{
			case 'q':
				queue = 0;
				break;
			case 'Q':
				queue = 1;
				break;
			case 'p':
				path = optarg;
				break;
			case 'f':
				fmt = optarg;
				break;
			case 'i':
			{
				if (state_outputs)
				{
					// create new output group
					grp = calloc (1, sizeof (Group));
					groups = eina_list_append (groups, grp);
					state_outputs = 0;
				}

				Input *in = calloc (1, sizeof (Input));

				int proto = lo_url_get_protocol_id (optarg);
				if (proto == -1)
					fprintf (stderr, "protocol not supported\n");
				char *port = lo_url_get_port (optarg);

				int _port = atoi (port);
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
					in->serv = lo_server_thread_new_with_proto (port, proto, _error); //FIXME disable queing

				free (port);

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

				in->queue = queue;
				queue = 0;

				inputs = eina_list_append (inputs, in);
				grp->inputs = eina_list_append (grp->inputs, in);
				break;
			}
			case 'd':
				delay = atof (optarg);
				break;
			case 'o':
			{
				if (!state_outputs)
					state_outputs = 1;

				Output *out = calloc (1, sizeof (Output));
				out->addr = lo_address_new_from_url (optarg);

				out->delay = delay;
				delay = 0.0;

				out->bundles = NULL;

				outputs = eina_list_append (outputs, out);
				grp->outputs = eina_list_append (grp->outputs, out);
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
		fprintf (stderr, "usage: %s {{[q|Q] [-p PATH] [-f FORMAT] -i PORT} {[-d DELAY] -o [HOST:]PORT}}\n\n", argv[0]);
		return (1);
	}

	// set callbacks and start servers
	Eina_List *L;
	EINA_LIST_FOREACH (groups, L, grp)
	{
		Eina_List *l;
		Input *in;
		EINA_LIST_FOREACH (grp->inputs, l, in)
		{
			lo_server_thread_add_method (in->serv, in->path, in->fmt, _msg_handler, grp->outputs);
			lo_server *serv = lo_server_thread_get_server (in->serv);
			lo_server_add_bundle_handlers (serv, _bundle_start_handler, _bundle_end_handler, grp->outputs);
			lo_server_enable_queue (serv, in->queue, 1);

			if (!in->duplicate)
				lo_server_thread_start (in->serv);
		}
	}

	while (1)
		usleep (10);

	Input *in;
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

	EINA_LIST_FREE (groups, grp)
		free (grp);

	eina_shutdown ();
	
	return 0;
}
