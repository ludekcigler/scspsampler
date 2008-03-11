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

#ifndef UTILS_H_
#define UTILS_H_

VarType random_select(const Domain *d);

VarType random_select(const Domain *aDomain, VarType aLowerBound, VarType aUpperBound);

void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ");

template<class T> T parseArg(const std::string & aArg) {
        std::istringstream istr(aArg);
        T result;

        istr >> result;
        return result;
}

extern const double EPSILON;

int min(int x, int y);

double min(double x, double y);

int max(int x, int y);

double max(double x, double y);

#endif // UTILS_H_

