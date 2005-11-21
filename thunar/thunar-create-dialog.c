/* $Id$ */
/*-
 * Copyright (c) 2005 Benedikt Meurer <benny@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <thunar/thunar-create-dialog.h>
#include <thunar/thunar-icon-factory.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_FILENAME,
  PROP_MIME_INFO,
};



static void thunar_create_dialog_class_init   (ThunarCreateDialogClass *klass);
static void thunar_create_dialog_init         (ThunarCreateDialog      *dialog);
static void thunar_create_dialog_dispose      (GObject                 *object);
static void thunar_create_dialog_get_property (GObject                 *object,
                                               guint                    prop_id,
                                               GValue                  *value,
                                               GParamSpec              *pspec);
static void thunar_create_dialog_set_property (GObject                 *object,
                                               guint                    prop_id,
                                               const GValue            *value,
                                               GParamSpec              *pspec);
static void thunar_create_dialog_realize      (GtkWidget               *widget);
static void thunar_create_dialog_update_image (ThunarCreateDialog      *dialog);
static void thunar_create_dialog_text_changed (GtkWidget               *entry,
                                               ThunarCreateDialog      *dialog);



struct _ThunarCreateDialogClass
{
  GtkDialogClass __parent__;
};

struct _ThunarCreateDialog
{
  GtkDialog __parent__;

  GtkWidget         *image;
  GtkWidget         *entry;

  ThunarVfsMimeInfo *mime_info;
};



G_DEFINE_TYPE (ThunarCreateDialog, thunar_create_dialog, GTK_TYPE_DIALOG);



static void
thunar_create_dialog_class_init (ThunarCreateDialogClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = thunar_create_dialog_dispose;
  gobject_class->get_property = thunar_create_dialog_get_property;
  gobject_class->set_property = thunar_create_dialog_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = thunar_create_dialog_realize;

  /**
   * ThunarCreateDialog::filename:
   *
   * The filename entered in the dialog's entry box.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FILENAME,
                                   g_param_spec_string ("filename",
                                                        _("Filename"),
                                                        _("Filename"),
                                                        NULL,
                                                        EXO_PARAM_READWRITE));

  /**
   * ThunarCreateDialog::mime-info:
   *
   * The #ThunarVfsMimeInfo of the file to create.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MIME_INFO,
                                   g_param_spec_boxed ("mime-info",
                                                       _("Mime Info"),
                                                       _("Mime Info"),
                                                       THUNAR_VFS_TYPE_MIME_INFO,
                                                       EXO_PARAM_READWRITE));
}



static void
thunar_create_dialog_init (ThunarCreateDialog *dialog)
{
  AtkRelationSet *relations;
  AtkRelation    *relation;
  AtkObject      *object;
  GtkWidget      *label;
  GtkWidget      *table;

  /* configure the dialog itself */
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, FALSE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 300, -1);

  table = g_object_new (GTK_TYPE_TABLE, "border-width", 6, "column-spacing", 6, "row-spacing", 3, NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  dialog->image = g_object_new (GTK_TYPE_IMAGE, "xpad", 6, "ypad", 6, NULL);
  gtk_table_attach (GTK_TABLE (table), dialog->image, 0, 1, 0, 2, GTK_FILL, GTK_FILL, 0, 0); 
  gtk_widget_show (dialog->image);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("Enter the new name:"), "xalign", 0.0f, NULL);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  dialog->entry = g_object_new (GTK_TYPE_ENTRY, "activates-default", TRUE, NULL);
  g_signal_connect (G_OBJECT (dialog->entry), "changed", G_CALLBACK (thunar_create_dialog_text_changed), dialog);
  gtk_table_attach (GTK_TABLE (table), dialog->entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->entry);

  /* set Atk label relation for the entry */
  object = gtk_widget_get_accessible (dialog->entry);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));
}



static void
thunar_create_dialog_dispose (GObject *object)
{
  ThunarCreateDialog *dialog = THUNAR_CREATE_DIALOG (object);

  /* release the mime info (if any) */
  thunar_create_dialog_set_mime_info (dialog, NULL);

  (*G_OBJECT_CLASS (thunar_create_dialog_parent_class)->dispose) (object);
}



