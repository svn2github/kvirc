//=============================================================================
//
//   File : libkviobjects.cpp
//   Creation date : Wed Sep 09 2000 20:59:01 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2000-2006 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc. ,59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//=============================================================================

#include <qwidgetlist.h>
#include <qobjectlist.h>
#include <qlistview.h>
#include <qimage.h>
#include "kvi_iconmanager.h"
#include "kvi_window.h"
#include "kvi_out.h"
#include "kvi_mirccntrl.h"

#include "kvi_app.h"

#include "kvi_module.h"

#include "kvi_locale.h"
#include "kvi_error.h"
#include "kvi_cmdformatter.h"
#include "kvi_out.h"
#include "kvi_app.h"

#include "class_button.h"
#include "class_checkbox.h"
#include "class_urllabel.h"
#include "class_lineedit.h"
#include "class_multilineedit.h"
#include "class_combobox.h"
#include "class_widget.h"
#include "class_list.h"
#include "class_file.h"
#include "class_socket.h"
#include "class_layout.h"
#include "class_menubar.h"
#include "class_window.h"
#include "class_listbox.h"
#include "class_progressbar.h"
#include "class_slider.h"
#include "class_spinbox.h"
#include "class_lcd.h"
#include "class_radiobutton.h"
#include "class_process.h"
#include "class_groupbox.h"
#include "class_tabwidget.h"
#include "class_workspace.h"
#include "class_popupmenu.h"
#include "class_listview.h"
#include "class_listviewitem.h"
#include "class_toolbar.h"
#include "class_mainwindow.h"
#include "class_toolbutton.h"
#include "class_wizard.h"
#include "class_tbrowser.h"
#include "class_painter.h"
#include "class_pixmap.h"
#include "class_wrapper.h"
#include "class_dialog.h"
#include "class_xmlreader.h"
#include "class_image.h"
#include "class_dockwindow.h"
#include "class_vbox.h"
#include "class_hbox.h"

static void dumpChildObjects(KviWindow *pWnd, QObject *parent, const char *spacing, bool bWidgetsOnly, KviKvsArray *n, int &idx);

static bool objects_module_cleanup(KviModule *m)
{
	KviKvsObject_process::unregisterSelf();
	KviKvsObject_socket::unregisterSelf();
	KviKvsObject_image::unregisterSelf();
	KviKvsObject_xmlreader::unregisterSelf();
	KviKvsObject_wrapper::unregisterSelf();
	KviKvsObject_file::unregisterSelf();
	KviKvsObject_progressbar::unregisterSelf();
	KviKvsObject_dockwindow::unregisterSelf();
	KviKvsObject_wizard::unregisterSelf();
	KviKvsObject_window::unregisterSelf();
	KviKvsObject_urlabel::unregisterSelf();
	KviKvsObject_toolbutton::unregisterSelf();
	KviKvsObject_toolbar::unregisterSelf();
	KviKvsObject_tabwidget::unregisterSelf();
	KviKvsObject_spinbox::unregisterSelf();
	KviKvsObject_slider::unregisterSelf();
	KviKvsObject_radiobutton::unregisterSelf();
	KviKvsObject_popupmenu::unregisterSelf();
	KviKvsObject_pixmap::unregisterSelf();
	KviKvsObject_painter::unregisterSelf();
	KviKvsObject_mainwindow::unregisterSelf();
	KviKvsObject_listviewitem::unregisterSelf();
	KviKvsObject_listview::unregisterSelf();
	KviKvsObject_listbox::unregisterSelf();
	KviKvsObject_combobox::unregisterSelf();
	KviKvsObject_workspace::unregisterSelf();
	KviKvsObject_menubar::unregisterSelf();
	KviKvsObject_lineedit::unregisterSelf();
	KviKvsObject_lcd::unregisterSelf();
	KviKvsObject_layout::unregisterSelf();
	KviKvsObject_hbox::unregisterSelf();
	KviKvsObject_vbox::unregisterSelf();
	KviKvsObject_label::unregisterSelf();
	KviKvsObject_groupbox::unregisterSelf();	
	KviKvsObject_dialog::unregisterSelf();
	KviKvsObject_checkbox::unregisterSelf();
	KviKvsObject_textbrowser::unregisterSelf();
	KviKvsObject_mledit::unregisterSelf();
	KviKvsObject_button::unregisterSelf();
	KviKvsObject_widget::unregisterSelf();
	KviKvsObject_list::unregisterSelf();

	return true;
}


