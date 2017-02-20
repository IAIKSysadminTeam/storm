#include "../StormEigen/Core"

#ifdef STORMEIGEN_SHOULD_FAIL_TO_BUILD
#define CV_QUALIFIER const
#else
#define CV_QUALIFIER
#endif

using namespace StormEigen;

void foo(CV_QUALIFIER Matrix3d &m){
    CwiseUnaryView<internal::scalar_real_ref_op<double>,Matrix3d> t(m);
}

int main() {}
