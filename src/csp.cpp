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
#include <sstream>

#include <queue>

#include "csp.h"
#include "utils.h"

Variable::Variable(const VarIdType &id, VarType aMinValue, VarType aMaxValue):
        mId(id) {
        mDomain = new Domain();

        for (VarType i = aMinValue; i <= aMaxValue; ++i) {
                mDomain->insert(i);
        }
}


CSPProblem::CSPProblem(VariableMap *v, ConstraintList *c):
        mVariables(v), mConstraints(c) {
        assert(v);
        assert(c);

        // Initialize the constraint map
        for (ConstraintList::iterator constIt = mConstraints->begin(); constIt != mConstraints->end(); ++constIt) {
                Scope s = (*constIt)->getScope();
                for (Scope::const_iterator scopeIt = s.begin(); scopeIt != s.end(); ++scopeIt) {
                        mConstraintMap[*scopeIt].push_back(*constIt);
                }
        }
};

CSPProblem::~CSPProblem() {
        for (ConstraintList::iterator constIt = mConstraints->begin(); constIt != mConstraints->end(); ++constIt) {
                delete *constIt;
        }

        for (VariableMap::iterator varIt = mVariables->begin(); varIt != mVariables->end(); ++varIt) {
                delete varIt->second;
        }

        delete mConstraints;
        delete mVariables;
}

double CSPProblem::evalAssignment(Assignment &a) const {

        double evaluation = 1.0;
        for (ConstraintList::const_iterator constIt = mConstraints->begin(); constIt != mConstraints->end(); ++constIt) {
                evaluation *= (**constIt)(a);
        }

        return evaluation;
}

bool scope_compare_size(const Scope & a, const Scope & b) {
        return a.size() > b.size();
}