static bool objects_kvs_cmd_killClass(KviKvsModuleCommandCall * c)
{ 
	/*
		@doc: objects.killclass
		@title:
			objects.killclass
		@type:
			command
		@short:
			Removes a class definition
		@syntax:
			objects.killclass [-q] <classname:string>
		@switches:
			!sw: -q | --quiet
			Suppresses any warning message
		@description:
			Removes the definition of the class <class_name>.[br]
			All the instances of the class are killed (thus children objects are killed too).[br]
			All the derived class definitions are removed as well.[br]
			Builtin class definitions cannot be removed and this commands
			kills only all the instances of that class (derived class definitions
			and instances in this case are NOT killed).[br]
		@seealso:
			[cmd]class[/cmd], [cmd]objects.clear[/cmd], [fnc]$classDefined[/fnc](),
			[doc:objects]Objects documentation[/doc]
	*/


	QString szClass;
	KVSM_PARAMETERS_BEGIN(c)
		KVSM_PARAMETER("class",KVS_PT_NONEMPTYSTRING,0,szClass)
	KVSM_PARAMETERS_END(c)
	KviKvsObjectClass * pClass=KviKvsKernel::instance()->objectController()->lookupClass(szClass);
	if(pClass)
	{
		if(pClass->isBuiltin())
		{
			KviKvsKernel::instance()->objectController()->killAllObjectsWithClass(pClass);
		} else {
			KviKvsKernel::instance()->objectController()->deleteClass(pClass); //Yahoo!!
		}
	}
	else if(!c->hasSwitch('q',"quiet"))c->warning(__tr2qs("Class '%Q' is not definited"),&szClass);
	return true;
}


static bool objects_kvs_cmd_clearObjects(KviKvsModuleCommandCall * c)
{ 
	/*
		@doc: objects.clear
		@title:
			objects.clear
		@type:
			command
		@short:
			Removes all the user class definitions
		@syntax:
			objects.clear [-i]
		@description:
			Removes the definition of all the user classes and kill all the object
			instances (also instances of the builtin classes).[br]
			If the -i switch is used, only the object instances are cleared
			but the class definitions are left unchanged.[br]
			This command should be used only for debugging & testing purposes
			and in particular should be NEVER used from inside an object
			function handler (leads to SIGSEGV :).[br]
		@seealso:
			[cmd]class[/cmd], [cmd]objects.killclass[/cmd],
			[doc:objects]Objects documentation[/doc]
	*/

	KviKvsKernel::instance()->objectController()->clearInstances();
	if(!c->switches()->find('i',"ii")) KviKvsKernel::instance()->objectController()->clearUserClasses();
	return true;
}


