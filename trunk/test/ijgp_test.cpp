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
#include <vector>

#include "../src/celar.h"
#include "../src/ijgp.h"
#include "../src/csp.h"
#include "../src/ijgp_sampler.h"

const double CELAR_MOBILITY_COSTS[] = {0, 0, 0, 0};
const double CELAR_INTERFERENCE_COSTS[] = {1.0, 1, 1, 1};
const unsigned int CELAR_MOBILITY_COSTS_SIZE = 4;
const unsigned int CELAR_INTERFERENCE_COSTS_SIZE = 4;

const unsigned int IJGP_MAX_ITERATIONS = 10;

int main(int argc, char ** argv) {

        celar_load_costs("data/ludek/03/costs.txt");
        ConstraintList * c = celar_load_constraints("data/ludek/03/ctr.txt");
        std::vector<Domain> * d = celar_load_domains("data/ludek/03/dom.txt");
        VariableMap * v = new VariableMap();
        celar_load_variables("data/ludek/03/var.txt", d, v, c);

        CSPProblem * p = new CSPProblem(v, c);

        JoinGraph * jg = JoinGraph::createJoinGraph(p, 3);
        jg->pprint();

        IJGPSampler * sampler = new IJGPSampler(p, 3, 1.0, 10);
        
        Assignment a;
        for (int i = 0; i < 100; ++i) {
                if (!sampler->getSample(a)) {
                        std::cout << "No solution exists" << std::endl;
                        return EXIT_FAILURE;
                }
                std::cout << "New sample, eval " << p->evalAssignment(a) << " ";
                assignment_pprint(a);
                std::cout << std::endl;
        }

        return EXIT_SUCCESS;
}
