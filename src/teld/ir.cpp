#include <sstream>
#include <fstream>

#include "../utils/rts2config.h"
#include "ir.h"

using namespace OpenTPL;

#define LONGITUDE -3.3847222
#define LATITUDE 37.064167
#define ALTITUDE 2896
#define BLIND_SIZE 1.0

ErrorTime::ErrorTime (int in_error)
{
  error = in_error;
  time (&etime);
}

int
ErrorTime::clean (time_t now)
{
  if (now - etime > 600)
    return 1;
  return 0;
}

int
ErrorTime::isError (int in_error)
{
  if (error == in_error)
    {
      time (&etime);
      return 1;
    }
  return 0;
}

template < typename T > int
Rts2DevTelescopeIr::tpl_get (const char *name, T & val, int *status)
{
  int cstatus;

  if (!*status)
    {
      Request & r = tplc->Get (name, false);
      cstatus = r.Wait (USEC_SEC);

      if (cstatus != TPLC_OK)
	{
	  logStream (MESSAGE_ERROR) << "IR tpl_get error " << cstatus <<
	    sendLog;
	  *status = 1;
	}

      if (!*status)
	{
	  RequestAnswer & answr = r.GetAnswer ();

	  if (answr.begin ()->second.first == TPL_OK)
	    val = (T) answr.begin ()->second.second;
	  else
	    *status = 2;
	}
      r.Dispose ();
    }
  return *status;
}

template < typename T > int
Rts2DevTelescopeIr::tpl_set (const char *name, T val, int *status)
{
  if (!*status)
    {
      tplc->Set (name, Value (val), true);	// change to set...?
    }
  return *status;
}

template < typename T > int
Rts2DevTelescopeIr::tpl_setw (const char *name, T val, int *status)
{
  int cstatus;

  if (!*status)
    {
      Request & r = tplc->Set (name, Value (val), false);	// change to set...?
      cstatus = r.Wait ();

      if (cstatus != TPLC_OK)
	{
	  logStream (MESSAGE_ERROR) << "IR tpl_setw error " << cstatus <<
	    sendLog;
	  *status = 1;
	}
      r.Dispose ();
    }
  return *status;
}

int
Rts2DevTelescopeIr::coverClose ()
{
  int status = 0;
  double targetPos;
  if (cover_state == CLOSED)
    return 0;
  status = tpl_set ("COVER.TARGETPOS", 0, &status);
  status = tpl_set ("COVER.POWER", 1, &status);
  status = tpl_get ("COVER.TARGETPOS", targetPos, &status);
  cover_state = CLOSING;
  logStream (MESSAGE_DEBUG) << "IR coverClose status " << status <<
    " target position " << targetPos << sendLog;
  return status;
}

int
Rts2DevTelescopeIr::coverOpen ()
{
  int status = 0;
  if (cover_state == OPENED)
    return 0;
  status = tpl_set ("COVER.TARGETPOS", 1, &status);
  status = tpl_set ("COVER.POWER", 1, &status);
  cover_state = OPENING;
  logStream (MESSAGE_DEBUG) << "IR coverOpen status " << status << sendLog;
  return status;
}

Rts2DevTelescopeIr::Rts2DevTelescopeIr (int in_argc, char **in_argv):Rts2DevTelescope (in_argc,
		  in_argv)
{
  ir_ip = NULL;
  ir_port = 0;
  tplc = NULL;

  irTracking = 4;
  irConfig = "/etc/rts2/ir.ini";

  rotatorOffset = 0;

  addOption ('I', "ir_ip", 1, "IR TCP/IP address");
  addOption ('P', "ir_port", 1, "IR TCP/IP port number");
  addOption ('t', "ir_tracking", 1,
	     "IR tracking (1, 2, 3 or 4 - read OpenTCI doc; default 4");
  addOption ('c', "ir_config", 1,
	     "IR config file (with model, used for load_model/save_model");
  addOption ('r', "rotator_offset", 1, "rotator offset, default to 0");

  strcpy (telType, "BOOTES_IR");
  strcpy (telSerialNumber, "001");

  cover = 0;
  cover_state = CLOSED;
}