static bool objects_kvs_cmd_connect(KviKvsModuleCommandCall * c)
{ 

	/*
		@doc: objects.connect
		@title:
			objects.connect
		@type:
			command
		@short:
			Connects a signal to a slot
		@syntax:
			objects.connect <source_object:hobject> <signal_name:string> <target_object:hobject> <slot_name:string>
		@description:
			Connects the <source_object>'s signal <signal_name> to the
			<target_object>'s slot <slot_name>.
			When one of the two objects is destroyed, the signal/slot
			connection is automatically removed.[br]
			WARNING: This command name collides with the [doc:rfc2812]RFC2812[/doc]
			CONNECT IRC Op command: this IRC command is available to operators only
			and is rather rarely used: you can use it by the means of [doc:raw]raw[/doc].
		@seealso:
			[cmd]class[/cmd], [cmd]object.disconnect[/cmd], [doc:objects]objects documentation[/doc]
	*/

	/*
		@doc: connect
		@title:
			connect
		@type:
			command
		@short:
			Connects a signal to a slot
		@syntax:
			[Deprecated]connect <source_object:hobject> <signal_name:string> <target_object:hobject> <slot_name:string>
		@description:
			[DEPRECATED]
			Connects the <source_object>'s signal <signal_name> to the
			<target_object>'s slot <slot_name>.
			When one of the two objects is destroyed, the signal/slot
			connection is automatically removed.[br]
			WARNING: This command name collides with the [doc:rfc2812]RFC2812[/doc]
			CONNECT IRC Op command: this IRC command is available to operators only
			and is rather rarely used: you can use it by the means of [doc:raw]raw[/doc].
			For this reason this command is actually deprecated and will be removed
			in a future release in favor of [cmd]objects.connect[/cmd]. Don't use it.
		@seealso:
			[cmd]class[/cmd], [cmd]objects.disconnect[/cmd], [doc:objects]objects documentation[/doc]
	*/

	KviKvsObject *obSrc;
	KviKvsObject *obTrg;
	QString szSignal,szSlot;
	kvs_hobject_t hSrc,hTrg;
	KVSM_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("object source",KVS_PT_HOBJECT,0,hSrc)
		KVSM_PARAMETER("signal name",KVS_PT_NONEMPTYSTRING,0,szSignal)
		KVSO_PARAMETER("object target",KVS_PT_HOBJECT,0,hTrg)
		KVSM_PARAMETER("slot",KVS_PT_NONEMPTYSTRING,0,szSlot)
	KVSM_PARAMETERS_END(c)
	obTrg=KviKvsKernel::instance()->objectController()->lookupObject(hTrg);
	obSrc=KviKvsKernel::instance()->objectController()->lookupObject(hSrc);
	if(!obTrg)
	{
		c->warning(__tr2qs("Inexisting target object for objects.connect"));
		return true;
	}
	if(!obSrc)
	{
		c->warning(__tr2qs("Inexisting source object for objects.connect"));
		return true;
	}
	obSrc->connectSignal(szSignal,obTrg,szSlot);
	return true;
}


static bool objects_kvs_fnc_exists(KviKvsModuleFunctionCall * c)
{ 
	/*
		@doc: objects.exists
		@title:
			objects.exists
		@type:
			command
		@short:
			Check if an object exists.
		@syntax:
			<boolean> $object.exists(<obejct id:hobject>)
		@description:
			Check if an objects exists and return a boolean value.
		@seealso:
			[doc:objects]objects documentation[/doc]
	*/


	kvs_hobject_t hObj;
	KVSM_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("object",KVS_PT_HOBJECT,0,hObj)
	KVSM_PARAMETERS_END(c)
	KviKvsObject *ob=KviKvsKernel::instance()->objectController()->lookupObject(hObj);
	c->returnValue()->setBoolean(ob ? true : false);
	return true;
}

