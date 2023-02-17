/****************************************************************************
  This file is part of LoLA.

  LoLA is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  LoLA is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
  more details.

  You should have received a copy of the GNU Affero General Public License
  along with LoLA. If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

/*!
\file
\author Andreas
\status approved 11.04.2012
*/

#pragma once
extern bool overflow;

#include <config.h>
#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Core/Runtime.h>

/// computes the ggt of two unsigned integers
inline long long int ggt(long long int a, long long int b)
{
    assert(b != 0);
    if(a < 0) a = -a;
    if(b < 0) b = -b;
    while (true) // LCOV_EXCL_LINE
    {
        a %= b;
        if (!a)
        {
            return b;
        }
        b %= a;
        if (!b)
        {
            return a;
        }
    }
}

/// multiplies two integers with respect to overflows
inline long long int safeMult(long long int a, long long int b)
{
    //overflow handling
    ///\todo overflow handling
    if (b > 1 && a > INTMAX_MAX / b)
    {
	overflow = true;
	return 0;
        assert(false);
    }

    return (a * b);
}

/*!
A matrix is used to store variables (=columns) and their equations (=rows).
Using places (transitions) as variables and their effect to the net as rows,
the reduce() method can find the significant places (transitions).
*/
class Matrix
{
public:
    /// A row is used to store a row.
    class Row
    {
        friend class Matrix;

    public:
        /// Generate and initialize a row based on Net.h types
        Row(const arrayindex_t, const arrayindex_t *, const long long int *, const arrayindex_t = 0);
        /// Delete a row
        ~Row();

        /// Eleminate current first variable of a row in its successor row (same first variable)
        bool apply(Matrix &, arrayindex_t rowToChange = ARRAYINDEX_T_MAX);

        /// Write row to cout
        void DEBUG__printRow() const;

        /// Checks whether the variables are ordered properly
        bool DEBUG__checkRow() const;

        /// Number of variables in current row with non zero coefficients
        const arrayindex_t varCount;
        /// Array of variable indizes in current row with non zero coefficients
        arrayindex_t *variables;
        /// Array of non zero coefficients in current row (same order as variables)
        long long int *coefficients;

        /// Reference number of current row
        const arrayindex_t reference;

        /// Pointer to successor row with same first variable (NULL if none present)
        Row *next;
    };
public:
    /// Array of rows for number of varibales (columns)
    Row **matrix;
    /// Number of stored rows in current matrix
    arrayindex_t rowCount;
    /// Number of stored columns in current matrix
    const arrayindex_t colCount;
    /// Number of non empty columns
    arrayindex_t significantColCount;

    bool DEBUG__checkReduced() const;

    /// Generate and initialize a matrix
    explicit Matrix(const arrayindex_t);
    /// Delete a matrix
    ~Matrix();

    /// Add a new row to the matrix
    void addRow(const arrayindex_t, const arrayindex_t *, const long long int *, arrayindex_t = 0);

    /// Delete the successor of a row in the matrix
    void deleteRow(Row *);

    /// Generate the triangular form of the matrix (original one gets lost)
    bool reduce();

    /// Approach the diagonal form of the matrix (with a remainder in linear dependent columns)
    bool diagonalise();

    /// Returns true iff a column with given index is significant
    bool isSignificant(const arrayindex_t) const;

    /// Returns row of the first row with given index
    Row *getRow(const arrayindex_t) const;

    /// Returns the number of significant (= not empty) columns
    arrayindex_t getSignificantColCount() const;

    /// Write matrix to cout
    //void DEBUG__printMatrix() const;
};