Rts2DevTelescopeIr::~Rts2DevTelescopeIr (void)
{
  delete ir_ip;
  delete tplc;
}

int
Rts2DevTelescopeIr::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'I':
      ir_ip = new std::string (optarg);
      break;
    case 'P':
      ir_port = atoi (optarg);
      break;
    case 't':
      irTracking = atoi (optarg);
      break;
    case 'c':
      irConfig = optarg;
      break;
    case 'r':
      rotatorOffset = atof (optarg);
      break;
    default:
      return Rts2DevTelescope::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevTelescopeIr::initDevice ()
{
  Rts2Config *config = Rts2Config::instance ();
  config->loadFile (NULL);
  // try to get default from config file
  if (!ir_ip)
    config->getString ("ir", "ip", &ir_ip);
  if (!ir_port)
    config->getInteger ("ir", "port", ir_port);
  if (!ir_ip || !ir_port)
    {
      fprintf (stderr, "Invalid port or IP address of mount controller PC\n");
      return -1;
    }

  tplc = new Client (*ir_ip, ir_port);

  syslog (LOG_DEBUG,
	  "Status: ConnID = %i, connected: %s, authenticated %s, Welcome Message = %s",
	  tplc->ConnID (), (tplc->IsConnected ()? "yes" : "no"),
	  (tplc->IsAuth ()? "yes" : "no"), tplc->WelcomeMessage ().c_str ());

  // are we connected ?
  if (!tplc->IsAuth () || !tplc->IsConnected ())
    {
      syslog (LOG_ERR, "Connection to server failed");
      return -1;
    }
  return 0;
}

int
Rts2DevTelescopeIr::init ()
{
  int ret;
  ret = Rts2DevTelescope::init ();
  if (ret)
    return ret;

  ret = initDevice ();
  if (ret)
    return ret;

  initCoverState ();
  return 0;
}

// decode IR error
int
Rts2DevTelescopeIr::getError (int in_error, std::string & desc)
{
  char *txt;
  std::string err_desc;
  std::ostringstream os;
  int status;
  int errNum = in_error & 0x00ffffff;
  asprintf (&txt, "CABINET.STATUS.TEXT[%i]", errNum);
  status = 0;
  status = tpl_get (txt, err_desc, &status);
  if (status)
    os << "Telescope getting error: " << status
      << " sev:" << std::hex << (in_error & 0xff000000)
      << " err:" << std::hex << errNum;
  else
    os << "Telescope sev: " << std::hex << (in_error & 0xff000000)
      << " err:" << std::hex << errNum << " desc: " << err_desc;
  free (txt);
  desc = os.str ();
  return status;
}

void
Rts2DevTelescopeIr::addError (int in_error)
{
  std::string desc;
  std::list < ErrorTime * >::iterator errIter;
  ErrorTime *errt;
  int status;
  int errNum = in_error & 0x00ffffff;
  // try to park if on limit switches..
  if (errNum == 88 || errNum == 89)
    {
      double zd;
      status = 0;
      logStream (MESSAGE_ERROR) << "IR checkErrors soft limit in ZD (" <<
	errNum << ")" << sendLog;
      status = tpl_get ("ZD.CURRPOS", zd, &status);
      if (!status)
	{
	  if (zd < -80)
	    zd = -20;
	  if (zd > 80)
	    zd = 20;
	  status = tpl_set ("POINTING.TRACK", 0, &status);
	  logStream (MESSAGE_DEBUG) << "IR checkErrors set pointing status "
	    << status << sendLog;
	  sleep (1);
	  status = tpl_set ("ZD.TARGETPOS", zd, &status);
	  logStream (MESSAGE_ERROR) << "IR checkErrors zd soft limit reset "
	    << zd << " (" << status << ")" << sendLog;
	  unsetTarget ();
	}
    }
  if ((getState (0) & TEL_MASK_MOVING) != TEL_PARKING)
    {
      if (errNum == 58 || errNum == 59 || errNum == 90 || errNum == 91)
	{
	  // emergency park..
	  Rts2DevTelescope::startPark (NULL);
	}
    }
  for (errIter = errorcodes.begin (); errIter != errorcodes.end (); errIter++)
    {
      errt = *errIter;
      if (errt->isError (in_error))
	return;
    }
  // new error
  getError (in_error, desc);
  logStream (MESSAGE_ERROR) << "IR checkErrors " << desc.c_str () << sendLog;
  errt = new ErrorTime (in_error);
  errorcodes.push_back (errt);
}

