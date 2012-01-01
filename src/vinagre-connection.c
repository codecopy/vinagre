/*
 * vinagre-connection.c
 * This file is part of vinagre
 *
 * Copyright (C) 2007,2008 - Jonh Wendell <wendell@bani.com.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "vinagre-connection.h"
#include "vinagre-bookmarks.h"

VinagreConnection *
vinagre_connection_new ()
{
  VinagreConnection *conn;

  conn = g_new (VinagreConnection, 1);
  conn->host = NULL;
  conn->port = 0;
  conn->name = NULL;
  conn->password = NULL;
  conn->desktop_name = NULL;
  conn->type = VINAGRE_CONNECTION_TYPE_VNC;

  return conn;
}

void 
vinagre_connection_set_host (VinagreConnection *conn,
			     const char *host)
{
  if (conn->host)
    g_free (conn->host);
  conn->host = g_strdup (host);
}

void
vinagre_connection_set_port (VinagreConnection *conn,
			     int port)
{
  conn->port = port;
}

void
vinagre_connection_set_password (VinagreConnection *conn,
				 const char *password)
{
  if (conn->password)
    g_free (conn->password);
  conn->password = g_strdup (password);
}

void 
vinagre_connection_set_name (VinagreConnection *conn,
			     const char *name)
{
  if (conn->name)
    g_free (conn->name);
  conn->name = g_strdup (name);
}

void 
vinagre_connection_set_desktop_name (VinagreConnection *conn,
				     const char *desktop_name)
{
  if (conn->desktop_name)
    g_free (conn->desktop_name);
  conn->desktop_name = g_strdup (desktop_name);
}

void
vinagre_connection_free (VinagreConnection *conn)
{
  if (conn) {

    if (conn->host)
      g_free (conn->host);
    conn->host = NULL;

    if (conn->password)
      g_free (conn->password);
    conn->password = NULL;

    if (conn->name)
      g_free (conn->name);
    conn->name = NULL;

    if (conn->desktop_name)
      g_free (conn->desktop_name);
    conn->desktop_name = NULL;

    g_free (conn);
    conn = NULL;
  }
}

gchar *
vinagre_connection_best_name (VinagreConnection *conn)
{
  g_return_val_if_fail (conn != NULL, NULL);

  if (conn->name)
    return g_strdup (conn->name);

  if (conn->desktop_name)
    return g_strdup (conn->desktop_name);

  if (conn->host)
    return g_strdup_printf ("%s:%d", conn->host, conn->port);

  return NULL;
}

VinagreConnection *
vinagre_connection_clone (VinagreConnection *conn)
{
  VinagreConnection *new_conn;

  new_conn = vinagre_connection_new ();

  vinagre_connection_set_host (new_conn, conn->host);
  vinagre_connection_set_port (new_conn, conn->port);
  vinagre_connection_set_password (new_conn, conn->password);
  vinagre_connection_set_name (new_conn, conn->name);
  vinagre_connection_set_desktop_name (new_conn, conn->desktop_name);

  return new_conn;
}

VinagreConnection *
vinagre_connection_new_from_string (const gchar *uri, gchar **error_msg)
{
  VinagreConnection *conn;
  gchar **server, **url;
  gint    port;
  gchar  *host;

  *error_msg = NULL;

  url = g_strsplit (uri, "://", 2);
  if (g_strv_length (url) == 2)
    {
      if (g_strcmp0 (url[0], "vnc"))
	{
	  *error_msg = g_strdup_printf (_("The protocol %s is not supported."),
					url[0]);
	  g_strfreev (url);
	  return NULL;
	}
      host = url[1];
    }
  else
    host = (gchar *) uri;

  server = g_strsplit (host, ":", 2);
  host = server[0];
  port = server[1] ? atoi (server[1]) : 5900;

  conn = vinagre_bookmarks_exists (host, port);
  if (!conn)
    {
      conn = vinagre_connection_new ();
      vinagre_connection_set_host (conn, host);
      vinagre_connection_set_port (conn, port);
    }

  g_strfreev (server);
  g_strfreev (url);

  return conn;
}

VinagreConnection *
vinagre_connection_new_from_file (const gchar *uri, gchar **error_msg)
{
  GKeyFile          *file;
  GError            *error = NULL;
  gboolean           loaded;
  VinagreConnection *conn = NULL;
  gchar             *host = NULL;
  gint               port;
  int                file_size;
  char              *data = NULL;
  GFile             *file_a;

  *error_msg = NULL;

  file_a = g_file_new_for_commandline_arg (uri);
  loaded = g_file_load_contents (file_a,
				 NULL,
				 &data,
				 &file_size,
				 NULL,
				 &error);
  if (!loaded)
    {
      if (error)
	{
	  *error_msg = g_strdup (error->message);
	  g_error_free (error);
	}

      if (data)
	g_free (data);

      g_object_unref (file_a);
      return NULL;
    }

  file = g_key_file_new ();
  loaded = g_key_file_load_from_data (file,
				      data,
				      file_size,
				      G_KEY_FILE_NONE,
				      &error);
  if (loaded)
    {
      host = g_key_file_get_string (file, "connection", "host", NULL);
      port = g_key_file_get_integer (file, "connection", "port", NULL);
      if (host)
	{
	  conn = vinagre_bookmarks_exists (host, port);
	  if (!conn)
	    {
	      conn = vinagre_connection_new ();
	      vinagre_connection_set_host (conn, host);
	      vinagre_connection_set_port (conn, port);
	    }
	  g_free (host);
	}
    }
  else
    {
      if (error)
	{
	  *error_msg = g_strdup (error->message);
	  g_error_free (error);
	}
    }

  if (data)
    g_free (data);

  g_key_file_free (file);
  g_object_unref (file_a);

  return conn;
}

GdkPixbuf *
vinagre_connection_get_icon (VinagreConnection *conn)
{
  GdkPixbuf         *pixbuf;
  GtkIconTheme      *icon_theme;

  icon_theme = gtk_icon_theme_get_default ();
  pixbuf = gtk_icon_theme_load_icon (icon_theme,
				     "application-x-vnc",
				     16,
				     0,
				     NULL);

  return pixbuf;
}
/* vim: ts=8 */