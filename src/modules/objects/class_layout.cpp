//=============================================================================
//
//   File : class_layout.cpp
//   Creation date : Fri Now 22 2002 00:50:01 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2002-2009 Szymon Stefanek (pragma at kvirc dot net)
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
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================

#define _KVI_DEBUG_CHECK_RANGE_
#include "kvi_debug.h"
#include "kvi_locale.h"
#include "class_layout.h"

#include <QGridLayout>



// Tables used in $setAlignment & $alignment
const char * const align_tbl[] = {
			"Left",
			"Right",
			"HCenter",
			"VCenter",
			"Center",
			"Top",
			"Bottom",
			   };



const int align_cod[] = {
                Qt::AlignLeft,
		Qt::AlignRight,
	    Qt::AlignHCenter,
	    Qt::AlignVCenter,
	    Qt::AlignCenter,
	 	Qt::AlignTop,
	    Qt::AlignBottom,
	};
#define align_num	(sizeof(align_tbl) / sizeof(align_tbl[0]))
/*
	@doc: layout
	@keyterms:
		layout object class, child widgets
	@title:
		layout class
	@type:
		class
	@short:
		Manages child widget geometry
	@inherits:
		[class]object[/class]
	@description:
		The layout is a geometry management tool for child widgets.
		You create a layout , give it some widgets to manage and it will layout them
		automatically.[br]
		The parent of the layout must be the widget for which child widget geometries have to be managed.
		A layout is a grid of NxM cells in which you insert child widgets with [classfnc:layout]$addWidget[/classfnc]().[br]
		Widgets that must span multiple cells can be added to the layout with [classfnc:layout]$addMultiCellWidget[/classfnc]().[br]
	@functions:
		!fn: $addWidget(<widget:object widget>,<row:uint>,<column:uint>)
		Adds a widget to this layout placing it at position <row>,<column> in the grid
		!fn: $addMultiCellWidget(<widget:object widget>,<start_row:uint>,<end_row:uint>,<start_col:uint>,<end_col:uint>)
		Adds a widget to this layout spanning multiple grid cells
		!fn: $setRowStretch(<row:uint>,<stretch:uint>)
		Sets the stretch value for a particular row of this layout. The <stretch_value>
		must be a positive integer. The rows with bigger stretch values will take more space
		in the layout.
                !fn: $setColumnStretch(<column:uint>,<stretch:uint>)
		Sets the stretch value for a particular column in this layout. The <stretch_value>
		must be a positive integer. The rows with bigger stretch values will take more space
		in the layout.
		!fn: $addRowSpacing(<row:uint>,<spacing:uint>)
		Sets the minimum height of the specified <row> to <spacing> which must be a positive integer
		!fn: $addColSpacing(<column:uint>,<spacing:uint>)
		Sets the minimum width of the specigfied <column> to <spacing> which must be a positive integer
		!fn: $setSpacing(<spacing:uint>)
		Sets the default spacing of the widgets in pixels
		!fn: $setMargin(<margin:uint>)
		Sets the dimension of the layout margin : the distance from the border to the outermost child widget edges.
		!fn: $setAlignment(<flag1:string>, <flag2:string>, ...)
		Sets the alignment for widget w to  flags, given as parameters.
		Valid flags are:Right,Left,Top,Bottom,HCenter,VCenter,Center
		!fn: $setResizeMode(<resize_mode:string>)
		Sets the resize mode of the parent widget in relation to this layout.
		<mode> can be one of:[br]
		-Auto: this is the default[br]
		-Fixed: the parent widget of this layout is resized to the "sizeHint" value and it cannot be resized by the user.[br]
		-Minimum: the minimum size of the parent widget of this layout is set to minimumSize() and it cannot be smaller[br]
		-FreeResize: the parent widget of this layout is not constrained at all[br]
*/


KVSO_BEGIN_REGISTERCLASS(KviKvsObject_layout,"layout","object")
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,addWidget)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,addMultiCellWidget)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,setRowStretch)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,setColumnStretch)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,addRowSpacing)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,addColSpacing)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,setMargin)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,setSpacing)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,setResizeMode)
	KVSO_REGISTER_HANDLER_BY_NAME(KviKvsObject_layout,setAlignment )
