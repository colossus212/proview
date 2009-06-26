/* 
 * Proview   $Id$
 * Copyright (C) 2005 SSAB Oxel�sund AB.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation, either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with the program, if not, write to the Free Software 
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

package jpwr.jop;
import jpwr.rt.*;
import java.awt.event.*;

public class GeDynSetValue extends GeDynElem {
  String attribute;
  String value;

  public GeDynSetValue( GeDyn dyn, String attribute, String value) {
    super( dyn, GeDyn.mDynType_No, GeDyn.mActionType_SetValue);
    this.attribute = attribute;
    this.value = value;
  }
  public void action( int eventType, MouseEvent e) {
    switch ( eventType) {
    case GeDyn.eEvent_MB1Down:
      dyn.comp.setColorInverse( 1);
      dyn.repaintNow = true;
      break;
    case GeDyn.eEvent_MB1Up:
      dyn.comp.setColorInverse( 0);
      dyn.repaintNow = true;
      break;
    case GeDyn.eEvent_MB1Click:
      if ( (dyn.actionType & GeDyn.mActionType_Confirm) != 0)
	break;

      PwrtStatus sts;
      boolean localDb = dyn.isLocalDb(attribute);
      int typeId = GeDyn.getTypeId( attribute);

      try {

	if ( typeId == Pwr.eType_Float32) {
	  float inputValue = Float.parseFloat( value);
	  String attrName = dyn.getAttrNameNoSuffix( attribute);
	  if ( !localDb)
	      sts = dyn.en.gdh.setObjectInfo( attrName, inputValue);
	  else
	      sts = dyn.en.ldb.setObjectInfo( dyn.comp.dynamicGetRoot(), attrName, inputValue);
	  if ( sts.evenSts())
	      System.out.println( "setObjectInfoError " + sts);
	}
	else if ( typeId == Pwr.eType_Int32 ||
		  typeId == Pwr.eType_UInt32 ||
		  typeId == Pwr.eType_Int16 ||
		  typeId == Pwr.eType_UInt16 ||
		  typeId == Pwr.eType_Int8 ||
		  typeId == Pwr.eType_UInt8) {
	  int inputValue = Integer.parseInt( value, 10);
	  String attrName = dyn.getAttrNameNoSuffix( attribute);        
	  if ( !localDb)
	      sts = dyn.en.gdh.setObjectInfo( attrName, inputValue);
	  else
	      sts = dyn.en.ldb.setObjectInfo( dyn.comp.dynamicGetRoot(), attrName, inputValue);
	  if ( sts.evenSts())
	      System.out.println( "setObjectInfoError " + sts);
	}
	else if ( typeId == Pwr.eType_Boolean) {
	  int inputValueInt = Integer.parseInt( value, 10);
	  boolean inputValue;
	  if ( inputValueInt == 0)
	    inputValue = false;
	  else if ( inputValueInt == 1)
	    inputValue = true;
          else
	    break;

	  String attrName = dyn.getAttrNameNoSuffix( attribute);
	  if ( !localDb)
	    sts = dyn.en.gdh.setObjectInfo( attrName, inputValue);
	  else
	    sts = dyn.en.ldb.setObjectInfo( dyn.comp.dynamicGetRoot(), attrName, inputValue);
	  if ( sts.evenSts())
	    System.out.println( "setObjectInfoError " + sts);
	}
	else if ( typeId == Pwr.eType_String) {
	  String attrName = dyn.getAttrNameNoSuffix( attribute);        
	  if ( !localDb)
	    sts = dyn.en.gdh.setObjectInfo( attrName, value);
	  else
	    sts = dyn.en.gdh.setObjectInfo( attrName, value);
	  if ( sts.evenSts())
	    System.out.println( "setObjectInfoError " + sts);
	}
      }
      catch(NumberFormatException ex) {
	  System.out.println( ex.toString() );
      }

      break;
    }
  }
}





