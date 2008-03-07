#include <iostream>
#include <sstream>
#include "assert.h"
#include "math.h"

#include "domain_interval.h"
#include "utils.h"

std::string DomainInterval::pprint() const {
        std::ostringstream out;
        out << "(" << lowerBound << ", " << upperBound << " | " << probability << ")";
        return out.str();
}

/**
 * Returns merged interval list, non-normalized
 */
DomainIntervalSet merge_intervals(const DomainIntervalSet & aList1, const DomainIntervalSet & aList2) {
        DomainIntervalSet result;

        DomainIntervalSet::const_iterator it1 = aList1.begin(), it2 = aList2.begin();
        VarType lb1, lb2, ub1, ub2;
        double prob1, prob2;

        if (it1 != aList1.end()) {
                lb1 = it1->lowerBound; ub1 = it1->upperBound;
                prob1 = it1->probability;
        }

        if (it2 != aList2.end()) {
                lb2 = it2->lowerBound; ub2 = it2->upperBound;
                prob2 = it2->probability;
        }

        while (it1 != aList1.end() && it2 != aList2.end()) {
                if (ub1 <= lb2) {
                        ++it1;
                        if (it1 != aList1.end()) {
                                lb1 = it1->lowerBound; ub1 = it1->upperBound;
                                prob1 = it1->probability;
                        }
                        continue;
                }

                if (ub2 <= lb1) {
                        ++it2;
                        if (it2 != aList2.end()) {
                                lb2 = it2->lowerBound; ub2 = it2->upperBound;
                                prob2 = it2->probability;
                        }
                        continue;
                }

                VarType intervalLength = max(0, min(ub1, ub2) - max(lb1, lb2)); // Interval length should be at least one
                assert(intervalLength > 0);

                double probability = (((double)intervalLength) / (ub1 - lb1) * prob1) *
                                        (((double)intervalLength) / (ub2 - lb2) * prob2);

                result.insert(DomainInterval(max(lb1, lb2), min(ub1, ub2), probability));


                // Move to next intervals in the lists
                if (ub1 == min(ub1, ub2)) {
                        // The upper bound needs to be moved
                        ++it1;
                        prob2 *= (double)(ub2 - ub1) / (ub2 - lb2); // Adjust the remaining probability
                        lb2 = ub1;
                        if (it1 != aList1.end()) {
                                lb1 = it1->lowerBound; ub1 = it1->upperBound;
                                prob1 = it1->probability;
                        }
                        continue;
                }

                if (ub2 == min(ub1, ub2)) {
                        // The upper bound needs to be moved
                        ++it2;
                        prob1 *= (double)(ub1 - ub2) / (ub1 - lb1); // Adjust the remaining probability
                        lb1 = ub2;
                        if (it2 != aList1.end()) {
                                lb2 = it2->lowerBound; ub2 = it2->upperBound;
                                prob2 = it2->probability;
                        }
                        continue;
                }
        }

        return result;
}

DomainIntervalSet normalize_intervals(const DomainIntervalSet & aList) {
        double totalProbability;
        DomainIntervalSet result;
        for (DomainIntervalSet::iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                totalProbability += intIt->probability;
        }

        for (DomainIntervalSet::iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                result.insert(DomainInterval(intIt->lowerBound, intIt->upperBound, intIt->probability / totalProbability));
        }
        return result;
}

DomainIntervalSet join_intervals(const DomainIntervalSet & aList, unsigned int aMaxDomainIntervals) {
        // TODO: Add splitting of too big intervals
        assert(aMaxDomainIntervals > 0);

        DomainIntervalSet result;
        
        double intervalProbability = 1.0 / aMaxDomainIntervals;

        DomainIntervalSet splittedIntervals;
        // Split big intervals with many values into smaller ones
        for (DomainIntervalSet::const_iterator intIt = aList.begin(); intIt != aList.end(); ++ intIt) {
                int intervalLength = intIt->upperBound - intIt->lowerBound;
                int numSplits = min(intervalLength, max(1, (int)ceil(2.0 * intIt->probability / intervalProbability)));
                VarType lb = intIt->lowerBound;

                for (int i = 0; i < numSplits; ++i) {
                        VarType ub = intIt->lowerBound + (VarType)ceil(intervalLength * (i + 1) / (double)numSplits);
                        double splitProbability = (double)(ub - lb) / intervalLength * intIt->probability;
                        splittedIntervals.insert(DomainInterval(lb, ub, splitProbability));

                        lb = ub;
                }
        }

        std::cout << "Splitted:\t";
        interval_list_pprint(splittedIntervals);
        std::cout << std::endl;

        VarType lb, ub;
        DomainIntervalSet::iterator intIt = splittedIntervals.begin();

        double cumulatedProbability = 0.0;

        while (intIt != splittedIntervals.end()) {
                if (cumulatedProbability <= 0.0) {
                        if (intIt->probability >= intervalProbability) {
                                // Append current interval to the list (without joining)
                                result.insert(*intIt);
                        } else {
                                lb = intIt->lowerBound;
                                ub = intIt->upperBound;
                                cumulatedProbability = intIt->probability;
                        }
                } else if (cumulatedProbability + intIt->probability >= 2.0*intervalProbability ||
                                (cumulatedProbability + intIt->probability >= 1.8*intervalProbability &&
                                intIt->probability > intervalProbability)) {
                        // We can add the previous interval as one and the current as another
                        result.insert(DomainInterval(lb, ub, cumulatedProbability));
                        result.insert(*intIt);
                        cumulatedProbability = 0.0;
                } else if (cumulatedProbability + intIt->probability >= intervalProbability) {
                        result.insert(DomainInterval(lb, intIt->upperBound, cumulatedProbability + intIt->probability));
                        cumulatedProbability = 0.0;
                } else {
                        ub = intIt->upperBound;
                        cumulatedProbability += intIt->probability;
                }

                ++intIt;
        }

        // Add any remaining stuff into the list
        if (cumulatedProbability > 0.0) {
                result.insert(DomainInterval(lb, ub, cumulatedProbability));
        }

        return result;
}

void interval_list_pprint(const DomainIntervalSet & aList) {
        for (DomainIntervalSet::const_iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                std::cout << intIt->pprint() << " ";
        }
        std::cout << std::endl;
}

DomainIntervalSet adjust_intervals_to_domain(const DomainIntervalSet & aList, const Domain & aDomain) {
        DomainIntervalSet result;

        for (DomainIntervalSet::const_iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                Domain::const_iterator domLB = aDomain.lower_bound(intIt->lowerBound);
                Domain::const_iterator domUB = aDomain.lower_bound(intIt->upperBound);
                if (domLB == aDomain.end()) {
                        // If there are no greater keys in the domain than in the interval, quit
                        break;
                } else if (domLB == domUB) {
                        // If there are no items between lower and upper bound, continue
                        // with next interval
                        continue;
                } else {
                        --domUB;
                        result.insert(DomainInterval(*domLB, *domUB + 1, intIt->probability));
                }
        }

        return result;
}
