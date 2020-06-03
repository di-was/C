/**
 * \file
 * \brief [Kohonen self organizing
 * map](https://en.wikipedia.org/wiki/Self-organizing_map) (1D)
 *
 * This example implements a powerful self organizing map algorithm in 1D.
 * The algorithm creates a connected network of weights that closely
 * follows the given data points. This this creates a chain of nodes that
 * resembles the given input shape.
 */
#define _USE_MATH_DEFINES // required for MS Visual C
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

/**
 * Helper function to generate a random number in a given interval.
 * \n Steps:
 * 1. `r1 = rand() % 100` gets a random number between 0 and 99
 * 2. `r2 = r1 / 100` converts random number to be between 0 and 0.99
 * 3. scale and offset the random number to given range of \f$[a,b]\f$
 *
 * \param[in] a lower limit
 * \param[in] b upper limit
 * \returns random number in the range \f$[a,b]\f$
 */
double _random(double a, double b)
{
    return ((b - a) * (rand() % 100) / 100.f) + a;
}

/**
 * Save a given data martix to file.
 * \param[in] fname filename to save in (gets overwriten without confirmation)
 * \param[in] X matrix to save
 * \param[in] num_points number of rows in the matrix
 * \param[in] num_features number of columns in the matrix
 */
void save_2d_data(const char *fname, const double *const *X, int num_points,
                  int num_features)
{
    FILE *fp = fopen(fname, "wt");
    for (int i = 0; i < num_points; i++) // for each point in the array
    {
        for (int j = 0; j < num_features; j++) // for each feature in the array
        {
            fprintf(fp, "%.4g", X[i][j]); // print the feature value
            if (j < num_features - 1)     // if not the last feature
                fprintf(fp, ",");         // suffix comma
        }
        if (i < num_points - 1) // if not the last row
            fprintf(fp, "\n");  // start a new line
    }
    fclose(fp);
}

/**
 * Get minimum value and index of the value in a vector
 * \param[in] x vector to search
 * \param[in] N number of points in the vector
 * \param[out] val minimum value found
 * \param[out] idx index where minimum value was found
 */
void get_min_1d(double const *X, int N, double *val, int *idx)
{
    val[0] = INFINITY; // initial min value

    for (int i = 0; i < N; i++) // check each value
    {
        if (X[i] < val[0]) // if a lower value is found
        {                  // save the value and its index
            idx[0] = i;
            val[0] = X[i];
        }
    }
}

/**
 * Update weights of the SOM using Kohonen algorithm
 *
 * \param[in] X data point
 * \param[in,out] W weights matrix
 * \param[in,out] D temporary vector to store distances
 * \param[in] num_out number of output points
 * \param[in] num_features number of features per input sample
 * \param[in] alpha learning rate \f$0<\alpha\le1\f$
 * \param[in] R neighborhood range
 */
void update_weights(double const *x, double *const *W, double *D, int num_out,
                    int num_features, double alpha, int R)
{
    int j, k;

#ifdef _OPENMP
#pragma omp for
#endif
    // step 1: for each output point
    for (j = 0; j < num_out; j++)
    {
        D[j] = 0.f;
        // compute Euclidian distance of each output
        // point from the current sample
        for (k = 0; k < num_features; k++)
            D[j] += (W[j][k] - x[k]) * (W[j][k] - x[k]);
    }

    // step 2:  get closest node i.e., node with snallest Euclidian distance to
    // the current pattern
    int d_min_idx;
    double d_min;
    get_min_1d(D, num_out, &d_min, &d_min_idx);

    // step 3a: get the neighborhood range
    int from_node = 0 > (d_min_idx - R) ? 0 : d_min_idx - R;
    int to_node = num_out < (d_min_idx + R + 1) ? num_out : d_min_idx + R + 1;

    // step 3b: update the weights of nodes in the
    // neighborhood
#ifdef _OPENMP
#pragma omp for
#endif
    for (j = from_node; j < to_node; j++)
        for (k = 0; k < num_features; k++)
            // update weights of nodes in the neighborhood
            W[j][k] += alpha * (x[k] - W[j][k]);
}

/**
 * Apply incremental algorithm with updating neighborhood and learning rates
 * on all samples in the given datset.
 *
 * \param[in] X data set
 * \param[in,out] W weights matrix
 * \param[in] D temporary vector to store distances
 * \param[in] num_samples number of output points
 * \param[in] num_features number of features per input sample
 * \param[in] num_out number of output points
 * \param[in] alpha_min terminal value of alpha
 */
void kohonen_som_tracer(const double *const *X, double *const *W,
                        int num_samples, int num_features, int num_out,
                        double alpha_min)
{
    int R = num_out >> 2, iter = 0;
    double alpha = 1.f;
    double *D = (double *)malloc(num_out * sizeof(double));

    // Loop alpha from 1 to slpha_min
    for (; alpha > alpha_min; alpha -= 0.01, iter++)
    {
        // Loop for each sample pattern in the data set
        for (int sample = 0; sample < num_samples; sample++)
        {
            const double *x = X[sample];
            // update weights for the current input pattern sample
            update_weights(x, W, D, num_out, num_features, alpha, R);
        }

        // every 10th iteration, reduce the neighborhood range
        if (iter % 10 == 0 && R > 1)
            R--;
    }

    free(D);
}

/** Creates a random set of points distributed *near* the circumference
 * of a circle and trains an SOM that finds that circular pattern. The
 * generating function is
 * \f{eqnarray*}{ \f}
 *
 * \param[out] data matrix to store data in
 * \param[in] N number of points required
 */
