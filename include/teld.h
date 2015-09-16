/* 
 * Telescope control daemon.
 * Copyright (C) 2003-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TELD_CPP__
#define __RTS2_TELD_CPP__

#include <libnova/libnova.h>
#include <sys/time.h>
#include <time.h>
#include "pluto/norad.h"

#include "device.h"
#include "objectcheck.h"

// pointing models
#define POINTING_RADEC       0
#define POINTING_ALTAZ       1
#define POINTING_ALTALT      2


#define SIDEREAL_HOURS       23.9344696

namespace rts2telmodel
{
	class Model;
};

/**
 * Telescope interface and relating things (modelling, pointing, ...)
 */
namespace rts2teld
{

/**
 * Basic class for telescope drivers.
 *
 * This class provides telescope driver with basic functionalities, such as
 * handles to start movement, do all target calculation and end movement.
 *
 * Coordinates entering driver are in J2000. When it is required to do so,
 * driver handles all transformations from J2000 to current, including
 * refraction. Then this number is feeded to TPoint model and used to point
 * telescope.
 *
 * Those values are important for telescope:
 *
 *   - <b>ORIRA, ORIDEC</b> contains original, J2000 coordinates. Those are usually entered by observing program (rts2-executor).
 *   - <b>OFFSRA, OFFSDEC</b> contains offsets applied to ORI coordinates.
 *   - <b>OBJRA,OBJDEC</b> contains offseted coordinates. OBJRA = ORIRA + OFFSRA, OBJDEC = ORIDEC + OFFSDEC.
 *   - <b>TARRA,TARDEC</b> contains precessed etc. coordinates. Those coordinates do not contain modelling, which is stored in MORA and MODEC
 *   - <b>CORR_RA, CORR_DEC</b> contains offsets from on-line astrometry which are fed totelescope. Telescope coordinates are then calculated as TARRA-CORR_RA, TAR_DEC - CORR_DEC
 *   - <b>TELRA,TELDEC</b> contains coordinates read from telescope driver. In ideal word, they should eaual to TARRA - CORR_RA - MORA, TARDEC - CORR_DEC - MODEC. But they might differ. The two major sources of differences are: telescope do not finish movement as expected and small deviations due to rounding errors in mount or driver.
 *   - <b>MORA,MODEC</b> contains offsets comming from ponting model. They are shown only if this information is available from the mount (OpenTpl) or when they are caculated by RTS2 (Paramount).
 *
 * Following auxiliary values are used to track telescope offsets:
 *
 *   - <b>woffsRA, woffsDEC</b> contains offsets which weren't yet applied. They can be set either directly or when you change OFFS value. When move command is executed, OFFSRA += woffsRA, OFFSDEC += woffsDEC and woffsRA and woffsDEC are set to 0.
 *   - <b>wcorrRA, wcorrDEC</b> contains corrections which weren't yet applied. They can be set either directy or by correct command. When move command is executed, CORRRA += wcorrRA, CORRDEC += wcorrDEC and wcorrRA = 0, wcorrDEC = 0.
 *
 * Please see startResync() documentation for functions available to
 * retrieve various coordinates. startResync is the routine which is called
 * each time coordinates written as target should be matched to physical
 * telescope coordinates. Using various methods in the class, driver can 
 * get various coordinates which will be put to telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Telescope:public rts2core::Device
{
	public:
		Telescope (int argc, char **argv, bool diffTrack = false, bool hasTracking = false, bool hasUnTelCoordinates = false);
		virtual ~ Telescope (void);

		virtual void postEvent (rts2core::Event * event);

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

		virtual rts2core::DevClient *createOtherType (rts2core::Connection * conn, int other_device_type);

		double getLatitude () { return telLatitude->getValueDouble (); }
		double getLongitude () { return telLongitude->getValueDouble (); }
		double getAltitude () { return telAltitude->getValueDouble (); }

		// callback functions from telescope connection
		virtual int info ();

		virtual int scriptEnds ();

		int setTo (rts2core::Connection * conn, double set_ra, double set_dec);
		int setToPark (rts2core::Connection * conn);

		int startPark (rts2core::Connection * conn);

		virtual int getFlip ();

		/**
		 * Apply corrections to position.
		 */
		void applyCorrections (struct ln_equ_posn *pos, double JD);

