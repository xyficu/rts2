/* 
 * Client for filter wheel attached to the camera.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "cliwheel.h"
#include "rts2command.h"

using namespace rts2camd;

ClientFilterCamera::ClientFilterCamera (Rts2Conn * conn):rts2core::Rts2DevClientFilter (conn)
{
	activeConn = NULL;
}

ClientFilterCamera::~ClientFilterCamera (void)
{
	getMaster ()->postEvent (new Rts2Event (EVENT_FILTER_MOVE_END, activeConn));
	activeConn = NULL;
}

void ClientFilterCamera::filterMoveEnd ()
{
	getMaster ()->postEvent (new Rts2Event (EVENT_FILTER_MOVE_END, activeConn));
	activeConn = NULL;
	rts2core::Rts2DevClientFilter::filterMoveEnd ();
}

void ClientFilterCamera::filterMoveFailed (int status)
{
	getMaster ()->postEvent (new Rts2Event (EVENT_FILTER_MOVE_END, activeConn));
	activeConn = NULL;
	rts2core::Rts2DevClientFilter::filterMoveFailed (status);
}

void ClientFilterCamera::postEvent (Rts2Event * event)
{
	struct filterStart *fs;
	switch (event->getType ())
	{
		case EVENT_FILTER_START_MOVE:
			fs = (filterStart *) event->getArg ();
			if (!strcmp (getName (), fs->filterName) && fs->filter >= 0)
			{
				connection->queCommand (new rts2core::Rts2CommandFilter (this, fs->filter));
				fs->filter = -1;
				activeConn = fs->arg;
			}
			break;
		case EVENT_FILTER_GET:
			fs = (filterStart *) event->getArg ();
			if (!strcmp (getName (), fs->filterName))
				fs->filter = getConnection ()->getValueInteger ("filter");
			break;
	}
	rts2core::Rts2DevClientFilter::postEvent (event);
}

void  ClientFilterCamera::valueChanged (rts2core::Value * value)
{
	if (value->getName () == "filter")
	{
		getMaster ()->postEvent (new Rts2Event (EVENT_FILTER_MOVE_END, activeConn));
		activeConn = NULL;
	}
	rts2core::Rts2DevClientFilter::valueChanged (value);
}