void
Rts2DevTelescopeIr::checkErrors ()
{
  int status = 0;
  int listCount;
  time_t now;
  std::list < ErrorTime * >::iterator errIter;
  ErrorTime *errt;

  status = tpl_get ("CABINET.STATUS.LIST_COUNT", listCount, &status);
  if (status == 0 && listCount > 0)
    {
      // print errors to log & ends..
      string::size_type pos = 1;
      string::size_type lastpos = 1;
      std::string list;
      status = tpl_get ("CABINET.STATUS.LIST", list, &status);
      if (status == 0)
	logStream (MESSAGE_ERROR) << "IR checkErrors Telescope errors " <<
	  list.c_str () << sendLog;
      // decode errors
      while (true)
	{
	  int errn;
	  if (lastpos > list.size ())
	    break;
	  pos = list.find (',', lastpos);
	  if (pos == string::npos)
	    pos = list.find ('"', lastpos);
	  if (pos == string::npos)
	    break;		// we reach string end..
	  errn = atoi (list.substr (lastpos, pos - lastpos).c_str ());
	  addError (errn);
	  lastpos = pos + 1;
	}
      tpl_set ("CABINET.STATUS.CLEAR", 1, &status);
    }
  // clean from list old errors..
  time (&now);
  for (errIter = errorcodes.begin (); errIter != errorcodes.end ();)
    {
      errt = *errIter;
      if (errt->clean (now))
	{
	  errIter = errorcodes.erase (errIter);
	  delete errt;
	}
      else
	{
	  errIter++;
	}
    }
}

void
Rts2DevTelescopeIr::checkCover ()
{
  int status = 0;
  switch (cover_state)
    {
    case OPENING:
      tpl_get ("COVER.REALPOS", cover, &status);
      if (cover == 1.0)
	{
	  tpl_set ("COVER.POWER", 0, &status);
#ifdef DEBUG_EXTRA
	  logStream (MESSAGE_DEBUG) << "IR checkCover opened " << status <<
	    sendLog;
#endif
	  cover_state = OPENED;
	  break;
	}
      setTimeout (USEC_SEC);
      break;
    case CLOSING:
      tpl_get ("COVER.REALPOS", cover, &status);
      if (cover == 0.0)
	{
	  tpl_set ("COVER.POWER", 0, &status);
#ifdef DEBUG_EXTRA
	  logStream (MESSAGE_DEBUG) << "IR checkCover closed " << status <<
	    sendLog;
#endif
	  cover_state = CLOSED;
	  break;
	}
      setTimeout (USEC_SEC);
      break;
    case OPENED:
    case CLOSED:
      break;
    }
}

