#ifndef TYPES_H_
#define TYPES_H_

#include <set>
#include <vector>
#include <map>

typedef int VarType;

typedef unsigned int VarIdType;

typedef std::set<VarType> Domain;

typedef std::map<VarIdType, VarType> Assignment;

typedef std::set<VarIdType> Scope;

typedef std::vector<Scope> Bucket;

typedef std::map<VarType, double> ProbabilityDistribution;

#endif // TYPES_H_
