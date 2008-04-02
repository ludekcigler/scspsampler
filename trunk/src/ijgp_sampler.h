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

#ifndef IJGP_SAMPLER_H_
#define IJGP_SAMPLER_H_

#include "csp.h"
#include "ijgp.h"

class IJGPSampler: public CSPSampler {
public:
        IJGPSampler(CSPProblem * aProblem, unsigned int aMaxBucketSize, double aIJGPProbability,
                        unsigned int aMaxIJGPIterations);
        ~IJGPSampler();
        
        virtual bool getSample(Assignment & aAssignment);
private:
        /**
         * Sample from given distribution for a given variable
         */
        VarType _sampleFromDistribution(Variable * aVariable, const ProbabilityDistribution & aDistribution);

        bool _getSampleInternal(Assignment & aEvidence, VariableMap::const_iterator aVarIterator,
                VarIdType aLastChangedVariable = 0);

        JoinGraph * mJoinGraph, *mOriginalJoinGraph;
        unsigned int mMaxBucketSize;
        double mIJGPProbability;
        unsigned int mMaxIJGPIterations;

        bool mNoSolutionExists;
};

#endif // IJGP_SAMPLER_H_