static bool objects_kvs_fnc_instances(KviKvsModuleFunctionCall * c)
{ 
	/*
		@doc: objects.instances
		@title:
			objects.instances
		@type:
			command
		@short:
			List object instances
		@syntax:
			<array> $object.instances(<class name:string>[,<flags:string>])
		@description:
			Returns an array with the instance handles of
			of objects of the specified class. <flags> is a
			string containing modifiers for the function behaviour.[br]
			If <flags> contains the letter "s" then
			only unsubclassed instances of the specified
			class are returned. This means that if B is
			a subclass of A then when asked for instances
			of A without the flag "s" this function will return
			both instances of A and B, and with the flag "s"
			this function will return only instances of A (and not B).
			If the class name does not define a valid class
			then an empty array is returned and a warning is
			printed unless the "q" flag is used.
		@examples:
			[example]
			foreach(%x,$objects.instances(object))
			{
				echo "OBJECT WITH CLASS" %x->$className() "AND NAME \""%x->$name()"\""
			}
			[/example]
		@seealso:
			[doc:objects]objects documentation[/doc]
	*/
	
	QString szClassName;
	QString szFlags;
	KVSM_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("class name",KVS_PT_NONEMPTYSTRING,0,szClassName)
		KVSO_PARAMETER("flags",KVS_PT_STRING,KVS_PF_OPTIONAL,szFlags)
	KVSM_PARAMETERS_END(c)

	KviKvsArray * pArry = new KviKvsArray();
	c->returnValue()->setArray(pArry);

	KviKvsObjectClass * pClass = KviKvsKernel::instance()->objectController()->lookupClass(szClassName);
	if(!pClass)
	{
		if(!szFlags.contains(QChar('q')))
			c->warning(__tr2qs("The class '%Q' does not exist"),&szClassName);
		return true;
	}
	QPtrDict<KviKvsObject> * od = KviKvsKernel::instance()->objectController()->objectDict();
	QPtrDictIterator<KviKvsObject> it(*od);
	kvs_uint_t uIdx = 0;
	if(szFlags.contains(QChar('s')))
	{
		while(KviKvsObject * ob = it.current())
		{
			if(ob->getExactClass() == pClass)
			{
				pArry->set(uIdx,new KviKvsVariant(ob->handle()));
				uIdx++;
			}
			++it;
		}
	} else {
		while(KviKvsObject * ob = it.current())
		{
			if(ob->inheritsClass(pClass))
			{
				pArry->set(uIdx,new KviKvsVariant(ob->handle()));
				uIdx++;
			}
			++it;
		}
	}
	return true;
}

static bool objects_kvs_cmd_disconnect(KviKvsModuleCommandCall * c)
{ 
	/*
		@doc: objects.disconnect
		@title:
			objects.disconnect
		@type:
			command
		@short:
			Disconnects a signal from a slot
		@syntax:
			disconnect <source_object:hobject> <signal_name:string> <target_object:hobject> <slot_name:string>
		@description:
			Disconnects the <source_object>'s signal <signal_name> from the
			<target_object>'s slot <slot_name>.
			When one of the two objects is destroyed, the signal/slot
			connection is automatically removed.
		@seealso:
			[cmd]class[/cmd], [cmd]objects.connect[/cmd], [doc:objects]objects documentation[/doc]
	*/

	/*
		@doc: disconnect
		@title:
			disconnect
		@type:
			command
		@short:
			[Deprecated]Disconnects a signal from a slot
		@syntax:
			disconnect <source_object:hobject> <signal_name:string> <target_object:hobject> <slot_name:string>
		@description:
			Disconnects the <source_object>'s signal <signal_name> from the
			<target_object>'s slot <slot_name>.
			When one of the two objects is destroyed, the signal/slot
			connection is automatically removed.
		@seealso:
			[cmd]class[/cmd], [cmd]connect[/cmd], [doc:objects]objects documentation[/doc]
	*/


	KviKvsObject *obSrc;
	KviKvsObject *obTrg;
	QString szSignal,szSlot;
	kvs_hobject_t hSrc,hTrg;
	KVSM_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("object source",KVS_PT_HOBJECT,0,hSrc)
		KVSM_PARAMETER("signal name",KVS_PT_NONEMPTYSTRING,0,szSignal)
		KVSO_PARAMETER("object target",KVS_PT_HOBJECT,0,hTrg)
		KVSM_PARAMETER("slot",KVS_PT_NONEMPTYSTRING,0,szSlot)
	KVSM_PARAMETERS_END(c)
	obTrg=KviKvsKernel::instance()->objectController()->lookupObject(hTrg);
	obSrc=KviKvsKernel::instance()->objectController()->lookupObject(hSrc);
	obSrc->disconnectSignal(szSignal,obTrg,szSlot);
	return true;
}



