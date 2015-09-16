/* 
 * Abstract class for GEMs.
 * Copyright (C) 2007-2008,2010 Petr Kubanek <petr@kubanek.net>
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

#include "gem.h"
#include "configuration.h"

#include "libnova_cpp.h"

using namespace rts2teld;

int GEM::sky2counts (int32_t & ac, int32_t & dc)
{
	double JD;
	int32_t homeOff;
	struct ln_equ_posn pos;
	int ret;

	JD = ln_get_julian_from_sys ();

	ret = getHomeOffset (homeOff);
	if (ret)
		return -1;

	getTarget (&pos);

	int actual_flip = useParkFlipping ? parkFlip->getValueInteger () : flipping->getValueInteger ();

	return sky2counts (&pos, ac, dc, JD, homeOff, actual_flip);
}

int GEM::sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int32_t homeOff, int actual_flip)
{
	double ls, ha, dec;
	struct ln_hrz_posn hrz;
	int ret;

	int32_t t_ac, t_dc;

	if (isnan(pos->ra) || isnan(pos->dec))
	{
		logStream (MESSAGE_ERROR) << "sky2counts called with nan ra/dec" << sendLog;
		return -1;
	}
	ls = getLstDeg (JD);

	ln_get_hrz_from_equ (pos, rts2core::Configuration::instance ()->getObserver (), JD, &hrz);
	if (hrz.alt < -1)
	{
		logStream (MESSAGE_ERROR) << "object is below horizon, azimuth is "
			<< hrz.az  << " and altitude " << hrz.alt  << " RA/DEC targets was " << LibnovaRaDec (pos)
			<< ", check observatory time and location (long & latitude)"
			<< sendLog;
		return -1;
	}

	// get hour angle
	ha = ln_range_degrees (ls - pos->ra);
	if (ha > 180.0)
		ha -= 360.0;

	// pretend we are at north hemispehere.. at least for dec
	dec = pos->dec;
	if (telLatitude->getValueDouble () < 0)
		dec *= -1;

	// convert to count values
	t_ac = (int32_t) ((ha - haZero->getValueDouble ()) * haCpd->getValueDouble ());
	t_dc = (int32_t) ((dec - decZero->getValueDouble ()) * decCpd->getValueDouble ());

	// gets the limits
	ret = updateLimits ();
	if (ret)
	{
		return -1;
	}

	// if we cannot move with those values, we cannot move with the any other more optiomal setting, so give up
	ret = checkCountValues (pos, ac, dc, t_ac, t_dc, JD, ls, dec);

	// let's see what will different flip do..
	int32_t tf_ac = t_ac - (int32_t) (fabs(haCpd->getValueDouble () * 360.0)/2.0);
	int32_t tf_dc = t_dc + (int32_t) ((90 - dec) * 2 * decCpd->getValueDouble ());

	int ret_f = checkCountValues (pos, ac, dc, tf_ac, tf_dc, JD, ls, dec);

	// there isn't path, give up...
	if (ret != 0 && ret_f != 0)
		return -1;
	// only flipped..
	else if (ret != 0 && ret_f == 0)
	{
		t_ac = tf_ac;
		t_dc = tf_dc;
	}
	// both ways are possible, decide base on flipping parameter
	else if (ret == 0 && ret_f == 0)
	{
#define max(a,b) ((a) > (b) ? (a) : (b))
		switch (actual_flip)
		{
			// shortest
			case 0:
				{
					int32_t diff_nf = max (fabs (ac - t_ac), fabs (dc - t_dc));
					int32_t diff_f = max (fabs (ac - tf_ac), fabs (dc - tf_dc));

					if (diff_f < diff_nf)
					{
						t_ac = tf_ac;
						t_dc = tf_dc;
					}
				}
				break;
			// same
			case 1:
				if (flip_move_start == 1)
				{
					t_ac = tf_ac;
					t_dc = tf_dc;
				}
				break;
			// opposite
			case 2:
				if (flip_move_start == 0)
				{
					t_ac = tf_ac;
					t_dc = tf_dc;
				}
				break;
			// west
			case 3:
				break;
			// east
			case 4:
				t_ac = tf_ac;
				t_dc = tf_dc;
				break;
			// longest
			case 5:
				{
					switch (flip_longest_path)
					{
						case 0:
							break;
						case 1:
							t_ac = tf_ac;
							t_dc = tf_dc;
							break;
						default:
							{
								int32_t diff_nf = max (fabs (ac - t_ac), fabs (dc - t_dc));
								int32_t diff_f = max (fabs (ac - tf_ac), fabs (dc - tf_dc));

								if (diff_f > diff_nf)
								{
									t_ac = tf_ac;
									t_dc = tf_dc;
									flip_longest_path = 1;
								}
								else
								{
									flip_longest_path = 0;
								}
							}
					}
				}
				break;
			// counterweight down
			case 6:
			// counterweight up
			case 7:
				// calculate distance towards "meridian" - counterweight down - position
				// normalize distances by ra_ticks
				double diff_nf = fabs (fmod (getHACWDAngle (t_ac), 360));
				double diff_f = fabs (fmod (getHACWDAngle (tf_ac), 360));
				// normalize to degree distance to HA
				if (diff_nf > 180)
					diff_nf = 360 - diff_nf;
				if (diff_f > 180)
					diff_f = 360 - diff_f;
				logStream (MESSAGE_DEBUG) << "cw diffs flipped " << diff_f << " nf " << diff_nf << sendLog;
				if (actual_flip == 6)
				{
					if (diff_f < diff_nf)
					{
						t_ac = tf_ac;
						t_dc = tf_dc;
					}
				}
				else
				{
					if (diff_f > diff_nf)
					{
						t_ac = tf_ac;
						t_dc = tf_dc;
					}
				}
		}
	}
	// otherwise, non-flipped is the only way, stay on it..

	if ((t_dc < dcMin->getValueLong ()) || (t_dc > dcMax->getValueLong ()))
	{
		logStream (MESSAGE_ERROR) << "target declination position is outside limits. RA/DEC target "
			<< LibnovaRaDec (pos) << " dc:" << t_dc << " dcMin:" << dcMin->getValueLong () << " dcMax:" << dcMax->getValueLong () << sendLog;
		return -1;
	}

	if ((t_ac < acMin->getValueLong ()) || (t_ac > acMax->getValueLong ()))
	{
		logStream (MESSAGE_ERROR) << "target RA position is outside limits. RA/DEC target "
			<< LibnovaRaDec (pos) << " ac:" << t_ac << " acMin:" << acMin->getValueLong () << " acMax:" << acMax->getValueDouble () << sendLog;
		return -1;
	}

	t_ac -= homeOff;

	ac = t_ac;
	dc = t_dc;

	return 0;
}

int GEM::sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc)
{
	int actual_flip = useParkFlipping ? parkFlip->getValueInteger () : flipping->getValueInteger ();

	// returns without home offset, which will be removed in future
	return sky2counts (pos, ac, dc, JD, 0, actual_flip);
}

int GEM::counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec, double JD)
{
	double ls, ha;
	int32_t homeOff;
	int ret;

	ret = getHomeOffset (homeOff);
	if (ret)
		return -1;

	ls = getLstDeg (JD);

	ac += homeOff;

	ha = (double) (ac / haCpd->getValueDouble ()) + haZero->getValueDouble ();
	dec = (double) (dc / decCpd->getValueDouble ()) + decZero->getValueDouble ();

	ra = ls - ha;

	un_ra = ra;
	un_dec = dec;

	// flipped
	if (fabs (dec) > 90.0)
	{
		while (fabs (dec) > 90)
		{
			flip = flip ? 0 : 1;
			if (dec > 0)
				dec = 180.0 - dec;
			else
				dec = -180.0 - dec;
			ra += 180.0;
		}
	}
	else
	{
		flip = 0;
	}

	dec = ln_range_degrees (dec);
	if (dec > 180.0)
		dec -= 360.0;

	ra = ln_range_degrees (ra);

	if (telLatitude->getValueDouble () < 0)
	{
		dec *= -1;
		un_dec *= -1;
	}

	return 0;
}

GEM::GEM (int in_argc, char **in_argv, bool diffTrack, bool hasTracking, bool hasUnTelCoordinates):Telescope (in_argc, in_argv, diffTrack, hasTracking, hasUnTelCoordinates)
{
	createValue (flipping, "FLIPPING", "mount flipping strategy", false, RTS2_VALUE_WRITABLE);
	flipping->addSelVal ("shortest");
	flipping->addSelVal ("same");
	flipping->addSelVal ("opposite");
	flipping->addSelVal ("west");
	flipping->addSelVal ("east");
	flipping->addSelVal ("longest");
	flipping->addSelVal ("cw down");
	flipping->addSelVal ("cw up");

	createValue (haCWDAngle, "ha_cwd_angle", "[deg] angle between HA axis and local meridian", false);

	createValue (haZeroPos, "_ha_zero_pos", "position of the telescope on zero", false);
	haZeroPos->addSelVal ("EAST");
	haZeroPos->addSelVal ("WEST");

	createValue (haZero, "_ha_zero", "HA zero offset", false);
	createValue (decZero, "_dec_zero", "DEC zero offset", false);

	createValue (haCpd, "_ha_cpd", "HA counts per degree", false);
	createValue (decCpd, "_dec_cpd", "DEC counts per degree", false);

	createValue (acMin, "_ac_min", "HA minimal count value", false);
	createValue (acMax, "_ac_max", "HA maximal count value", false);
	createValue (dcMin, "_dc_min", "DEC minimal count value", false);
	createValue (dcMax, "_dc_max", "DEC maximal count value", false);

	createValue (ra_ticks, "_ra_ticks", "RA ticks per full loop (no effect)", false);
	createValue (dec_ticks, "_dec_ticks", "DEC ticks per full loop (no effect)", false);
}

GEM::~GEM (void)
{

}

void GEM::unlockPointing ()
{
	haZeroPos->setWritable ();
	haZero->setWritable ();
	decZero->setWritable ();
	haCpd->setWritable ();
	decCpd->setWritable ();

	acMin->setWritable ();
	acMax->setWritable ();
	dcMin->setWritable ();
	dcMax->setWritable ();

	ra_ticks->setWritable ();
	dec_ticks->setWritable ();
	
	updateMetaInformations (haZero);
	updateMetaInformations (decZero);
	updateMetaInformations (haCpd);	
	updateMetaInformations (decCpd);

	updateMetaInformations (acMin);	
	updateMetaInformations (acMax);	
	updateMetaInformations (dcMin);	
	updateMetaInformations (dcMax);

	updateMetaInformations (ra_ticks);
	updateMetaInformations (dec_ticks);
}

double GEM::getHACWDAngle (int32_t ha_count)
{
	// sign of haCpd
	int haCpdSign = haCpd->getValueDouble () > 0 ? 1 : -1;
	switch (haZeroPos->getValueInteger ())
	{
		// TODO west (haZeroPos == 1), haCpd >0 is the only proved combination (and haZero was negative, but that should not play any role..).
		// Other values must be confirmed
		case 1:
			return -360.0 * ((ha_count + (haZero->getValueDouble () + haCpdSign * 90) * haCpd->getValueDouble ()) / ra_ticks->getValueDouble ());
		case 0:
		default:
			return 360.0 * ((ha_count + (haZero->getValueDouble () - haCpdSign * 90) * haCpd->getValueDouble ()) / ra_ticks->getValueDouble ());
	}
}

int GEM::checkCountValues (struct ln_equ_posn *pos, int32_t ac, int32_t dc, int32_t &t_ac, int32_t &t_dc, double JD, double ls, double dec)
{
	struct ln_equ_posn model_change;
	struct ln_equ_posn u_pos;

	int32_t full_ac = (int32_t) fabs(haCpd->getValueDouble () * 360.0);
	int32_t full_dc = (int32_t) fabs(decCpd->getValueDouble () * 360.0);

	int32_t half_ac = (int32_t) fabs(haCpd->getValueDouble () * 180.0);
	int32_t half_dc = (int32_t) fabs(decCpd->getValueDouble () * 180.0);

	// minimize movement from current position, don't rotate around axis more than once
	int32_t diff_ac = (ac - t_ac) % full_ac;
	int32_t diff_dc = (dc - t_dc) % full_dc;


	// still, when diff is bigger than half a circle, shorter path is opposite direction what's left
	if (diff_ac > half_ac)
		diff_ac -= full_ac;
	if (diff_dc < -half_ac)
		diff_ac += full_ac;

	if (diff_dc > half_dc)
		diff_dc -= full_dc;
	if (diff_dc < -half_dc)
		diff_dc += full_dc;

	t_ac = ac - diff_ac;
	t_dc = dc - diff_dc;

	// purpose of the following code is to get from west side of flip
	// on S, we prefer negative values
	if (telLatitude->getValueDouble () < 0)
	{
		while ((t_ac - acMargin) < acMin->getValueLong ())
		// ticks per revolution - don't have idea where to get that
		{
			t_ac += (int32_t)fabs(haCpd->getValueDouble () * 360.0);
		}
	}
	while ((t_ac + acMargin) > acMax->getValueLong ())
	{
		t_ac -= (int32_t)fabs(haCpd->getValueDouble () * 360.0);
	}
	// while on N we would like to see positive values
	if (telLatitude->getValueDouble () > 0)
	{
		while ((t_ac - acMargin) < acMin->getValueLong ())
			// ticks per revolution - don't have idea where to get that
		{
			t_ac += (int32_t) fabs(haCpd->getValueDouble () * 360.0);
		}
	}

	// put dc to correct numbers
	while (t_dc < dcMin->getValueLong ())
		t_dc += (int32_t) fabs(decCpd->getValueDouble () * 360.0);
	while (t_dc > dcMax->getValueLong ())
		t_dc -= (int32_t) fabs(decCpd->getValueDouble () * 360.0);

	if ((t_dc < dcMin->getValueLong ()) || (t_dc > dcMax->getValueLong ()))
	{
		return -1;
	}

	if ((t_ac < acMin->getValueLong ()) || (t_ac > acMax->getValueLong ()))
	{
		return -1;
	}

	// apply model (some modeling components are not cyclic => we want to use real mount coordinates)
	u_pos.ra = ls - ((double) (t_ac / haCpd->getValueDouble ()) + haZero->getValueDouble ());
	u_pos.dec = (double) (t_dc / decCpd->getValueDouble ()) + decZero->getValueDouble ();
	if (telLatitude->getValueDouble () < 0)
		u_pos.dec *= -1;
	applyModel (&u_pos, &model_change, 0, JD);	// we give raw (unflipped) position => flip=0 for model computation

	// when on south, change sign (don't take care of flip - we use raw position, applyModel takes it into account)
	if (telLatitude->getValueDouble () < 0)
		model_change.dec *= -1;

	#ifdef DEBUG_EXTRA
	LibnovaRaDec lchange (&model_change);

	logStream (MESSAGE_DEBUG) << "Before model " << t_ac << t_dc << lchange << sendLog;
	#endif						 /* DEBUG_EXTRA */

	t_ac -= -1.0 * (int32_t) (model_change.ra * haCpd->getValueDouble ());	// -1* is because ac is in HA, not in RA
	t_dc -= (int32_t) (model_change.dec * decCpd->getValueDouble ());

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "After model" << t_ac << t_dc << sendLog;
	#endif						 /* DEBUG_EXTRA */

	return 0;
}