KVSO_END_REGISTERCLASS(KviKvsObject_layout)

KVSO_BEGIN_CONSTRUCTOR(KviKvsObject_layout,KviKvsObject)

KVSO_END_CONSTRUCTOR(KviKvsObject_layout)


KVSO_BEGIN_DESTRUCTOR(KviKvsObject_layout)

KVSO_END_CONSTRUCTOR(KviKvsObject_layout)

bool KviKvsObject_layout::init(KviKvsRunTimeContext * pContext,KviKvsVariantList *)
{
	QWidget * w = parentScriptWidget();

	if(!w)
	{
		pContext->warning(__tr2qs_ctx("The parent of a layout must be a widget!","objects"));
		return false;
	}
	setObject(new QGridLayout(w));
	((QGridLayout *)object())->setVerticalSpacing(0);
	((QGridLayout *)object())->setHorizontalSpacing(0);
	setObjectName(getName());
	return true;
}

KVSO_CLASS_FUNCTION(layout,addWidget)
{
	CHECK_INTERNAL_POINTER(widget())
	KviKvsObject * pObject;
	kvs_hobject_t hObject;
	kvs_uint_t uCol,uRow;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("widget",KVS_PT_HOBJECT,0,hObject)
		KVSO_PARAMETER("row",KVS_PT_UNSIGNEDINTEGER,0,uRow)
		KVSO_PARAMETER("col",KVS_PT_UNSIGNEDINTEGER,0,uCol)
	KVSO_PARAMETERS_END(c)
	pObject=KviKvsKernel::instance()->objectController()->lookupObject(hObject);
	CHECK_HOBJECT_IS_WIDGET(pObject)
	((QGridLayout *)object())->addWidget(((QWidget *)(pObject->object())),uRow,uCol);
	return true;
}

KVSO_CLASS_FUNCTION(layout,addMultiCellWidget)
{
	CHECK_INTERNAL_POINTER(widget())
	KviKvsObject * pObject;
	kvs_hobject_t hObject;
	kvs_uint_t uStartCol,uStartRow,uEndCol,uEndRow;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("widget",KVS_PT_HOBJECT,0,hObject)
		KVSO_PARAMETER("start_row",KVS_PT_UNSIGNEDINTEGER,0,uStartRow)
		KVSO_PARAMETER("end_row",KVS_PT_UNSIGNEDINTEGER,0,uEndRow)
		KVSO_PARAMETER("start_column",KVS_PT_UNSIGNEDINTEGER,0,uStartCol)
		KVSO_PARAMETER("end_column",KVS_PT_UNSIGNEDINTEGER,0,uEndCol)
	KVSO_PARAMETERS_END(c)
	pObject=KviKvsKernel::instance()->objectController()->lookupObject(hObject);
	CHECK_HOBJECT_IS_WIDGET(pObject)
	// { addWidget(w, fromRow, fromCol, (toRow < 0) ? -1 : toRow - fromRow + 1, (toCol < 0) ? -1 : toCol - fromCol + 1, _align); }
	((QGridLayout *)object())->addWidget(((QWidget *)(pObject->object())),uStartRow, uStartCol, uEndRow - uStartRow + 1, uEndCol - uStartCol + 1);
	return true;
}

KVSO_CLASS_FUNCTION(layout,setRowStretch)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_uint_t uRow,uStretch;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("row",KVS_PT_UNSIGNEDINTEGER,0,uRow)
		KVSO_PARAMETER("stretch",KVS_PT_UNSIGNEDINTEGER,0,uStretch)
	KVSO_PARAMETERS_END(c)
	((QGridLayout *)object())->setRowStretch(uRow,uStretch);
	return true;
}

KVSO_CLASS_FUNCTION(layout,setColumnStretch)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_uint_t uCol,uStretch;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("column",KVS_PT_UNSIGNEDINTEGER,0,uCol)
		KVSO_PARAMETER("stretch",KVS_PT_UNSIGNEDINTEGER,0,uStretch)
	KVSO_PARAMETERS_END(c)
	((QGridLayout *)object())->setColumnStretch(uCol,uStretch);
	return true;
}

