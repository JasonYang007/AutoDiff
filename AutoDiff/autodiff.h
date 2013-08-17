#if !defined(INCLUDED_AUTODIFF_H)
#define INCLUDED_AUTODIFF_H

#include <iostream>
#include <functional>
#include <vector>

namespace autodiff
{

// Class declarations
template<class T, class TExpr>
struct ADExpr;

template<class T>
struct ADExpr_traits;

template<class T, class E>
struct ADExpr_traits<ADExpr<T,E> >
{
    typedef T type;
    typedef E expr_type;
};

struct ExpConst {};
struct ExpVar {};
struct ExpRandVar {};

template<class T>
struct ExpCore
{
	explicit ExpCore(T const val=T(), T const adj=T())
		: value(val), adjoint(adj) {}

	ExpCore& operator=(T const val) { value = val; return *this; }

	T & Value() { return value; }
	T const & Value() const { return value;}

	T & Adjoint() { return adjoint; }
	T const & Adjoint() const { return adjoint;}

	void SetAdjoint(T const adj) { adjoint = adj; }

private:
	T value;
	T adjoint;
};

template<class T0=void,class T1=void,class T2=void>
struct Tuple {};

template<int n, class Ttuple>
struct Select;

template<class T0, class T1, class T2>
struct Select<1,Tuple<T0,T1,T2> > 
{ 
	typedef T0 type;
    static T0 & call(T0 & x0, T1 & x1)
	{
        return x0;
	}
};

template<class T0, class T1, class T2>
struct Select<2,Tuple<T0,T1,T2> > 
{ 
	typedef T1 type;
    static T1 & call(T0 & x0, T1 & x1)
	{
        return x1;
	}
};

template<int n> 
struct Lambda
{
    template<class T0,class T1>
    typename Select<n,Tuple<T0,T1> >::type &
    operator()(T0 & x0, T1 & x1)
	{
        return Select<n,Tuple<T0,T1> >::call(x0,x1);
	}

