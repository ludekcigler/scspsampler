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
#include <map>
#include <math.h>

#include "../src/csp.h"
#include "../src/celar.h"
#include "../src/gibbs_sampler.h"

double CELAR_MOBILITY_COSTS[] = {0, 0, 0, 0};
double CELAR_INTERFERENCE_COSTS[] = {1.0, 1, 1, 1};
unsigned int CELAR_MOBILITY_COSTS_SIZE = 4;
unsigned int CELAR_INTERFERENCE_COSTS_SIZE = 4;

const unsigned int GIBBS_SAMPLER_BURN_IN = 1000;

int main(int argc, char ** argv) {
        ConstraintList * c = celar_load_constraints("data/ludek/01/ctr.txt");
        std::vector<Domain> * d = celar_load_domains("data/ludek/01/dom.txt");
        VariableMap * v = new VariableMap();
        celar_load_variables("data/ludek/01/var.txt", d, v, c);

        CSPProblem * p = new CSPProblem(v, c);
        Assignment evidence;
        //evidence[0] = 9;
        std::map<VarIdType, Domain> removedValues;

        if (!p->propagateConstraints(evidence, removedValues)) {
                std::cout << "No solutions at all" << std::endl;
        } else {
                std::cout << "Removed values: " << removedValues.size() << std::endl;
                for (std::map<VarIdType, Domain>::const_iterator valIt = removedValues.begin();
                                valIt != removedValues.end(); ++valIt) {

                        std::cout << valIt->first << ": ";
                        for (Domain::const_iterator domIt = valIt->second.begin(); domIt != valIt->second.end(); ++domIt) {
                                std::cout << *domIt << ", ";
                        }
                        std::cout << std::endl;
                }

                std::cout << "Domains:" << std::endl;
                for (VariableMap::iterator varIt = v->begin(); varIt != v->end(); ++varIt) {
                        const Domain *d = varIt->second->getDomain();

                        std::cout << varIt->first << ": ";
                        for (Domain::const_iterator domIt = d->begin(); domIt != d->end(); ++domIt) {
                                std::cout << *domIt << ", ";
                        }
                        std::cout << std::endl;
                }
        }

        p->restoreDomains(removedValues);
        std::cout << "Restored domains:" << std::endl;
        for (VariableMap::iterator varIt = v->begin(); varIt != v->end(); ++varIt) {
                const Domain *d = varIt->second->getDomain();

                std::cout << varIt->first << ": ";
                for (Domain::const_iterator domIt = d->begin(); domIt != d->end(); ++domIt) {
                        std::cout << *domIt << ", ";
                }
                std::cout << std::endl;
        }


        /*GibbsSampler * sampler = new GibbsSampler(p, GIBBS_SAMPLER_BURN_IN);
        
        Assignment a;
        for (int i = 0; i < 100; ++i) {
                a = sampler->getSample();
                std::cout << "New sample, eval " << p->evalAssignment(a) << " ";
                assignment_pprint(a);
        }*/
        return EXIT_SUCCESS;
}
