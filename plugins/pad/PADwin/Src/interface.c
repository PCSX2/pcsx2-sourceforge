/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

GtkWidget*
create_Config (void)
{
  GtkWidget *Config;
  GtkWidget *vbox1;
  GtkWidget *hbox1;
  GSList *hbox1_group = NULL;
  GtkWidget *radiobutton1;
  GtkWidget *radiobutton2;
  GtkWidget *fixed2;
  GtkWidget *Key6;
  GtkWidget *Key2;
  GtkWidget *Key9;
  GtkWidget *Key1;
  GtkWidget *Key10;
  GtkWidget *Key3;
  GtkWidget *Key4;
  GtkWidget *Key7;
  GtkWidget *Key5;
  GtkWidget *Key0;
  GtkWidget *Key12;
  GtkWidget *Key15;
  GtkWidget *Key13;
  GtkWidget *Key14;
  GtkWidget *Key11;
  GtkWidget *Key8;
  GtkWidget *Key16;
  GtkWidget *Key19;
  GtkWidget *Key21;
  GtkWidget *Key17;
  GtkWidget *Key18;
  GtkWidget *Key22;
  GtkWidget *Key20;
  GtkWidget *Key23;
  GtkWidget *vbox3;
  GtkWidget *GtkCheckButton_Analog;
  GtkWidget *hbuttonbox1;
  GtkWidget *button1;
  GtkWidget *button2;

  Config = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (Config), "Config", Config);
  gtk_container_set_border_width (GTK_CONTAINER (Config), 5);
  gtk_window_set_title (GTK_WINDOW (Config), "PADconfig");
  gtk_window_set_position (GTK_WINDOW (Config), GTK_WIN_POS_CENTER);

  vbox1 = gtk_vbox_new (FALSE, 5);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (Config), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (Config), vbox1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 5);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (Config), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);

  radiobutton1 = gtk_radio_button_new_with_label (hbox1_group, "Pad1");
  hbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
  gtk_widget_ref (radiobutton1);
  gtk_object_set_data_full (GTK_OBJECT (Config), "radiobutton1", radiobutton1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton1);
  gtk_box_pack_start (GTK_BOX (hbox1), radiobutton1, TRUE, FALSE, 0);

  radiobutton2 = gtk_radio_button_new_with_label (hbox1_group, "Pad2");
  hbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
  gtk_widget_ref (radiobutton2);
  gtk_object_set_data_full (GTK_OBJECT (Config), "radiobutton2", radiobutton2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton2);
  gtk_box_pack_start (GTK_BOX (hbox1), radiobutton2, TRUE, FALSE, 0);

  fixed2 = gtk_fixed_new ();
  gtk_widget_ref (fixed2);
  gtk_object_set_data_full (GTK_OBJECT (Config), "fixed2", fixed2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (fixed2);
  gtk_box_pack_start (GTK_BOX (vbox1), fixed2, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (fixed2), 5);

  Key6 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key6);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key6", Key6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key6);
  gtk_fixed_put (GTK_FIXED (fixed2), Key6, 48, 136);
  gtk_widget_set_uposition (Key6, 48, 136);
  gtk_widget_set_usize (Key6, 65, 22);

  Key2 = gtk_button_new_with_label ("button7");
  gtk_widget_ref (Key2);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key2", Key2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key2);
  gtk_fixed_put (GTK_FIXED (fixed2), Key2, 48, 40);
  gtk_widget_set_uposition (Key2, 48, 40);
  gtk_widget_set_usize (Key2, 65, 22);

  Key9 = gtk_button_new_with_label ("button8");
  gtk_widget_ref (Key9);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key9", Key9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key9);
  gtk_fixed_put (GTK_FIXED (fixed2), Key9, 88, 8);
  gtk_widget_set_uposition (Key9, 88, 8);
  gtk_widget_set_usize (Key9, 65, 22);

  Key1 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key1);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key1", Key1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key1);
  gtk_fixed_put (GTK_FIXED (fixed2), Key1, 168, 8);
  gtk_widget_set_uposition (Key1, 168, 8);
  gtk_widget_set_usize (Key1, 65, 22);

  Key10 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key10);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key10", Key10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key10);
  gtk_fixed_put (GTK_FIXED (fixed2), Key10, 248, 8);
  gtk_widget_set_uposition (Key10, 248, 8);
  gtk_widget_set_usize (Key10, 65, 22);

  Key3 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key3);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key3", Key3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key3);
  gtk_fixed_put (GTK_FIXED (fixed2), Key3, 208, 40);
  gtk_widget_set_uposition (Key3, 208, 40);
  gtk_widget_set_usize (Key3, 65, 22);

  Key4 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key4);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key4", Key4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key4);
  gtk_fixed_put (GTK_FIXED (fixed2), Key4, 48, 72);
  gtk_widget_set_uposition (Key4, 48, 72);
  gtk_widget_set_usize (Key4, 65, 22);

  Key7 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key7);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key7", Key7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key7);
  gtk_fixed_put (GTK_FIXED (fixed2), Key7, 8, 104);
  gtk_widget_set_uposition (Key7, 8, 104);
  gtk_widget_set_usize (Key7, 65, 22);

  Key5 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key5);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key5", Key5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key5);
  gtk_fixed_put (GTK_FIXED (fixed2), Key5, 88, 104);
  gtk_widget_set_uposition (Key5, 88, 104);
  gtk_widget_set_usize (Key5, 65, 22);

  Key0 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key0);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key0", Key0,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key0);
  gtk_fixed_put (GTK_FIXED (fixed2), Key0, 8, 8);
  gtk_widget_set_uposition (Key0, 8, 8);
  gtk_widget_set_usize (Key0, 65, 22);

  Key12 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key12);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key12", Key12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key12);
  gtk_fixed_put (GTK_FIXED (fixed2), Key12, 208, 72);
  gtk_widget_set_uposition (Key12, 208, 72);
  gtk_widget_set_usize (Key12, 65, 22);

  Key15 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key15);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key15", Key15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key15);
  gtk_fixed_put (GTK_FIXED (fixed2), Key15, 168, 104);
  gtk_widget_set_uposition (Key15, 168, 104);
  gtk_widget_set_usize (Key15, 65, 22);

  Key13 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key13);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key13", Key13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key13);
  gtk_fixed_put (GTK_FIXED (fixed2), Key13, 248, 104);
  gtk_widget_set_uposition (Key13, 248, 104);
  gtk_widget_set_usize (Key13, 65, 22);

  Key14 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key14);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key14", Key14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key14);
  gtk_fixed_put (GTK_FIXED (fixed2), Key14, 208, 136);
  gtk_widget_set_uposition (Key14, 208, 136);
  gtk_widget_set_usize (Key14, 65, 22);

  Key11 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key11);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key11", Key11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key11);
  gtk_fixed_put (GTK_FIXED (fixed2), Key11, 80, 168);
  gtk_widget_set_uposition (Key11, 80, 168);
  gtk_widget_set_usize (Key11, 65, 22);

  Key8 = gtk_button_new_with_label ("button5");
  gtk_widget_ref (Key8);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key8", Key8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key8);
  gtk_fixed_put (GTK_FIXED (fixed2), Key8, 176, 168);
  gtk_widget_set_uposition (Key8, 176, 168);
  gtk_widget_set_usize (Key8, 65, 22);

  Key16 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key16);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key16", Key16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key16);
  gtk_fixed_put (GTK_FIXED (fixed2), Key16, 360, 8);
  gtk_widget_set_uposition (Key16, 360, 8);
  gtk_widget_set_usize (Key16, 65, 22);

  Key19 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key19);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key19", Key19,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key19);
  gtk_fixed_put (GTK_FIXED (fixed2), Key19, 320, 40);
  gtk_widget_set_uposition (Key19, 320, 40);
  gtk_widget_set_usize (Key19, 65, 22);

  Key21 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key21);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key21", Key21,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key21);
  gtk_fixed_put (GTK_FIXED (fixed2), Key21, 400, 136);
  gtk_widget_set_uposition (Key21, 400, 136);
  gtk_widget_set_usize (Key21, 65, 22);

  Key17 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key17);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key17", Key17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key17);
  gtk_fixed_put (GTK_FIXED (fixed2), Key17, 400, 40);
  gtk_widget_set_uposition (Key17, 400, 40);
  gtk_widget_set_usize (Key17, 65, 22);

  Key18 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key18);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key18", Key18,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key18);
  gtk_fixed_put (GTK_FIXED (fixed2), Key18, 360, 72);
  gtk_widget_set_uposition (Key18, 360, 72);
  gtk_widget_set_usize (Key18, 65, 22);

  Key22 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key22);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key22", Key22,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key22);
  gtk_fixed_put (GTK_FIXED (fixed2), Key22, 360, 168);
  gtk_widget_set_uposition (Key22, 360, 168);
  gtk_widget_set_usize (Key22, 65, 22);

  Key20 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key20);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key20", Key20,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key20);
  gtk_fixed_put (GTK_FIXED (fixed2), Key20, 360, 104);
  gtk_widget_set_uposition (Key20, 360, 104);
  gtk_widget_set_usize (Key20, 65, 22);

  Key23 = gtk_button_new_with_label ("button4");
  gtk_widget_ref (Key23);
  gtk_object_set_data_full (GTK_OBJECT (Config), "Key23", Key23,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Key23);
  gtk_fixed_put (GTK_FIXED (fixed2), Key23, 320, 136);
  gtk_widget_set_uposition (Key23, 320, 136);
  gtk_widget_set_usize (Key23, 65, 22);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (Config), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox3, TRUE, TRUE, 0);

  GtkCheckButton_Analog = gtk_check_button_new_with_label ("Analog Support");
  gtk_widget_ref (GtkCheckButton_Analog);
  gtk_object_set_data_full (GTK_OBJECT (Config), "GtkCheckButton_Analog", GtkCheckButton_Analog,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (GtkCheckButton_Analog);
  gtk_box_pack_start (GTK_BOX (vbox3), GtkCheckButton_Analog, FALSE, FALSE, 0);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (Config), "hbuttonbox1", hbuttonbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, TRUE, TRUE, 0);

  button1 = gtk_button_new_with_label ("Ok");
  gtk_widget_ref (button1);
  gtk_object_set_data_full (GTK_OBJECT (Config), "button1", button1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button1);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  button2 = gtk_button_new_with_label ("Cancel");
  gtk_widget_ref (button2);
  gtk_object_set_data_full (GTK_OBJECT (Config), "button2", button2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button2);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (radiobutton1), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Pad1),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (radiobutton2), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Pad2),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key6), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key2), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key9), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key1), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key10), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key3), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key4), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key7), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key5), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key0), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key12), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key15), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key13), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key14), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key11), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Key8), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Key),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Ok),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button2), "clicked",
                      GTK_SIGNAL_FUNC (OnConf_Cancel),
                      NULL);

  return Config;
}