void CSPProblem::schematicMiniBucket(unsigned int aMaxBucketSize, const std::vector<VarIdType> & aOrdering,
                std::vector<Bucket> * aMiniBuckets, std::map<Scope, Scope> * aOutsideBucketArcs) {

        assert(aMiniBuckets);
        assert(aOutsideBucketArcs);

        // Place function scopes into appropriate buckets
        std::vector<Bucket> buckets(aOrdering.size());
        aMiniBuckets->resize(aOrdering.size());
        std::set<Scope> scopes;
        for (ConstraintList::iterator ctrIt = this->mConstraints->begin(); ctrIt != this->mConstraints->end(); ++ctrIt) {
                Scope s = (*ctrIt)->getScope();
                scopes.insert(s);
        }

        for (int i = aOrdering.size() - 1; i >= 0; --i) {
                for (std::set<Scope>::iterator scIt = scopes.begin(); scIt != scopes.end(); ++scIt) {
                        // If the current vertex is found in a given scope, place it in the corresponding bucket
                        if (scIt->find(aOrdering[i]) != scIt->end()) {
                                buckets[i].push_back(*scIt);
                        }
                }

                // Now erase all the scopes that have been succesfully placed
                for (Bucket::const_iterator scIt = buckets[i].begin(); scIt != buckets[i].end(); ++scIt) {
                        scopes.erase(*scIt);
                }
        }

        /*
        // Print buckets
        for (int i = buckets.size() - 1; i >= 0; --i) {
                std::cout << "bucket[" << aOrdering[i] << "]: ";
                for (Bucket::iterator scopeIt = buckets[i].begin(); scopeIt != buckets[i].end(); ++scopeIt) {
                        scope_pprint(*scopeIt);
                }
                std::cout << std::endl;
        }*/
#ifdef DEBUG
        /*
        // PPrint remaining scopes
        for (std::set<Scope>::iterator scIt = scopes.begin(); scIt != scopes.end(); ++scIt) {
                scope_pprint(*scIt);
                std::cout << std::endl;
        }
        */
#endif

        assert(scopes.empty());

        std::map<Scope, Scope> futureArcPool;

        for (int i = buckets.size() - 1; i >= 0; --i) {
                // Partition bucket i into mini-buckets
                
                std::vector<Scope> miniBuckets;

                /**
                 * When an outside-bucket arc has to be created, we need to first store a
                 * pointer to an iterator for the mini-bucket to which it has to point.
                 * This map stores these pointers.
                 *
                 * key          Mini-bucket from some previously handled bucket
                 * value        Iterator to a mini-bucket in the currently handled bucket (bucket[i])
                 */
                std::map<Scope, size_t > outsideBucketArcPointers;

                // Sort the scopes in the bucket in descending order according to their size
                std::sort(buckets[i].begin(), buckets[i].end(), scope_compare_size);

                for (Bucket::iterator scopeIt = buckets[i].begin(); scopeIt != buckets[i].end(); ++scopeIt) {
                        // Place the scope in the right mini-bucket using best-fit heuristics
                        // ie. the scope is placed (ideally) in a mini-bucket which is a superset of the scope

                        std::vector<Scope>::iterator bestFitBucket = miniBuckets.end();
                        size_t bestFitSize = aMaxBucketSize + 1; 
                        size_t bestFitBucketIndex = -1;
                        size_t currentBucketIndex = 0;
                        // We cannot do any better than aMaxBucketSize (this has also the nice effect that
                        // when the scope of a function is larger than aMaxBucketSize, it gets added as a new separate bucket
                        
                        Scope bestFitUnionScope;

                        for (std::vector<Scope>::iterator mbIt = miniBuckets.begin(); mbIt != miniBuckets.end(); 
                                        ++mbIt, ++currentBucketIndex ) {

                                Scope unionScope;
                                std::set_union(scopeIt->begin(), scopeIt->end(), mbIt->begin(), mbIt->end(),
                                                std::inserter(unionScope, unionScope.begin()));

                                if (unionScope.size() == mbIt->size()) {
                                        // The mini-bucket is a superset of the scope
                                        bestFitBucket = mbIt;
                                        bestFitUnionScope = unionScope;
                                        bestFitBucketIndex = currentBucketIndex;
                                        break; // No need to search any longer
                                }

                                if (unionScope.size() < bestFitSize) {
                                        // Otherwise we store the current best fit
                                        bestFitBucket = mbIt;
                                        bestFitSize = unionScope.size();
                                        bestFitUnionScope = unionScope;
                                        bestFitBucketIndex = currentBucketIndex;
                                }
                        }

                        if (bestFitBucket != miniBuckets.end()) {
                                *bestFitBucket = bestFitUnionScope;
                        } else {
                                // Add a new mini-bucket
                                bestFitBucketIndex = miniBuckets.size();
                                bestFitBucket = miniBuckets.insert(miniBuckets.end(), *scopeIt);
                                bestFitUnionScope = *scopeIt;
                        }

                        // Store any link which might have been created in the past (now we know the mini-bucket in which
                        // the scope-functions from the past have fit)
                        std::map<Scope, Scope>::iterator futureArcIt = futureArcPool.find(*scopeIt);

                        if (futureArcIt != futureArcPool.end()) {
                                // There is indeed an arc stored in the past,
                                // so we create the outside-bucket arc pointer (an index of the best-fit bucket)
                                assert(bestFitBucketIndex >= 0);
                                outsideBucketArcPointers[futureArcIt->second] = bestFitBucketIndex;

                                futureArcPool.erase(futureArcIt);
                        }
                }

                // Add the outside-bucket arcs based on pointers stored in the outsideBucketArcPointers map
                while (!outsideBucketArcPointers.empty()) {
                        std::map<Scope, size_t>::iterator pointerIt = outsideBucketArcPointers.begin();

                        (*aOutsideBucketArcs)[pointerIt->first] = miniBuckets[pointerIt->second];

                        outsideBucketArcPointers.erase(pointerIt);
                }

                assert(outsideBucketArcPointers.empty());

                // Add new scope functions (with the current variable removed) to the appropriate bucket
                // and add a link to a future mini-bucket containing this scope
                
                for (std::vector<Scope>::iterator mbIt = miniBuckets.begin(); mbIt != miniBuckets.end(); ++mbIt) {
                        Scope smallScope = *mbIt;
                        smallScope.erase(aOrdering[i]);

                        // Do not add empty scope
                        if (smallScope.empty())
                                continue;

                        // Store the new scope function into the appropriate bucket
                        int maxBucket = -1;
                        for (Scope::iterator smallScIt = smallScope.begin(); smallScIt != smallScope.end(); ++smallScIt) {
                                maxBucket = max(std::find(aOrdering.begin(), aOrdering.end(), *smallScIt) - aOrdering.begin(), maxBucket);
                        }
                        if (maxBucket >= 0)
                                buckets[maxBucket].push_back(smallScope);

                        // Store pointer to a future arc
                        futureArcPool[smallScope] = *mbIt;
                }

#ifdef DEBUG
                /*
                std::cout << "mini-bucket[" << aOrdering[i] << "]: ";
                for (std::vector<Scope>::iterator mbIt1 = miniBuckets.begin(); mbIt1 != miniBuckets.end(); ++mbIt1) {

                        scope_pprint(*mbIt1);
                }
                std::cout << std::endl;
                */
#endif

                (*aMiniBuckets)[i] = miniBuckets;
        }

        assert(futureArcPool.empty());

}

