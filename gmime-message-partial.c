/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2002 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "gmime-message-partial.h"
#include "gmime-stream-cat.h"
#include "gmime-stream-mem.h"
#include "gmime-parser.h"


/* GObject class methods */
static void g_mime_message_partial_class_init (GMimeMessagePartialClass *klass);
static void g_mime_message_partial_init (GMimeMessagePartial *catpart, GMimeMessagePartialClass *klass);
static void g_mime_message_partial_finalize (GObject *object);

/* GMimeObject class methods */
static void message_partial_init (GMimeObject *object);
static void message_partial_add_header (GMimeObject *object, const char *header, const char *value);
static void message_partial_set_header (GMimeObject *object, const char *header, const char *value);


static GMimePartClass *parent_class = NULL;


GType
g_mime_message_partial_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GMimeMessagePartialClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) g_mime_message_partial_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (GMimeMessagePartial),
			0,    /* n_preallocs */
			(GInstanceInitFunc) g_mime_message_partial_init,
		};
		
		type = g_type_register_static (GMIME_TYPE_PART, "GMimeMessagePartial", &info, 0);
	}
	
	return type;
}


static void
g_mime_message_partial_class_init (GMimeMessagePartialClass *klass)
{
	GMimeObjectClass *object_class = GMIME_OBJECT_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_ref (GMIME_TYPE_PART);
	
	gobject_class->finalize = g_mime_message_partial_finalize;
	
	object_class->init = message_partial_init;
	object_class->add_header = message_partial_add_header;
	object_class->set_header = message_partial_set_header;
}

static void
g_mime_message_partial_init (GMimeMessagePartial *message_partial, GMimeMessagePartialClass *klass)
{
	;
}

static void
g_mime_message_partial_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
message_partial_init (GMimeObject *object)
{
	/* no-op */
	GMIME_OBJECT_CLASS (parent_class)->init (object);
}

static void
message_partial_add_header (GMimeObject *object, const char *header, const char *value)
{
	/* RFC 1864 states that you cannot set a Content-MD5 on a message part */
	if (!strcasecmp ("Content-MD5", header))
		return;
	
	/* Make sure that the header is a Content-* header, else it
           doesn't belong on a mime part */
	
	if (!strncasecmp ("Content-", header, 8))
		GMIME_OBJECT_CLASS (parent_class)->add_header (object, header, value);
}

static void
message_partial_set_header (GMimeObject *object, const char *header, const char *value)
{
	/* RFC 1864 states that you cannot set a Content-MD5 on a message part */
	if (!strcasecmp ("Content-MD5", header))
		return;
	
	/* Make sure that the header is a Content-* header, else it
           doesn't belong on a mime part */
	
	if (!strncasecmp ("Content-", header, 8))
		GMIME_OBJECT_CLASS (parent_class)->set_header (object, header, value);
}


/**
 * g_mime_message_partial_new:
 * @id: message/partial part id
 * @number: message/partial part number
 * @total: total message/partial parts that combine to form a single message/rfc822 part.
 *
 * Creates a new MIME message/partial object.
 *
 * Returns an empty MIME message/partial object.
 **/
GMimeMessagePartial *
g_mime_message_partial_new (const char *id, int number, int total)
{
	GMimeMessagePartial *partial;
	GMimeContentType *type;
	char *num;
	
	partial = g_object_new (GMIME_TYPE_MESSAGE_PARTIAL, NULL, NULL);
	
	type = g_mime_content_type_new ("message", "partial");
	g_mime_content_type_set_parameter (type, "id", id);
	num = g_strdup_printf ("%d", number);
	g_mime_content_type_set_parameter (type, "number", num);
	g_free (num);
	num = g_strdup_printf ("%d", total);
	g_mime_content_type_set_parameter (type, "total", num);
	g_free (num);
	
	g_mime_object_set_content_type (GMIME_OBJECT (partial), type);
	
	return partial;
}


const char *
g_mime_message_partial_get_id (GMimeMessagePartial *partial)
{
	g_return_val_if_fail (GMIME_IS_MESSAGE_PARTIAL (partial), NULL);
	
	return g_mime_object_get_content_type_parameter (GMIME_OBJECT (partial), "id");
}


int
g_mime_message_partial_get_number (GMimeMessagePartial *partial)
{
	const char *number;
	
	g_return_val_if_fail (GMIME_IS_MESSAGE_PARTIAL (partial), -1);
	
	number = g_mime_object_get_content_type_parameter (GMIME_OBJECT (partial), "number");
	if (!number)
		return -1;
	
	return atoi (number);
}


int
g_mime_message_partial_get_total (GMimeMessagePartial *partial)
{
	const char *total;
	
	g_return_val_if_fail (GMIME_IS_MESSAGE_PARTIAL (partial), -1);
	
	total = g_mime_object_get_content_type_parameter (GMIME_OBJECT (partial), "total");
	if (!total)
		return -1;
	
	return atoi (total);
}


static int
partial_compare (const void *partial1, const void *partial2)
{
	int num1, num2;
	
	num1 = g_mime_message_partial_get_number (GMIME_MESSAGE_PARTIAL (partial1));
	num2 = g_mime_message_partial_get_number (GMIME_MESSAGE_PARTIAL (partial2));
	
	return num1 - num2;
}


