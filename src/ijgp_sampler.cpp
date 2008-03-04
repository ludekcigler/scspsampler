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

#include <iostream>

#include "csp.h"
#include "utils.h"
#include "ijgp.h"
#include "ijgp_sampler.h"

IJGPSampler::IJGPSampler(CSPProblem * aProblem, unsigned int aMaxBucketSize, double aIJGPProbability,
                unsigned int aMaxIJGPIterations):
        CSPSampler(aProblem), mMaxBucketSize(aMaxBucketSize), mIJGPProbability(aIJGPProbability),
        mMaxIJGPIterations(aMaxIJGPIterations) {
        
        mJoinGraph = JoinGraph::createJoinGraph(aProblem, aMaxBucketSize);
}

IJGPSampler::~IJGPSampler() {
        delete mJoinGraph;
}

const Assignment IJGPSampler::getSample() {
        mJoinGraph->purgeMessages();
        Assignment evidence;

        for (VariableMap::const_iterator varIt = mProblem->getVariables()->begin();
                        varIt != mProblem->getVariables()->end(); ++varIt) {

                Variable * targetVar = varIt->second;

                // If we are handling the first variable or with mIJGPProbability probability,
                // run IJGP
                if (evidence.empty() ||
                                ((1.0*rand()) / RAND_MAX) <= mIJGPProbability) {
                        mJoinGraph->iterativePropagation(mProblem, evidence, mMaxIJGPIterations);
                }

                const ProbabilityDistribution dist = mJoinGraph->conditionalDistribution(mProblem,
                                targetVar, evidence);

                evidence[targetVar->getId()] = _sampleFromDistribution(targetVar, dist);

        }

        return evidence;
}

VarType IJGPSampler::_sampleFromDistribution(Variable * aVariable, const ProbabilityDistribution & aDistribution) {
        double totalProbability = 0.0;

        for (ProbabilityDistribution::const_iterator pIt = aDistribution.begin();
                        pIt != aDistribution.end(); ++pIt) {

                totalProbability += pIt->second;
        }

        if (totalProbability == 0.0) {
                return random_select(aVariable->getDomain());
        }

        double selectedProbability = (rand()*1.0/RAND_MAX) * totalProbability;
        double accumulatedProbability = 0.0;

        for (ProbabilityDistribution::const_iterator pIt = aDistribution.begin();
                        pIt != aDistribution.end(); ++pIt) {

                accumulatedProbability += pIt->second;

                if (selectedProbability <= accumulatedProbability)
                        return pIt->first;
        }

        assert(false); // We should have already selected a value
}
