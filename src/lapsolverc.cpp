#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <algorithm>
#include <vector>
#include <limits>
#include <iostream>

#include "dense.hpp"

namespace py = pybind11;

//py::isinstance<py::array_t<std::int32_t>>(buf)

py::tuple solve_dense_wrap(py::array_t<double, py::array::c_style | py::array::forcecast> input1) {
    auto buf1 = input1.request();

    if (buf1.ndim != 2)
        throw std::runtime_error("Number of dimensions must be two");

    const int nrows = int(buf1.shape[0]);
    const int ncols = int(buf1.shape[1]);    

    if (nrows == 0 || ncols == 0) {
        return py::make_tuple(py::array(), py::array());
    }

    double *data = (double *)buf1.ptr;

    bool any_finite = false;
    double LARGE_COST = 0.;
    for(int i = 0; i < nrows*ncols; ++i) {
        if (std::isfinite(data[i])) {
            any_finite = true;
            LARGE_COST = std::max<double>(LARGE_COST, data[i]);
        }
    }
         
    if (nrows == 0 || ncols == 0 || !any_finite) {
        return py::make_tuple(py::array(), py::array());
    }

    LARGE_COST = 2*LARGE_COST + 1;
    const int n = std::max(nrows, ncols);
    std::vector<std::vector<double>> costs(n, std::vector<double>(n, LARGE_COST));

    for (int i = 0; i < nrows; i++)
    {   
        double *cptr = data + i*ncols;
        for (int j =0; j < ncols; j++)
        {
            const double c = cptr[j];
            if (std::isfinite(c))
                costs[i][j] = c;
        }
    }


    std::vector<int> Lmate, Rmate;
    solve_dense(costs, Lmate, Rmate);

    std::vector<int> rowids, colids;

    for (int i = 0; i < nrows; i++)
    {
        int mate = Lmate[i];
		if (Lmate[i] < ncols && costs[i][mate] != LARGE_COST)
		{
            rowids.push_back(i);
            colids.push_back(mate);
		}
    }

    return py::make_tuple(py::array(rowids.size(), rowids.data()), py::array(colids.size(), colids.data()));
}



PYBIND11_MODULE(lapsolverc, m) {
    m.doc() = R"pbdoc(
        Linear assignment problem solver using native c-extensions.
    )pbdoc";

    m.def("solve_dense", &solve_dense_wrap, R"pbdoc(
        Min cost bipartite matching via shortest augmenting paths for dense matrices

        This is an O(n^3) implementation of a shortest augmenting path
        algorithm for finding min cost perfect matchings in dense
        graphs.  In practice, it solves 1000x1000 problems in around 1
        second.

            rids, cids = solve_dense(costs)
            total_cost = costs[rids, cids].sum()

        Params
        ------
        costs : MxN array
            Array containing costs.

        Returns
        -------
        rids : array
            Array of row ids of matching pairings
        cids : array
            Array of column ids of matching pairings
  
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}