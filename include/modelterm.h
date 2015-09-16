/* 
 * Telescope model terms.
 * Copyright (C) 2003-2006 Martin Jelinek <mates@iaa.es>
 * Copyright (C) 2006-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_MODELTERM__
#define __RTS2_MODELTERM__

#include "telmodel.h"
#include "value.h"

namespace rts2telmodel
{

/**
 * @file
 * Include classes for various TPoint terms.
 *
 * @defgroup RTS2TPointTerm Modelling terms for TPOINT
 */

class ObsConditions;

/**
 * Represents model term. Child of that class are created
 * in TelModel::load, and used in apply functions.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class ModelTerm:public rts2core::ValueDouble
{
	public:
		ModelTerm (const char *in_name, double in_corr, double in_sigma);
		virtual ~ ModelTerm (void) {}
		
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions) = 0;
		virtual void reverse (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
		{
			struct ln_equ_posn pos_old;
			pos_old.ra = pos->ra;
			pos_old.dec = pos->dec;
			apply (pos, obs_conditions);
			pos->ra = 2 * pos_old.ra - pos->ra;
			pos->dec = 2 * pos_old.dec - pos->dec;
		}

		std::ostream & print (std::ostream & os);
	protected:
		double sigma;			 // model sigma
	private:
		// term name
		std::string name;
};

std::ostream & operator << (std::ostream & os, ModelTerm * term);

/**
 * Polar axis misalignment in elevation term.
 *
 * @author Martin Jelinek <Martin Jelinek <mates@iaa.es>@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermME:public ModelTerm
{
	public:
		TermME (double in_corr, double in_sigma):ModelTerm ("ME", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Polar axis misalignment in azimuth term.
 *
 * @author Martin Jelinek <Martin Jelinek <mates@iaa.es>@iaa.es>a
 *
 * @ingroup RTS2TPointTerm
 */
class TermMA:public ModelTerm
{
	public:
		TermMA (double in_corr, double in_sigma):ModelTerm ("MA", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Index error in hour angle.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermIH:public ModelTerm
{
	public:
		TermIH (double in_corr, double in_sigma):ModelTerm ("IH", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Index error in declination.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermID:public ModelTerm
{
	public:
		TermID (double in_corr, double in_sigma):ModelTerm ("ID", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * East-west collimation error.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermCH:public ModelTerm
{
	public:
		TermCH (double in_corr, double in_sigma):ModelTerm ("CH", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * HA/Dec non-perpendicularity.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermNP:public ModelTerm
{
	public:
		TermNP (double in_corr, double in_sigma):ModelTerm ("NP", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Fork flexure.
 *
 * @author Jan Strobl
 *
 * @ingroup RTS2TPointTerm
 */
class TermFO:public ModelTerm
{
	public:
		TermFO (double in_corr, double in_sigma):ModelTerm ("FO", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Step size in h (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermPHH:public ModelTerm
{
	public:
		TermPHH (double in_corr, double in_sigma):ModelTerm ("PHH", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Step size in declination (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 */
class TermPDD:public ModelTerm
{
	public:
		TermPDD (double in_corr, double in_sigma):ModelTerm ("PDD", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Aux1 to h (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermA1H:public ModelTerm
{
	public:
		TermA1H (double in_corr, double in_sigma):ModelTerm ("A1H", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Aux1 to in declination (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermA1D:public ModelTerm
{
	public:
		TermA1D (double in_corr, double in_sigma):ModelTerm ("A1D", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Tube flexture.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermTF:public ModelTerm
{
	public:
		TermTF (double in_corr, double in_sigma):ModelTerm ("TF", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Tube flexure (tangent)
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @ingroup RTS2TPointTerm
 */
class TermTX:public ModelTerm
{
	public:
		TermTX (double in_corr, double in_sigma):ModelTerm ("TX", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * HA centering error, cosine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class TermHCEC:public ModelTerm
{
	public:
		TermHCEC (double in_corr, double in_sigma):ModelTerm ("HCEC", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * HA centering error, sine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class TermHCES:public ModelTerm
{
	public:
		TermHCES (double in_corr, double in_sigma):ModelTerm ("HCES", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Dec centering error, cosine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class TermDCEC:public ModelTerm
{
	public:
		TermDCEC (double in_corr, double in_sigma):ModelTerm ("DCEC", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Dec centering error, sine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class TermDCES:public ModelTerm
{
	public:
		TermDCES (double in_corr, double in_sigma):ModelTerm ("DCES", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Declination axis bending.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class TermDAB:public ModelTerm
{
	public:
		TermDAB (double in_corr, double in_sigma):ModelTerm ("DAB", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

/**
 * Declination axis flop.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class TermDAF:public ModelTerm
{
	public:
		TermDAF (double in_corr, double in_sigma):ModelTerm ("DAF", in_corr, in_sigma) {}
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);
};

typedef enum
{ SIN, COS, NOT }
sincos_t;

/**
 * Class which calculate harmonics terms.
 * It have to construct function from name, using sin & cos etc..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPointTerm
 */
class TermHarmonics:public ModelTerm
{
	public:
		TermHarmonics (double in_corr, double in_sigma, const char *in_name);
		virtual void apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions);

	private:
		char resType;
		sincos_t func[2];
		char param[2];
		int mul[2];

		const char *getFunc (const char *in_func, int i);
		double getValue (struct ln_equ_posn *pos, ObsConditions * obs_conditions, int i);
		double getMember (struct ln_equ_posn *pos, ObsConditions * obs_conditions, int i);
};

};
#endif							 /*! __RTS2_MODELTERM__ */
