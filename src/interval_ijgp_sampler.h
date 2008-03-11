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

#ifndef INTERVAL_IJGP_SAMPLER_H_
#define INTERVAL_IJGP_SAMPLER_H_

#include "csp.h"
#include "interval_ijgp.h"

class IntervalIJGPSampler: public CSPSampler {
public:
        IntervalIJGPSampler(CSPProblem * aProblem, unsigned int aMaxBucketSize, double aIJGPProbability,
                        unsigned int aMaxIJGPIterations, unsigned int aMaxDomainIntervals,
                        unsigned int aMaxValuesFromInterval);

        ~IntervalIJGPSampler();
        
        virtual bool getSample(Assignment & aAssignment);

private:
        bool _getSampleInternal(Assignment & aEvidence, VariableMap::const_iterator aVarIterator,
                VarIdType aLastChangedVariable = 0);
        /**
         * Sample from given distribution for a given variable
         */
        VarType _sampleFromDistribution(Variable * aVariable, const IntervalProbabilityDistribution & aDistribution);

        static void _eraseValueFromDist(const Variable * aVariable, VarType aValue,
                        IntervalProbabilityDistribution &aDistribution);

        IntervalJoinGraph * mJoinGraph;
        unsigned int mMaxBucketSize;
        double mIJGPProbability;
        unsigned int mMaxIJGPIterations;

        /**
         * Maximum number of intervals per domain
         */
        unsigned int mMaxDomainIntervals;

        /**
         * Maximum number of values from single interval used for sampling
         */
        unsigned int mMaxValuesFromInterval;
};

std::string interval_probability_distribution_pprint(const IntervalProbabilityDistribution & aDist);

#endif // INTERVAL_IJGP_SAMPLER_H_

