//*****************************************************************//
//    Albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#if !defined(tensor_LinearAlgebra_i_cc)
#define tensor_LinearAlgebra_i_cc

namespace LCM {

  //
  // R^N tensor Frobenius norm
  // \return \f$ \sqrt{A:A} \f$
  //
  template<typename T>
  inline
  T
  norm(Tensor<T> const & A)
  {
    const Index
    N = A.get_dimension();

    T s = 0.0;

    switch (N) {

    default:
      s = dotdot(A, A);
      break;

    case 3:
      s+= A(0,0)*A(0,0) + A(0,1)*A(0,1) + A(0,2)*A(0,2);
      s+= A(1,0)*A(1,0) + A(1,1)*A(1,1) + A(1,2)*A(1,2);
      s+= A(2,0)*A(2,0) + A(2,1)*A(2,1) + A(2,2)*A(2,2);
      break;

    case 2:
      s+= A(0,0)*A(0,0) + A(0,1)*A(0,1);
      s+= A(1,0)*A(1,0) + A(1,1)*A(1,1);
      break;

    }

    return sqrt(s);
  }

  //
  // R^N tensor 1-norm
  // \return \f$ \max_{j \in {0,\cdots,N}}\Sigma_{i=0}^N |A_{ij}| \f$
  //
  template<typename T>
  inline
  T
  norm_1(Tensor<T> const & A)
  {
    const Index
    N = A.get_dimension();

    Vector<T> v(N);

    T s = 0.0;

    switch (N) {

    default:

      for (Index i = 0; i < N; ++i) {
        T t = 0.0;
        for (Index j = 0; j < N; ++j) {
          t += std::abs(A(j, i));
        }
        v(i) = t;
      }

      for (Index i = 0; i < N; ++i) {
        s = std::max(s, v(i));
      }
      break;

    case 3:
      v(0) = std::abs(A(0,0)) + std::abs(A(1,0)) + std::abs(A(2,0));
      v(1) = std::abs(A(0,1)) + std::abs(A(1,1)) + std::abs(A(2,1));
      v(2) = std::abs(A(0,2)) + std::abs(A(1,2)) + std::abs(A(2,2));

      s = std::max(std::max(v(0),v(1)),v(2));
      break;

    case 2:
      v(0) = std::abs(A(0,0)) + std::abs(A(1,0));
      v(1) = std::abs(A(0,1)) + std::abs(A(1,1));

      s = std::max(v(0),v(1));
      break;

    }

    return s;
  }

  //
  // R^N tensor infinity-norm
  // \return \f$ \max_{i \in {0,\cdots,N}}\Sigma_{j=0}^N |A_{ij}| \f$
  //
  template<typename T>
  inline
  T
  norm_infinity(Tensor<T> const & A)
  {
    const Index
    N = A.get_dimension();

    Vector<T> v(N);

    T s = 0.0;

    switch (N) {

    default:
      for (Index i = 0; i < N; ++i) {
        T t = 0.0;
        for (Index j = 0; j < N; ++j) {
          t += std::abs(A(i, j));
        }
        v(i) = t;
      }

      for (Index i = 0; i < N; ++i) {
        s = std::max(s, v(i));
      }
      break;

    case 3:
      v(0) = std::abs(A(0,0)) + std::abs(A(0,1)) + std::abs(A(0,2));
      v(1) = std::abs(A(1,0)) + std::abs(A(1,1)) + std::abs(A(1,2));
      v(2) = std::abs(A(2,0)) + std::abs(A(2,1)) + std::abs(A(2,2));

      s = std::max(std::max(v(0),v(1)),v(2));
      break;

    case 2:
      v(0) = std::abs(A(0,0)) + std::abs(A(0,1));
      v(1) = std::abs(A(1,0)) + std::abs(A(1,1));

      s = std::max(v(0),v(1));
      break;

    }

    return s;
  }