void
Rts2DevTelescopeIr::checkPower ()
{
  int status;
  double power_state;
  double referenced;
  status = 0;
  status = tpl_get ("CABINET.POWER_STATE", power_state, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR checkPower tpl_ret " << status <<
	sendLog;
      return;
    }
  if (power_state == 0)
    {
      status = tpl_setw ("CABINET.POWER", 1, &status);
      status = tpl_get ("CABINET.POWER_STATE", power_state, &status);
      if (status)
	{
	  logStream (MESSAGE_ERROR) << "IR checkPower set power ot 1 ret " <<
	    status << sendLog;
	  return;
	}
      while (power_state == 0.5)
	{
	  logStream (MESSAGE_DEBUG) << "IR checkPower waiting for power up" <<
	    sendLog;
	  sleep (5);
	  status = tpl_get ("CABINET.POWER_STATE", power_state, &status);
	  if (status)
	    {
	      logStream (MESSAGE_ERROR) << "IR checkPower power_state ret " <<
		status << sendLog;
	      return;
	    }
	}
    }
  while (true)
    {
      status = tpl_get ("CABINET.REFERENCED", referenced, &status);
      if (status)
	{
	  logStream (MESSAGE_ERROR) << "IR checkPower get referenced " <<
	    status << sendLog;
	  return;
	}
      if (referenced == 1)
	break;
      logStream (MESSAGE_DEBUG) << "IR checkPower referenced " << referenced
	<< sendLog;
      if (referenced == 0)
	{
	  int reinit = (nextReset == RESET_COLD_START ? 1 : 0);
	  status = tpl_set ("CABINET.REINIT", reinit, &status);
	  if (status)
	    {
	      logStream (MESSAGE_ERROR) << "IR checkPower reinit " << status
		<< sendLog;
	      return;
	    }
	}
      sleep (1);
    }
  // force close of cover..
  initCoverState ();
  switch (getMasterState ())
    {
    case SERVERD_DUSK:
    case SERVERD_NIGHT:
    case SERVERD_DAWN:
      coverOpen ();
      break;
    default:
      coverClose ();
      break;
    }
}

void
Rts2DevTelescopeIr::initCoverState ()
{
  int status = 0;
  tpl_get ("COVER.REALPOS", cover, &status);
  if (cover == 0)
    cover_state = CLOSED;
  else if (cover == 1)
    cover_state = OPENED;
  else
    cover_state = CLOSING;
}

int
Rts2DevTelescopeIr::idle ()
{
  // check for power..
  checkPower ();
  // check for errors..
  checkErrors ();
  checkCover ();
  return Rts2DevTelescope::idle ();
}

int
Rts2DevTelescopeIr::ready ()
{
  return !tplc->IsConnected ();
}

int
Rts2DevTelescopeIr::baseInfo ()
{
  std::string serial;
  int status = 0;

  telLongtitude = LONGITUDE;
  telLatitude = LATITUDE;
  telAltitude = ALTITUDE;
  tpl_get ("CABINET.SETUP.HW_ID", serial, &status);
  strncpy (telSerialNumber, serial.c_str (), 64);
  return 0;
}

int
Rts2DevTelescopeIr::info ()
{
  double zd, az;
#ifdef DEBUG_EXTRA
  double zd_acc, zd_speed, az_acc, az_speed;
#endif // DEBUG_EXTRA
  int status = 0;
  int track = 0;

  if (!(tplc->IsAuth () && tplc->IsConnected ()))
    return -1;

  status = tpl_get ("POINTING.CURRENT.RA", telRa, &status);
  telRa *= 15.0;
  status = tpl_get ("POINTING.CURRENT.DEC", telDec, &status);
  status = tpl_get ("POINTING.TRACK", track, &status);
  if (status)
    return -1;

  telSiderealTime = getLocSidTime ();
  telLocalTime = 0;

  status = tpl_get ("ZD.REALPOS", zd, &status);
  status = tpl_get ("AZ.REALPOS", az, &status);

#ifdef DEBUG_EXTRA
  status = tpl_get ("ZD.CURRSPEED", zd_speed, &status);
  status = tpl_get ("ZD.CURRACC", zd_acc, &status);
  status = tpl_get ("AZ.CURRSPEED", az_speed, &status);
  status = tpl_get ("AZ.CURRACC", az_acc, &status);

  logStream (MESSAGE_DEBUG) << "IR info ra " << telRa << " dec " << telDec <<
    " az " << az << " az_speed " << az_speed << " az_acc " << az_acc << " zd "
    << zd << " zd_speed " << zd_speed << " zd_acc " << zd_acc << " track " <<
    track << sendLog;
#endif // DEBUG_EXTRA

  if (!track)
    {
      // telRA, telDec are invalid: compute them from ZD/AZ
      struct ln_hrz_posn hrz;
      struct ln_lnlat_posn observer;
      struct ln_equ_posn curr;
      hrz.az = az;
      hrz.alt = 90 - fabs (zd);
      observer.lng = telLongtitude;
      observer.lat = telLatitude;
      ln_get_equ_from_hrz (&hrz, &observer, ln_get_julian_from_sys (), &curr);
      telRa = curr.ra;
      telDec = curr.dec;
      telFlip = 0;
    }
  else
    {
      telFlip = 0;
    }

  if (status)
    return -1;


  telAxis[0] = az;
  telAxis[1] = zd;

  return Rts2DevTelescope::info ();
}

