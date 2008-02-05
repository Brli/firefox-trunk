/*
 * Copyright (C) 2008  Canonical Ltd.
 * Author: Alexander Sack <asac@jwsdot.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */ 

#define GETTEXT_PACKAGE "ubuntu-migrator"
#define LOCALEDIR "po"
#define WINDOW_ICON_PATH "/usr/share/pixmaps/firefox-3.0.png"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static int main_response = 128;

static void
do_reply(GtkWidget *dialog,
	 gint response,
	 gpointer udata)
{
  gtk_widget_destroy(dialog);
  gtk_main_quit();
  main_response = response;
}

int
main(int argc, char** argv)
{

  GtkWidget *dialog, *label;

  /* intialize gettext */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init(&argc, &argv);
 
  gtk_window_set_default_icon_from_file (WINDOW_ICON_PATH, NULL);

  /* Create the widgets */
  dialog = gtk_dialog_new_with_buttons ("Firefox 3 - Beta Support",
					NULL,
					GTK_DIALOG_MODAL,
					_("Keep Firefox 3 Settings"), 1,
					_("Decide Later"), 0,
					_("Import Settings"), 2,
					NULL);

  gtk_dialog_set_default_response (GTK_DIALOG(dialog),
				   0);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
					   2, 1, 0,
					   -1);

  g_signal_connect (dialog,
		    "response", 
		    G_CALLBACK (do_reply),
		    NULL);

  GtkWidget *hbox = gtk_hbox_new(FALSE,
				 5);

  GtkWidget *icon_view = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
						  GTK_ICON_SIZE_DIALOG);
  
  label = gtk_label_new (_("Do you want to import your bookmarks and other "
			   "settings from Firefox 2, replacing your settings "
			   "from Firefox 3 Alpha?"));
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

  gtk_box_pack_start_defaults(GTK_BOX(hbox), icon_view);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox),
		      hbox, TRUE, TRUE, 10);
  gtk_widget_show_all (dialog);
  gtk_main();
  return main_response < 0 ? 0 : main_response;
}