void Constraint::addIntervalProbability(DomainIntervalAssignment &aAssignment, double aProbability) {
        Scope scope = getScope();

        for (Scope::iterator scopeIt = scope.begin(); scopeIt != scope.end(); ++scopeIt) {
                assert(aAssignment.find(*scopeIt) != aAssignment.end());

                VarIdType varId = *scopeIt;
                DomainIntervalMap & varIntervals = mDomainIntervals[varId];

                DomainIntervalMap::iterator domIt = varIntervals.find(aAssignment[varId]);
                assert(domIt != varIntervals.end());

                domIt->second += aProbability;
        }
}

void Constraint::initDomainIntervals(CSPProblem &aProblem, unsigned int aMaxIntervals) {
        Scope scope = getScope();
        mDomainIntervals.clear();

        std::map<VarIdType, std::map<VarType, double> > varProbabilities;
        double totalProbability;
        Assignment a;
        
        // First store probabilities for each of the variables in
        // tables indexed by their values

        // Generate all combinations of values for variables in the constraint scope
        _initDomainIntervalsInternal(scope, aProblem, a, varProbabilities, totalProbability);

        // If the total probability is zero, the constraint cannot be satisfied, and if it is a hard
        // constraint, we clear all domains
        if (totalProbability <= 0.0 && !isSoft()) {
                for (Scope::const_iterator scopeIt = scope.begin(); scopeIt != scope.end(); ++scopeIt) {
                        Variable * var = aProblem.getVariableById(*scopeIt);
                        var->clearDomain();
                        std::map<VarType, double> & varProbTable = varProbabilities[*scopeIt];
                        varProbTable.clear();
                }
        } else if (totalProbability <= 0.0 && isSoft()) {
                // Re-initialize the probabilities to equal values
                totalProbability = 1.0;

                for (Scope::const_iterator scopeIt = scope.begin(); scopeIt != scope.end(); ++scopeIt) {
                        const Variable * var = aProblem.getVariableById(*scopeIt);
                        const Domain * domain = var->getDomain();

                        std::map<VarType, double> & varProbTable = varProbabilities[*scopeIt];
                        for (std::map<VarType, double>::iterator probIt = varProbTable.begin();
                                probIt != varProbTable.end(); ++probIt) {

                                probIt->second = 1.0 / domain->size();
                        }
                }
        } 

        // Now create intervals
        for (Scope::const_iterator scopeIt = scope.begin(); scopeIt != scope.end(); ++scopeIt) {

                std::map<VarType, double> & varProbTable = varProbabilities[*scopeIt];
                for (std::map<VarType, double>::iterator probIt = varProbTable.begin();
                        probIt != varProbTable.end(); ++probIt) {

                        if (probIt->second > 0.0) {
                                mDomainIntervals[*scopeIt][DomainInterval(probIt->first, probIt->first + 1)] = 
                                        probIt->second / totalProbability;
                        }
                }

                //std::cout << "Initialized " << *scopeIt << ": " << interval_list_pprint(mDomainIntervals[*scopeIt]);

                // Join the intervals
                mDomainIntervals[*scopeIt] = join_intervals(mDomainIntervals[*scopeIt], aMaxIntervals);
        }
}