int
Rts2DevTelescopeIr::startMoveReal (double ra, double dec)
{
  int status = 0;
  status = tpl_set ("POINTING.TARGET.RA", ra / 15.0, &status);
  status = tpl_set ("POINTING.TARGET.DEC", dec, &status);
  status = tpl_set ("POINTING.TRACK", irTracking, &status);
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "IR startMove TRACK status " << status <<
    sendLog;
#endif
  if (rotatorOffset != 0)
    status = tpl_set ("DEROTATOR[3].OFFSET", rotatorOffset, &status);

  return status;
}

int
Rts2DevTelescopeIr::startMove (double ra, double dec)
{
  int status = 0;
  double sep;

  target.ra = ra;
  target.dec = dec;

  // move to zenit - move to different dec instead
  if (fabs (dec - telLatitude) <= BLIND_SIZE)
    {
      if (fabs (ra / 15.0 - getLocSidTime ()) <= BLIND_SIZE / 15.0)
	{
	  target.dec = telLatitude - BLIND_SIZE;
	}
    }

  double az_off = 0;
  double alt_off = 0;
  status = tpl_set ("AZ.OFFSET", az_off, &status);
  status = tpl_set ("ZD.OFFSET", alt_off, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR startMove cannot zero offset" <<
	sendLog;
      return -1;
    }

  status = startMoveReal (ra, dec);
  if (status)
    return -1;

  // wait till we get it processed
  sep = getMoveTargetSep ();
  if (sep > 2)
    sleep (3);
  else if (sep > 2 / 60.0)
    usleep (USEC_SEC / 10);

  time (&timeout);
  timeout += 120;
  return 0;
}

int
Rts2DevTelescopeIr::isMoving ()
{
  int status = 0;
  double poin_dist;
  time_t now;
  status = tpl_get ("POINTING.TARGETDISTANCE", poin_dist, &status);
  time (&now);
  // 0.01 = 36 arcsec
  if (fabs (poin_dist) <= 0.01)
    {
#ifdef DEBUG_EXTRA
      logStream (MESSAGE_DEBUG) << "IR isMoving target distance " << poin_dist
	<< sendLog;
#endif
      return -2;
    }
  // finish due to timeout
  if (timeout < now)
    {
      logStream (MESSAGE_ERROR) << "IR isMoving target distance in timeout "
	<< poin_dist << "(" << status << ")" << sendLog;
      return -1;
    }
  return USEC_SEC / 100;
}

int
Rts2DevTelescopeIr::startPark ()
{
  int status = 0;
  // Park to south+zenith
  status = tpl_set ("POINTING.TRACK", 0, &status);
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "IR startPark tracking status " << status <<
    sendLog;
#endif
  sleep (1);
  status = tpl_set ("AZ.TARGETPOS", 0, &status);
  status = tpl_set ("ZD.TARGETPOS", 0, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR startPark ZD.TARGETPOS status " <<
	status << sendLog;
      return -1;
    }
  time (&timeout);
  timeout += 300;
  return 0;
}

