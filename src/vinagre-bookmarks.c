/*
 * vinagre-bookmarks.c
 * This file is part of vinagre
 *
 * Copyright (C) 2007  Jonh Wendell <wendell@bani.com.br>
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

#include "vinagre-bookmarks.h"
#include "vinagre-utils.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glade/glade.h>
#include <string.h>

#define VINAGRE_FAVORITES_FILE  "vinagre.bookmarks"

GKeyFile *bookmarks = NULL;

static gchar *
filename (void)
{
  return g_build_filename (g_get_home_dir (),
			   ".gnome2",
			   VINAGRE_FAVORITES_FILE,
			   NULL);
}

static void
vinagre_bookmarks_save_file (void)
{
  gchar    *file;
  gchar    *data;
  gsize    length;
  GError   *error;

  error = NULL;
  data = g_key_file_to_data (bookmarks,
			     &length,
			     &error);
  if (!data)
    {
      if (error)
	{
	  g_warning (_("Error while saving bookmarks: %s"), error->message);
	  g_error_free (error);
	}

      return;

    }

  file  = filename ();
  error = NULL;

  if (!g_file_set_contents (file,
			    data,
			    length,
			    &error))
    {
      if (error)
	{
	  g_warning (_("Error while saving bookmarks: %s"), error->message);
	  g_error_free (error);
	}
    }

  g_free (file);
  g_free (data);
}

void
vinagre_bookmarks_init (void)
{
  gchar    *file;
  gboolean loaded;
  GError   *error;

  g_return_if_fail (bookmarks == NULL);

  bookmarks = g_key_file_new ();

  loaded = FALSE;
  error  = NULL;

  file = filename ();
  if (g_file_test (file, G_FILE_TEST_EXISTS))
    loaded = g_key_file_load_from_file (bookmarks,
					file,
					G_KEY_FILE_NONE,
					&error);
  g_free (file);

  if (!loaded)
    {
      if (error)
	{
	  g_warning (_("Error while initializing bookmarks: %s"), error->message);
	  g_error_free (error);
	}
    }
}

void
vinagre_bookmarks_finalize (void)
{
  g_return_if_fail (bookmarks != NULL);

  g_key_file_free (bookmarks);
}

gboolean
vinagre_bookmarks_add (VinagreConnection *conn,
		       VinagreWindow     *window)
{
  gint result;
  GladeXML    *xml;
  const gchar *glade_file;
  GtkWidget   *dialog;
  const gchar *name;

  g_return_val_if_fail (conn != NULL, FALSE);

  glade_file = vinagre_utils_get_glade_filename ();
  xml = glade_xml_new (glade_file, "add_to_bookmarks_dialog", NULL);
  dialog = glade_xml_get_widget (xml, "add_to_bookmarks_dialog");
  gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(window));

  gtk_widget_show_all (dialog);
 
  result = gtk_dialog_run (GTK_DIALOG (dialog));

  if (result == GTK_RESPONSE_OK)
    {
      name = gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (xml, "bookmark_name_entry")));
      if (strlen(name) < 1)
	name = conn->host;

      g_key_file_set_string (bookmarks,
			     name,
			     "host",
			     conn->host);
      g_key_file_set_integer (bookmarks,
			      name,
			      "port",
			      conn->port);

      vinagre_connection_set_name (conn, name);
      vinagre_bookmarks_save_file ();
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (G_OBJECT (xml));

  return (result == GTK_RESPONSE_OK);
}

gboolean
vinagre_bookmarks_edit (VinagreConnection *conn,
		        VinagreWindow     *window)
{
  gint result;
  GladeXML    *xml;
  const gchar *glade_file;
  GtkWidget   *dialog, *host_entry, *name_entry, *port_entry;
  const gchar *name;

  g_return_val_if_fail (conn != NULL, FALSE);

  glade_file = vinagre_utils_get_glade_filename ();
  xml = glade_xml_new (glade_file, "edit_bookmark_dialog", NULL);
  dialog = glade_xml_get_widget (xml, "edit_bookmark_dialog");
  gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(window));

  name_entry = glade_xml_get_widget (xml, "edit_bookmark_name_entry");
  host_entry = glade_xml_get_widget (xml, "edit_bookmark_host_entry");
  port_entry = glade_xml_get_widget (xml, "edit_bookmark_port_entry");

  gtk_entry_set_text (GTK_ENTRY(name_entry), conn->name);
  gtk_entry_set_text (GTK_ENTRY(host_entry), conn->host);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (port_entry), conn->port);

  gtk_widget_show_all (dialog);
 
  result = gtk_dialog_run (GTK_DIALOG (dialog));

  if (result == GTK_RESPONSE_OK)
    {

      g_key_file_remove_group (bookmarks, conn->name, NULL);

      name = gtk_entry_get_text (GTK_ENTRY (name_entry));
      vinagre_connection_set_host (conn, gtk_entry_get_text (GTK_ENTRY (host_entry)));
      vinagre_connection_set_port (conn, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (port_entry)));

      if (strlen(conn->name) < 1)
	name = conn->host;

      g_key_file_set_string (bookmarks,
			     name,
			     "host",
			     conn->host);
      g_key_file_set_integer (bookmarks,
			      name,
			      "port",
			      conn->port);

      vinagre_connection_set_name (conn, name);
      vinagre_bookmarks_save_file ();
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (G_OBJECT (xml));

  return (result == GTK_RESPONSE_OK);
}

GList *
vinagre_bookmarks_get_all (void)
{
  GList *list = NULL;
  gsize length, i;
  gchar **conns;
  VinagreConnection *conn;
  gchar *s_value;
  gint i_value;

  g_return_val_if_fail (bookmarks != NULL, NULL);

  conns = g_key_file_get_groups (bookmarks, &length);
  for (i=0; i<length; i++)
    {
      conn = vinagre_connection_new ();
      vinagre_connection_set_name (conn, conns[i]);

      s_value = g_key_file_get_string (bookmarks, conns[i], "host", NULL);
      vinagre_connection_set_host (conn, s_value);
      g_free (s_value);

      i_value = g_key_file_get_integer (bookmarks, conns[i], "port", NULL);
      vinagre_connection_set_port (conn, i_value);

      list = g_list_append (list, conn);
    }
  g_strfreev (conns);

  return list;
}

gboolean
vinagre_bookmarks_del (VinagreConnection *conn,
		       VinagreWindow     *window)
{
  gint       result;
  GtkWidget *dialog;
  gchar     *name;
  GError    *error = NULL;

  g_return_val_if_fail (bookmarks != NULL, FALSE);
  g_return_val_if_fail (conn != NULL, FALSE);

  name = vinagre_connection_best_name (conn);
  g_return_val_if_fail (g_key_file_has_group (bookmarks, name), FALSE);

  dialog = gtk_message_dialog_new (GTK_WINDOW (window),
				   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_OK_CANCEL,
				   _("Confirm removal?"));

  gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
					    _("Are you sure you want to exclude <i>%s</i> from bookmarks?"),
					    name);
 
  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (result == GTK_RESPONSE_OK)
    {
      g_key_file_remove_group (bookmarks, name, &error);
      if (error)
	{
	  g_warning (_("Error while removing %s from bookmarks: %s"),
			name,
			error->message);
	  g_error_free (error);
	  g_free (name);
	  return FALSE;
	}
      g_free (name);
      vinagre_bookmarks_save_file ();
    }

  return (result == GTK_RESPONSE_OK);
}

VinagreConnection *
vinagre_bookmarks_exists (const char *host, int port)
{
  VinagreConnection *conn = NULL;
  gsize length, i;
  gchar **conns = NULL;
  gchar *s_host = NULL;
  gint  i_port;

  g_return_val_if_fail (bookmarks != NULL, NULL);

  conns = g_key_file_get_groups (bookmarks, &length);
  for (i=0; i<length; i++)
    {
      s_host = g_key_file_get_string (bookmarks, conns[i], "host", NULL);
      i_port = g_key_file_get_integer (bookmarks, conns[i], "port", NULL);

      if ( (g_str_equal (host, s_host)) && (port == i_port) )
	{
	  conn = vinagre_connection_new ();
	  vinagre_connection_set_name (conn, conns[i]);

	  vinagre_connection_set_host (conn, host);
	  vinagre_connection_set_port (conn, port);

	  g_free (s_host);
	  s_host = NULL;

	  break;
	}

      if (s_host)
        {
	  g_free (s_host);
          s_host = NULL;
        }
    }

  if (s_host)
    g_free (s_host);

  if (conns)
    g_strfreev (conns);

  return conn;
}
/* vim: ts=8 */