static bool objects_kvs_cmd_bitBlt(KviKvsModuleCommandCall * c)
{ 
	/*
		@doc: objects.bitblt
		@type:
			command
		@title:
			objects.bitblt
		@keyterms:
			widget pixmap painter
		@short:
			Copies a block of pixels from a source to a destination.
		@syntax:
			objects.bitblt <source> <x> <y> <destination>
		@description:
			Copies a block of pixels from <source> to <destination> at the coordinates <x>,<y> .[br]
			Source and destination must be a widget or pixmap.[br]
		
	*/

	KviKvsObject *obSrc;
	KviKvsObject *obDst;
	kvs_int_t iXdst,iYdst,iXsrc,iYsrc,uW,uH;
	kvs_hobject_t hDst,hSrc;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("destination",KVS_PT_HOBJECT,0,hDst)
		KVSO_PARAMETER("x offset dest",KVS_PT_INT,0,iXdst)
		KVSO_PARAMETER("y offset dest",KVS_PT_INT,0,iYdst)
		KVSO_PARAMETER("source",KVS_PT_HOBJECT,0,hSrc)
		KVSO_PARAMETER("x offset source",KVS_PT_INT,0,iXsrc)
		KVSO_PARAMETER("y offset source",KVS_PT_INT,0,iYsrc)
		KVSO_PARAMETER("width",KVS_PT_UNSIGNEDINTEGER,0,uW)
		KVSO_PARAMETER("height",KVS_PT_UNSIGNEDINTEGER,0,uH)
	KVSO_PARAMETERS_END(c)
	obSrc=KviKvsKernel::instance()->objectController()->lookupObject(hSrc);
	if (!obSrc)
	{
			c->warning(__tr2qs("Source is not an object"));
		return true;
	}
	obDst=KviKvsKernel::instance()->objectController()->lookupObject(hDst);
	if (!obDst)
	{
			c->warning(__tr2qs("Destination is not an object"));
		return true;
	}

	QImage * imgSource=0;
	QPaintDevice * pdSource = 0;
	if (obSrc->inherits("KviKvsObject_image")) 
		imgSource=((KviKvsObject_image *)obSrc)->image();
	else{

	if(obSrc->inherits("KviKvsObject_pixmap")) pdSource = ((KviKvsObject_pixmap *)obSrc)->pixmap();
	else if (obSrc->inherits("KviKvsObject_widget")) pdSource=((KviKvsObject_widget *)obSrc)->widget();
	if (!pdSource)
	{
		c->warning(__tr2qs("Widget, Image or Pixmap required "));
		return true;
	}
	}
	QPaintDevice * pdDest = 0;
	if(obDst->inherits("KviKvsObject_pixmap")) pdDest= ((KviKvsObject_pixmap *)obDst)->pixmap();
	else if (obDst->inherits("KviKvsObject_widget")) pdDest=((KviKvsObject_widget *)obDst)->widget();
	if (!pdDest)
	{
		c->warning(__tr2qs("Widget or Pixmap required"));
		return true;
	}
	if (obSrc->inherits("KviKvsObject_image")) 
	bitBlt(pdDest,iXdst,iYdst,imgSource,iXsrc,iYsrc,uW,uH);
	else
		bitBlt(pdDest,iXdst,iYdst,pdSource,iXsrc,iYsrc,uW,uH);
	return true;
}
	/*
		@doc: objects.blend
		@type:
			command
		@title:
			objects.blend
		@keyterms:
			widget pixmap image
		@short:
			Blends two images  to a destination.
		@syntax:
			objects.blend <background: image hobject> <x_offset_bkg:unsigned integer> <y_offset_bkg:unsigned integer> 
			<foreground: image hobject> <x_offset_fgr:unsigned integer> <y_offset_fgr:unsigned integer> 
			<destination: image, pixmap. widget hobject> <x_offset_dest:unsigned integer> <y_offset_dest:unsigned integer> <width:unsigned integer> <height:unsigned integer> <blend_factor>
		@description:
			Blends two images <background> and <foreground> and copy the result to <destination> at the coordinates <x_offset>,<y_offset> with a <blend_factor> value .[br]
			Destination must be a widget or pixmap.[br]
		
	*/

