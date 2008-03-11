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
#include <sstream>

#include "csp.h"
#include "utils.h"
#include "interval_ijgp.h"
#include "interval_ijgp_sampler.h"

IntervalIJGPSampler::IntervalIJGPSampler(CSPProblem * aProblem, unsigned int aMaxBucketSize, double aIJGPProbability,
                unsigned int aMaxIJGPIterations, unsigned int aMaxDomainIntervals, unsigned int aMaxValuesFromInterval):
        CSPSampler(aProblem), mMaxBucketSize(aMaxBucketSize), mIJGPProbability(aIJGPProbability),
        mMaxIJGPIterations(aMaxIJGPIterations), mMaxDomainIntervals(aMaxDomainIntervals),
        mMaxValuesFromInterval(aMaxValuesFromInterval) {
        
        mJoinGraph = IntervalJoinGraph::createJoinGraph(aProblem, aMaxBucketSize, aMaxDomainIntervals, aMaxValuesFromInterval);
}

IntervalIJGPSampler::~IntervalIJGPSampler() {
        delete mJoinGraph;
}

bool IntervalIJGPSampler::getSample(Assignment & aAssignment) {
        mJoinGraph->purgeMessages();
        mJoinGraph->initDomainIntervals(mProblem);
        aAssignment = Assignment();

        return _getSampleInternal(aAssignment, mProblem->getVariables()->begin());

        /*
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

                const IntervalProbabilityDistribution dist = mJoinGraph->conditionalDistribution(mProblem,
                                targetVar, evidence);

                evidence[targetVar->getId()] = _sampleFromDistribution(targetVar, dist);

        }

        aAssignment = evidence;
        return true;*/
}

bool IntervalIJGPSampler::_getSampleInternal(Assignment & aEvidence, VariableMap::const_iterator aVarIterator,
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

                        IntervalProbabilityDistribution dist = mJoinGraph->conditionalDistribution(mProblem,
                                        targetVar, aEvidence);

                        std::cout << "Dist for " << targetVar->getId() << ": " << interval_probability_distribution_pprint(dist) << std::endl;

                        std::list<VarType> triedValues;

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

                                        _eraseValueFromDist(targetVar, value, dist);
                                        std::cout << "No sample found, erasing " << value << " from " << targetVar->getId() << std::endl;

                                        targetVar->eraseFromDomain(value);
                                        removedValues[targetVar->getId()].insert(value);

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

VarType IntervalIJGPSampler::_sampleFromDistribution(Variable * aVariable, 
                const IntervalProbabilityDistribution & aDistribution) {

        double totalProbability = 0.0;

        for (IntervalProbabilityDistribution::const_iterator pIt = aDistribution.begin();
                        pIt != aDistribution.end(); ++pIt) {

                totalProbability += pIt->second;
        }

        if (totalProbability == 0.0) {
                return random_select(aVariable->getDomain());
        }

        double selectedProbability = (rand()*1.0/RAND_MAX) * totalProbability;
        double accumulatedProbability = 0.0;

        for (IntervalProbabilityDistribution::const_iterator pIt = aDistribution.begin();
                        pIt != aDistribution.end(); ++pIt) {

                accumulatedProbability += pIt->second;

                if (selectedProbability <= accumulatedProbability) {
                        // Sample uniformly from values in the given interval
                        return random_select(aVariable->getDomain(), pIt->first.lowerBound, pIt->first.upperBound);
                }
        }

        assert(false); // We should have already selected a value
}

void IntervalIJGPSampler::_eraseValueFromDist(const Variable * aVariable, VarType aValue, 
                IntervalProbabilityDistribution &aDistribution) {
        // Find the interval which should contain aValue
        IntervalProbabilityDistribution::iterator pdIt = aDistribution.lower_bound(DomainInterval(aValue, aValue + 1));

        if (pdIt == aDistribution.end() || pdIt->first.lowerBound > aValue) {
                // There is no interval containing the value, this seems strange...
                assert(false);
                return;
        }

        unsigned int numValuesInRange = aVariable->getNumValuesInDomainRange(pdIt->first.lowerBound, pdIt->first.upperBound);

        if (numValuesInRange > 1) {
                pdIt->second -= 1.0/numValuesInRange;
        } else {
                aDistribution.erase(pdIt);
        }
}

std::string interval_probability_distribution_pprint(const IntervalProbabilityDistribution & aDistribution) {
        std::ostringstream out;


        for (IntervalProbabilityDistribution::const_iterator pIt = aDistribution.begin();
                        pIt != aDistribution.end(); ++pIt) {
                out << pIt->first.pprint() << ": " << pIt->second << ", ";
        }
        out << std::endl;
        return out.str();
}
