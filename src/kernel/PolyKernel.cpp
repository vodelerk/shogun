#include "lib/common.h"
#include "kernel/PolyKernel.h"
#include "features/Features.h"
#include "features/RealFeatures.h"
#include "lib/io.h"
//#include "lib/f77blas.h"

#include <assert.h>

CPolyKernel::CPolyKernel(long size, double d, bool hom, bool rescale_) 
  : CRealKernel(size),degree(d),homogene(hom),rescale(rescale_),scale(1.0)
{
}

CPolyKernel::~CPolyKernel() 
{
}
  
void CPolyKernel::init(CFeatures* l, CFeatures* r)
{
	CRealKernel::init((CRealFeatures*) l, (CRealFeatures*) r); 

	if (rescale)
		init_rescale() ;
}

void CPolyKernel::init_rescale()
{
	double sum=0;
	scale=1.0;
	for (long i=0; (i<lhs->get_num_vectors() && i<rhs->get_num_vectors()); i++)
			sum+=compute(i, i);

	scale=sum/math.min(lhs->get_num_vectors(), rhs->get_num_vectors());
	CIO::message("rescaling kernel by %g (sum:%g num:%d)\n",scale, sum, math.min(lhs->get_num_vectors(), rhs->get_num_vectors()));
}

void CPolyKernel::cleanup()
{
}

bool CPolyKernel::load_init(FILE* src)
{
	return false;
}

bool CPolyKernel::save_init(FILE* dest)
{
	return false;
}
  
bool CPolyKernel::check_features(CFeatures* f) 
{
  return (f->get_feature_type()==F_REAL);
}

REAL CPolyKernel::compute(long idx_a, long idx_b)
{
  long alen, blen;
  bool afree, bfree;

  //fprintf(stderr, "LinKernel.compute(%ld,%ld)\n", idx_a, idx_b) ;
  double* avec=((CRealFeatures*) lhs)->get_feature_vector(idx_a, alen, afree);
  double* bvec=((CRealFeatures*) rhs)->get_feature_vector(idx_b, blen, bfree);
  
  assert(alen==blen);
  //fprintf(stderr, "LinKernel.compute(%ld,%ld) %d\n", idx_a, idx_b, alen) ;

//  double sum=0;
//  for (long i=0; i<alen; i++)
//	  sum+=avec[i]*bvec[i];

//  CIO::message("%ld,%ld -> %f\n",idx_a, idx_b, sum);

  int skip=1;
  int ialen=(int) alen;
  //REAL result=F77CALL(ddot)(REF ialen, avec, REF skip, bvec, REF skip)/scale;

#ifdef NO_LAPACK
  REAL result=0;
  {
    for (int i=0; i<ialen; i++)
      result+=avec[i]*bvec[i];
  }
#else
  REAL result=ddot_(&ialen, avec, &skip, bvec, &skip);
#endif // NO_LAPACK

  if (!homogene)
	  result+=1;

  result=pow(result,degree)/scale;

//  REAL result=sum/scale;
  ((CRealFeatures*) lhs)->free_feature_vector(avec, idx_a, afree);
  ((CRealFeatures*) rhs)->free_feature_vector(bvec, idx_b, bfree);

  return result;
}