GtkWidget*
create_About (void)
{
  GtkWidget *About;
  GtkWidget *vbox2;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *hbuttonbox2;
  GtkWidget *button3;

  About = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (About), "About", About);
  gtk_container_set_border_width (GTK_CONTAINER (About), 5);
  gtk_window_set_title (GTK_WINDOW (About), "PADabout");
  gtk_window_set_position (GTK_WINDOW (About), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (About), TRUE);

  vbox2 = gtk_vbox_new (FALSE, 5);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (About), "vbox2", vbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (About), vbox2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);

  label1 = gtk_label_new ("PADwin Plugin");
  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (About), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox2), label1, FALSE, FALSE, 0);

  label2 = gtk_label_new ("Authors:\nlinuzappz <linuzappz@hotmail.com>\nflorin sasu <florinsasu@hotmail.com>");
  gtk_widget_ref (label2);
  gtk_object_set_data_full (GTK_OBJECT (About), "label2", label2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label2);
  gtk_box_pack_start (GTK_BOX (vbox2), label2, FALSE, FALSE, 0);

  hbuttonbox2 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox2);
  gtk_object_set_data_full (GTK_OBJECT (About), "hbuttonbox2", hbuttonbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbuttonbox2, TRUE, TRUE, 0);

  button3 = gtk_button_new_with_label ("Ok");
  gtk_widget_ref (button3);
  gtk_object_set_data_full (GTK_OBJECT (About), "button3", button3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button3);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), button3);
  GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (button3), "clicked",
                      GTK_SIGNAL_FUNC (OnAbout_Ok),
                      NULL);

  return About;
}

