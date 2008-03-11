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

#ifndef INTEL_H_
#define INTEL_H_

#include "csp.h"

class IntelEqualityConstraint: public Constraint {
public:
        IntelEqualityConstraint(VarIdType aVar1, VarIdType aVar2, VarIdType aIntervalVar, unsigned int aWeight):
                mVar1(aVar1), mVar2(aVar2), mIntervalVar(aIntervalVar), mWeight(aWeight) {};

        virtual double operator()(Assignment &a) const;

        virtual Scope getScope() const {
                Scope s;
                s.insert(mVar1);
                s.insert(mVar2);
                s.insert(mIntervalVar);
                return s;
        };

        virtual bool isSoft() const {
                return mWeight > 0;
        };

        virtual bool hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence);
private:
        VarIdType mVar1, mVar2, mIntervalVar;
        unsigned int mWeight;
};

class IntelInequalityConstraint: public Constraint {
public:
        IntelInequalityConstraint(VarIdType aVar1, VarIdType aVar2, unsigned int aWeight):
                mVar1(aVar1), mVar2(aVar2), mWeight(aWeight) {};

        virtual double operator()(Assignment &a) const;

        virtual Scope getScope() const {
                Scope s;
                s.insert(mVar1);
                s.insert(mVar2);
                return s;
        };

        virtual bool isSoft() const {
                return mWeight > 0;
        };

        virtual bool hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence);
private:
        VarIdType mVar1, mVar2;
        unsigned int mWeight;
};

class IntelIntervalsNotEqualConstraint: public Constraint {
public:
        IntelIntervalsNotEqualConstraint(VarIdType aVar1, VarIdType aVar2, VarIdType aVar3, VarIdType aVar4, unsigned int aWeight):
                mVar1(aVar1), mVar2(aVar2), mVar3(aVar3), mVar4(aVar4), mWeight(aWeight) {};

        virtual double operator()(Assignment &a) const;

        virtual Scope getScope() const {
                Scope s;
                s.insert(mVar1);
                s.insert(mVar2);
                s.insert(mVar3);
                s.insert(mVar4);
                return s;
        };

        virtual bool isSoft() const {
                return mWeight > 0;
        };

        virtual bool hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment &aEvidence);
private:
        VarIdType mVar1, mVar2, mVar3, mVar4;
        unsigned int mWeight;
};

/**
 * Functions to load Intel data from files
 */

ConstraintList * intel_load_constraints(const char * fileName);

ConstraintList * intel_load_interval_inequality_constraints(const char * fileName);

/**
 * Loads variables and adds possible additional modification constraints
 */
void intel_load_variables(const char * fileName, const std::vector<Domain> * domains,
                VariableMap * variables, ConstraintList * constraints);

/**
 * Loads domains from the specified file
 */
std::vector<Domain> * intel_load_domains(const char * fileName);

#endif // INTEL_H_

