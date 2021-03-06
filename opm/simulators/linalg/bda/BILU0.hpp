/*
  Copyright 2019 Equinor ASA

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BILU0_HPP
#define BILU0_HPP

#include <opm/simulators/linalg/bda/BlockedMatrix.hpp>
#include <opm/simulators/linalg/bda/ILUReorder.hpp>

#include <opm/simulators/linalg/bda/opencl.hpp>
#include <opm/simulators/linalg/bda/ChowPatelIlu.hpp>

namespace bda
{

    /// This class implementa a Blocked ILU0 preconditioner
    /// The decomposition is done on CPU, and reorders the rows of the matrix
    template <unsigned int block_size>
    class BILU0
    {

    private:
        int N;       // number of rows of the matrix
        int Nb;      // number of blockrows of the matrix
        int nnz;     // number of nonzeroes of the matrix (scalar)
        int nnzbs;   // number of blocks of the matrix
        std::unique_ptr<BlockedMatrix<block_size> > Lmat = nullptr, Umat = nullptr, LUmat = nullptr;
        std::shared_ptr<BlockedMatrix<block_size> > rmat = nullptr; // only used with PAR_SIM
        double *invDiagVals = nullptr;
        int *diagIndex = nullptr;
        std::vector<int> rowsPerColor;  // color i contains rowsPerColor[i] rows, which are processed in parallel
        int *toOrder = nullptr, *fromOrder = nullptr;
        int numColors;
        int verbosity;

        ILUReorder opencl_ilu_reorder;

        typedef struct {
            cl::Buffer Lvals, Uvals, invDiagVals;
            cl::Buffer Lcols, Lrows;
            cl::Buffer Ucols, Urows;
            cl::Buffer rowsPerColor;
        } GPU_storage;

        cl::make_kernel<cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, const unsigned int, cl::LocalSpaceArg> *ILU_apply1;
        cl::make_kernel<cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, const unsigned int, cl::LocalSpaceArg> *ILU_apply2;
        GPU_storage s;
        cl::Context *context;
        cl::CommandQueue *queue;
        int work_group_size = 0;
        int total_work_items = 0;
        int lmem_per_work_group = 0;
        bool pattern_uploaded = false;

        ChowPatelIlu chowPatelIlu;

        void chow_patel_decomposition();

    public:

        BILU0(ILUReorder opencl_ilu_reorder, int verbosity);

        ~BILU0();

        // analysis
        bool init(BlockedMatrix<block_size> *mat);

        // ilu_decomposition
        bool create_preconditioner(BlockedMatrix<block_size> *mat);

        // apply preconditioner, y = prec(x)
        void apply(cl::Buffer& x, cl::Buffer& y);

        void setOpenCLContext(cl::Context *context);
        void setOpenCLQueue(cl::CommandQueue *queue);
        void setKernelParameters(const unsigned int work_group_size, const unsigned int total_work_items, const unsigned int lmem_per_work_group);
        void setKernels(
            cl::make_kernel<cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, const unsigned int, cl::LocalSpaceArg> *ILU_apply1,
            cl::make_kernel<cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, cl::Buffer&, cl::Buffer&, cl::Buffer&, const unsigned int, const unsigned int, cl::LocalSpaceArg> *ILU_apply2
            );

        int* getToOrder()
        {
            return toOrder;
        }

        int* getFromOrder()
        {
            return fromOrder;
        }

        BlockedMatrix<block_size>* getRMat()
        {
            return rmat.get();
        }

    };

} // end namespace bda

#endif

