#include <iostream>
#include <sstream>
#include "assert.h"
#include "math.h"

#include "domain_interval.h"
#include "utils.h"

std::string DomainInterval::pprint() const {
        std::ostringstream out;
        out << "(" << lowerBound << ", " << upperBound << ")";
        return out.str();
}

/**
 * Returns merged interval list, non-normalized
 */
DomainIntervalMap merge_intervals(const DomainIntervalMap & aList1, const DomainIntervalMap & aList2) {
        DomainIntervalMap result;

        DomainIntervalMap::const_iterator it1 = aList1.begin(), it2 = aList2.begin();
        VarType lb1, lb2, ub1, ub2;
        double prob1, prob2;

        if (it1 != aList1.end()) {
                lb1 = it1->first.lowerBound; ub1 = it1->first.upperBound;
                prob1 = it1->second;
        }

        if (it2 != aList2.end()) {
                lb2 = it2->first.lowerBound; ub2 = it2->first.upperBound;
                prob2 = it2->second;
        }

        while (it1 != aList1.end() && it2 != aList2.end()) {
                if (ub1 <= lb2) {
                        ++it1;
                        if (it1 != aList1.end()) {
                                lb1 = it1->first.lowerBound; ub1 = it1->first.upperBound;
                                prob1 = it1->second;
                        }
                        continue;
                }

                if (ub2 <= lb1) {
                        ++it2;
                        if (it2 != aList2.end()) {
                                lb2 = it2->first.lowerBound; ub2 = it2->first.upperBound;
                                prob2 = it2->second;
                        }
                        continue;
                }

                VarType intervalLength = max(0, min(ub1, ub2) - max(lb1, lb2)); // Interval length should be at least one
                assert(intervalLength > 0);

                double probability = (((double)intervalLength) / (ub1 - lb1) * prob1) *
                                        (((double)intervalLength) / (ub2 - lb2) * prob2);

                result[DomainInterval(max(lb1, lb2), min(ub1, ub2))] = probability;


                // Move to next intervals in the lists
                if (ub1 == min(ub1, ub2)) {
                        // The upper bound needs to be moved
                        ++it1;
                        prob2 *= (double)(ub2 - ub1) / (ub2 - lb2); // Adjust the remaining probability
                        lb2 = ub1;
                        if (it1 != aList1.end()) {
                                lb1 = it1->first.lowerBound; ub1 = it1->first.upperBound;
                                prob1 = it1->second;
                        }
                        continue;
                }

                if (ub2 == min(ub1, ub2)) {
                        // The upper bound needs to be moved
                        ++it2;
                        prob1 *= (double)(ub1 - ub2) / (ub1 - lb1); // Adjust the remaining probability
                        lb1 = ub2;
                        if (it2 != aList1.end()) {
                                lb2 = it2->first.lowerBound; ub2 = it2->first.upperBound;
                                prob2 = it2->second;
                        }
                        continue;
                }
        }

        return result;
}

DomainIntervalMap normalize_intervals(const DomainIntervalMap & aList) {
        double totalProbability;
        DomainIntervalMap result;
        for (DomainIntervalMap::const_iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                totalProbability += intIt->second;
        }

        for (DomainIntervalMap::const_iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                result[DomainInterval(intIt->first.lowerBound, intIt->first.upperBound)] =  intIt->second / totalProbability;
        }
        return result;
}