		/**
		 * Set telescope correctios.
		 *
		 * @param _aberation     If aberation should be calculated.
		 * @param _precession    If precession should be calculated.
		 * @param _refraction    If refraction should be calculated.
		 */
		void setCorrections (bool _aberation, bool _precession, bool _refraction)
		{
			calAberation->setValueBool (_aberation);
			calPrecession->setValueBool (_precession);
			calRefraction->setValueBool (_refraction);
		}

		/**
		 * If aberation should be calculated in RTS2.
		 */
		bool calculateAberation () { return calAberation->getValueBool (); }

		/**
		 * If precession should be calculated in RTS2.
		 */
		bool calculatePrecession () { return calPrecession->getValueBool (); }

		/**
		 * If refraction should be calculated in RTS2.
		 */
		bool calculateRefraction () { return calRefraction->getValueBool (); }

		/**
		 * Switch model off model will not be used to transform coordinates.
		 */
		void modelOff () { calModel->setValueBool (false); }

		void modelOn () { calModel->setValueBool (true); }

		bool isModelOn () { return (calModel->getValueBool ()); }

		virtual int commandAuthorized (rts2core::Connection * conn);

		virtual void setFullBopState (rts2_status_t new_state);

		/**
		 * Check if given hrz position is above horizon.
		 *
		 * @param hrz   horizontal position to check
		 *
		 * @return -1 if hard horizon was not specified, 0 if not above hard horizon, 1 if above hard horizon
		 */
		int isGood (struct ln_hrz_posn *hrz)
		{
			if (hardHorizon)
				return hardHorizon->is_good (hrz);
			else
				return -1;
		}
		
	protected:
		void applyOffsets (struct ln_equ_posn *pos)
		{
			pos->ra = oriRaDec->getRa () + offsRaDec->getRa ();
			pos->dec = oriRaDec->getDec () + offsRaDec->getDec ();
		}

		/**
		 * Called before corrections are processed. If returns 0, then corrections
		 * will skip the standard correcting mechanism.
		 *
		 * @param ra  RA correction
		 * @param dec DEC correction
		 */
		virtual int applyCorrectionsFixed (double ra, double dec) { return -1; }
	
		/**
		 * Returns telescope target RA.
		 */
		double getTelTargetRa () { return telTargetRaDec->getRa (); }

		/**
		 * Returns telescope target DEC.
		 */
		double getTelTargetDec () { return telTargetRaDec->getDec (); }

		/**
		 * Return telescope target coordinates.
		 */
		void getTelTargetRaDec (struct ln_equ_posn *equ)
		{
			equ->ra = getTelTargetRa ();
			equ->dec = getTelTargetDec ();
			normalizeRaDec (equ->ra, equ->dec);
		}
		  
		/**
		 * Set telescope RA.
		 *
		 * @param ra Telescope right ascenation in degrees.
		 */
		void setTelRa (double new_ra) { telRaDec->setRa (new_ra); }

		/**
		 * Set telescope DEC.
		 *
		 * @param new_dec Telescope declination in degrees.
		 */
		void setTelDec (double new_dec) { telRaDec->setDec (new_dec); }

		/**
		 * Set telescope RA and DEC.
		 */
		void setTelRaDec (double new_ra, double new_dec) 
		{
			setTelRa (new_ra);
			setTelDec (new_dec);
		}

		/**
		 * Returns current telescope RA.
		 *
		 * @return Current telescope RA.
		 */
		double getTelRa () { return telRaDec->getRa (); }

		/**
		 * Returns current telescope DEC.
		 *
		 * @return Current telescope DEC.
		 */
		double getTelDec () { return telRaDec->getDec (); }

		/**
		 * Returns telescope RA and DEC.
		 *
		 * @param tel ln_equ_posn which will be filled with telescope RA and DEC.
		 */
		void getTelRaDec (struct ln_equ_posn *tel)
		{
			tel->ra = getTelRa ();
			tel->dec = getTelDec ();
		}


		/**
		 * Set telescope untouched (i.e. physical) RA.
		 *
		 * @param ra Telescope right ascenation in degrees.
		 */
		void setTelUnRa (double new_ra) { telUnRaDec->setRa (new_ra); }

		/**
		 * Set telescope untouched (i.e. physical) DEC.
		 *
		 * @param new_dec Telescope declination in degrees.
		 */
		void setTelUnDec (double new_dec) { telUnRaDec->setDec (new_dec); }

		/**
		 * Set telescope untouched (i.e. physical) RA and DEC.
		 */
		void setTelUnRaDec (double new_ra, double new_dec) 
		{
			setTelUnRa (new_ra);
			setTelUnDec (new_dec);
		}

