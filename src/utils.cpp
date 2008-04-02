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

const double EPSILON = 1.0e-25;

VarType random_select(const Domain *d) {
        assert(d);

        unsigned int selectedIndex = (unsigned int)(d->size() * (rand() / (RAND_MAX + 1.0)));
        unsigned int domainCounter = 0;
        for (Domain::const_iterator domIt = d->begin(); domIt != d->end(); ++domIt, ++domainCounter) {
                if (selectedIndex == domainCounter)
                        return (*domIt);
        }

        assert(false);
}

VarType random_select(const Domain *aDomain, VarType aLowerBound, VarType aUpperBound) {
        assert(aDomain);
        Domain::const_iterator aBegin = aDomain->lower_bound(aLowerBound);
        Domain::const_iterator aEnd = aDomain->lower_bound(aUpperBound);

        unsigned int domainSize = 0;
        for (Domain::const_iterator domIt = aBegin; domIt != aEnd; ++domIt) {
                ++domainSize;
        }
        
        /*
        std::cout << "Random select" << std::endl;
        std::cout << "\tDomain size " << domainSize << std::endl;
        std::cout << "\tLower bound " << aLowerBound << ", " << ((aBegin != aDomain->end()) ? *aBegin : -1) << std::endl;
        std::cout << "\tUpper bound " << aUpperBound << ", " << ((aEnd != aDomain->end()) ? *aEnd : -1) << std::endl;
        */

        unsigned int selectedIndex = (unsigned int)(domainSize * (rand() / (RAND_MAX + 1.0)));
        unsigned int domainCounter = 0;
        for (Domain::const_iterator domIt = aBegin; domIt != aEnd; ++domIt, ++domainCounter) {
                if (selectedIndex == domainCounter)
                        return (*domIt);
        }

        std::cout << "Random select" << std::endl;
        std::cout << "\tDomain size " << domainSize << std::endl;
        std::cout << "\tLower bound " << aLowerBound << ", " << ((aBegin != aDomain->end()) ? *aBegin : -1) << std::endl;
        std::cout << "\tUpper bound " << aUpperBound << ", " << ((aEnd != aDomain->end()) ? *aEnd : -1) << std::endl;

        assert(false);
}

void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters)
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the std::vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

int min(int x, int y) {
        return (((x) < (y)) ? (x) : (y));
}

double min(double x, double y) {
        return (((x) < (y)) ? (x) : (y));
}

int max(int x, int y) {
        return (((x) > (y)) ? (x) : (y));
}

unsigned long max(unsigned long x, unsigned long y) {
        return (((x) > (y)) ? (x) : (y));
}

double max(double x, double y) {
        return (((x) > (y)) ? (x) : (y));
}
