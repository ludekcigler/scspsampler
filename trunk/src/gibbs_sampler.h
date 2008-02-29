/*
 * Copyright 2008 Luděk Cigler <luc@matfyz.cz>
 * $Id$
 *
 * This file is part of SCSPSampler.
 *
 * SCSPSampler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Hollo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GIBBS_SAMPLER_H_
#define GIBBS_SAMPLER_H_

class GibbsSampler: public CSPSampler {
public:
        GibbsSampler(CSPProblem * p, unsigned int burn_in);
        ~GibbsSampler();
        
        virtual const Assignment getSample();

private:
        void initSampleInternal();
        void modifySampleInternal();

        unsigned int mBurnIn; // How many steps we should perform before outputting a given sample

        Assignment mSample;

        bool mInitialized;
};

#endif // GIBBS_SAMPLER_H_
