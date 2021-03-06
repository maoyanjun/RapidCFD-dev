/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2012 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "jumpCyclicAMIFvPatchFields.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "lduAddressingFunctors.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

makePatchFieldsTypeName(jumpCyclicAMI);

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<>
void Foam::jumpCyclicAMIFvPatchField<scalar>::updateInterfaceMatrix
(
    scalargpuField& result,
    const scalargpuField& psiInternal,
    const scalargpuField& coeffs,
    const direction cmpt,
    const Pstream::commsTypes
) const
{
    const labelgpuList& nbrFaceCells =
        this->cyclicAMIPatch().cyclicAMIPatch().neighbPatch().getFaceCells();

    scalargpuField pnf(psiInternal, nbrFaceCells);

    pnf = this->cyclicAMIPatch().interpolate(pnf);

    // only apply jump to original field
    if (&psiInternal == &this->internalField())
    {
        gpuField<scalar> jf(this->jump());

        if (!this->cyclicAMIPatch().owner())
        {
            jf *= -1.0;
        }

        pnf -= jf;
    }

    // Transform according to the transformation tensors
    this->transformCoupleField(pnf, cmpt);

    // Multiply the field by coefficients and add into the result
    matrixPatchOperation
    (
        this->patch().index(),
        result,
        this->patch().boundaryMesh().mesh().lduAddr(),
        matrixInterfaceFunctor<scalar>
        (
            coeffs.data(),
            pnf.data()
        )
    );
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