static void
thunar_create_dialog_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ThunarCreateDialog *dialog = THUNAR_CREATE_DIALOG (object);

  switch (prop_id)
    {
    case PROP_FILENAME:
      g_value_set_string (value, thunar_create_dialog_get_filename (dialog));
      break;

    case PROP_MIME_INFO:
      g_value_set_boxed (value, thunar_create_dialog_get_mime_info (dialog));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_create_dialog_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ThunarCreateDialog *dialog = THUNAR_CREATE_DIALOG (object);

  switch (prop_id)
    {
    case PROP_FILENAME:
      thunar_create_dialog_set_filename (dialog, g_value_get_string (value));
      break;

    case PROP_MIME_INFO:
      thunar_create_dialog_set_mime_info (dialog, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_create_dialog_realize (GtkWidget *widget)
{
  ThunarCreateDialog *dialog = THUNAR_CREATE_DIALOG (widget);

  /* let GtkWidget realize the widget */
  (*GTK_WIDGET_CLASS (thunar_create_dialog_parent_class)->realize) (widget);

  /* update the image */
  thunar_create_dialog_update_image (dialog);
}



static void
thunar_create_dialog_update_image (ThunarCreateDialog *dialog)
{
  ThunarIconFactory *icon_factory;
  GtkIconTheme      *icon_theme;
  const gchar       *icon_name;
  GdkPixbuf         *icon = NULL;

  /* check if we have a mime info */
  if (G_LIKELY (dialog->mime_info != NULL))
    {
      /* determine the icon theme and factory for the current screen */
      icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (dialog)));
      icon_factory = thunar_icon_factory_get_for_icon_theme (icon_theme);

      /* determine the icon name for the mime info */
      icon_name = thunar_vfs_mime_info_lookup_icon_name (dialog->mime_info, icon_theme);

      /* try to load the icon for the given name */
      icon = thunar_icon_factory_load_icon (icon_factory, icon_name, 48, NULL, FALSE);

      /* release the icon factory */
      g_object_unref (G_OBJECT (icon_factory));
    }

  /* setup the image */
  if (G_LIKELY (icon != NULL))
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (dialog->image), icon);
      g_object_unref (G_OBJECT (icon));
      gtk_widget_show (dialog->image);
    }
  else
    {
      gtk_widget_hide (dialog->image);
    }
}



static void
thunar_create_dialog_text_changed (GtkWidget          *entry,
                                   ThunarCreateDialog *dialog)
{
  const gchar *text;
  const gchar *p;

  g_return_if_fail (GTK_IS_ENTRY (entry));
  g_return_if_fail (THUNAR_IS_CREATE_DIALOG (dialog));
  g_return_if_fail (dialog->entry == entry);

  /* verify the new text */
  text = gtk_entry_get_text (GTK_ENTRY (entry));
  for (p = text; *p != '\0'; ++p)
    if (G_UNLIKELY (G_IS_DIR_SEPARATOR (*p)))
      break;

  /* enable/disable the "OK" button appropriately */
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, (*text != '\0' && *p == '\0'));
}



/**
 * thunar_create_dialog_new:
 *
 * Allocates a new #ThunarCreateDialog instance.
 *
 * Return value: the newly allocated #ThunarCreateDialog instance.
 **/
GtkWidget*
thunar_create_dialog_new (void)
{
  return g_object_new (THUNAR_TYPE_CREATE_DIALOG, NULL);
}



/**
 * thunar_create_dialog_get_filename:
 * @dialog : a #ThunarCreateDialog.
 *
 * Returns the filename currently selected in @dialog.
 *
 * Return value: the filename currently selected in @dialog.
 **/
const gchar*
thunar_create_dialog_get_filename (const ThunarCreateDialog *dialog)
{
  g_return_val_if_fail (THUNAR_IS_CREATE_DIALOG (dialog), NULL);
  return gtk_entry_get_text (GTK_ENTRY (dialog->entry));
}



/**
 * thunar_create_dialog_set_filename:
 * @dialog   : a #ThunarCreateDialog.
 * @filename : the new filename to set.
 *
 * Sets the filename currently selected in @dialog
 * to the specified @filename.
 **/
void
thunar_create_dialog_set_filename (ThunarCreateDialog *dialog,
                                   const gchar        *filename)
{
  g_return_if_fail (THUNAR_IS_CREATE_DIALOG (dialog));
  g_return_if_fail (filename != NULL);
      
  /* setup the new filename */
  gtk_entry_set_text (GTK_ENTRY (dialog->entry), filename);

  /* notify listeners */
  g_object_notify (G_OBJECT (dialog), "filename");
}



/**
 * thunar_create_dialog_get_mime_info:
 * @dialog : a #ThunarCreateDialog.
 *
 * Returns the current mime info for @dialog
 * or %NULL.
 *
 * Return value: the mime info for @dialog.
 **/
ThunarVfsMimeInfo*
thunar_create_dialog_get_mime_info (const ThunarCreateDialog *dialog)
{
  g_return_val_if_fail (THUNAR_IS_CREATE_DIALOG (dialog), NULL);
  return dialog->mime_info;
}



/**
 * thunar_create_dialog_set_mime_info:
 * @dialog    : a #ThunarCreateDialog.
 * @mime_info : the new #ThunarVfsMimeInfo or %NULL.
 * 
 * Set the mime info for @dialog to @mime_info.
 **/
void
thunar_create_dialog_set_mime_info (ThunarCreateDialog *dialog,
                                    ThunarVfsMimeInfo  *mime_info)
{
  g_return_if_fail (THUNAR_IS_CREATE_DIALOG (dialog));

  /* release the previous mime info */
  if (G_UNLIKELY (dialog->mime_info != NULL))
    thunar_vfs_mime_info_unref (dialog->mime_info);

  /* activate the new mime info */
  dialog->mime_info = mime_info;

  /* take a reference on the mime info */
  if (G_LIKELY (mime_info != NULL))
    thunar_vfs_mime_info_ref (mime_info);

  /* update the image if we're already realized */
  if (GTK_WIDGET_REALIZED (dialog))
    thunar_create_dialog_update_image (dialog);

  /* notify listeners */
  g_object_notify (G_OBJECT (dialog), "mime-info");
}