static bool objects_kvs_cmd_blend(KviKvsModuleCommandCall * c)
{
	
	KviKvsObject *obFor;
	KviKvsObject *obBck;
	KviKvsObject *obDest;

	kvs_int_t iBkX,iBkY;
	kvs_int_t iFoX,iFoY;
	kvs_int_t iDesX,iDesY,uW,uH;
    kvs_real_t iBlend;

	kvs_hobject_t hBk,hFo,hDest;

	KVSO_PARAMETERS_BEGIN(c)
	
		KVSO_PARAMETER("image_background",KVS_PT_HOBJECT,0,hBk)
		KVSO_PARAMETER("x_offset_background",KVS_PT_UNSIGNEDINTEGER,0,iBkX)
		KVSO_PARAMETER("y_offset_background",KVS_PT_UNSIGNEDINTEGER,0,iBkY)
		KVSO_PARAMETER("image_foreground",KVS_PT_HOBJECT,0,hFo)
		KVSO_PARAMETER("x_offset_foreground",KVS_PT_UNSIGNEDINTEGER,0,iFoX)
		KVSO_PARAMETER("y_offset_foreground",KVS_PT_UNSIGNEDINTEGER,0,iFoY)
		KVSO_PARAMETER("destination",KVS_PT_HOBJECT,0,hDest)
		KVSO_PARAMETER("x_offset_dest",KVS_PT_INT,0,iDesX)
		KVSO_PARAMETER("y_offset_dest",KVS_PT_INT,0,iDesY)
		KVSO_PARAMETER("width",KVS_PT_UNSIGNEDINTEGER,0,uW)
		KVSO_PARAMETER("height",KVS_PT_UNSIGNEDINTEGER,0,uH)

		KVSO_PARAMETER("blend_value",KVS_PT_REAL,0,iBlend)

	

	KVSO_PARAMETERS_END(c)

	obFor=KviKvsKernel::instance()->objectController()->lookupObject(hFo);
	

	obBck=KviKvsKernel::instance()->objectController()->lookupObject(hBk);
	obDest=KviKvsKernel::instance()->objectController()->lookupObject(hDest);
	if (!obFor || !obBck || !obDest)
	{
		c->warning(__tr2qs("One o more of background, foreground or destination aren't objects"));
		return true;
	}
/*	if (!obFor->object() || !obBck->object() || !obDest->object())
	{
		c->warning(__tr2qs("One o more of background, foreground or destination aren't valid objects"));
		return true;
	}
*/

	if (!obBck->inherits("KviKvsObject_image") || !obFor->inherits("KviKvsObject_image"))
	{
		c->warning(__tr2qs("Image objects required"));
		return true;
	}

	QPaintDevice * pdDest = 0;
	if(obDest->inherits("KviKvsObject_pixmap")) pdDest = ((KviKvsObject_pixmap *)obDest)->pixmap();
	else if (obDest->inherits("KviKvsObject_widget")) pdDest=((KviKvsObject_widget *)obDest)->widget();
	
	if (!pdDest)
	{
		c->warning(__tr2qs("Widget or Pixmap required "));
		return true;
	}



	QImage *img_back=((KviKvsObject_image *)obBck)->image();
	QImage *img_fore=((KviKvsObject_image *)obFor)->image();

	// check size
	if ((iBkX+uW>img_back->width())||(iBkY+uH>img_back->height()))
	{
		c->warning(__tr2qs("Values for background are out of image size "));
		return true;
	}
	if ((iFoX+uW>img_fore->width())||(iFoY+uH>img_fore->height()))
	{
		c->warning(__tr2qs("Values for foreground are out of image size "));
		return true;
	}
	QImage buffer;
	buffer.create(uW,uH,32);
	buffer.setAlphaBuffer(true);
	for(int y = 0;y < buffer.height();y++)
	{
		QRgb * dst = (QRgb *)buffer.scanLine(y);
		QRgb * end = dst + buffer.width();
		QRgb * bkg = (QRgb *)img_back->scanLine(y+iBkY);
		bkg += iBkX;
		QRgb * fgn = (QRgb *)img_fore->scanLine(y+iFoY);
		fgn += iFoX;
		double dRemaining = 1.0 - iBlend;
		int a=0;
		while(dst < end)
		{
			
	
		*dst = qRgba(
						(int)((qRed(*bkg) * dRemaining) + (qRed(*fgn) * iBlend)),
						(int)((qGreen(*bkg) * dRemaining) + (qGreen(*fgn) * iBlend)),
						(int)((qBlue(*bkg) * dRemaining) + (qBlue(*fgn) * iBlend)),
						(int)((qAlpha(*bkg) * dRemaining) + (qAlpha(*fgn) * iBlend))
						);

			dst++;
			bkg++;
			fgn++;
		}
		

	}

	bitBlt(pdDest,iDesX,iDesY,&buffer,0,0,buffer.width(),buffer.height());
	return true;

}