		/**
		 * Returns current telescope untouched (i.e. physical) RA.
		 *
		 * @return Current telescope untouched (i.e. physical) RA.
		 */
		double getTelUnRa () { return telUnRaDec->getRa (); }

		/**
		 * Returns current telescope untouched (i.e. physical) DEC.
		 *
		 * @return Current telescope untouched (i.e. physical) DEC.
		 */
		double getTelUnDec () { return telUnRaDec->getDec (); }

		/**
		 * Returns telescope untouched (i.e. physical) RA and DEC.
		 *
		 * @param tel ln_equ_posn which will be filled with telescope RA and DEC.
		 */
		void getTelUnRaDec (struct ln_equ_posn *tel)
		{
			tel->ra = getTelUnRa ();
			tel->dec = getTelUnDec ();
		}


		/**
		 * Set ignore correction - size bellow which correction commands will
		 * be ignored.
		 */
		void setIgnoreCorrection (double new_ign) { ignoreCorrection->setValueDouble (new_ign); }


		/**
		 * Set ponting model. 0 is EQU, 1 is ALT-AZ
		 *
		 * @param pModel 0 for EQU, 1 for ALT-AZ.
		 */
		void setPointingModel (int pModel) { pointingModel->setValueInteger (pModel); }


		/**
		 * Return telescope pointing model.
		 *
		 * @return 0 if pointing model is EQU, 1 if it is ALT-AZ
		 */
		int getPointingModel () { return pointingModel->getValueInteger (); }

		/**
		 * Creates values for guiding movements.
		 */
		void createRaGuide ();
		void createDecGuide ();

		/**
		 * Telescope can track.
		 */
		void createTracking ();

		virtual int processOption (int in_opt);

		virtual int init ();
		virtual int initValues ();
		virtual int idle ();

		/**
		 * Increment number of parks. Shall be called
		 * every time mount homing commands is issued.
		 *
		 * ParkNum is used in the modelling to find observations
		 * taken in interval with same park numbers, e.g. with sensors 
		 * homed at same location.
		 */
		void setParkTimeNow () { mountParkTime->setNow (); }

		/**
		 * Apply corrRaDec. Return -1if correction is above correctionLimit and was not applied.
		 */
		int applyCorrRaDec (struct ln_equ_posn *pos, bool invertRa = false, bool invertDec = false);
		void zeroCorrRaDec () {corrRaDec->setValueRaDec (0, 0); corrRaDec->resetValueChanged (); wcorrRaDec->setValueRaDec (0, 0); wcorrRaDec->resetValueChanged ();};