void Constraint::_initDomainIntervalsInternal(Scope &aScope, const CSPProblem &aProblem, Assignment & aAssignment,
                std::map<VarIdType, std::map<VarType, double> > &outVarProbabilities, double &outTotalProbability) {

        if (aScope.empty()) {
                double probability = (*this)(aAssignment);
                outTotalProbability += probability;

                Scope constraintScope = getScope();
                for (Scope::const_iterator scopeIt = constraintScope.begin();
                                scopeIt != constraintScope.end(); ++scopeIt) {

                        VarIdType varId = *scopeIt;
                        std::map<VarType, double> & varProbabilities = outVarProbabilities[varId];
                        if (varProbabilities.find(aAssignment[varId]) == varProbabilities.end()) {
                                varProbabilities[aAssignment[varId]] = probability;
                        } else {
                                varProbabilities[aAssignment[varId]] += probability;
                        }
                }
        } else {
                const Variable * var = aProblem.getVariableById(*(aScope.begin()));
                aScope.erase(aScope.begin());
                const Domain * domain = var->getDomain();
                for (Domain::const_iterator domIt = domain->begin(); domIt != domain->end(); ++domIt) {
                        aAssignment[var->getId()] = *domIt;

                        _initDomainIntervalsInternal(aScope, aProblem, aAssignment, outVarProbabilities,
                                        outTotalProbability);

                        aAssignment.erase(var->getId());
                }

                aScope.insert(var->getId());
        }
}

bool CSPProblem::propagateConstraints(Assignment &aEvidence, std::map<VarIdType, Domain> & outRemovedValues) {
        // Add all constraints into the queue
        std::set<std::pair<Constraint *, VarIdType> > constraintQueue;

        // Initialize the queue with all of the constraints
        for (ConstraintList::iterator ctrIt = mConstraints->begin();
                        ctrIt != mConstraints->end(); ++ctrIt) {

                Scope s = (*ctrIt)->getScope();

                for (Scope::const_iterator scopeIt = s.begin(); scopeIt != s.end(); ++scopeIt) {
                        // Add the variable for revision only if it is not in the evidence
                        if (aEvidence.find(*scopeIt) == aEvidence.end())
                                constraintQueue.insert(std::make_pair(*ctrIt, *scopeIt));
                }
        }

        return _propagateConstraintsInternal(aEvidence, constraintQueue, outRemovedValues);
}

/**
 * This version of constraint propagation only starts propagating from constraints which affect given
 * variable
 */
bool CSPProblem::propagateConstraints(Assignment &aEvidence, 
                std::map<VarIdType, Domain> & outRemovedValues, VarIdType aChangedVariable) {

        std::set<std::pair<Constraint *, VarIdType> > constraintQueue;

        // Initialize the queue of constraints from the list of constraints which
        // have the aChangedVariable in their scopes
        for (ConstraintList::iterator ctrIt = mConstraintMap[aChangedVariable].begin();
                        ctrIt != mConstraintMap[aChangedVariable].end(); ++ctrIt) {

                Scope s = (*ctrIt)->getScope();
                s.erase(aChangedVariable);

                for (Scope::const_iterator scopeIt = s.begin(); scopeIt != s.end(); ++scopeIt) {
                        // Add the variable for revision only if it is not in the evidence
                        if (aEvidence.find(*scopeIt) == aEvidence.end())
                                constraintQueue.insert(std::make_pair(*ctrIt, *scopeIt));
                }
        }

        return _propagateConstraintsInternal(aEvidence, constraintQueue, outRemovedValues);
}

bool CSPProblem::_propagateConstraintsInternal(Assignment &aEvidence, 
                std::set<std::pair<Constraint *, VarIdType> > & aConstraintQueue,
                std::map<VarIdType, Domain> & outRemovedValues) {

        while (!aConstraintQueue.empty()) {
                std::set<std::pair<Constraint *, VarIdType> >::iterator queueIt = aConstraintQueue.begin();
                aConstraintQueue.erase(queueIt);

                bool domainChanged = false;

                Constraint * c = queueIt->first;
                VarIdType varId = queueIt->second;

                const Domain *d = getVariableById(varId)->getDomain();

                // Revise the selected constraint
                Domain::const_iterator domIt = d->begin();
                while (domIt != d->end()) {
                        VarType value = *domIt;
                        // Check support for the value, and if there is no support, remove that variable
                        if (!c->hasSupport(varId, value, *this, aEvidence)) {
                                domainChanged = true;

                                // Remove the variable
                                (*mVariables)[varId]->eraseFromDomain(value);
                                outRemovedValues[varId].insert(value);

                                domIt = d->lower_bound(value); // Restore the iterator (which was invalidated by the erase)
                                continue;
                        }
                        ++domIt;
                }

                if (d->empty()) {
                        // If the domain becomes empty, end the propagation with failure
                        return false;
                }


                if (domainChanged) {
                        // Add all constraints with the scope of the current variable to the queue
                        for (ConstraintList::iterator ctrIt = mConstraintMap[varId].begin();
                                        ctrIt != mConstraintMap[varId].end(); ++ctrIt) {
                                Scope s = (*ctrIt)->getScope();
                                s.erase(varId);
                                
                                for (Scope::iterator scopeIt = s.begin(); scopeIt != s.end(); ++scopeIt) {
                                        if (aEvidence.find(*scopeIt) == aEvidence.end())
                                                aConstraintQueue.insert(std::make_pair(*ctrIt, *scopeIt));

                                }
                        }
                }
        }
        return true;
}

