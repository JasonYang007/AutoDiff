// AutoDiff.cpp : Defines the entry point for the console application.
//

#include <autodiff.h>

using namespace autodiff;

void nop()
{
}

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
	ADArray<double> z(2);
	ADVar<double> res;
	res.SetAsRoot();

	EvaluateValueAdjoint((
		z[0] = x*x
		, z[1] = z[0] * (x + y)
		, res = z[1] / y
	));

	std::cout << "x*x*(x+y)/y = " << res << std::endl;
	PrintXY(x,y);
}

//--- nested_apply
template<class T>
struct Func;

template<class T, class E>
struct Func<ADExpr<T,E> >
{
    typedef ADExpr<T,E> var_t;
    typedef EXP_ASSIGN(var_t,
                EXP_DIV(
                   EXP_MULT(
                        EXP_MULT(var_t,var_t),
                        EXP_PLUS(var_t,var_t)),
                   var_t))
            return_t;
    
    return_t operator()(var_t const & x, var_t const & y, ADVar<T> & z)
    {
        return z = (x*x)*(x+y)/y;
    }
};

void test_func_nested(double xx, double yy)
{
	ADVar<double> x=xx;
	ADVar<double> y=yy;
	ADVar<double> z;
    z.SetAsRoot();

    EvaluateValueAdjoint((
        Func<ADExpr<double,ExpVar> >()(x,y,z)
    ));
        
	std::cout << "F(x) = " << z << std::endl;
	PrintXY(x,y);
}

void autodiff_test()
{
	double x=6.0;
	double y=3.0;
	
	test_func_add(x,y);
	test_func_mult(x,y);
    test_func_nested(x,y);

	nop();
}
