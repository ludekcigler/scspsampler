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
#include "celar.h"
#include "utils.h"

double CelarInterferenceConstraint::operator()(Assignment &a) const {
        bool satisfied;
        VarType diff = abs(a[mVar1] - a[mVar2]);

        switch (mOperator) {
        case CELAR_OPERATOR_EQ:
                satisfied = (diff == mTargetValue);
                break;
        case CELAR_OPERATOR_LT:
                satisfied = (diff < mTargetValue);
                break;
        case CELAR_OPERATOR_GT:
                satisfied = (diff > mTargetValue);
                break;
        }

        if (mWeight == 0) {
                return (double)satisfied;
        } else {
                return exp((1.0)*CELAR_INTERFERENCE_COSTS[mWeight - 1] * 
                                ((double)satisfied));
        }
}

double CelarModificationConstraint::operator()(Assignment &a) const {
        bool satisfied = (a[mVar] == mDefaultValue);

        if (mWeight == 0) {
                return (double)satisfied;
        } else {
               return exp(-CELAR_MOBILITY_COSTS[mWeight - 1] * (double)satisfied); 
        }
}


ConstraintList * celar_load_constraints(const char *fileName) {
        std::ifstream file(fileName);
        std::string line;

        ConstraintList * result = new ConstraintList();

        while (std::getline(file, line)) {
                std::vector<std::string> words;
                tokenize(line, words);

                if (words.size() < 5) {
                        std::cout << "Wrong constraint specified: " << line << std::endl;
                        continue;
                }

                VarIdType var1, var2;
                CelarOperator op;
                VarType targetValue;
                unsigned int weight = 0;

                var1 = atol(words[0].c_str());
                var2 = atol(words[1].c_str());
                
                if (words[3] == ">")
                        op = CELAR_OPERATOR_GT;
                else if (words[3] == "<")
                        op = CELAR_OPERATOR_LT;
                else
                        op = CELAR_OPERATOR_EQ;

                targetValue = atol(words[4].c_str());
                
                if (words.size() >= 6)
                        weight = atol(words[5].c_str());

                CelarInterferenceConstraint * c = 
                        new CelarInterferenceConstraint(var1, var2, op, targetValue, weight);
                result->push_back(c);
        }
                

        std::cout << "Constraints: " << result->size() << std::endl;
        return result;
}

void celar_load_variables(const char * fileName, const std::vector<Domain> * domains,
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

                if (words.size() >= 4) {
                        constraints->push_back(
                                new CelarModificationConstraint(varId, atol(words[2].c_str()),
                                        atol(words[3].c_str())));
                }
                
        }
        std::cout << "Variables: " << variables->size() << std::endl;
        std::cout << "Constraints: " << constraints->size() << std::endl;
}

std::vector<Domain> * celar_load_domains(const char * fileName) {
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
        std::cout << "Domains: " << result->size() << std::endl;
        return result;
}
