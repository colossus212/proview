/* 
 * Proview   Open Source Process Control.
 * Copyright (C) 2005-2012 SSAB EMEA AB.
 *
 * This file is part of Proview.
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
 * along with Proview. If not, see <http://www.gnu.org/licenses/>
 *
 * Linking Proview statically or dynamically with other modules is
 * making a combined work based on Proview. Thus, the terms and 
 * conditions of the GNU General Public License cover the whole 
 * combination.
 *
 * In addition, as a special exception, the copyright holders of
 * Proview give you permission to, from the build function in the
 * Proview Configurator, combine Proview with modules generated by the
 * Proview PLC Editor to a PLC program, regardless of the license
 * terms of these modules. You may copy and distribute the resulting
 * combined work under the terms of your choice, provided that every 
 * copy of the combined work is accompanied by a complete copy of 
 * the source code of Proview (the version used to produce the 
 * combined work), being distributed under the terms of the GNU 
 * General Public License plus this exception.
 */

package jpwr.jopg;
import jpwr.rt.*;
import java.io.*;
import java.util.*;

public class GlowCon extends GlowArrayElem {
    public static final int MAX_POINT = 8;

    GrowCmn 	cmn;
    String 	cc_name;
    GlowConClass cc;
    String 	c_name;
    double	x_right;
    double	x_left;
    double	y_high;
    double	y_low;
    GrowNode   	dest_node;
    GrowNode   	source_node;
    int	       	dest_conpoint;
    int	       	source_conpoint;
    int	       	source_direction;
    int	       	dest_direction;
    int	       	p_num;
    int	       	l_num;
    int	       	a_num;
    int	       	arrow_num;
    int	       	ref_num;
    double[]   	point_x = new double[MAX_POINT];
    double[]   	point_y = new double[MAX_POINT];
    Vector<GlowArrayElem> line_a = new Vector<GlowArrayElem>();
    Vector<GlowArrayElem> arc_a = new Vector<GlowArrayElem>();
    Vector<GlowArrayElem> arrow_a = new Vector<GlowArrayElem>();
    Vector<GlowArrayElem> ref_a = new Vector<GlowArrayElem>();
    int	       	temporary_ref;
    int	       	source_ref_cnt;
    int	       	dest_ref_cnt;
    String     	trace_object;
    String     	trace_attribute;
    int	       	trace_attr_type;
    int 	border;
    int		shadow;
    int		highlight;
    int		hot;
    
    public GlowCon(GrowCmn cmn) {
	this.cmn = cmn;
    }

    public int type() {
	return Glow.eObjectType_Con;
    }