int GEM::checkTrajectory (int32_t ac, int32_t dc, int32_t &at, int32_t &dt, int32_t as, int32_t ds, unsigned int steps, double alt_margin, double az_margin, bool ignore_soft_beginning, bool dont_flip)
{
	// nothing to check
	if (hardHorizon == NULL)
		return 0;

	int32_t t_a = ac;
	int32_t t_d = dc;

	int32_t step_a = as;
	int32_t step_d = ds;

	int32_t soft_a;
	int32_t soft_d;

	if (ac > at)
		step_a = -as;

	if (dc > dt)
		step_d = -ds;

	int first_flip = telFlip->getValueInteger ();

	double JD = ln_get_julian_from_sys ();  // fixed time; assuming slew does not take too long, this will work

	// turned to true if we are in "soft" boundaries, e.g hit with margin applied
	bool soft_hit = false;

	for (; steps > 0; steps--)
	{
		// check if still visible
		struct ln_equ_posn pos, un_pos;
		struct ln_hrz_posn hrz;
		int flip = 0;
		int ret;

		int32_t n_a;
		int32_t n_d;

		// if we already reached destionation, e.g. currently computed position is within step to target, don't go further..
		if (labs (t_a - at) < as)
		{
			n_a = at;
			step_a = 0;
		}
		else
		{
			n_a = t_a + step_a;
		}

		if (labs (t_d - dt) < ds)
		{
			n_d = dt;
			step_d = 0;
		}
		else
		{
			n_d = t_d + step_d;
		}

		ret = counts2sky (n_a, n_d, pos.ra, pos.dec, flip, un_pos.ra, un_pos.dec);
		if (ret)
			return -1;

		if (dont_flip == true && first_flip != flip)
		{
			at = n_a;
			dt = n_d;
			return 4;
		}

		ln_get_hrz_from_equ (&pos, rts2core::Configuration::instance ()->getObserver (), JD, &hrz);

		if (soft_hit == true || ignore_soft_beginning == true)
		{
			// if we really cannot go further
			if (hardHorizon->is_good (&hrz) == 0)
			{
				logStream (MESSAGE_DEBUG) << "hit hard limit at alt az " << hrz.alt << " " << hrz.az << " " << soft_a << " " << soft_d << " " << n_a << " " << n_d << sendLog;
				if (soft_hit == true)
				{
					// then use last good position, and return we reached horizon..
					at = soft_a;
					dt = soft_d;
					return 2;
				}
				else
				{
					// case when moving within soft will lead to hard hit..we don't want this path
					at = t_a;
					dt = t_d;
					return 3;
				}
			}
		}

		// we don't need to move anymore, full trajectory is valid
		if (step_a == 0 && step_d == 0)
			return 0;

		if (soft_hit == false)
		{
			// check soft margins..
			if (hardHorizon->is_good_with_margin (&hrz, alt_margin, az_margin) == 0)
			{
				if (ignore_soft_beginning == false)
				{
					soft_hit = true;
					soft_a = t_a;
					soft_d = t_d;
				}
			}
			else
			{
				// we moved away from soft hit region
				if (ignore_soft_beginning == true)
				{
					ignore_soft_beginning = false;
					soft_a = t_a;
					soft_d = t_d;
				}
			}
		}

		t_a = n_a;
		t_d = n_d;
	}

	if (soft_hit == true)
	{
		at = soft_a;
		dt = soft_d;
	}
	else
	{
		at = t_a;
		dt = t_d;
	}

	// we are fine to move at least to the given coordinates
	return 1;
}
