// AutoDiff.cpp : Defines the entry point for the console application.
//

#include <autodiff.h>

using namespace autodiff;

void nop()
{
}

//template<class T, class E>
//T EvaluateValueAdjoint(ADExpr<T,E> & expr)
//{
//	T res = expr.EvaluateValue();
//	expr.Adjoint() = T(1);
//	expr.EvaluateAdjoint();
//	return res;
//}

template<class T, class E>
T EvaluateValueAdjoint(ADExpr<T,E> const & ee)
{
	ADExpr<T,E> & expr = const_cast<ADExpr<T,E> &>(ee);
	T res = expr.EvaluateValue();
	expr.SetAdjoint(1.0);
	expr.EvaluateAdjoint();
	return res;
}

template<class T>
void arg_test(T const & x)
{
}

template<class E1, class E2>
void PrintXY(E1 const & x, E2 const & y)
{
	std::cout << "x = " << x << std::endl;
	std::cout << "y = " << y << std::endl;
}

void test_func_add(double xx, double yy)
{
	ADVar<double> x=xx;
	ADVar<double> y=yy;
	ADVar<double> res;
	res.SetAsRoot();
	EvaluateValueAdjoint( res = x * x - y );
	std::cout << "x*x-y = " << res << std::endl;
	PrintXY(x,y);
}

void test_func_mult(double xx, double yy)
{
	ADVar<double> x=xx;
	ADVar<double> y=yy;
	ADVar<double> z1;
	ADVar<double> z2;
	ADVar<double> res;
	res.SetAsRoot();

	EvaluateValueAdjoint((
		z1 = x*x
		, z2 = z1 * (x + y)
		, res = z2 / y
	));

	std::cout << "x*x/y = " << res << std::endl;
	PrintXY(x,y);
}

void autodiff_test()
{
	double x=6.0;
	double y=3.0;
	
	test_func_add(x,y);
	test_func_mult(x,y);

	nop();
}