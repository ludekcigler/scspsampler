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

void assignment_pprint(const Assignment & a) {
        for (Assignment::const_iterator it = a.begin(); it != a.end(); ++it) {
                if (it != a.begin()) {
                        std::cout << ", ";
                }
                std::cout << "" << it->first << ": " << it->second;
        }
}

void scope_pprint(const Scope & a) {
        std::cout << "{";
        for (Scope::const_iterator it = a.begin(); it != a.end(); ++it) {
                if (it != a.begin()) {
                        std::cout << ", ";
                }
                std::cout << *it;
        }
        std::cout << "} ";
}