// ASSASSINI! LA DOVETE INDENTARE PER BENE :D


static bool objects_kvs_fnc_listObjects(KviKvsModuleFunctionCall * cmd)
{ 
/*
        @doc: objects.dump
        @type:
            function
        @title:
			$objects.dump
        @keyterms:
                dump objects 
        @short:
                Dump the kvirc widgets.
        @syntax:
                $objects.dump(<flag b>)
        @description:
                This function returns the tree of the widgets of the kvirc, if the flag <b> is planned to 1,[br]
                will return it on the currente window, if it' is planned to 0 instead it will return it as a string.[br] 
                ES:
                %A[]=$split("\|",$objects.dump(0))
                %i=0
                while (%i != %A[]#)
                {
                        echo %A[%i]
                        %i++
                }        
*/


	bool bFlag;
	KVSO_PARAMETERS_BEGIN(cmd)
		KVSO_PARAMETER("flag on video",KVS_PT_BOOL,0,bFlag)
	KVSO_PARAMETERS_END(cmd)
	if (bFlag) cmd->window()->output(80, "Objects dump:");
    QWidgetList *l = g_pApp->topLevelWidgets();
	l->setAutoDelete(false);
    QWidgetListIt it(*l);
    KviStr spacing = ">";
	QString  szTemp;
	KviKvsArray * n = new KviKvsArray();
	int idx=0;
    while( it.current() )
    {
		if( it.current()->isWidgetType())
        {
			if (bFlag)
			{
				cmd->window()->output(80, "Ptr %u: top level object: %c%s%c, class %s, %s, rect = %d, %d, %d, %d",
                it.current(),
                KVI_TEXT_BOLD, it.current()->name(), KVI_TEXT_BOLD,
                it.current()->className(),
                it.current()->isVisible() ? "visible" : "hidden",
                it.current()->x(), it.current()->y(), it.current()->width(), it.current()->height());
			}
			QString  szClass=it.current()->className();
			QString szObj=it.current()->name();
			QString szTemp;
			szTemp = szClass + "::" + szObj;
			KviKvsVariant v;
			v.setString(szTemp);
			n->set(idx,new KviKvsVariant(v));
			debug ("string %s",szTemp.latin1());
			debug ("class %s",szClass.latin1());
			debug ("Obj %s",szObj.latin1());
	
			idx++;
	
			dumpChildObjects(cmd->window(), it.current(), spacing.ptr(), bFlag,n,idx);
		
	
        }
        ++it;
	}
    cmd->returnValue()->setArray(n);
    delete l;
    return true;
                
}

static void dumpChildObjects(KviWindow *pWnd, QObject *parent, const char *spacing, bool bFlag, KviKvsArray *n, int &idx)
{

        const QObjectList *l = parent->children();
    	if( !l ) return;
        if( l->isEmpty() ) return;
        QObjectListIt it(*l);
        QString sp(spacing);
        sp.append(">");
        while( it.current() ) 
                {
                
                if( it.current()->isWidgetType() ) 
				{
					if (bFlag)
					{
                        pWnd->output(80, "%sPtr %u: object: %c%s%c, class %s",
                        spacing, it.current(), KVI_TEXT_BOLD,
                        it.current()->name(), KVI_TEXT_BOLD, it.current()->className()
                        );
					}
				QString szClass=it.current()->className();
                QString szObj=it.current()->name();
				QString szTemp;
				szTemp=spacing+szClass+"::"+szObj;
				KviKvsVariant v;
				v.setString(szTemp);
				n->set(idx,new KviKvsVariant(v));
				debug ("string %s",szTemp.latin1());
				idx++;
                dumpChildObjects(pWnd, it.current(), sp, bFlag, n, idx );
                }
          ++it;
        }
}