void test_circle(double *const *data, int N)
{
    const double R = 0.75, dr = 0.3;
    double a_t = 0., b_t = 2.f * M_PI; // theta random between 0 and 2*pi
    double a_r = R - dr, b_r = R + dr; // radius random between R-dr and R+dr
    int i;

#ifdef _OPENMP
#pragma omp for
#endif
    for (i = 0; i < N; i++)
    {
        double r = _random(a_r, b_r);     // random radius
        double theta = _random(a_t, b_t); // random theta
        data[i][0] = r * cos(theta);      // convert from polar to cartesian
        data[i][1] = r * sin(theta);
    }
}

/** Test that creates a random set of points distributed *near* the
 * circumference of a circle and trains an SOM that finds that circular pattern.
 * The following [CSV](https://en.wikipedia.org/wiki/Comma-separated_values)
 * files are created to validate the execution:
 * * `test1.csv`: random test samples points with a circular pattern
 * * `w11.csv`: initial random map
 * * `w12.csv`: trained SOM map
 *
 * The outputs can be readily plotted in [gnuplot](https:://gnuplot.info) using
 * the following snippet
 * ```gnuplot
 * set datafile separator ','
 * plot "test1.csv" title "original", \
 *      "w11.csv" title "w1", \
 *      "w12.csv" title "w2"
 * ```
 */
void test1()
{
    int j, N = 500;
    int features = 2;
    int num_out = 50;
    double **X = (double **)malloc(N * sizeof(double *));
    double **W = (double **)malloc(num_out * sizeof(double *));
    for (int i = 0; i < (num_out > N ? num_out : N);
         i++) // loop till max(N, num_out)
    {
        if (i < N) // only add new arrays if i < N
            X[i] = (double *)malloc(features * sizeof(double));
        if (i < num_out) // only add new arrays if i < num_out
        {
            W[i] = (double *)malloc(features * sizeof(double));
#ifdef _OPENMP
#pragma omp for
#endif
            // preallocate with random initial weights
            for (j = 0; j < features; j++)
                W[i][j] = _random(-1, 1);
        }
    }

    test_circle(X, N); // create test data around circumference of a circle
    save_2d_data("test1.csv", X, N, 2);           // save test data points
    save_2d_data("w11.csv", W, num_out, 2);       // save initial random weights
    kohonen_som_tracer(X, W, N, 2, num_out, 0.1); // train the SOM
    save_2d_data("w12.csv", W, num_out, 2);       // save the resultant weights

    for (int i = 0; i < (num_out > N ? num_out : N); i++)
    {
        if (i < N)
            free(X[i]);
        if (i < num_out)
            free(W[i]);
    }
}

/** Creates a random set of points distributed *near* the locus
 * of the [Lamniscate of
 * Gerono](https://en.wikipedia.org/wiki/Lemniscate_of_Gerono) and trains an SOM
 * that finds that circular pattern. \param[out] data matrix to store data in
 * \param[in] N number of points required
 */
void test_lamniscate(double *const *data, int N)
{
    const double dr = 0.2;
    int i;

#ifdef _OPENMP
#pragma omp for
#endif
    for (i = 0; i < N; i++)
    {
        double dx = _random(-dr, dr);    // random change in x
        double dy = _random(-dr, dr);    // random change in y
        double theta = _random(0, M_PI); // random theta
        data[i][0] = dx + cos(theta);    // convert from polar to cartesian
        data[i][1] = dy + sin(2. * theta) / 2.f;
    }
}

/** Test that creates a random set of points distributed *near* the locus
 * of the [Lamniscate of
 * Gerono](https://en.wikipedia.org/wiki/Lemniscate_of_Gerono) and trains an SOM
 * that finds that circular pattern. The following
 * [CSV](https://en.wikipedia.org/wiki/Comma-separated_values) files are created
 * to validate the execution:
 * * `test2.csv`: random test samples points with a circular pattern
 * * `w21.csv`: initial random map
 * * `w22.csv`: trained SOM map
 *
 * The outputs can be readily plotted in [gnuplot](https:://gnuplot.info) using
 * the following snippet
 * ```gnuplot
 * set datafile separator ','
 * plot "test2.csv" title "original", \
 *      "w21.csv" title "w1", \
 *      "w22.csv" title "w2"
 * ```
 */
void test2()
{
    int j, N = 500;
    int features = 2;
    int num_out = 20;
    double **X = (double **)malloc(N * sizeof(double *));
    double **W = (double **)malloc(num_out * sizeof(double *));
    for (int i = 0; i < (num_out > N ? num_out : N); i++)
    {
        if (i < N) // only add new arrays if i < N
            X[i] = (double *)malloc(features * sizeof(double));
        if (i < num_out) // only add new arrays if i < num_out
        {
            W[i] = (double *)malloc(features * sizeof(double));

#ifdef _OPENMP
#pragma omp for
#endif
            // preallocate with random initial weights
            for (j = 0; j < features; j++)
                W[i][j] = _random(-1, 1);
        }
    }

    test_lamniscate(X, N); // create test data around the lamniscate
    save_2d_data("test2.csv", X, N, 2);     // save test data points
    save_2d_data("w21.csv", W, num_out, 2); // save initial random weights
    kohonen_som_tracer(X, W, N, 2, num_out, 0.01); // train the SOM
    save_2d_data("w22.csv", W, num_out, 2);        // save the resultant weights

    for (int i = 0; i < (num_out > N ? num_out : N); i++)
    {
        if (i < N)
            free(X[i]);
        if (i < num_out)
            free(W[i]);
    }
    free(X);
    free(W);
}

/** Main function */
int main(int argc, char **argv)
{
    test1();
    test2();
    return 0;
}