		/**
		 * Apply model for RA/DEC position pos, for specified flip and JD.
		 * All resulting coordinates also includes corrRaDec corection. 
		 * Also changes tel_target (telTargetRA) variable, including model computation and corrRaDec.
		 * Also sets MO_RTS2 (modelRaDec) variable, mirroring (only) computed model difference.
		 * Can be used to compute non-cyclic model, with flip=0 and pos in raw mount coordinates.
		 *
		 * @param pos ln_equ_posn RA/DEC position (typically TAR, i.e. precessed coordinates), will be corrected by computed model and correction corrRaDec.
		 * @param model_change ln_equ_posn difference against original pos position, includes coputed model's difference together with correction corrRaDec.
		 */
		void applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD);

		/**
		 * Apply precomputed model by computeModel (), set everything equivalently what applyModel () does.
		 * Sets MO_RTS2 (modelRaDec) and tel_target (telTargetRA) variables, also includes applyCorrRaDec if applyCorr parameter set to true.
		 */
		void applyModelPrecomputed (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, bool applyCorr);

		/**
		 * Compute model for RA/DEC position pos, for specified flip and JD.
		 * Can be used to compute non-cyclic model, with flip=0 and pos in raw mount coordinates.
		 *
		 * @param pos ln_equ_posn RA/DEC position (typically TAR, i.e. precessed coordinates), will be corrected by computed model.
		 * @param model_change ln_equ_posn coputed model's difference.
		 */
		void computeModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD);

		/**
		 * Apply corrections (at system time).
		 * Will apply corrections (precession, refraction,..) at system time.
		 * Override this method for any custom corrections. You most
		 * probably would like to call parent method in overrided
		 * method to get coordinates to start with.
		 *
		 * void MyClass::applyCorrections (double &tar_ra, double &tar_dec)
		 * {
		 *      Telescope::applyCorrections (tar_ra, tar_dec);
		 *      tar_ra += myoffs.ra);
		 *      tar_dec += myoffs.dec;
		 * }
		 *
		 * @param tar_ra  target RA, returns its value.
		 * @param tar_dec target DEC, returns its value.
		 */
		virtual void applyCorrections (double &tar_ra, double &tar_dec);

		virtual int willConnect (rts2core::NetworkAddress * in_addr);
		char telType[64];
		rts2core::ValueAltAz *telAltAz;

		rts2core::ValueInteger *telFlip;

		int flip_move_start;
		int flip_longest_path;

		double defaultRotang;

		rts2core::ValueDouble *rotang;

		rts2core::ValueDouble *telLongitude;
		rts2core::ValueDouble *telLatitude;
		rts2core::ValueDouble *telAltitude;

		/**
		 * Check if telescope is moving to fixed position. Called during telescope
		 * movement to detect if the target destination was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isMovingFixed
		 * call.
		 *
		 * @see isMoving()
		 */
		virtual int isMovingFixed () { return isMoving (); }

		/**
		 * Check if telescope is moving. Called during telescope
		 * movement to detect if the target destination was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isMoving
		 * call.
		 */
		virtual int isMoving () = 0;

		/**
		 * Check if telescope is parking. Called during telescope
		 * park to detect if parking position was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isParking
		 * call.
		 */
		virtual int isParking () { return -2; }

		/**
		 * Returns local sidereal time in hours (0-24 range).
		 * Multiply return by 15 to get degrees.
		 *
		 * @return Local sidereal time in hours (0-24 range).
		 */
		double getLocSidTime () { return getLocSidTime (ln_get_julian_from_sys ()); }

		/**
		 * Returns local sidereal time in hours (0-24 range).
		 * Multiply return by 15 to get degrees.
		 *
		 * @param JD Julian date for which sideral time will be returned.
		 *
		 * @return Local sidereal time in hours (0-24 range).
		 */
		double getLocSidTime (double JD);

		/**
		 * Returns true if origin was changed from the last movement.
		 *
		 * @see targetChangeFromLastResync
		 */
		bool originChangedFromLastResync () { return oriRaDec->wasChanged (); }

		/**
		 * Returns original, J2000 coordinates, used as observational
		 * target.
		 */
		void getOrigin (struct ln_equ_posn *_ori)
		{
			_ori->ra = oriRaDec->getRa ();
			_ori->dec = oriRaDec->getDec ();
		}
	
		void setOrigin (double ra, double dec, bool withObj = false)
		{
			oriRaDec->setValueRaDec (ra, dec);
			if (withObj)
				objRaDec->setValueRaDec (ra, dec);
		}

		/**
		 * Sets new movement target.
		 *
		 * @param ra New object right ascenation.
		 * @param dec New object declination.
		 */
		void setTarget (double ra, double dec)
		{
			tarRaDec->setValueRaDec (ra, dec);
			telTargetRaDec->setValueRaDec (ra, dec);
			modelRaDec->setValueRaDec (0, 0);
		}

		/**
		 * Set WCS reference values telescope is reporting.
		 *
		 * @param ra  WCS RA (CRVAL1)
		 * @param dec WCS DEC (CRVAL2)
		 */
		void setCRVAL (double ra, double dec)
		{
			if (wcs_crval1)
			{
				wcs_crval1->setValueDouble (ra);
				sendValueAll (wcs_crval1);
			}

			if (wcs_crval2)
			{
				wcs_crval2->setValueDouble (dec);
				sendValueAll (wcs_crval2);
			}
		}

		/**
		 * Sets target to nan. If startResync is called, it forced
		 * it to recompute target positions.
		 */
		void resetTelTarget ()
		{
			tarRaDec->setValueRaDec (NAN, NAN);
		}

		/**
		 * Sets ALT-AZ target. The program should call moveAltAz() to
		 * start alt-az movement.
		 *
		 * @see moveAltAz()
		 */
		void setTargetAltAz (double alt, double az) { telAltAz->setValueAltAz (alt, az); }

		/**
		 * Return target position. This is equal to ORI[RA|DEC] +
		 * OFFS[RA|DEC] + any transformations required for mount
		 * operation.
		 *
		 * @param out_tar Target position
		 */
		void getTarget (struct ln_equ_posn *out_tar)
		{
			out_tar->ra = tarRaDec->getRa ();
			out_tar->dec = tarRaDec->getDec ();
		}

		/**
		 * Calculate target position for given JD.
		 * This function shall be used for adaptive tracking.
		 *
		 * @param JD              date for which position will be calculated
		 * @param out_tar         target position
		 * @param tar_distance    distance to target (in m), if know (satellites,..)
		 * @param ac              current (input) and target (output) HA axis value
		 * @param dc              current (input) and target (output) DEC axis value
		 */
		int calculateTarget (double JD, double secdiff, struct ln_equ_posn *out_tar, double &tar_distance, int32_t &ac, int32_t &dc);

		/**
		 * Transforms sky coordinates to axis coordinates. Placeholder, used only for
		 * telescopes with possibility to command directly telescope axes.
		 */
		virtual int sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc);

		void addDiffRaDec (struct ln_equ_posn *tar, double secdiff);

		void getTargetAltAz (struct ln_hrz_posn *hrz)
		{
			hrz->alt = telAltAz->getAlt ();
			hrz->az = telAltAz->getAz ();
		}

		/**
		 * Returns time when move start was commanded (in ctime).
		 */
		double getTargetStarted ()
		{
			return targetStarted->getValueDouble ();
		}

		double getOffsetRa () { return offsRaDec->getRa (); }

		double getOffsetDec () { return offsRaDec->getDec (); }

		/**
		 * Returns true if target was changed from the last
		 * sucessfull move command. Target position is position which includes
		 * telescope transformations (precession, modelling) and offsets specified by
		 * OFFS.
		 *
		 * If telescope has different strategy for setting offsets,
		 * e.g. offsets can be send by separate command, please use
		 * originChangedFromLastResync() and apply offsets retrieved by
		 * getUnappliedRaOffsetRa() and getUnappliedDecOffsetDec().
		 */
		bool targetChangeFromLastResync () { return tarRaDec->wasChanged (); }

		/**
		 * Return corrections in RA/HA.
		 *
		 * @return RA correction (in degrees).
		 */
		double getCorrRa () { return corrRaDec->getRa (); }

		/**
		 * Return corrections in DEC.
		 *
		 * @return DEC correction.
		 */
		double getCorrDec () { return corrRaDec->getDec (); }

		/**
		 * Return offset from last applied correction.
		 *
		 * @return RA offset - corrections which arrives from last applied correction.
		 */
		double getWaitCorrRa () { return wcorrRaDec->getRa (); }

		/**
		 * Return offset from last applied correction.
		 *
		 * @return DEC offset - corrections which arrives from last applied correction.
		 */
		double getWaitCorrDec () { return wcorrRaDec->getDec (); }

		/**
		 * Update target and corrected ALT AZ coordinates.
		 *
		 * This call will update tarAltAz and corrAltAz coordinates, based on actuall
		 * tarRaDec and corrRaDec values.
		 */
		void calculateCorrAltAz ();

		/**
		 * Return corrections in zenit distance.
		 *
		 * @return Correction in zenit distance.
		 */
		double getCorrZd ();

		/**
		 * Return corrections in altitude.
		 *
		 * @return Correction in altitude.
		 */
		double getCorrAlt () { return -getCorrZd (); }

		/**
		 * Return corrections in azimuth.
		 *
		 * @return Correction in azimuth.
		 */
		double getCorrAz ();

		/**
		 * Return distance in degrees to target position.
		 * You are responsible to call info() before this call to
		 * update telescope coordinates.
		 * 
		 * @return Sky distance in degrees to target, 0 - 180. -1 on error.
		 */
		double getTargetDistance ();

		/**
		 * Returns ALT AZ coordinates of target.
		 *
		 * @param hrz ALT AZ coordinates of target.
		 * @param jd  Julian date for which position will be calculated.
		 */
		void getTelTargetAltAz (struct ln_hrz_posn *hrz, double jd);

		double getTargetHa ();
		double getTargetHa (double jd);

		double getLstDeg (double JD);

		virtual bool isBellowResolution (double ra_off, double dec_off) { return (ra_off == 0 && dec_off == 0); }

		void needStop () { maskState (TEL_MASK_NEED_STOP, TEL_NEED_STOP); }

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void valueChanged (rts2core::Value * changed_value);

		virtual int deleteConnection (rts2core::Connection * in_conn)
		{
			if (in_conn == move_connection)
				move_connection = NULL;
			return rts2core::Device::deleteConnection (in_conn);
		}

		// reload model
		virtual void signaledHUP ();

		/**
		 * Send telescope to requested coordinates. This function does not
		 * have any parameters, as they are various ways how to obtain
		 * telescope coordinates.
		 *
		 * If you want to get raw, J2000 target coordinates, without any offsets, check with
		 *
		 *
		 * @return 0 on success, -1 on error.
		 *
		 * @see originChangedFromLastResync
		 * @see getOrigin()
		 */
		virtual int startResync () = 0;

		/**
		 * Offset telescope coordinates. Called when some offset values (either OFFS or CORR) changed
		 * and telescope needs to be reposiented.
		 */
		virtual void startOffseting (rts2core::Value *changed_value);

		/**
		 * Move telescope to target ALTAZ coordinates.
		 */
		virtual int moveAltAz ();

		/**
		 * Issue cupola synchronization event.
		 * Should use getTarget to obtain telescope target coordinates. Please note that
		 * if you modify telescope coordinates in startResync, and do not update telTarget (
		 * with setTarget call), you are responsible to overwrite this method and modify
		 * coordinates accordingly.
		 */
		virtual void startCupolaSync ();

		/**
		 * Called at the end of telescope movement, after isMoving return
		 * -2.
		 *
		 * @return 0 on success, -1 on failure
		 */
		virtual int endMove ();

		/**
		 * Stop telescope movement. It is called in two cases. Either when new
		 * target is entered and telescope should stop movement to current target,
		 * or when some failure of telescope is detected and telescope should stop 
		 * current movement in order to prevent futher damage to the hardware.
		 *
		 * @return 0 on success, -1 on failure
		 */
		virtual int stopMove () = 0;

		/**
		 * Set telescope to match given coordinates
		 *
		 * This function is mainly used to tell the telescope, where it
		 * actually is at the beggining of observation
		 *
		 * @param ra		setting right ascennation
		 * @param dec		setting declination
		 *
		 * @return -1 on error, otherwise 0
		 */
		virtual int setTo (double set_ra, double set_dec) { return -1; }

		/**
		 * Set telescope to park position.
		 *
		 * @return -1 on error, otherwise 0
		 */
		virtual int setToPark () { return -1; }

		/**
		 * Called when park command is issued. Moves telescope to park position. Target
		 * positions are set to NAN after startPark returns 0. If driver needs to retain
		 * set target position (e.g. it is using moveAltAz to move to predefined AltAz pozition),
		 * startPark must reuturn 1.
		 *
		 * @return 0 on success, 1 on success if target value reset is not needed, -1 on failure
		 */
		virtual int startPark () = 0;

		/**
		 * Called when parking of the telescope is finished. Can do various
		 * important thinks - ussually switch of mount tracking, but can
		 * also switch of some power supply etc..
		 *
		 * @return 0 on success, -1 on failure
		 */
		virtual int endPark () = 0;

		/**
		 * Save model from telescope to file.
		 */
		virtual int saveModel () { return -1; }

		/**
		 * Load model from telescope.
		 */
		virtual int loadModel () { return -1; }

		virtual int resetMount () { return 0; }

		/**
		 * Get current telescope altitude and azimuth. This
		 * function updates telAltAz value. If you want to get target
		 * altitude and azimuth, please use getTargetAltAz().
		 */
		virtual void getTelAltAz (struct ln_hrz_posn *hrz);

		/**
		 * Return expected time in seconds it will take to reach
		 * destination from current position.  This abstract method
		 * returns getTargetDistance / 2.0, estimate mount slew speed
		 * to 2 degrees per second. You can provide own estimate by
		 * overloading this method.
		 */
		virtual double estimateTargetTime ();

		double getTargetReached () { return targetReached->getValueDouble (); }

		/**
		 * Set telescope tracking.
		 *
		 * @param track                0 - no tracking, 1 - on object, 2 - sidereal
		 * @param addTrackingTimer     if true and tracking, add tracking timer; cannot be set when called from tracking function!
		 * @param send                 if true, set rts2value and send in to all connections
		 * @return 0 on success, -1 on error
		 */
		virtual int setTracking (int track, bool addTrackingTimer = false, bool send = true);

		/**
		 * Called to run tracking. It is up to driver implementation
		 * to send updated position to telescope driver.
		 *
		 * If tracking timer shall not be called agin, call setTracking (false).
		 * Run every trackingInterval seconds.
		 *
		 * @see setTracking
		 * @see trackingInterval
		 */
		virtual void runTracking ();

		/**
		 * Calculate TLE RA DEC for given time.
		 */
		void calculateTLE (double JD, double &ra, double &dec, double &dist_to_satellite);

		/**
		 * Set differential tracking values. All inputs is in degrees / hour.
		 *
		 * @param dra  differential tracking in RA
		 * @param ddec differential tracking in DEC
		 */
		virtual void setDiffTrack (double dra, double ddec);

		/**
		 * Hard horizon. Use it to check if telescope coordinates are within limits.
		 */
		ObjectCheck *hardHorizon;

		/**
		 * Telescope parking position.
		 */
		rts2core::ValueAltAz *parkPos;
		
		/**
		 * Desired flip when parking.
		 */
		rts2core::ValueInteger *parkFlip;

		/**
		 * Add option for parking position.
		 *
		 * @warning parkPos is created only if this option was passed to the programe. If you
		 * need parkPos to exists (and fill some default value), call createParkPos first.
		 */
		void addParkPosOption ();

		/**
		 * Create parkPos variable.
		 */
		void createParkPos (double alt, double az, int flip);

		bool useParkFlipping;

		/**
		 * Local sidereal time.
		 */
		rts2core::ValueDouble *lst;

		/**
		 * Telescope idea of julian date.
		 */
		rts2core::ValueDouble *jdVal;

		rts2core::ValueSelection *raGuide;
		rts2core::ValueSelection *decGuide;

		rts2core::ValueSelection *tracking;
		rts2core::ValueFloat *trackingInterval;

		/**
		 * Returns differential tracking values. Telescope must support
		 * differential tracking for those to not core dump. Those methods
		 * should be called only from child subclass which pass true for
		 * diffTrack in Telescope contructor.
		 */
		double getDiffTrackRa () { return diffTrackRaDec->getRa (); }
		double getDiffTrackDec () { return diffTrackRaDec->getDec (); }

		void setBlockMove () { blockMove->setValueBool (true); sendValueAll (blockMove); }
		void unBlockMove () { blockMove->setValueBool (false); sendValueAll (blockMove); }
	private:
		rts2core::Connection * move_connection;
		int moveInfoCount;
		int moveInfoMax;

		/**
		 * Last error.
		 */
		rts2core::ValueDouble *posErr;

		/**
		 * If correction is bellow that value, it is ignored.
		 */
		rts2core::ValueDouble *ignoreCorrection;

		rts2core::ValueDouble *defIgnoreCorrection;

		/**
		 * If correction is bellow that value, it is considered as small correction.
		 */
		rts2core::ValueDouble *smallCorrection;

		/**
		 * Limit for corrections.
		 */
		rts2core::ValueDouble *correctionLimit; 

		/**
		 * If move is above this limit, correction is rejected.
		 */
		rts2core::ValueDouble *modelLimit;

		/**
		 * If correction is above that limit, cancel exposures and
		 * move immediatelly. This is to signal we are out of all cameras
		 * FOV.
		 */
		rts2core::ValueDouble *telFov;

		/**
		 * Object we are observing original positions (in J2000).
		 */
		rts2core::ValueRaDec *oriRaDec;

		/**
		 * User offsets, used to create dithering pattern.
		 */
		rts2core::ValueRaDec *offsRaDec;

		/**
		 * Offsets which should be applied from last movement.
		 */
		rts2core::ValueRaDec *woffsRaDec;

		rts2core::ValueRaDec *diffTrackRaDec;

		/**
		 * Start time of differential tracking.
		 */
		rts2core::ValueDouble *diffTrackStart;

		/**
		 * Coordinates of the object, after offsets are applied (in J2000).
		 * OBJ[RA|DEC] = ORI[|RA|DEC] + OFFS[|RA|DEC]
		 */
		rts2core::ValueRaDec *objRaDec;

		/**
		 * Real sky coordinates of target, with computed corrections (precession, aberation, refraction). Still without corrRaDec (astrometry feedback) and tpoint model.
		 * TAR[RA|DEC] = OBJ[RA|DEC] + precession, etc.
		 */
		rts2core::ValueRaDec *tarRaDec;

		/**
		 * Corrections from astrometry/user.
		 */
		rts2core::ValueRaDec *corrRaDec;

		/**
		 * RA DEC correction which waits to be applied.
		 */
		rts2core::ValueRaDec *wcorrRaDec;

		rts2core::ValueRaDec *total_offsets;

		/**
		 * Modelling changes.
		 */
		rts2core::ValueRaDec *modelRaDec;

		/**
		 * Corrected, modelled coordinates feeded to telescope.
		 */
		rts2core::ValueRaDec *telTargetRaDec;

		/**
		 * If this value is true, any software move of the telescope is blocked.
		 */
		rts2core::ValueBool *blockMove;

		rts2core::ValueBool *blockOnStandby;

		// object + telescope position

		rts2core::ValueBool *calAberation;
		rts2core::ValueBool *calPrecession;
		rts2core::ValueBool *calRefraction;
		rts2core::ValueBool *calModel;

		rts2core::StringArray *cupolas;

		rts2core::StringArray *rotators;

		/**
		 * Target HRZ coordinates.
		 */
		struct ln_hrz_posn tarAltAz;

		/**
		 * Target HRZ coordinates with corrections applied.
		 */
		struct ln_hrz_posn corrAltAz;

		rts2core::ValueDouble *wcs_crval1;
		rts2core::ValueDouble *wcs_crval2;

		/**
		 * Telescope RA and DEC. In perfect world read from sensors, transformed to sky coordinates (i.e. within standard limits)
		 * target + model + corrRaDec = requested position -> telRaDec
		 */
		rts2core::ValueRaDec *telRaDec;

		/**
		 * Telescope untouched physical RA and DEC, read from sensors, without flip-transformation.
		 * Equivalent to telRaDec, but reflects real physical mount position.
		 */
		rts2core::ValueRaDec *telUnRaDec;

		/**
		 * Current airmass.
		 */
		rts2core::ValueDouble *airmass;

		/**
		 * Hour angle.
		 */
		rts2core::ValueDouble *hourAngle;

		/**
		 * Distance to target in degrees.
		 */
		rts2core::ValueDouble *targetDistance;

		/**
		 * Time when movement was started.
		 */
		rts2core::ValueTime *targetStarted;

		/**
		 * Estimate time when current movement will be finished.
		 */
		rts2core::ValueTime *targetReached;

		/**
		 *
		 * @param correction   correction type bitmask - 0 for no corerction, 1 for offsets, 2 for correction
		 */
		int startResyncMove (rts2core::Connection * conn, int correction);

		/**
		 * Date and time when last park command was issued.
		 */
		rts2core::ValueTime *mountParkTime;

		rts2core::ValueInteger *moveNum;
		rts2core::ValueInteger *corrImgId;

		rts2core::ValueInteger *wCorrImgId;

		/**
		 * Tracking / idle refresh interval
		 */
		rts2core::ValueDouble *refreshIdle;

		/**
		 * Slewing refresh interval
		 */
		rts2core::ValueDouble *refreshSlew;

		void checkMoves ();

		struct timeval dir_timeouts[4];

		char *modelFile;
		rts2telmodel::Model *model;

		rts2core::ValueSelection *standbyPark;
		const char *horizonFile;

		/**
		 * Apply aberation correction.
		 */
		void applyAberation (struct ln_equ_posn *pos, double JD);

		/**
		 * Apply precision correction.
		 */
		void applyPrecession (struct ln_equ_posn *pos, double JD);

		/**
		 * Apply refraction correction.
		 */
		void applyRefraction (struct ln_equ_posn *pos, double JD);

		/**
		 * Zero's all corrections, increment move count. Called before move.
		 */
		void incMoveNum ();

		/** 
		 * Which coordinates are used for pointing (eq, alt-az,..)
		 */
		rts2core::ValueSelection *pointingModel;

		struct ln_ell_orbit mpec_orbit;

		/**
		 * Minor Planets Ephemerids one-line element. If set, target position and differential
		 * tracking are calculated from this string.
		 */
		rts2core::ValueString *mpec;

		rts2core::ValueDouble *mpec_refresh;
		rts2core::ValueDouble *mpec_angle;

		/**
		 * Satellite (from Two Line Element, passed as string with lines separated by :)
		 * tracking.
		 */

		rts2core::ValueString *tle_l1;
		rts2core::ValueString *tle_l2;
		rts2core::ValueInteger *tle_ephem;
		rts2core::ValueDouble *tle_distance;
		rts2core::ValueDouble *tle_rho_sin_phi;
		rts2core::ValueDouble *tle_rho_cos_phi;

		rts2core::ValueDouble *tle_refresh;

		tle_t tle;

		// Value for RA DEC differential tracking
		rts2core::ValueRaDec *diffRaDec;

		void recalculateMpecDIffs ();
		void recalculateTLEDiffs ();

		char wcs_multi;

		rts2core::ValueFloat *decUpperLimit;

		void resetMpecTLE ();
};

};
#endif							 /* !__RTS2_TELD_CPP__ */
