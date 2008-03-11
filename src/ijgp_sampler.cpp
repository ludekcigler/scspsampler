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

bool IJGPSampler::getSample(Assignment & aAssignment) {
        mJoinGraph->purgeMessages();
        aAssignment = Assignment();

        return _getSampleInternal(aAssignment, mProblem->getVariables()->begin());
}

bool IJGPSampler::_getSampleInternal(Assignment & aEvidence, VariableMap::const_iterator aVarIterator,
                VarIdType aLastChangedVariable) {

        if (aVarIterator == mProblem->getVariables()->end()) {
                return true; // We have reached the last variable
        } else {
                std::map<VarIdType, Domain> removedValues;
                bool domainsNotEmpty;

                if (aEvidence.empty()) {
                        // If the evidence is empty, there has not been any variable changed yet
                        domainsNotEmpty = mProblem->propagateConstraints(aEvidence, removedValues);
                } else {
                        domainsNotEmpty = mProblem->propagateConstraints(aEvidence, removedValues, aLastChangedVariable);
                }

                if (domainsNotEmpty) {
                        // Select variable, and its value
                        Variable * targetVar = aVarIterator->second;
                        mJoinGraph->iterativePropagation(mProblem, aEvidence, mMaxIJGPIterations);

                        ProbabilityDistribution dist = mJoinGraph->conditionalDistribution(mProblem,
                                        targetVar, aEvidence);

                        //std::cout << probability_distribution_pprint(dist) << std::endl;

                        while (!dist.empty()) {
                                VarType value = _sampleFromDistribution(targetVar, dist);
                                aEvidence[targetVar->getId()] = value;

                                Domain targetVarRemovedValues;

                                targetVar->restrictDomainToValue(value, targetVarRemovedValues);
                                ++aVarIterator;

                                bool sampleFound = _getSampleInternal(aEvidence, aVarIterator, targetVar->getId());

                                --aVarIterator;
                                targetVar->restoreRestrictedDomain(targetVarRemovedValues);

                                if (!sampleFound) {
                                        aEvidence.erase(targetVar->getId());
                                        dist.erase(value);
                                } else {
                                        mProblem->restoreDomains(removedValues);
                                        return true; // Yep, we have a sample
                                }
                        }
                } else {
                        std::cout << "No solution found, backtracking from evidence " << assignment_pprint(aEvidence) << std::endl;
                }
                
                // If no try has been succesful, restore the domains and return false
                mProblem->restoreDomains(removedValues);

                return false;
        }
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
