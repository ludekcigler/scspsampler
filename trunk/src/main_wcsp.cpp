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

#include "optparse/optparse.h"

#include "csp.h"
#include "wcsp.h"
#include "ijgp.h"
#include "gibbs_sampler.h"
#include "ijgp_sampler.h"
#include "interval_ijgp_sampler.h"
#include "utils.h"


int main(int argc, char ** argv) {
        OptionParser parser(argc, argv, "scspsampler [OPTIONS] ARGS");

        parser.addOption(/*aShortName*/ 's', /*aLongName*/ "sampler", /*aAlias*/ "sampler",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "ijgp", /*aHelpText*/ "Sampler type; Either \"gibbs\", \"ijgp\" or \"interval-ijgp\"");

        parser.addOption(/*aShortName*/ 0, /*aLongName*/ "dataset", /*aAlias*/ "dataset",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "/home/luigi/Matfyz/Diplomka/scspsampler/trunk/data/wcsp/celar_small.wcsp",
                        /*aHelpText*/ "Path to CELAR dataset directory");

        parser.addOption(/*aShortName*/ 0, /*aLongName*/ "ijgpIter", /*aAlias*/ "ijgpIter",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ false,
                        /*aArg*/ "10", /*aHelpText*/ "Maximum number of iterations in one IJGP run");

        parser.addOption(/*aShortName*/ 'b', /*aLongName*/ "bucketSize", /*aAlias*/ "bucketSize",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "3", /*aHelpText*/ "Maximum size of a single mini-bucket");

        parser.addOption(/*aShortName*/ 'p', /*aLongName*/ "ijgpProbability", /*aAlias*/ "ijgpProbability",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "1.0", /*aHelpText*/ "Probability with which the IJGP is performed during sampling");

        parser.addOption(/*aShortName*/ 0, /*aLongName*/ "burnIn", /*aAlias*/ "burnIn",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "1000", /*aHelpText*/ "Number of burn-in steps for the Gibbs sampler");

        parser.addOption(/*aShortName*/ 'n', /*aLongName*/ "numSamples", /*aAlias*/ "numSamples",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "1", /*aHelpText*/ "Number of samples we should generate");

        parser.addOption(/*aShortName*/ 0, /*aLongName*/ "domainIntervals", /*aAlias*/ "domainIntervals",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "5", /*aHelpText*/ "Maximum number of domain intervals (for interval-IJGP)");

        parser.addOption(/*aShortName*/ 0, /*aLongName*/ "valuesFromInterval", /*aAlias*/ "valuesFromInterval",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "2", /*aHelpText*/ "Maximum number of values from a single interval (for interval-IJGP)");

        parser.addOption(/*aShortName*/ 'k', /*aLongName*/ "koef", /*aAlias*/ "koef",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "0.01", /*aHelpText*/ "Default weight koefitient for constraints");

        try {
                parser.parseOptions();
        } catch (const OptionNotRecognized & e) {
                std::cerr << e.what() << std::endl << std::endl;
                std::cout << parser.usage();
                return EXIT_FAILURE;
        }

        if (parser.isSpecified("help")) {
                std::cout << parser.usage();
                return EXIT_SUCCESS;
        }

        // Load info about CSP problem
        std::string dataDir = parser.getOptionArg("dataset");

        CSPProblem * p = load_wcsp_problem(dataDir.c_str());

        std::string samplerId = parser.getOptionArg("sampler");
        CSPSampler * sampler;
        EXP_K = parseArg<double>(parser.getOptionArg("koef"));

        unsigned int numSamples = parseArg<unsigned int>(parser.getOptionArg("numSamples"));
        std::cout << "PARAMS:" << std::endl;
        std::cout << "sampler:\t" << samplerId << std::endl;
        std::cout << "dataset:\t" << dataDir << std::endl;
        std::cout << "numSamples:\t" << numSamples << std::endl;
        std::cout << "koef:\t" << EXP_K << std::endl;

        if (samplerId == "ijgp") {
                int miniBucketSize = parseArg<int>(parser.getOptionArg("bucketSize"));
                unsigned int ijgpIter = parseArg<unsigned int>(parser.getOptionArg("ijgpIter"));
                double ijgpProbability = parseArg<double>(parser.getOptionArg("ijgpProbability"));

                sampler = new IJGPSampler(p, miniBucketSize, ijgpProbability, ijgpIter);
                std::cout << "mini-bucket size:\t" << miniBucketSize << std::endl;
                std::cout << "IJGP probability:\t" << ijgpProbability << std::endl;
                std::cout << "IJGP iterations:\t" << ijgpIter << std::endl;
        } else if (samplerId == "gibbs") {
                int burnIn = parseArg<int>(parser.getOptionArg("burnIn"));

                sampler = new GibbsSampler(p, burnIn);
                std::cout << "burn-in:\t" << burnIn << std::endl;
        } else if (samplerId == "interval-ijgp") {
                int miniBucketSize = parseArg<int>(parser.getOptionArg("bucketSize"));
                unsigned int ijgpIter = parseArg<unsigned int>(parser.getOptionArg("ijgpIter"));
                double ijgpProbability = parseArg<double>(parser.getOptionArg("ijgpProbability"));
                unsigned int maxDomainIntervals = parseArg<unsigned int>(parser.getOptionArg("domainIntervals"));
                unsigned int maxValuesFromInterval = parseArg<unsigned int>(parser.getOptionArg("valuesFromInterval"));


                std::cout << "mini-bucket size:\t" << miniBucketSize << std::endl;
                std::cout << "IJGP probability:\t" << ijgpProbability << std::endl;
                std::cout << "IJGP iterations:\t" << ijgpIter << std::endl;
                std::cout << "Max domain intervals:\t" << maxDomainIntervals << std::endl;
                std::cout << "Max values from interval:\t" << maxValuesFromInterval << std::endl;

                sampler = new IntervalIJGPSampler(p, miniBucketSize, ijgpProbability, ijgpIter, maxDomainIntervals,
                                maxValuesFromInterval);
        }
        std::cout << std::endl;


        Assignment a;
        for (unsigned int i = 0; i < numSamples; ++i) {
                bool solutionExists = sampler->getSample(a);
                if (solutionExists) {
                        std::cout << "SAMPLE " << p->evalAssignment(a) << " | ";
                        std::cout << assignment_pprint(a);
                        std::cout << std::endl;
                } else {
                        std::cout << "No solution exists." << std::endl;
                        break;
                }
        }

        return EXIT_SUCCESS;
}