  //
  // Swap row. Exchange rows i and j in place
  // \param A tensor
  // \param i index
  // \param j index
  //
  template<typename T>
  void
  swap_row(Tensor<T> & A, Index i, Index j)
  {
    const Index
    N = A.get_dimension();

    if (i != j) {
      for (Index k = 0; k < N; ++k) {
        std::swap(A(i, k), A(j, k));
      }
    }
    return;
  }

  //
  // Swap column. Exchange columns i and j in place
  // \param A tensor
  // \param i index
  // \param j index
  //
  template<typename T>
  void
  swap_col(Tensor<T> & A, Index i, Index j)
  {
    const Index
    N = A.get_dimension();

    if (i != j) {
      for (Index k = 0; k < N; ++k) {
        std::swap(A(k, i), A(k, j));
      }
    }
    return;
  }

  //
  // R^N determinant
  // Laplace expansion. Warning: no pivoting.
  // Casual use only. Use Teuchos LAPACK interface for
  // more efficient and robust techniques.
  // \param A tensor
  // \return \f$ \det A \f$
  //
  template<typename T>
  inline
  T
  det(Tensor<T> const & A)
  {
    const Index
    N = A.get_dimension();

    T s = 0.0;

    switch (N) {

    default:
      {
        int sign = 1;
        for (Index i = 0; i < N; ++i) {
          const T d = det(subtensor(A, i, 1));
          s += sign * d * A(i, 1);
          sign *= -1;
        }
      }
      break;

    case 3:
      s =
      -A(0,2)*A(1,1)*A(2,0) + A(0,1)*A(1,2)*A(2,0) +
       A(0,2)*A(1,0)*A(2,1) - A(0,0)*A(1,2)*A(2,1) -
       A(0,1)*A(1,0)*A(2,2) + A(0,0)*A(1,1)*A(2,2);
      break;

    case 2:
      s = A(0,0) * A(1,1) - A(1,0) * A(0,1);
      break;

    }

    return s;
  }

  //
  // R^N trace
  // \param A tensor
  // \return \f$ A:I \f$
  //
  template<typename T>
  inline
  T
  trace(Tensor<T> const & A)
  {
    const Index
    N = A.get_dimension();

    T s = 0.0;

    switch (N) {

    default:
      for (Index i = 0; i < N; ++i) {
        s += A(i,i);
      }
      break;

    case 3:
      s = A(0,0) + A(1,1) + A(2,2);
      break;

    case 2:
      s = A(0,0) + A(1,1);
      break;

    }

    return s;
  }

  //
  // R^N first invariant, trace
  // \param A tensor
  // \return \f$ I_A = A:I \f$
  //
  template<typename T>
  inline
  T
  I1(Tensor<T> const & A)
  {
    return trace(A);
  }

  //
  // R^N second invariant
  // \param A tensor
  // \return \f$ II_A = \frac{1}{2}((I_A)^2-I_{A^2}) \f$
  //
  template<typename T>
  inline
  T
  I2(Tensor<T> const & A)
  {
    const Index
    N = A.get_dimension();

    T s = 0.0;
    const T trA = trace(A);

    switch (N) {

    default:
      s = 0.5 * (trA * trA - trace(A * A));
      break;

    case 3:
      s = 0.5 * (trA*trA - A(0,0)*A(0,0) - A(1,1)*A(1,1) - A(2,2)*A(2,2)) -
          A(0,1)*A(1,0) - A(0,2)*A(2,0) - A(1,2)*A(2,1);
      break;

    case 2:
      s =  0.5 * (trA * trA - trace(A * A));
      break;

    }

    return s;
  }

  //
  // R^N third invariant
  // \param A tensor
  // \return \f$ III_A = \det A \f$
  //
  template<typename T>
  inline
  T
  I3(Tensor<T> const & A)
  {
    return det(A);
  }

} // namespace LCM

#endif // tensor_LinearAlgebra_i_cc