void CSPProblem::restoreDomains(const std::map<VarIdType, Domain> &aRemovedValues) {
        for (std::map<VarIdType, Domain>::const_iterator valIt = aRemovedValues.begin();
                        valIt != aRemovedValues.end(); ++valIt) {
                VarIdType varId = valIt->first;
                const Domain & domain = valIt->second;
                Variable * var = (*mVariables)[varId];

                for (Domain::const_iterator domIt = domain.begin(); domIt != domain.end(); ++domIt) {
                        var->addToDomain(*domIt);
                }
        }
}

unsigned int Variable::getNumValuesInDomainRange(VarType aLowerBound, VarType aUpperBound) const {
        Domain::const_iterator domItLB = mDomain->lower_bound(aLowerBound);
        Domain::const_iterator domItUB = mDomain->lower_bound(aUpperBound);

        unsigned int numValues = 0;
        while (domItLB != domItUB) {
                ++numValues;
                ++domItLB;
        };
        return numValues;
}

void Variable::restrictDomainToValue(VarType aValue, Domain & outRemovedValues) {
        outRemovedValues = *mDomain;
        mDomain->clear();
        mDomain->insert(aValue);
}

void Variable::restoreRestrictedDomain(const Domain & aRemovedValues) {
        for (Domain::const_iterator domIt = aRemovedValues.begin(); domIt != aRemovedValues.end(); ++domIt) {
                mDomain->insert(*domIt);
        }
}


std::string Variable::pprint() const {
        std::ostringstream out;
        out << "x_" << mId << ": ";
        for (Domain::const_iterator domIt = mDomain->begin(); domIt != mDomain->end(); ++domIt) {
                out << *domIt << ", ";
        }
        return out.str();
}

std::string CSPProblem::pprintVariables() const {
        std::ostringstream out;
        for (VariableMap::const_iterator varIt = mVariables->begin(); varIt != mVariables->end(); ++varIt) {
                out << varIt->second->pprint() << std::endl;
        }
        return out.str();
}

unsigned int CSPProblem::getNumValuesInDomainRange(VarIdType aVarId, VarType aLowerBound, VarType aUpperBound) const {
        return (*mVariables)[aVarId]->getNumValuesInDomainRange(aLowerBound, aUpperBound);
}

std::string assignment_pprint(const Assignment & a) {
        std::ostringstream out;
        for (Assignment::const_iterator it = a.begin(); it != a.end(); ++it) {
                if (it != a.begin()) {
                        out << ", ";
                }
                out << "" << it->first << ": " << it->second;
        }
        return out.str();
}

std::string scope_pprint(const Scope & a) {
        std::ostringstream out;
        out << "{";
        for (Scope::const_iterator it = a.begin(); it != a.end(); ++it) {
                if (it != a.begin()) {
                        out << ", ";
                }
                out << *it;
        }
        out << "} ";
        return out.str();
}

std::string probability_distribution_pprint(const ProbabilityDistribution & aDist) {
        std::ostringstream out;
        out << "{";
        for (ProbabilityDistribution::const_iterator it = aDist.begin(); it != aDist.end(); ++it) {
                if (it != aDist.begin()) {
                        out << ", ";
                }
                out << it->first << ": " << it->second;
        }
        out << "} ";
        return out.str();
}
