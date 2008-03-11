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
#include <math.h>

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

#include "csp.h"
#include "graph.h"
#include "utils.h"

Graph * Graph::createCSPPrimalGraph(CSPProblem * p) {
        std::map<VarIdType, Vertex *> *vertices = new std::map<VarIdType, Vertex *>();

        for (VariableMap::const_iterator varIt = p->getVariables()->begin(); varIt != p->getVariables()->end(); ++varIt) {
                // Create vertex for each variable
                Vertex * v = new Vertex(varIt->first);
                (*vertices)[varIt->first] = v;
        }

        for (ConstraintList::const_iterator ctrIt = p->getConstraints()->begin(); ctrIt != p->getConstraints()->end(); ++ctrIt) {
                // Add an edge for each constraint
                Scope s = (*ctrIt)->getScope();

                for (Scope::iterator scIt1 = s.begin(); scIt1 != s.end(); ++scIt1) {
                        Vertex * v1 = (*vertices)[*scIt1];
                        Scope::iterator scIt2 = scIt1;
                        ++scIt2;
                        for (; scIt2 != s.end(); ++scIt2) {
                                Vertex * v2 = (*vertices)[*scIt2];
                                v1->addNeighbour(v2);
                                v2->addNeighbour(v1);
                        }
                }
        }

        Graph * g = new Graph(vertices);

        return g;
}

std::vector<VarIdType> Graph::minInducedWidthOrdering() {

        std::map<VarIdType, Vertex *> vertices;

        // Copy vertices 
        for (VertexDict::iterator vIt = this->mVertices->begin(); vIt != this->mVertices->end(); ++vIt) {
                Vertex *v = new Vertex(*(vIt->second));
                vertices[v->getId()] = v;
        }

        // Fix the neighbouring links
        for (std::map<VarIdType, Vertex *>::iterator vIt = vertices.begin(); vIt != vertices.end(); ++vIt) {
                for (std::vector<Vertex *>::iterator nIt = vIt->second->neighbours.begin();
                                nIt != vIt->second->neighbours.end(); ++nIt) {

                        *nIt = vertices[(*nIt)->getId()];
                }
        }

        std::vector<VarIdType> ordering(vertices.size());
        for (int i = ordering.size() - 1; i >= 0; --i) {
                // Find a node with the smallest degree

                size_t minDegree = vertices.size() + 1;
                Vertex * minVertex;
                for (std::map<VarIdType, Vertex *>::iterator vIt = vertices.begin(); vIt != vertices.end(); ++vIt) {
                        if (vIt->second->neighbours.size() < minDegree) {
                                minDegree = vIt->second->neighbours.size();
                                minVertex = vIt->second;
                        }
                }

                // Store the found vertex
                ordering[i] = minVertex->getId();

                // Connect the neighbours of the found vertex
                for (std::vector<Vertex *>::iterator nIt1 = minVertex->neighbours.begin();
                                nIt1 != minVertex->neighbours.end(); ++nIt1) {

                        for (std::vector<Vertex *>::iterator nIt2 = (*nIt1)->neighbours.begin();
                                        nIt2 != (*nIt1)->neighbours.end(); ++nIt2) {

                                        if (*nIt2 == minVertex)
                                                continue;

                                        (*nIt1)->addNeighbour(*nIt2);
                        }

                        (*nIt1)->removeNeighbour(minVertex);
                }

                vertices.erase(minVertex->getId());

                delete minVertex;
        }

        return ordering;
}
