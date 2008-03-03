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

#ifndef CELAR_H_
#define CELAR_H_

#include "csp.h"

enum CelarOperator {
        CELAR_OPERATOR_EQ,
        CELAR_OPERATOR_LT,
        CELAR_OPERATOR_GT
};

enum {
        CELAR_MAX_CONSTRAINT_WEIGHT = 4,
        CELAR_SOFT_CONSTRAINT_WEIGHT_STEP = 10
};

class CelarInterferenceConstraint: public Constraint {
public:
        CelarInterferenceConstraint(VarIdType var1, VarIdType var2, CelarOperator op, VarType targetValue, unsigned int weight):
                mVar1(var1), mVar2(var2), mOperator(op), mTargetValue(targetValue), mWeight(weight) {};

        virtual double operator()(Assignment &a) const;

        virtual Scope getScope() const {
                Scope s;
                s.insert(mVar1);
                s.insert(mVar2);
                return s;
        }

        static std::vector<double> COSTS;
protected:
        VarIdType mVar1, mVar2;
        CelarOperator mOperator;
        VarType mTargetValue;
        unsigned int mWeight;
};

class CelarModificationConstraint: public Constraint {
public:
        CelarModificationConstraint(VarIdType var, VarType defaultValue, unsigned int weight):
                mVar(var), mDefaultValue(defaultValue), mWeight(weight) {};

        virtual double operator()(Assignment &a) const;

        virtual Scope getScope() const {
                Scope s;
                s.insert(mVar);
                return s;
        }

        static std::vector<double> COSTS;
protected:
        VarIdType mVar;
        VarType mDefaultValue;
        unsigned int mWeight;
};

/**
 * Functions to load celar data from files
 */

ConstraintList * celar_load_constraints(const char * fileName);

/**
 * Loads variables and adds possible additional modification constraints
 */
void celar_load_variables(const char * fileName, const std::vector<Domain> * domains,
                VariableMap * variables, ConstraintList * constraints);

/**
 * Loads domains from the specified file
 */
std::vector<Domain> * celar_load_domains(const char * fileName);

/**
 * Load costs for CelarModificationConstraint and CelarInterferenceConstraint
 */
void celar_load_costs(const char * fileName);

#endif // CELAR_H_