DomainIntervalMap join_intervals(const DomainIntervalMap & aList, unsigned int aMaxDomainIntervals) {
        // TODO: Add splitting of too big intervals
        assert(aMaxDomainIntervals > 0);

        DomainIntervalMap result;
        
        double intervalProbability = 1.0 / aMaxDomainIntervals;

        DomainIntervalMap splittedIntervals;
        // Split big intervals with many values into smaller ones
        for (DomainIntervalMap::const_iterator intIt = aList.begin(); intIt != aList.end(); ++ intIt) {
                int intervalLength = intIt->first.upperBound - intIt->first.lowerBound;
                int numSplits = min(intervalLength, max(1, (int)ceil(2.0 * intIt->second / intervalProbability)));
                VarType lb = intIt->first.lowerBound;

                for (int i = 0; i < numSplits; ++i) {
                        VarType ub = intIt->first.lowerBound + (VarType)ceil(intervalLength * (i + 1) / (double)numSplits);
                        double splitProbability = (double)(ub - lb) / intervalLength * intIt->second;
                        splittedIntervals[DomainInterval(lb, ub)] = splitProbability;

                        lb = ub;
                }
        }

        /*
        std::cout << "Splitted:\t";
        interval_list_pprint(splittedIntervals);
        std::cout << std::endl;
        */

        VarType lb, ub;
        DomainIntervalMap::iterator intIt = splittedIntervals.begin();

        double cumulatedProbability = 0.0;

        while (intIt != splittedIntervals.end()) {
                if (cumulatedProbability <= 0.0) {
                        if (intIt->second >= intervalProbability) {
                                // Append current interval to the list (without joining)
                                result[intIt->first] = intIt->second;
                        } else {
                                lb = intIt->first.lowerBound;
                                ub = intIt->first.upperBound;
                                cumulatedProbability = intIt->second;
                        }
                } else if (cumulatedProbability + intIt->second >= 2.0*intervalProbability ||
                                (cumulatedProbability + intIt->second >= 1.8*intervalProbability &&
                                intIt->second > intervalProbability)) {
                        // We can add the previous interval as one and the current as another
                        result[DomainInterval(lb, ub)] = cumulatedProbability;
                        result[intIt->first] = intIt->second;
                        cumulatedProbability = 0.0;
                } else if (cumulatedProbability + intIt->second >= intervalProbability) {
                        result[DomainInterval(lb, intIt->first.upperBound)] = cumulatedProbability + intIt->second;
                        cumulatedProbability = 0.0;
                } else {
                        ub = intIt->first.upperBound;
                        cumulatedProbability += intIt->second;
                }

                ++intIt;
        }

        // Add any remaining stuff into the list
        if (cumulatedProbability > 0.0) {
                result[DomainInterval(lb, ub)] = cumulatedProbability;
        }

        return result;
}

std::string interval_list_pprint(const DomainIntervalMap & aList) {
        std::ostringstream out;
        for (DomainIntervalMap::const_iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                out << intIt->first.pprint() << ": " << intIt->second << "; ";
        }
        out << std::endl;
        return out.str();
}

DomainIntervalMap adjust_intervals_to_domain(const DomainIntervalMap & aList, const Domain & aDomain) {
        DomainIntervalMap result;

        for (DomainIntervalMap::const_iterator intIt = aList.begin(); intIt != aList.end(); ++intIt) {
                Domain::const_iterator domLB = aDomain.lower_bound(intIt->first.lowerBound);
                Domain::const_iterator domUB = aDomain.lower_bound(intIt->first.upperBound);
                if (domLB == aDomain.end()) {
                        // If there are no greater keys in the domain than in the interval, quit
                        break;
                } else if (domLB == domUB) {
                        // If there are no items between lower and upper bound, continue
                        // with next interval
                        continue;
                } else {
                        --domUB;
                        result[DomainInterval(*domLB, *domUB + 1)] = intIt->second;
                }
        }

        return result;
}

DomainIntervalMap uniform_intervals_for_domain(const Domain & aDomain, unsigned int aMaxIntervals) {
        DomainIntervalMap result;
        int valuesPerInterval = max(aDomain.size() / aMaxIntervals, 1);

        Domain::const_iterator domIt = aDomain.begin();

        while (domIt != aDomain.end()) {
                int i = 0;
                VarType lb = *domIt, ub;
                double probability = 0.0;
                while (i < valuesPerInterval && domIt != aDomain.end()) {
                        ub = *domIt + 1;
                        probability += 1.0 / aDomain.size();
                        ++i;
                        ++domIt;
                }
                result[DomainInterval(lb, ub)] = probability;
        }

        return result;
}
