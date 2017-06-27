/*********************                                                        */
/*! \file Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Engine.h"
#include "InputQuery.h"
#include "PiecewiseLinearConstraint.h"

Engine::Engine()
{
}

Engine::~Engine()
{
}

bool Engine::solve()
{
    // Todo: If l >= u for some var, fail immediately

    while ( true )
    {
        _tableau->computeAssignment();
        _tableau->computeBasicStatus();

        _tableau->dumpAssignment();

        // If all variables are within bounds, we're done
        if ( !_tableau->existsBasicOutOfBounds() )
            return true;

        // If a simplex step fails, the query is unsat
        if ( !performSimplexStep() )
            return false;
    }
}

bool Engine::performSimplexStep()
{
    _tableau->computeCostFunction();
    _tableau->dumpCostFunction();

    if ( !_tableau->pickEnteringVariable( &_dantzigsRule ) )
        return false;

    _tableau->computeD();
    _tableau->pickLeavingVariable();
    _tableau->performPivot();

    return true;
}

void Engine::processInputQuery( const InputQuery &inputQuery )
{
    const List<InputQuery::Equation> equations( inputQuery.getEquations() );

    unsigned m = equations.size();
    unsigned n = inputQuery.getNumberOfVariables();
    _tableau->setDimensions( m, n );

    unsigned equationIndex = 0;
    for ( const auto &equation : equations )
    {
        _tableau->markAsBasic( equation._auxVariable );
        _tableau->setRightHandSide( equationIndex, equation._scalar );

        for ( const auto &addend : equation._addends )
            _tableau->setEntryValue( equationIndex, addend._variable, addend._coefficient );

        ++equationIndex;
    }

    for ( unsigned i = 0; i < n; ++i )
    {
        _tableau->setLowerBound( i, inputQuery.getLowerBound( i ) );
        _tableau->setUpperBound( i, inputQuery.getUpperBound( i ) );
    }

    _tableau->initializeTableau();

    _plConstraints = inputQuery.getPiecewiseLinearConstraints();
    for ( const auto &constraint : _plConstraints )
    {
        List<unsigned> participatingVariables = constraint->getParticiatingVariables();
        for ( const auto &var : participatingVariables )
            _plVarAssignment[var] = 0;
    }
}

void Engine::extractSolution( InputQuery &inputQuery )
{
    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
        inputQuery.setSolutionValue( i, _tableau->getValue( i ) );
}

bool Engine::allVarsWithinBounds() const
{
    return !_tableau->existsBasicOutOfBounds();
}

bool Engine::allPlConstraintsHold()
{
    extractPlAssignment();

    for ( const auto &constraint : _plConstraints )
        if ( constraint->satisfied( _plVarAssignment ) )
            return false;

    return true;
}

void Engine::extractPlAssignment()
{
    for ( auto it : _plVarAssignment )
        it.second = _tableau->getValue( it.first );
}

const Set<unsigned> Engine::getVarsInPlConstraints()
{
    return _plVarAssignment.keys();
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//