	void EvaluateAdjoint() {}
};

template<class E, class TArgs>
struct Lambda_traits;

template<class T,int n>
struct ADExpr<T,Lambda<n> >
	: Lambda<n>
    , ExpCore<T>
{
	T EvaluateValue() { return T(); }
};

template<class T, int n, class TArgs>
struct Lambda_traits<ADExpr<T,Lambda<n> >, TArgs> 
{
    typedef typename Select<n,TArgs>::type type;
};


template<class T,int n>
struct ADPos
	: ADExpr<T,Lambda<n> >
{};



template<class D>
struct ExpLambdaCall
{
    template<class T0, class T1>
    D & operator()(T0 & x0, T1 & x1)
	{
        return *static_cast<D *>(this);
	}
};

template<class E, E Op>
struct op_traits;

//*** BINARY OPERATIONS
//--- ENUM & traits
enum AD_BINOP { AD_COMMA,
				AD_ASSIGN, 
				AD_PLUS, AD_MINUS, AD_MULT, AD_DIV 
			  };

namespace autdiff_binary
{
	template<class T>
	T & assign(T & left, T const & right)
	{
		return left = right;
	}
}

#define AD_DEFINE_BINOP_TRAITS(OPNAME,OP,STR) \
	template<> \
	struct op_traits<AD_BINOP, OPNAME>\
	{\
		template<class T>\
		static T call(T const x, T const y) { return OP(x,y); }\
		\
		static char const * str() { return STR; }\
	};

AD_DEFINE_BINOP_TRAITS(AD_PLUS, std::plus<T>(), "+")
AD_DEFINE_BINOP_TRAITS(AD_MINUS, std::minus<T>(), "-")
AD_DEFINE_BINOP_TRAITS(AD_MULT, std::multiplies<T>(), "*")
AD_DEFINE_BINOP_TRAITS(AD_DIV, std::divides<T>(), "/")
AD_DEFINE_BINOP_TRAITS(AD_COMMA, , ",")

template<> 
struct op_traits<AD_BINOP, AD_ASSIGN>
{
template<class T>
static T & call(T & x, T const y) { return x=y; }

static char const * str() { return "="; }
};

//--- Binary expression
template<class T, AD_BINOP BinOp, class E1, class E2>
struct ExpBinary;

template<class T, AD_BINOP BinOp, class E1, class E2>
struct ADExpr<T, ExpBinary<T, BinOp, E1, E2> >
	: public ExpBinary<T, BinOp, E1, E2>
{
	typedef ExpBinary<T, BinOp, E1, E2> base_t;
	ADExpr(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}
};

template<class T1, class T2>
struct assert_same_type;

template<class T>
struct assert_same_type<T,T> {};

template<AD_BINOP BinOp, class E1, class E2>
struct BinExprClass
{
    typedef typename ADExpr_traits<E1>::type value_type;
    typedef typename ADExpr_traits<E2>::type value_type2;
    typedef assert_same_type<value_type, value_type2> asserted_t;
    typedef ADExpr<value_type, ExpBinary<value_type, BinOp, E1, E2> > type;
};

#define ARG(...) __VA_ARGS__
#define AD_BIN_EXPR(BINOP, A1, A2)  typename BinExprClass<BINOP,A1,A2>::type
#define EXP_ASSIGN(A1,A2) AD_BIN_EXPR(AD_ASSIGN,ARG(A1),ARG(A2))
#define EXP_PLUS(A1,A2)   AD_BIN_EXPR(AD_PLUS,ARG(A1),ARG(A2))
#define EXP_MINUS(A1,A2)  AD_BIN_EXPR(AD_MINUS,ARG(A1),ARG(A2))
#define EXP_MULT(A1,A2)   AD_BIN_EXPR(AD_MULT,ARG(A1),ARG(A2))
#define EXP_DIV(A1,A2)    AD_BIN_EXPR(AD_DIV,ARG(A1),ARG(A2))

template<class T, AD_BINOP BinOp, class E1, class E2>
struct ExpBinaryCore;

template<class T, AD_BINOP BinOp, class E1, class E2, class T1, class T2>
struct Lambda_traits<ADExpr<T,ExpBinary<T,BinOp,E1,E2> >, Tuple<T1,T2> >
{
    typedef Tuple<T1,T2> tuple_t;
    typedef typename Lambda_traits<E1,tuple_t>::type A1;
    typedef typename Lambda_traits<E2,tuple_t>::type A2;
    typedef ADExpr<T,ExpBinary<T,BinOp,A1,A2> > type;
};


template<class T, AD_BINOP BinOp, class E1, class E2>
struct ExpBinaryCore
	: ExpCore<T>
{
	typedef ExpCore<T> base_t;
	ExpBinaryCore(E1 const & e1, E2 const & e2)
		: base_t(), expr1(e1), expr2(e2) {}

	T EvaluateValue()
	{
		T val1 = expr1.EvaluateValue();
		T val2 = expr2.EvaluateValue();
		return this->Value() = op_traits<AD_BINOP,BinOp>::call(val1, val2);
	}

    template<class A1, class A2>
    typename Lambda_traits<ADExpr<T,ExpBinary<T,BinOp,E1,E2> >, Tuple<ADExpr<T,A1>,ADExpr<T,A2> > >::type
    operator()(ADExpr<T,A1> & x1, ADExpr<T,A2> & x2)
	{
        typedef typename Lambda_traits< ADExpr<T,ExpBinary<T,BinOp,E1,E2> >
			                          , Tuple<ADExpr<T,A1>,ADExpr<T,A2> > >::type return_t;
        return return_t(expr1(x1,x2), expr2(x1,x2));
	}

	E1 expr1;
	E2 expr2;
};

template<class T, class E1, class E2>
struct ExpBinaryCore<T,AD_ASSIGN,E1,E2>
	: ExpCore<T>
{
	typedef ExpCore<T> base_t;
	ExpBinaryCore(E1 const & e1, E2 const & e2)
		: base_t(), expr1(e1), expr2(e2) {}

	T EvaluateValue()
	{
		return this->Value() = op_traits<AD_BINOP,AD_ASSIGN>::call(expr1.Value(), expr2.EvaluateValue());
	}

	E1 expr1;
	E2 expr2;
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_COMMA, E1, E2>
	:  ExpBinaryCore<T,AD_COMMA,E1,E2>
{
	typedef ExpBinaryCore<T,AD_COMMA,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	//void EvaluateAdjoint(bool bIsTop=false)
	void EvaluateAdjoint()
	{
		//T adj = bIsTop ? T(1) : T(0);
		//this->expr2.SetAdjoint(adj);
		this->expr2.EvaluateAdjoint();

		//this->expr1.SetAdjoint(T(0));
		this->expr1.EvaluateAdjoint();	
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_ASSIGN, E1, E2>
	:  public ExpBinaryCore<T,AD_ASSIGN,E1,E2>
{
	typedef ExpBinaryCore<T,AD_ASSIGN,E1,E2>	base_t;
	typedef ADExpr<T,ExpBinary> expr_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->expr1.AdjointFetch();
		this->expr2.SetAdjoint(adj);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_PLUS, E1, E2>
	:  public ExpBinaryCore<T,AD_PLUS,E1,E2>
{
	typedef ExpBinaryCore<T,AD_PLUS,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		this->expr1.SetAdjoint(adj);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(adj);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_MINUS, E1, E2>
	:  public ExpBinaryCore<T,AD_MINUS,E1,E2>
{
	typedef ExpBinaryCore<T,AD_MINUS,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		this->expr1.SetAdjoint(adj);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(-adj);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_MULT, E1, E2>
	:  public ExpBinaryCore<T, AD_MULT, E1, E2>
{
	typedef ExpBinaryCore<T,AD_MULT,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		T x = this->expr1.Value();
		T y = this->expr2.Value();
		this->expr1.SetAdjoint(adj * y);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(adj * x);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_DIV, E1, E2>
	:  public ExpBinaryCore<T, AD_DIV, E1, E2>
{
	typedef ExpBinaryCore<T,AD_DIV,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		T x = this->expr1.Value();
		T y = this->expr2.Value();
		this->expr1.SetAdjoint(adj / y);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(adj * (-x/(y*y)));
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, AD_BINOP BinOp, class E1, class E2>
std::ostream & operator<<(std::ostream & ostr, ExpBinary<T, BinOp, E1, E2> const & expr)
{
	return ostr << '[' << op_traits<AD_BINOP,BinOp>::str() << ':' << expr.Value() << "]("
		   		<< expr.expr1 << ',' << expr.expr2 << ')';
}

// ADExpr: expression templates
//----	Const expression
template<class T>
struct ADExpr<T, ExpConst>
	: public ExpCore<T>
{
	typedef ExpCore<T> base_t;

	ADExpr(T const val)
		: base_t(val, T())
	{}
	
	T EvaluateValue() { return this->Value(); }
	void EvaluateAdjoint() {}
};

template<class T>
std::ostream & operator<<(std::ostream & ostr, ADExpr<T,ExpConst> const & expr)
{
	return ostr << expr.Value();
}

//----	Variable expression

template<class T>
struct ADExprVarCore
{
	ADExprVarCore(T * v_ptr, T *a_ptr)
		: adjoint(), val_ptr(v_ptr), adj_ptr(a_ptr)
	{}

	T & Value() { return *val_ptr; }
	T const & Value() const { return *val_ptr;}

	T & Adjoint() { return adjoint; }
	T const & Adjoint() const { return adjoint;}

	void SetAdjoint(T const adj) { adjoint = adj; }

	T EvaluateValue()
	{
		return Value();
	}

	void EvaluateAdjoint()
	{
		*adj_ptr += adjoint;
	}

	T & AdjointFetch()
	{
		return adjoint = *adj_ptr;
	}

	T const & AdjointFetch() const
	{
		return adjoint = *adj_ptr;
	}
	
protected:
	ADExprVarCore()
		: adjoint(), val_ptr(0L), adj_ptr(0L)
	{}

	void set_pointer(T * v_ptr, T * a_ptr)
	{
		val_ptr = v_ptr;
		adj_ptr = a_ptr;
	}
private:
	T  adjoint;
	T * val_ptr;
	T * adj_ptr;
};

template<class T>
struct ADExpr<T,ExpVar>
	: ADExprVarCore<T>
{
    typedef ADExprVarCore<T> base_t;

	ADExpr(T * v_ptr, T *a_ptr)
		: base_t(v_ptr, a_ptr)
	{}

protected:
	ADExpr()
		: base_t()
	{}
};


template<class T>
std::ostream & operator<<(std::ostream & ostr, ADExprVarCore<T> const & expr)
{
	return ostr << '(' << expr.Value() << ',' << expr.Adjoint() << ')';
}

//---- ADVar definition
#define ADVAR_DEFINE_ASSIGN_OP(ADTYPE, OP) \
	ADTYPE & operator OP(T const val)\
	{\
		base_data_t::Value() OP val;\
		return *this;\
	}

#define ADVAR_DEFINE_ASSIGN(ADTYPE)\
	ADVAR_DEFINE_ASSIGN_OP(ADTYPE, =)\
	ADVAR_DEFINE_ASSIGN_OP(ADTYPE, +=)\
	ADVAR_DEFINE_ASSIGN_OP(ADTYPE, -=)\
	ADVAR_DEFINE_ASSIGN_OP(ADTYPE, *=)\
	ADVAR_DEFINE_ASSIGN_OP(ADTYPE, /=)


template<class T, class TExpVar>
struct ADVarCore
	: ADExpr<T,TExpVar>, ExpCore<T>
{
	typedef ADExpr<T,TExpVar> base_t;
	typedef ExpCore<T>	 base_data_t;

	using base_data_t::Value;
	using base_data_t::Adjoint;

	ADVarCore(T const val = T())
		: base_t()
		, base_data_t(val,T())
	{
		base_t::set_pointer(&base_data_t::Value(), &base_data_t::Adjoint());
	}

	void SetAdjoint(T const adj)
	{
		base_t::Adjoint()=base_data_t::Adjoint()=adj;
	}

	void SetAsRoot()
	{
		this->SetAdjoint(T(1));
	}


};

template<class T>
struct ADVar
	: ADVarCore<T,ExpVar>
{
    typedef ADVarCore<T,ExpVar> core_base_t;
    typedef core_base_t::base_t base_t;

    ADVar(T const val = T())
		: core_base_t(val)
	{}

	ADVAR_DEFINE_ASSIGN(ADVar)

	template<class E>
    EXP_ASSIGN(base_t, ARG(ADExpr<T,E>))
	operator=(ADExpr<T,E> const & other)
	{
		return EXP_ASSIGN(base_t, ARG(ADExpr<T,E>))(*this, other);
	}
};

template<class T>
std::ostream & operator<<(std::ostream & ostr, ADVar<T> const & expr)
{
	return ostr << '(' << expr.Value() << ',' << expr.Adjoint() << ')';
}

//--- AdRandVar definition
template<class T>
struct ADExpr<T,ExpRandVar>
	: ADExprVarCore<T>
{
    typedef ADExprVarCore<T> base_t;

	ADExpr(T * v_ptr, T *a_ptr)
		: base_t(v_ptr, a_ptr)
	{}

protected:
	ADExpr()
		: base_t()
	{}
};

template<class T>
struct ADRandVar
	: ADVarCore<T,ExpRandVar>
{
    typedef ADVarCore<T,ExpRandVar> core_base_t;
    typedef core_base_t::base_t base_t;

    ADRandVar(T const val = T())
		: core_base_t(val)
	{}
    
	ADVAR_DEFINE_ASSIGN(ADRandVar)

	template<class E>
    EXP_ASSIGN(base_t, ARG(ADExpr<T,E>))
	operator=(ADExpr<T,E> const & other)
	{
		return EXP_ASSIGN(base_t, ARG(ADExpr<T,E>))(*this, other);
	}
};


//--- ADArray definition
template<class T>
struct ADArray
{
    typedef ADVar<T> element_t;

    ADArray(std::size_t len=0)
    : oVec(len) {}

    element_t & operator[](std::size_t idx)
    {
        return oVec[idx];
    }

    element_t const & operator[](std::size_t idx) const
    {
        return oVec[idx];
    }

    size_t size() const { return oVec.size(); }

private:
    std::vector<element_t> oVec;
};

//*** Binary operators
//--- Arithmetic
#define AD_DEFINE_BINARY_OP(OPNAME, BINFUNC)\
	template<class T, class E1, class E2>\
	ADExpr<T,ExpBinary<T,OPNAME,ADExpr<T,E1>,ADExpr<T,E2> > > \
		BINFUNC(ADExpr<T,E1> const & e1, ADExpr<T,E2> const & e2)\
	{\
		return ADExpr<T,ExpBinary<T,OPNAME,ADExpr<T,E1>,ADExpr<T,E2> > >(e1, e2);\
	}

AD_DEFINE_BINARY_OP(AD_PLUS, operator+)
AD_DEFINE_BINARY_OP(AD_MINUS, operator-)
AD_DEFINE_BINARY_OP(AD_MULT, operator*)
AD_DEFINE_BINARY_OP(AD_DIV, operator/)

//--- Comma operator
template<class T, class E1, class E2>
ADExpr<T,ExpBinary<T,AD_COMMA,ADExpr<T,E1>,ADExpr<T,E2> > > 
	operator ,(ADExpr<T,E1> const & e1, ADExpr<T,E2> const & e2)
{
	return ADExpr<T,ExpBinary<T,AD_COMMA,ADExpr<T,E1>,ADExpr<T,E2> > >(e1, e2);
}


} /* end of namespace: autodiff */


#endif /* INCLUDED_AUTODIFF_H */
