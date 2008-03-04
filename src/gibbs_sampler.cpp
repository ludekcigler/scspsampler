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

#include <assert.h>
#include <iostream>

#include "csp.h"
#include "utils.h"
#include "gibbs_sampler.h"

GibbsSampler::GibbsSampler(CSPProblem * p, unsigned int burn_in):
                CSPSampler(p), mBurnIn(burn_in), mInitialized(false) {
        assert(p);
};

GibbsSampler::~GibbsSampler() {}

const Assignment GibbsSampler::getSample() {
        if (mInitialized) {
                modifySampleInternal();
        } else {
                // Perform burn-in
                initSampleInternal();

                for (unsigned int i = 0; i < mBurnIn; ++i) {
                        std::cout << "GIBBS burn-in step " << i << std::endl;
                        modifySampleInternal();
                }
                mInitialized = true;
        }
        
        return mSample;
}

void GibbsSampler::initSampleInternal() {
        for (VariableMap::const_iterator varIt = mProblem->getVariables()->begin();
                        varIt != mProblem->getVariables()->end(); ++varIt) {

                mSample[varIt->first] = random_select(varIt->second->getDomain()); 
        }
}

void GibbsSampler::modifySampleInternal() {

        // Modify values for all variables in the problem
        for (VariableMap::const_iterator varIt = mProblem->getVariables()->begin();
                        varIt != mProblem->getVariables()->end(); ++varIt) {

                //std::cout << "Processing variable x_" << (*varIt)->getId() << std::endl;
                const Domain * dom = varIt->second->getDomain();
                size_t domSize = dom->size();

                // Sum of conditional probabilities P(X_j|X_{-j}), 
                // non-normalized (therefore we need to actually compute the total...)
                double totalProbability = 0; 

                double *domainProbabilities = new double[domSize];

                int domainCounter = 0;

                // Compute probability for each possible value from the domain
                for (Domain::iterator domIt = dom->begin(); domIt != dom->end(); ++domIt, ++domainCounter) {
                        mSample[varIt->second->getId()] = *domIt;
                        double e = mProblem->evalAssignment(mSample);
                        e = max(e, EPSILON);

                        domainProbabilities[domainCounter] = e;
                        totalProbability += domainProbabilities[domainCounter];
                }

                if (totalProbability < EPSILON) {
                        mSample[varIt->second->getId()] = random_select(dom);
                } else {

                        double selectedProbability = (rand()*1.0/RAND_MAX) * totalProbability;
                                 
                        double accumulatedProbability = 0.0;
                        domainCounter = 0;
                        for (Domain::iterator domIt = dom->begin();
                                        domIt != dom->end(); ++domIt, ++domainCounter) {
        
                                accumulatedProbability += domainProbabilities[domainCounter];
        
                                if (accumulatedProbability >= selectedProbability) {
                                        mSample[varIt->second->getId()] = *domIt;
                                        break;
                                }
                        }
                }
        }
}
