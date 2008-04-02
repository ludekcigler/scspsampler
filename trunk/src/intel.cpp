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
#include <math.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>

#include "csp.h"
#include "intel.h"
#include "utils.h"

const double EXP_ROOT = 2.0;

double IntelEqualityConstraint::operator()(Assignment &a) const {
        bool satisfied = (abs(a[mVar1] - a[mVar2]) == a[mIntervalVar]);

        if (mWeight == 0) {
                return satisfied ? 1.0 : EPSILON; // Let's give a smallish chance to unfeasible solutions
        } else {
                return exp(log(EXP_ROOT) * 5 * (double)satisfied);
        }
}

double IntelInequalityConstraint::operator()(Assignment &a) const {
        bool satisfied = (a[mVar1] != a[mVar2]);

        if (mWeight == 0) {
                return satisfied ? 1.0 : EPSILON; // Let's give a smallish chance to unfeasible solutions
        } else {
                return exp(log(EXP_ROOT) * 5 * (double)satisfied);
        }
}

double IntelIntervalsNotEqualConstraint::operator()(Assignment &a) const {
        bool satisfied = (abs(a[mVar1] - a[mVar2]) != abs(a[mVar3] - a[mVar4]));

        if (mWeight == 0) {
                return satisfied ? 1.0 : EPSILON; // Let's give a smallish chance to unfeasible solutions
        } else {
                return exp(log(EXP_ROOT) * 5 * (double)satisfied);
        }
}

double IntelEqualToConstantConstraint::operator()(Assignment &a) const {
        bool satisfied = (a[mVar] == mTargetValue);

        if (mWeight == 0) {
                return satisfied ? 1.0 : EPSILON; // Let's give a smallish chance to unfeasible solutions
        } else {
                return exp(log(EXP_ROOT) * 5 * (double)satisfied);
        }
}

bool IntelEqualityConstraint::hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence) {
        if (isSoft())
                return true;

        if (aVarId == mIntervalVar) {
                // Check if any two other values can satisfy the constraint
                const Domain * domain1 = aProblem.getVariableById(mVar1)->getDomain();
                const Domain * domain2 = aProblem.getVariableById(mVar2)->getDomain();

                for (Domain::const_iterator domIt1 = domain1->begin(); domIt1 != domain1->end(); ++domIt1) {
                        if (domain2->find(*domIt1 + aValue) != domain2->end() ||
                                        domain2->find(*domIt1 - aValue) != domain2->end()) {

                                return true; // Support indeed exists
                        }
                }
                return false;
        } else {
                VarIdType otherVarId = (aVarId == mVar1 ? mVar2 : mVar1);
                const Domain * otherVarDomain = aProblem.getVariableById(otherVarId)->getDomain();

                const Domain * intervalVarDomain = aProblem.getVariableById(mIntervalVar)->getDomain();

                for (Domain::const_iterator otherDomIt = otherVarDomain->begin(); otherDomIt != otherVarDomain->end(); ++otherDomIt) {
                        if (intervalVarDomain->find(abs(aValue - *otherDomIt)) != intervalVarDomain->end()) {
                                return true;
                        }
                }
                return false;
        }
}

bool IntelInequalityConstraint::hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence) {
        if (isSoft())
                return true;

        VarIdType otherVarId = (aVarId == mVar1 ? mVar2 : mVar1);

        const Domain * otherVarDomain = aProblem.getVariableById(otherVarId)->getDomain();
        if (otherVarDomain->size() > 1) {
                return true;
        } else if (otherVarDomain->size() == 1 && otherVarDomain->find(aValue) == otherVarDomain->end()) {
                return true;
        } else {
                return false; // Domain empty or containing only aValue
        }
}

bool IntelIntervalsNotEqualConstraint::hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence) {
        // These constraints are only soft
        return true;
}

bool IntelEqualToConstantConstraint::hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence) {
        if (isSoft())
                return true;

        return (aValue == mTargetValue);
}

Constraint * intel_load_equal_to_constant_constraint(const std::vector<std::string> & aWords) {
        return new IntelEqualToConstantConstraint(parseArg<VarIdType>(aWords[1]),
                        parseArg<VarType>(aWords[2]), parseArg<unsigned int>(aWords[3]));
}

ConstraintList * intel_load_constraints(const char * fileName) {
        std::ifstream file(fileName);
        std::string line;

        ConstraintList * result = new ConstraintList();

        while (std::getline(file, line)) {
                std::vector<std::string> words;
                tokenize(line, words);

                if (words.size() < 4)
                        continue;

                if (words[0] == "EQ" && words.size() >= 5) {
                        result->push_back(new IntelEqualityConstraint(parseArg<VarIdType>(words[1]),
                                                parseArg<VarIdType>(words[2]), parseArg<VarIdType>(words[3]),
                                                parseArg<unsigned int>(words[4])));
                } else if (words[0] == "NEQ") {
                        result->push_back(new IntelInequalityConstraint(parseArg<VarIdType>(words[1]), parseArg<VarIdType>(words[2]),
                                                parseArg<unsigned int>(words[3])));
                } else if (words[0] == "EQC") {
                        result->push_back(intel_load_equal_to_constant_constraint(words));
                }
        }

        return result;
}

ConstraintList * intel_load_interval_inequality_constraints(const char * fileName) {
        std::ifstream file(fileName);
        std::string line;

        ConstraintList * result = new ConstraintList();

        while (std::getline(file, line)) {
                std::vector<std::string> words;
                tokenize(line, words);


                if (words[0] == "INEQ") {
                        result->push_back(new IntelIntervalsNotEqualConstraint(parseArg<VarIdType>(words[1]),
                                        parseArg<VarIdType>(words[2]), parseArg<VarIdType>(words[3]),
                                        parseArg<VarIdType>(words[4]),
                                        parseArg<unsigned int>(words[5])));
                } else if (words[0] == "NEQ") {
                        result->push_back(new IntelInequalityConstraint(parseArg<VarIdType>(words[1]), parseArg<VarIdType>(words[2]),
                                                parseArg<unsigned int>(words[3])));
                } else if (words[0] == "EQC") {
                        result->push_back(intel_load_equal_to_constant_constraint(words));
                }
        }

        return result;
}


void intel_load_variables(const char * fileName, const std::vector<Domain> * domains,
                VariableMap * variables, ConstraintList * constraints) {
        assert(domains);
        assert(variables);
        assert(constraints);

        std::ifstream file(fileName);
        std::string line;

        while (std::getline(file, line)) {
                std::vector<std::string> words;
                tokenize(line, words);

                if (words.size() < 2)
                        continue; // TODO: throw an exception

                VarIdType varId = atol(words[0].c_str());

                unsigned int domainId = atol(words[1].c_str());
                if (domainId >= domains->size())
                        continue; // TODO: throw an exception

                Domain *d = new Domain((*domains)[domainId]);
                (*variables)[varId] = new Variable(varId, d);
        }
}

std::vector<Domain> * intel_load_domains(const char * fileName) {
        std::ifstream file(fileName);
        std::string line;

        std::vector<Domain> * result = new std::vector<Domain>();

        while (std::getline(file, line)) {
                std::vector<std::string> words;
                tokenize(line, words);

                Domain d;
                std::vector<std::string>::iterator it = words.begin();
                ++it;
                for (; it != words.end(); ++it) {
                        d.insert(atol(it->c_str()));
                }
                result->push_back(d);
        }
        return result;
}