KVSO_CLASS_FUNCTION(layout,setMargin)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_uint_t uMargin;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("margin",KVS_PT_UNSIGNEDINTEGER,0,uMargin)
	KVSO_PARAMETERS_END(c)
	((QGridLayout *)object())->setMargin(uMargin);
	return true;
}

KVSO_CLASS_FUNCTION(layout,setSpacing)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_uint_t uSpacing;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("spacing",KVS_PT_UNSIGNEDINTEGER,0,uSpacing)
	KVSO_PARAMETERS_END(c)
    ((QGridLayout *)object())->setSpacing(uSpacing);
	return true;
}

KVSO_CLASS_FUNCTION(layout,addRowSpacing)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_uint_t uSpacing,uRow;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("row",KVS_PT_UNSIGNEDINTEGER,0,uRow)
		KVSO_PARAMETER("spacing",KVS_PT_UNSIGNEDINTEGER,0,uSpacing)
	KVSO_PARAMETERS_END(c)
	//((QGridLayout *)object())->addRowSpacing(uRow,uSpacing);
	((QGridLayout *)object())->addItem(new QSpacerItem(0, uSpacing), uRow, 0);
	return true;
}

KVSO_CLASS_FUNCTION(layout,addColSpacing)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_uint_t uSpacing,uCol;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("column",KVS_PT_UNSIGNEDINTEGER,0,uCol)
		KVSO_PARAMETER("spacing",KVS_PT_UNSIGNEDINTEGER,0,uSpacing)
	KVSO_PARAMETERS_END(c)
	((QGridLayout *)object())->addItem(new QSpacerItem(uSpacing,0), 0, uCol);
	return true;
}

KVSO_CLASS_FUNCTION(layout,setResizeMode)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szMode;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("resize_mode",KVS_PT_STRING,0,szMode)
	KVSO_PARAMETERS_END(c)
	QLayout::SizeConstraint r = QLayout::SetDefaultConstraint;
	if(KviQString::equalCI(szMode,"FreeResize")) r = QLayout::SetNoConstraint;
	else if(KviQString::equalCI(szMode,"Minimum")) r = QLayout::SetMinimumSize;
	else if(KviQString::equalCI(szMode,"Fixed"))r = QLayout::SetFixedSize;
	else c->warning(__tr2qs_ctx("Invalid resize mode defaulting to Auto","objects"));
	((QGridLayout *)object())->setSizeConstraint(r);
	return true;

}

KVSO_CLASS_FUNCTION(layout,setAlignment)
{
	CHECK_INTERNAL_POINTER(widget())
	QStringList alignment;
	KviKvsObject * pObject;
	kvs_hobject_t hObject;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("widget",KVS_PT_HOBJECT,0,hObject)
		KVSO_PARAMETER("alignment",KVS_PT_STRINGLIST,KVS_PF_OPTIONAL,alignment)
	KVSO_PARAMETERS_END(c)
	pObject=KviKvsKernel::instance()->objectController()->lookupObject(hObject);
	CHECK_HOBJECT_IS_WIDGET(pObject)
	int index=((QGridLayout *)widget())->indexOf(((QWidget *)(pObject->object())));
	if(index ==-1)
	{
		c->warning(__tr2qs_ctx("The widget must be a child of this layout","objects"));
		return true;
	}

	int align,sum=0;
	for ( QStringList::Iterator it = alignment.begin(); it != alignment.end(); ++it )
	{
		align = 0;
		for(unsigned int j = 0; j < align_num; j++)
		{
			if(KviQString::equalCI((*it), align_tbl[j]))
			{
				align=align_cod[j];
				break;
			}
		}
		if(align)
			sum = sum | align;
		else
			c->warning(__tr2qs_ctx("Unknown alignment: '%Q'","objects"),&(*it));

	}
	if (widget()) ((QGridLayout *)widget())->setAlignment(((QWidget *)(pObject->object())),(Qt::Alignment)sum);
	return true;
}
#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "m_class_layout.moc"
#endif //!COMPILE_USE_STANDALONE_MOC_SOURCES