int
Rts2DevTelescopeIr::isParking ()
{
  return isMoving ();
}

int
Rts2DevTelescopeIr::endPark ()
{
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "IR endPark" << sendLog;
#endif
  return 0;
}

int
Rts2DevTelescopeIr::stopMove ()
{
  int status = 0;
  double zd;
  Rts2DevTelescope::stopMove ();
  info ();
  // ZD check..
  status = tpl_get ("ZD.CURRPOS", zd, &status);
  if (status)
    {
      logStream (MESSAGE_DEBUG) << "IR stopMove cannot get ZD! (" << status <<
	")" << sendLog;
      return -1;
    }
  if (fabs (zd) < 1)
    {
      logStream (MESSAGE_DEBUG) << "IR stopMove suspicious ZD.. " << zd <<
	sendLog;
      status = tpl_set ("POINTING.TRACK", 0, &status);
      if (status)
	{
	  logStream (MESSAGE_DEBUG) << "IR stopMove cannot set track: " <<
	    status << sendLog;
	  return -1;
	}
      return 0;
    }
  startMoveReal (telRa, telDec);
  return 0;
}

int
Rts2DevTelescopeIr::correctOffsets (double cor_ra, double cor_dec,
				    double real_ra, double real_dec)
{
  return -1;
}

int
Rts2DevTelescopeIr::correct (double cor_ra, double cor_dec, double real_ra,
			     double real_dec)
{
  // idea - convert current & astrometry position to alt & az, get
  // offset in alt & az, apply offset
  struct ln_equ_posn eq_astr;
  struct ln_equ_posn eq_target;
  struct ln_hrz_posn hrz_astr;
  struct ln_hrz_posn hrz_target;
  struct ln_lnlat_posn observer;
  double az_off = 0;
  double alt_off = 0;
  double sep;
  double jd = ln_get_julian_from_sys ();
  double quality;
  double zd;
  int sample = 1;
  int status = 0;

  eq_astr.ra = real_ra;
  eq_astr.dec = real_dec;
  eq_target.ra = real_ra + cor_ra;
  eq_target.dec = real_dec + cor_dec;
  applyLocCorr (&eq_target);
  observer.lng = telLongtitude;
  observer.lat = telLatitude;
  ln_get_hrz_from_equ (&eq_astr, &observer, jd, &hrz_astr);
  ln_get_hrz_from_equ (&eq_target, &observer, jd, &hrz_target);
  // calculate alt & az diff
  az_off = hrz_target.az - hrz_astr.az;
  alt_off = hrz_target.alt - hrz_astr.alt;

  status = tpl_get ("ZD.CURRPOS", zd, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR correct cannot get ZD.CURRPOS" <<
	sendLog;
      return -1;
    }
  if (zd > 0)
    alt_off *= -1;		// get ZD offset - when ZD < 0, it's actuall alt offset
  sep = ln_get_angular_separation (&eq_astr, &eq_target);
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "IR correct az_off " << az_off << " zd_off " <<
    alt_off << " sep " << sep << sendLog;
#endif
  if (sep > 2)
    return -1;
  status = tpl_set ("AZ.OFFSET", az_off, &status);
  status = tpl_set ("ZD.OFFSET", alt_off, &status);
  if (isModelOn ())
    return (status ? -1 : 1);
  // sample..
  status = tpl_set ("POINTING.POINTINGPARAMS.SAMPLE", sample, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.CALCULATE", quality, &status);
  logStream (MESSAGE_DEBUG) << "IR correct quality: " << quality << " status "
    << status << sendLog;
  return (status ? -1 : 1);
}

/**
 * OpenTCI/Bootes IR - POINTING.POINTINGPARAMS.xx:
 * AOFF, ZOFF, AE, AN, NPAE, CA, FLEX
 * AOFF, ZOFF = az / zd offset
 * AE = azimut tilt east
 * AN = az tilt north
 * NPAE = az / zd not perpendicular
 * CA = M1 tilt with respect to optical axis
 * FLEX = sagging of tube
 */
int
Rts2DevTelescopeIr::saveModel ()
{
  std::ofstream of;
  int status = 0;
  double aoff, zoff, ae, an, npae, ca, flex;
  status = tpl_get ("POINTING.POINTINGPARAMS.AOFF", aoff, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.ZOFF", zoff, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.AE", ae, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.AN", an, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.NPAE", npae, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.CA", ca, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.FLEX", flex, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR saveModel status: " << status <<
	sendLog;
      return -1;
    }
  of.open ("/etc/rts2/ir.model", ios_base::out | ios_base::trunc);
  of.precision (20);
  of << aoff << " "
    << zoff << " "
    << ae << " "
    << an << " " << npae << " " << ca << " " << flex << " " << std::endl;
  of.close ();
  if (of.fail ())
    {
      logStream (MESSAGE_ERROR) << "IR saveModel file" << sendLog;
      return -1;
    }
  return 0;
}

int
Rts2DevTelescopeIr::loadModel ()
{
  std::ifstream ifs;
  int status = 0;
  double aoff, zoff, ae, an, npae, ca, flex;
  try
  {
    ifs.open ("/etc/rts2/ir.model");
    ifs >> aoff >> zoff >> ae >> an >> npae >> ca >> flex;
  }
  catch (exception & e)
  {
    logStream (MESSAGE_DEBUG) << "IR loadModel error" << sendLog;
    return -1;
  }
  status = tpl_set ("POINTING.POINTINGPARAMS.AOFF", aoff, &status);
  status = tpl_set ("POINTING.POINTINGPARAMS.ZOFF", zoff, &status);
  status = tpl_set ("POINTING.POINTINGPARAMS.AE", ae, &status);
  status = tpl_set ("POINTING.POINTINGPARAMS.AN", an, &status);
  status = tpl_set ("POINTING.POINTINGPARAMS.NPAE", npae, &status);
  status = tpl_set ("POINTING.POINTINGPARAMS.CA", ca, &status);
  status = tpl_set ("POINTING.POINTINGPARAMS.FLEX", flex, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR saveModel status: " << status <<
	sendLog;
      return -1;
    }
  return 0;
}

int
Rts2DevTelescopeIr::stopWorm ()
{
  int status = 0;
  status = tpl_set ("POINTING.TRACK", 0, &status);
  if (status)
    return -1;
  return 0;
}

int
Rts2DevTelescopeIr::startWorm ()
{
  int status = 0;
  status = tpl_set ("POINTING.TRACK", irTracking, &status);
  if (status)
    return -1;
  return 0;
}

int
Rts2DevTelescopeIr::changeMasterState (int new_state)
{
  switch (new_state)
    {
    case SERVERD_DUSK:
    case SERVERD_NIGHT:
    case SERVERD_DAWN:
      coverOpen ();
      break;
    default:
      coverClose ();
      break;
    }
  return Rts2DevTelescope::changeMasterState (new_state);
}

int
Rts2DevTelescopeIr::resetMount (resetStates reset_mount)
{
  int status = 0;
  int power = 0;
  double power_state;
  status = tpl_setw ("CABINET.POWER", power, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR resetMount powering off: " << status <<
	sendLog;
      return -1;
    }
  while (true)
    {
      logStream (MESSAGE_DEBUG) << "IR resetMount waiting for power down" <<
	sendLog;
      sleep (5);
      status = tpl_get ("CABINET.POWER_STATE", power_state, &status);
      if (status)
	{
	  logStream (MESSAGE_ERROR) << "IR resetMount power_state ret: " <<
	    status << sendLog;
	  return -1;
	}
      if (power_state == 0 || power_state == -1)
	{
	  logStream (MESSAGE_DEBUG) << "IR resetMount final power_state: " <<
	    power_state << sendLog;
	  break;
	}
    }
  return Rts2DevTelescope::resetMount (reset_mount);
}
