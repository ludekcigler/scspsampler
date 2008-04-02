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

#ifndef WCSP_H_
#define WCSP_H_

#include <deque>

#include "csp.h"
#include "utils.h"

class WCSPConstraint: public Constraint {
public:
        WCSPConstraint(const Scope & aScope, unsigned long aDefaultWeight, unsigned long aHardConstraintWeight):
                mScope(aScope), mDefaultWeight(aDefaultWeight), mMaxTupleWeight(aDefaultWeight),
                mHardConstraintWeight(aHardConstraintWeight) {};

        virtual double operator()(Assignment &a) const;

        virtual Scope getScope() const;

        virtual bool isSoft() const;

        virtual bool hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence);

        void addDifferentWeightTuple(std::vector<VarType> aTuple, unsigned long aWeight) {
                mDifferentWeightTuples[aTuple] = aWeight;
                mMaxTupleWeight = max(mMaxTupleWeight, aWeight);

                if (aWeight >= mHardConstraintWeight) {
                        mDisallowedTuples.insert(aTuple);
                }
        };
private:
        bool _hasSupportInternal(Assignment &aEvidence, const CSPProblem &aProblem,
                        std::deque<VarIdType> & aVarsToAssign);

       Scope mScope; 
       std::map<std::vector<VarType>, unsigned long> mDifferentWeightTuples;
       std::set<std::vector<VarType> > mDisallowedTuples;
       unsigned long mDefaultWeight;
       unsigned long mMaxTupleWeight;
       unsigned long mHardConstraintWeight;
};

CSPProblem * load_wcsp_problem(const char * filename);

extern double EXP_K;

#endif // WCSP_H_
