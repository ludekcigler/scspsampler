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

#include <deque>
#include <iostream>
#include <fstream>
#include <sstream>
#include "math.h"

#include "csp.h"
#include "wcsp.h"

const double EXP_ROOT = 2.0;
double EXP_K = 0.001;

double WCSPConstraint::operator()(Assignment &a) const {
        std::vector<VarType> scopeAssignment;
        for (Scope::iterator scIt = mScope.begin(); scIt != mScope.end(); ++scIt) {
               scopeAssignment.push_back(a[*scIt]); 
        }

        unsigned int weight;
        std::map<std::vector<VarType>, unsigned long>::const_iterator weightIt = mDifferentWeightTuples.find(scopeAssignment);

        if (weightIt != mDifferentWeightTuples.end()) {
                weight = weightIt->second;
        } else {
                weight = mDefaultWeight;
        }

        if (weight >= mHardConstraintWeight) {
                return 0;
        } else {
                return exp(log(EXP_ROOT) * EXP_K * (-(double)weight));
        }
}

Scope WCSPConstraint::getScope() const {
        return mScope;
}

bool WCSPConstraint::isSoft() const {
        return mMaxTupleWeight < mHardConstraintWeight;
}

bool WCSPConstraint::hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence) {
        if (isSoft()) {
                return true;
        }

        Assignment::const_iterator aIt = aEvidence.find(aVarId);
        if (aIt != aEvidence.end()) {
                return aIt->second == aValue;
        }

        Assignment a = aEvidence;
        a[aVarId] = aValue;
        
        std::deque<VarIdType> variablesToAssign;

        for (Scope::iterator scIt = mScope.begin(); scIt != mScope.end(); ++scIt) {
                if (a.find(*scIt) == a.end())
                        variablesToAssign.push_back(*scIt);
        }

        return _hasSupportInternal(a, aProblem, variablesToAssign);
}


bool WCSPConstraint::_hasSupportInternal(Assignment &aEvidence, const CSPProblem &aProblem, std::deque<VarIdType> & aVarsToAssign) {
        if (aVarsToAssign.empty()) {
                std::vector<VarType> varAssignment;

                for (Scope::iterator scIt = mScope.begin(); scIt != mScope.end(); ++scIt) {
                        varAssignment.push_back(aEvidence[*scIt]);
                }

                // Now an assignment is allowed if it is not in mDisallowedTuples and the default value is not hard-constraint
                if (mDisallowedTuples.find(varAssignment) == mDisallowedTuples.end()) {
                        if (mDifferentWeightTuples.find(varAssignment) == mDifferentWeightTuples.end()) {
                                return (mDefaultWeight < mHardConstraintWeight);
                        } else {
                                return true;
                        }
                } else {
                        return false;
                }
        } else {
                VarIdType varId = aVarsToAssign.front();
                aVarsToAssign.pop_front();
                const Domain * d = aProblem.getVariableById(varId)->getDomain();

                for (Domain::const_iterator domIt = d->begin(); domIt != d->end(); ++domIt) {
                        aEvidence[varId] = *domIt;

                        if (_hasSupportInternal(aEvidence, aProblem, aVarsToAssign))
                                return true; // Once we find support, we don't need to search any further

                        aEvidence.erase(varId);
                }

                aVarsToAssign.push_front(varId);
                
                return false;
        }
}

CSPProblem * load_wcsp_problem(const char * fileName) {
        std::ifstream file(fileName);
        std::string line;

        unsigned int totalVariables, maxDomainSize, totalConstraints;
        unsigned long hardConstraintWeight;

        if (std::getline(file, line)) {
                std::vector<std::string> words;
                tokenize(line, words);

                totalVariables = parseArg<unsigned int>(words[1]);
                maxDomainSize = parseArg<unsigned int>(words[2]);
                totalConstraints = parseArg<unsigned int>(words[3]);
                hardConstraintWeight = parseArg<unsigned long>(words[4]);
        } else {
                return NULL;
        }

        VariableMap * variables = new VariableMap();

        if (std::getline(file, line)) {
                std::istringstream lineStream(line);
                unsigned long domainSize;
                VarIdType varId = 0;

                while (lineStream >> domainSize) {
                        // Create a new variable with domain of <0, domainSize - 1>
                        (*variables)[varId] = new Variable(varId, 0, domainSize - 1);
                        ++varId;
                }
        } else {
                return NULL;
        }


        ConstraintList * constraints = new ConstraintList();
        while (std::getline(file, line)) {
                // Read constraint specification line
                std::vector<std::string> words;
                tokenize(line, words);

                unsigned int constraintArity = parseArg<unsigned int>(words[0]);
                Scope scope;

                for (unsigned int scopeIdx = 0; scopeIdx < constraintArity && scopeIdx < words.size() - 2; ++scopeIdx) {
                        scope.insert(parseArg<VarIdType>(words[scopeIdx + 1]));
                }
                unsigned int defaultWeight = parseArg<unsigned int>(words[words.size() - 2]);
                unsigned int numDifferentTuples = parseArg<unsigned int>(words[words.size() - 1]);

                WCSPConstraint * ctr = new WCSPConstraint(scope, defaultWeight, hardConstraintWeight);

                for (unsigned int i = 0; i < numDifferentTuples; ++i) {
                        if (std::getline(file, line)) {
                                std::vector<VarType> tuple; 
                                std::vector<std::string> tupleWords;
                                tokenize(line, tupleWords);

                                for (unsigned j = 0; j < constraintArity; ++j) {
                                       tuple.push_back(parseArg<VarType>(tupleWords[j]));
                                }
                                unsigned int tupleWeight = parseArg<unsigned int>(tupleWords[tuple.size() - 1]);

                                ctr->addDifferentWeightTuple(tuple, tupleWeight);
                        } else {
                                break;
                        }
                }

                constraints->push_back(ctr);
        }

        return new CSPProblem(variables, constraints);
}