static bool objects_module_can_unload(KviModule *m)
{
	// Actually this object remains locked in memory
	// FIXME: unlock when no more instances & derived classes!
	return false;
}
    static bool objects_module_init(KviModule * m)
{
	// functions
	KVSM_REGISTER_FUNCTION(m,"exists",objects_kvs_fnc_exists);
	KVSM_REGISTER_FUNCTION(m,"instances",objects_kvs_fnc_instances);
	KVSM_REGISTER_FUNCTION(m,"dump",objects_kvs_fnc_listObjects);
	// commands
	KVSM_REGISTER_SIMPLE_COMMAND(m,"connect",objects_kvs_cmd_connect);
	KVSM_REGISTER_SIMPLE_COMMAND(m,"disconnect",objects_kvs_cmd_disconnect);
	KVSM_REGISTER_SIMPLE_COMMAND(m,"killClass",objects_kvs_cmd_killClass);
	KVSM_REGISTER_SIMPLE_COMMAND(m,"bitblt",objects_kvs_cmd_bitBlt);
	KVSM_REGISTER_SIMPLE_COMMAND(m,"blend",objects_kvs_cmd_blend);
	KVSM_REGISTER_SIMPLE_COMMAND(m,"clearObjects",objects_kvs_cmd_clearObjects);

	KviKvsObject_list::registerSelf();

	KviKvsObject_widget::registerSelf();
	KviKvsObject_button::registerSelf();
	KviKvsObject_mledit::registerSelf();

	KviKvsObject_textbrowser::registerSelf();
	KviKvsObject_checkbox::registerSelf();
	KviKvsObject_dialog::registerSelf();
	KviKvsObject_groupbox::registerSelf();
	KviKvsObject_label::registerSelf();
	KviKvsObject_vbox::registerSelf();
	KviKvsObject_hbox::registerSelf();
	KviKvsObject_layout::registerSelf();
	KviKvsObject_lcd::registerSelf();
	KviKvsObject_lineedit::registerSelf();
	KviKvsObject_menubar::registerSelf();
	KviKvsObject_workspace::registerSelf();
	


	KviKvsObject_combobox::registerSelf();
	KviKvsObject_listbox::registerSelf();
	KviKvsObject_listview::registerSelf();
	KviKvsObject_listviewitem::registerSelf();
	KviKvsObject_mainwindow::registerSelf();
	KviKvsObject_painter::registerSelf();
	KviKvsObject_pixmap::registerSelf();
	KviKvsObject_popupmenu::registerSelf();
	KviKvsObject_radiobutton::registerSelf();
	KviKvsObject_slider::registerSelf();
	KviKvsObject_spinbox::registerSelf();
	KviKvsObject_tabwidget::registerSelf();
	KviKvsObject_toolbar::registerSelf();
	KviKvsObject_toolbutton::registerSelf();
	KviKvsObject_urlabel::registerSelf();
	KviKvsObject_window::registerSelf();
	KviKvsObject_wizard::registerSelf();
	KviKvsObject_dockwindow::registerSelf();
	KviKvsObject_progressbar::registerSelf();
	KviKvsObject_file::registerSelf();
	KviKvsObject_wrapper::registerSelf();
	KviKvsObject_xmlreader::registerSelf();
	KviKvsObject_image::registerSelf();
	KviKvsObject_socket::registerSelf();
	KviKvsObject_process::registerSelf();

	
	return true;
}

KVIRC_MODULE(
	"Objects",
	"2.0.0",
	"Copyright (C) 2000-2006:\n" \
		"  Szymon Stefanek (pragma at kvirc dot net)\n" \
		"  Krzysztof Godlewski (kristoff at poczta dot wprost dot pl)\n" \
		"  Tonino Imbesi (grifisx at barmes dot org)\n" \
		"  Alessandro Carbone (noldor at barmes dot org)",
	"Object classes for KVIrc scripting language\n",
	objects_module_init,
	objects_module_can_unload,
	0,
	objects_module_cleanup
)
