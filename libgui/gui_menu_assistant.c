/*
*  gui_menu_assistant.c
*  Source code file for G-Code generation, simulation, and visualization
*  library. This software is Copyright (C) 2006 by Justin Shumaker
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "gui_menu_assistant.h"
#include "gui.h"
#include "gui_define.h"
#include "gui_menu_util.h"
#include "gcode.h"

static void
ngon_on_assistant_close_cancel (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (widget);
}


static void
ngon_on_assistant_prepare (GtkWidget *widget, GtkWidget *page, gpointer data)
{
  gint current_page, n_pages;
  gchar *title;

  current_page = gtk_assistant_get_current_page (GTK_ASSISTANT (widget));

  gtk_assistant_set_page_complete (GTK_ASSISTANT (widget),  gtk_assistant_get_nth_page (GTK_ASSISTANT (widget), 0), TRUE);

  n_pages = gtk_assistant_get_n_pages (GTK_ASSISTANT (widget));

  title = g_strdup_printf ("N-Gon");
  gtk_window_set_title (GTK_WINDOW (widget), title);
  g_free (title);
}


static void
ngon_on_assistant_apply (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  gui_t *gui;
  gcode_block_t *selected_block, *sketch_block, *line_block;
  gcode_sketch_t *sketch;
  gcode_line_t *line;
  gfloat_t sides, radius, angle;
  int i, si;


  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);

  sides = gtk_spin_button_get_value (GTK_SPIN_BUTTON (((GtkWidget **) gui->generic_ptr)[0]));
  radius = gtk_spin_button_get_value (GTK_SPIN_BUTTON (((GtkWidget **) gui->generic_ptr)[1]));

  /* Create Sketch */
  gcode_sketch_init (&gui->gcode, &sketch_block, selected_block);
  insert_primitive (gui, sketch_block, selected_block, &iter, GUI_INSERT_AFTER);
  sketch = (gcode_sketch_t *) sketch_block->pdata;

  /* Create Lines */
  si = (int) sides;
  for (i = 0; i < si; i++)
  {
    gcode_line_init (&gui->gcode, &line_block, sketch_block);
    line = (gcode_line_t *) line_block->pdata;

    get_selected_block (gui, &selected_block, &iter);

    line->p0[0] = 0.0;
    line->p0[1] = radius;
    angle = 360.0 * ((float) i) / ((float) sides);
    GCODE_MATH_ROTATE(line->p0, line->p0, angle);

    line->p1[0] = 0.0;
    line->p1[1] = radius;
    angle = 360.0 * ((float) (i+1)) / ((float) sides);
    GCODE_MATH_ROTATE(line->p1, line->p1, angle);

    insert_primitive (gui, line_block, selected_block, &iter, GUI_INSERT_AFTER);
  }

  /* XXX - this should technically not be there */
  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);

  gui_menu_util_modified (gui, 1);

  free (gui->generic_ptr);
}


static void
ngon_create_page1 (gui_t *gui, GtkWidget *assistant)
{
  GtkWidget *table, *label, *sides_spin, *radius_spin;
  GdkPixbuf *pixbuf;

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), TABLE_SPACING);
  gtk_table_set_row_spacings (GTK_TABLE (table), TABLE_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);

  label = gtk_label_new ("Sides");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

  sides_spin = gtk_spin_button_new_with_range (3, 20, 1);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (sides_spin), 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (sides_spin), 6);
  gtk_table_attach_defaults (GTK_TABLE (table), sides_spin, 1, 2, 0, 1);
  ((GtkWidget **) gui->generic_ptr)[0] = sides_spin;

  label = gtk_label_new ("Radius");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

  radius_spin = gtk_spin_button_new_with_range (0.0001, GCODE_UNITS ((&gui->gcode), 10.0), 0.001);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (radius_spin), MANTISSA);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (radius_spin), GCODE_UNITS ((&gui->gcode), 0.5));
  gtk_table_attach_defaults (GTK_TABLE (table), radius_spin, 1, 2, 1, 2);
  ((GtkWidget **) gui->generic_ptr)[1] = radius_spin;

  gtk_widget_show_all (table);

  gtk_assistant_append_page (GTK_ASSISTANT (assistant), table);
  gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), table, "N-Gon");
  gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), table, GTK_ASSISTANT_PAGE_CONFIRM);

  pixbuf = gtk_widget_render_icon (assistant, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_assistant_set_page_header_image (GTK_ASSISTANT (assistant), table, pixbuf);
  g_object_unref (pixbuf);
}


void
gui_menu_assistant_ngon_menuitem_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget *assistant;
  gui_t *gui;

  gui = (gui_t *) data;

  assistant = gtk_assistant_new ();
  gtk_window_set_default_size (GTK_WINDOW (assistant), -1, -1);
  gtk_window_set_screen (GTK_WINDOW (assistant), gtk_widget_get_screen (gui->window));
  gtk_window_set_transient_for (GTK_WINDOW (assistant), GTK_WINDOW (gui->window));

  /* Setup Global Widgets */
  gui->generic_ptr = malloc (2 * sizeof (GtkWidget *));

  ngon_create_page1 (gui, assistant);

  g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (ngon_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (ngon_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (ngon_on_assistant_prepare), NULL);
  g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (ngon_on_assistant_apply), gui);

  gtk_widget_show (assistant);
}