    public void open(BufferedReader reader) {
	String line;
	StringTokenizer token;
	boolean end_found = false;
	String name;
	int i;

	try {
	    while( (line = reader.readLine()) != null) {
		token = new StringTokenizer(line);
		int key = Integer.valueOf(token.nextToken());
		if ( cmn.debug) System.out.println( "GlowCon : " + line);

		switch ( key) {
		case Glow.eSave_Con: 
		    break;
		case Glow.eSave_Con_cc: 
		    cc_name = token.nextToken();
		    cc = (GlowConClass)cmn.ctx.get_conclass_from_name( cc_name);
		    break;
		case Glow.eSave_Con_c_name:
		    if ( token.hasMoreTokens())
			c_name = token.nextToken();
		    break;
		case Glow.eSave_Con_x_right: 
		    x_right = new Double(token.nextToken()).doubleValue(); 
		    break;
		case Glow.eSave_Con_x_left: 
		    x_left = new Double(token.nextToken()).doubleValue(); 
		    break;
		case Glow.eSave_Con_y_high: 
		    y_high = new Double(token.nextToken()).doubleValue(); 
		    break;
		case Glow.eSave_Con_y_low: 
		    y_low = new Double(token.nextToken()).doubleValue(); 
		    break;
		case Glow.eSave_Con_dest_node:
		    if ( token.hasMoreTokens()) {
			name = token.nextToken();
			dest_node = (GrowNode) cmn.ctx.get_node_from_name( name);
			if ( dest_node == null)
			    System.out.println("GlowCon:dest_node not found");
		    }
		    break;
		case Glow.eSave_Con_source_node:
		    if ( token.hasMoreTokens()) {
			name = token.nextToken();
			source_node = (GrowNode) cmn.ctx.get_node_from_name( name);
			if ( source_node == null)
			    System.out.println("GlowCon:source_node not found");
		    }
		    break;
		case Glow.eSave_Con_dest_conpoint: 
		    dest_conpoint = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_source_conpoint: 
		    source_conpoint = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_dest_direction: 
		    dest_direction = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_source_direction: 
		    source_direction = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_line_a: 
		    cmn.ctx.openVector( reader, cmn, line_a); 
		    break;
		case Glow.eSave_Con_arc_a: 
		    cmn.ctx.openVector( reader, cmn, arc_a); 
		    break;
		case Glow.eSave_Con_arrow_a: 
		    cmn.ctx.openVector( reader, cmn, arrow_a); 
		    break;
		case Glow.eSave_Con_ref_a: 
		    cmn.ctx.openVector( reader, cmn, ref_a); 
		    break;
		case Glow.eSave_Con_p_num: 
		    p_num = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_l_num: 
		    l_num = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_a_num: 
		    a_num = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_arrow_num: 
		    arrow_num = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_ref_num: 
		    ref_num = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_point_x:
		    for ( i = 0; i < p_num; i++) {
			reader.readLine();
			point_x[i] = new Double(line).doubleValue();
		    }
		    break;
		case Glow.eSave_Con_point_y:
		    for ( i = 0; i < p_num; i++) {
			reader.readLine();
			point_y[i] = new Double(line).doubleValue();
		    }
		    break;
		case Glow.eSave_Con_source_ref_cnt: 
		    source_ref_cnt = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_dest_ref_cnt: 
		    dest_ref_cnt = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_trace_object:
		    if ( token.hasMoreTokens())
			trace_object = token.nextToken();
		    break;
		case Glow.eSave_Con_trace_attribute:
		    if ( token.hasMoreTokens())
			trace_attribute = token.nextToken();
		    break;
		case Glow.eSave_Con_trace_attr_type: 
		    trace_attr_type = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_temporary_ref: 
		    temporary_ref = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_border: 
		    border = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_Con_shadow: 
		    shadow = Integer.valueOf(token.nextToken()); 
		    break;
		case Glow.eSave_End:
		    end_found = true;
		    break;
		default:
		    System.out.println( "Syntax error in GlowCon");
		    break;
		}
		if ( end_found)
		    break;
	    }
		
	} catch ( Exception e) {
	    System.out.println( "IOException GlowCon");
	}
    }

    public void draw() {
	    draw(null, highlight, hot, null, null);
    }

    public void draw(GlowTransform t, int highlight, int hot, Object node, Object colornode) {
	if ( cmn.nodraw != 0)
	    return;

	int tmp;
	int i;

	if ( temporary_ref != 0 || cc.con_type == Glow.eConType_Reference) {
	    // ref_a.draw( w, &cc->zero, highlight, hot, NULL);
	}
	else {
	    for ( i = 0; i < line_a.size(); i++) 
		((GlowLine)line_a.get(i)).draw( highlight, hot);
	    for ( i = 0; i < arc_a.size(); i++)
		((GlowArc)arc_a.get(i)).draw( highlight, hot);
	    // arrow_a.draw( highlight, hot);
	    if ( (shadow != 0 || border != 0) && cc.con_type == Glow.eConType_Routed && cc.corner == Glow.eCorner_Rounded) {
		for ( i = 0; i < line_a.size(); i++)
		    ((GlowLine)line_a.get(i)).draw_shadow( border, shadow, highlight, hot);
		for ( i = 0; i < arc_a.size(); i++)
		    ((GlowArc)arc_a.get(i)).draw_shadow( border, shadow, highlight, hot);
	    }
	}
    }


}