/**
 * g_mime_message_partial_reconstruct_message:
 * @partials: an array of message/partial mime parts
 * @num: the number of elements in @partials
 *
 * Reconstructs the GMimeMessage from the given message/partial parts
 * in @partials.
 *
 * Returns a GMimeMessage object on success or %NULL on error.
 **/
GMimeMessage *
g_mime_message_partial_reconstruct_message (GMimeMessagePartial **partials, size_t num)
{
	GMimeMessagePartial *partial;
	GMimeDataWrapper *wrapper;
	GMimeStream *cat, *stream;
	GMimeMessage *message;
	GMimeParser *parser;
	int total, number;
	const char *id;
	size_t i;
	
	g_return_val_if_fail (num > 0, NULL);
	
	id = g_mime_message_partial_get_id (partials[0]);
	if (!id)
		return NULL;
	
	/* get them into the correct order... */
	qsort ((void *) partials, num, sizeof (GMimeMessagePartial *),
	       partial_compare);
	
	/* only the last message/partial part is REQUIRED to have the total parameter */
	total = g_mime_message_partial_get_total (partials[num - 1]);
	if (num != total)
		return NULL;
	
	cat = g_mime_stream_cat_new ();
	
	for (i = 0; i < num; i++) {
		const char *partial_id;
		
		partial = partials[i];
		
		/* sanity check to make sure this part belongs... */
		partial_id = g_mime_message_partial_get_id (partial);
		if (!partial_id || strcmp (id, partial_id))
			goto exception;
		
		/* sanity check to make sure we aren't missing any parts */
		number = g_mime_message_partial_get_number (partial);
		if (number != i + 1)
			goto exception;
		
		wrapper = (GMimeDataWrapper *) g_mime_part_get_content_object (GMIME_PART (partial));
		stream = g_mime_data_wrapper_get_stream (wrapper);
		g_mime_stream_reset (stream);
		g_mime_stream_cat_add_source (GMIME_STREAM_CAT (cat), stream);
		g_mime_stream_unref (stream);
	}
	
	parser = g_mime_parser_new ();
	g_mime_parser_init_with_stream (parser, cat);
	g_mime_stream_unref (cat);
	
	message = g_mime_parser_construct_message (parser);
	g_object_unref (parser);
	
	return message;
	
 exception:
	
	g_mime_stream_unref (cat);
	
	return NULL;
}


static void
header_copy (const char *name, const char *value, gpointer user_data)
{
	GMimeMessage *message = (GMimeMessage *) user_data;
	
	g_mime_object_add_header (GMIME_OBJECT (message), name, value);
}

static GMimeMessage *
message_partial_message_new (GMimeMessage *base)
{
	GMimeMessage *message;
	
	message = g_mime_message_new (FALSE);
	g_mime_header_foreach (GMIME_OBJECT (base)->headers, header_copy, message);
	
	return message;
}

GMimeMessage **
g_mime_message_partial_split_message (GMimeMessage *message, size_t max_size, size_t *nparts)
{
	GMimeMessage **messages;
	GMimeMessagePartial *partial;
	GMimeStream *stream, *substream;
	GMimeDataWrapper *wrapper;
	GPtrArray *parts;
	char *message_id;
	const char *id;
	off_t start;
	size_t len;
	int i;
	
	*nparts = 0;
	
	g_return_val_if_fail (GMIME_IS_MESSAGE (message), NULL);
	
	stream = g_mime_stream_mem_new ();
	
	if (g_mime_object_write_to_stream (GMIME_OBJECT (message), stream) == -1) {
		g_mime_stream_unref (stream);
		return NULL;
	}
	
	g_mime_stream_reset (stream);
	
	len = g_mime_stream_length (stream);
	
	/* optimization */
	if (len <= max_size) {
		g_mime_stream_unref (stream);
		*nparts = 1;
		g_mime_object_ref (GMIME_OBJECT (message));
		return &message;
	}
	
	parts = g_ptr_array_new ();
	for (i = 0, start = 0; start < len; i++) {
		substream = g_mime_stream_substream (stream, start, MIN (len, start + max_size));
		g_ptr_array_add (parts, substream);
		start += max_size;
	}
	
	id = g_mime_message_get_message_id (message);
	
	for (i = 0; i < parts->len; i++) {
		partial = g_mime_message_partial_new (id, i + 1, parts->len);
		wrapper = g_mime_data_wrapper_new_with_stream (GMIME_STREAM (parts->pdata[i]),
							       GMIME_PART_ENCODING_DEFAULT);
		g_mime_stream_unref (GMIME_STREAM (parts->pdata[i]));
		g_mime_part_set_content_object (GMIME_PART (partial), wrapper);
		
		parts->pdata[i] = message_partial_message_new (message);
		g_mime_message_set_mime_part (GMIME_MESSAGE (parts->pdata[i]), GMIME_OBJECT (partial));
		g_mime_object_unref (GMIME_OBJECT (partial));
	}
	
	g_mime_stream_unref (stream);
	
	messages = (GMimeMessage **) parts->pdata;
	*nparts = parts->len;
	
	g_ptr_array_free (parts, FALSE);
	
	return messages;
}